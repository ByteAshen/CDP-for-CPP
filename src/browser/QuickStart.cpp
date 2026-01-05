

#include "cdp/browser/QuickStart.hpp"
#include "cdp/core/Base64.hpp"
#include "cdp/domains/Fetch.hpp"
#include <thread>
#include <chrono>
#include <fstream>
#include <sstream>

namespace cdp {
namespace quick {


void FetchAction::continueRequest() {
    page_->client().Fetch.continueRequestAsync(requestId_);
}

void FetchAction::continueRequest(const std::vector<HeaderEntry>& headers,
                                   const std::string& url,
                                   const std::string& method,
                                   const std::string& postData) {
    page_->client().Fetch.continueRequestAsync(requestId_, headers, url, method, postData);
}

void FetchAction::block(const std::string& reason) {
    page_->client().Fetch.failRequestAsync(requestId_, reason);
}

void FetchAction::fulfill(int statusCode,
                           const std::vector<HeaderEntry>& headers,
                           const std::string& body) {
    page_->client().Fetch.fulfillRequestAsync(requestId_, statusCode, headers, body);
}

void FetchAction::fulfillText(int statusCode, const std::string& text,
                               const std::string& contentType) {
    std::vector<HeaderEntry> headers = {
        {"Content-Type", contentType}
    };
    
    std::string encoded = Base64::encode(
        std::vector<uint8_t>(text.begin(), text.end()));
    page_->client().Fetch.fulfillRequestAsync(requestId_, statusCode, headers, encoded);
}

void FetchAction::fulfillJson(int statusCode, const std::string& json) {
    fulfillText(statusCode, json, "application/json");
}


QuickPage::QuickPage(std::unique_ptr<CDPClient> client, const std::string& targetId,
                     QuickBrowser* browser)
    : client_(std::move(client))
    , targetId_(targetId)
    , browser_(browser) {
    if (client_) {
        page_ = std::make_unique<highlevel::Page>(*client_);
    }
}

QuickPage::~QuickPage() {
    
}

QuickPage::QuickPage(QuickPage&& other) noexcept
    : client_(std::move(other.client_))
    , page_(std::move(other.page_))
    , targetId_(std::move(other.targetId_))
    , lastError_(std::move(other.lastError_))
    , browser_(other.browser_)
    , autoAccept_(other.autoAccept_)
    , promptText_(std::move(other.promptText_))
    , closed_(other.closed_) {
    other.browser_ = nullptr;
    other.closed_ = true;
}

QuickPage& QuickPage::operator=(QuickPage&& other) noexcept {
    if (this != &other) {
        client_ = std::move(other.client_);
        page_ = std::move(other.page_);
        targetId_ = std::move(other.targetId_);
        lastError_ = std::move(other.lastError_);
        browser_ = other.browser_;
        autoAccept_ = other.autoAccept_;
        promptText_ = std::move(other.promptText_);
        closed_ = other.closed_;
        other.browser_ = nullptr;
        other.closed_ = true;
    }
    return *this;
}


#define CDP_PAGE_GUARD() \
    if (closed_) { \
        lastError_ = "Page has been closed"; \
        return {}; \
    }

#define CDP_PAGE_GUARD_BOOL() \
    if (closed_) { \
        lastError_ = "Page has been closed"; \
        return false; \
    }

#define CDP_PAGE_GUARD_VOID() \
    if (closed_) { \
        lastError_ = "Page has been closed"; \
        return; \
    }


bool QuickPage::navigate(const std::string& url, int timeoutMs) {
    CDP_PAGE_GUARD_BOOL();
    highlevel::NavigateOptions opts;
    opts.waitUntil = highlevel::WaitUntil::Load;
    opts.timeoutMs = timeoutMs;
    auto result = page_->navigate(url, opts);
    if (!result.ok()) {
        lastError_ = result.error().message;
        return false;
    }
    return true;
}

bool QuickPage::back() {
    auto resp = client_->Page.navigateToHistoryEntry(-1);
    if (resp.hasError) {
        return exec("history.back()");
    }
    return true;
}

bool QuickPage::forward() {
    auto resp = client_->Page.navigateToHistoryEntry(1);
    if (resp.hasError) {
        return exec("history.forward()");
    }
    return true;
}

bool QuickPage::reload(bool ignoreCache) {
    auto resp = client_->Page.reload(ignoreCache);
    if (resp.hasError) {
        lastError_ = resp.errorMessage;
        return false;
    }
    return waitNavigation();
}

std::string QuickPage::url() {
    CDP_PAGE_GUARD();
    
    auto resp = client_->Target.getTargetInfo(targetId_);
    if (!resp.hasError) {
        auto* targetInfo = resp.result.find("targetInfo");
        if (targetInfo) {
            return (*targetInfo)["url"].getString();
        }
    }
    
    return evalString("window.location.href");
}

std::string QuickPage::title() {
    CDP_PAGE_GUARD();
    
    auto resp = client_->Target.getTargetInfo(targetId_);
    if (!resp.hasError) {
        auto* targetInfo = resp.result.find("targetInfo");
        if (targetInfo) {
            return (*targetInfo)["title"].getString();
        }
    }
    
    return evalString("document.title");
}


std::string QuickPage::html() {
    return evalString("document.documentElement.outerHTML");
}

std::string QuickPage::text(const std::string& selector) {
    if (selector.empty()) {
        return evalString("document.body.innerText");
    }
    return evalString("(function(){var e=document.querySelector('" + selector +
                      "');return e?e.innerText:''})()");
}

std::string QuickPage::attribute(const std::string& selector, const std::string& attr) {
    
    auto docResp = client_->DOM.getDocument(0, false);
    if (!docResp.hasError) {
        auto* root = docResp.result.find("root");
        if (root) {
            int rootNodeId = (*root)["nodeId"].asInt();
            auto queryResp = client_->DOM.querySelector(rootNodeId, selector);
            if (!queryResp.hasError) {
                int nodeId = queryResp.result["nodeId"].asInt();
                if (nodeId > 0) {
                    auto attrResp = client_->DOM.getAttributes(nodeId);
                    if (!attrResp.hasError) {
                        auto* attributes = attrResp.result.find("attributes");
                        if (attributes && attributes->isArray()) {
                            
                            for (size_t i = 0; i + 1 < attributes->size(); i += 2) {
                                if ((*attributes)[i].getString() == attr) {
                                    return (*attributes)[i + 1].getString();
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    
    return evalString("(function(){var e=document.querySelector('" + selector +
                      "');return e?e.getAttribute('" + attr + "'):''})()");
}

bool QuickPage::exists(const std::string& selector) {
    
    auto docResp = client_->DOM.getDocument(0, false);
    if (!docResp.hasError) {
        auto* root = docResp.result.find("root");
        if (root) {
            int nodeId = (*root)["nodeId"].asInt();
            auto queryResp = client_->DOM.querySelector(nodeId, selector);
            if (!queryResp.hasError) {
                int foundNodeId = queryResp.result["nodeId"].asInt();
                return foundNodeId > 0;
            }
        }
    }
    
    auto result = eval("document.querySelector('" + selector + "')!==null");
    return result.isBool() ? result.asBool() : false;
}

int QuickPage::count(const std::string& selector) {
    
    auto docResp = client_->DOM.getDocument(0, false);
    if (!docResp.hasError) {
        auto* root = docResp.result.find("root");
        if (root) {
            int nodeId = (*root)["nodeId"].asInt();
            auto queryResp = client_->DOM.querySelectorAll(nodeId, selector);
            if (!queryResp.hasError) {
                auto* nodeIds = queryResp.result.find("nodeIds");
                if (nodeIds && nodeIds->isArray()) {
                    return static_cast<int>(nodeIds->size());
                }
            }
        }
    }
    
    auto result = eval("document.querySelectorAll('" + selector + "').length");
    return result.isNumber() ? static_cast<int>(result.getNumber()) : 0;
}


bool QuickPage::click(const std::string& selector, int timeoutMs) {
    if (!waitFor(selector, timeoutMs)) {
        lastError_ = "Element not found: " + selector;
        return false;
    }
    return exec("document.querySelector('" + selector + "').click()");
}

bool QuickPage::doubleClick(const std::string& selector, int timeoutMs) {
    if (!waitFor(selector, timeoutMs)) {
        lastError_ = "Element not found: " + selector;
        return false;
    }
    return exec("var e=document.querySelector('" + selector +
                "');var evt=new MouseEvent('dblclick',{bubbles:true});e.dispatchEvent(evt)");
}

bool QuickPage::type(const std::string& selector, const std::string& text, int timeoutMs) {
    if (!clear(selector, timeoutMs)) {
        return false;
    }
    return append(selector, text, 0);
}

bool QuickPage::append(const std::string& selector, const std::string& text, int timeoutMs) {
    if (timeoutMs > 0 && !waitFor(selector, timeoutMs)) {
        lastError_ = "Element not found: " + selector;
        return false;
    }

    if (!focus(selector, 0)) {
        return false;
    }

    auto resp = client_->Input.insertText(text);
    if (resp.hasError) {
        lastError_ = resp.errorMessage;
        return false;
    }
    return true;
}

bool QuickPage::clear(const std::string& selector, int timeoutMs) {
    if (!waitFor(selector, timeoutMs)) {
        lastError_ = "Element not found: " + selector;
        return false;
    }
    return exec("var e=document.querySelector('" + selector + "');e.value='';e.focus()");
}

bool QuickPage::press(const std::string& key) {
    
    
    auto resp = client_->Input.dispatchKeyEvent(
        "keyDown", 0, 0, key, "", "", "", key, 0, 0, false, false, false, 0, {});

    if (!resp.hasError) {
        client_->Input.dispatchKeyEvent(
            "keyUp", 0, 0, key, "", "", "", key, 0, 0, false, false, false, 0, {});
    }

    return !resp.hasError;
}

bool QuickPage::select(const std::string& selector, const std::string& value, int timeoutMs) {
    if (!waitFor(selector, timeoutMs)) {
        lastError_ = "Element not found: " + selector;
        return false;
    }
    return exec("var e=document.querySelector('" + selector + "');e.value='" + value +
                "';e.dispatchEvent(new Event('change',{bubbles:true}))");
}

bool QuickPage::check(const std::string& selector, bool checked, int timeoutMs) {
    if (!waitFor(selector, timeoutMs)) {
        lastError_ = "Element not found: " + selector;
        return false;
    }
    return exec("var e=document.querySelector('" + selector + "');if(e.checked!==" +
                (checked ? "true" : "false") + ")e.click()");
}

bool QuickPage::focus(const std::string& selector, int timeoutMs) {
    if (timeoutMs > 0 && !waitFor(selector, timeoutMs)) {
        lastError_ = "Element not found: " + selector;
        return false;
    }
    return exec("document.querySelector('" + selector + "').focus()");
}

bool QuickPage::hover(const std::string& selector, int timeoutMs) {
    if (!waitFor(selector, timeoutMs)) {
        lastError_ = "Element not found: " + selector;
        return false;
    }

    auto pos = eval(
        "(function(){var e=document.querySelector('" + selector +
        "');var r=e.getBoundingClientRect();return {x:r.x+r.width/2,y:r.y+r.height/2}})()");

    if (!pos.isObject()) {
        lastError_ = "Failed to get element position";
        return false;
    }

    double x = pos["x"].getNumber();
    double y = pos["y"].getNumber();

    auto resp = client_->Input.dispatchMouseEvent("mouseMoved", x, y);
    return !resp.hasError;
}

bool QuickPage::scrollTo(const std::string& selector, int timeoutMs) {
    if (!waitFor(selector, timeoutMs)) {
        lastError_ = "Element not found: " + selector;
        return false;
    }
    return exec("document.querySelector('" + selector + "').scrollIntoView({behavior:'smooth',block:'center'})");
}

bool QuickPage::scroll(int x, int y) {
    return exec("window.scrollBy(" + std::to_string(x) + "," + std::to_string(y) + ")");
}


JsonValue QuickPage::eval(const std::string& expression) {
    CDP_PAGE_GUARD();
    auto resp = client_->Runtime.evaluate(expression);
    if (resp.hasError) {
        lastError_ = resp.errorMessage;
        return JsonValue();  
    }

    auto* result = resp.result.find("result");
    if (!result) return JsonValue();

    
    auto* exceptionDetails = resp.result.find("exceptionDetails");
    if (exceptionDetails) {
        auto* exception = exceptionDetails->find("exception");
        if (exception) {
            auto* description = exception->find("description");
            if (description && description->isString()) {
                lastError_ = "JavaScript error: " + description->asString();
            }
        }
        return JsonValue();
    }

    
    auto* value = result->find("value");
    if (value) {
        return *value;
    }

    
    auto* objectId = result->find("objectId");
    if (objectId && objectId->isString()) {
        
        JsonObject params;
        params["objectId"] = objectId->asString();
        params["functionDeclaration"] = "function() { return JSON.stringify(this); }";
        params["returnByValue"] = true;

        auto serializeResp = client_->sendCommand("Runtime.callFunctionOn", JsonValue(params));
        if (!serializeResp.hasError) {
            auto* serResult = serializeResp.result.find("result");
            if (serResult) {
                auto* serValue = serResult->find("value");
                if (serValue && serValue->isString()) {
                    try {
                        return JsonValue::parse(serValue->asString());
                    } catch (const std::exception&) {
                        
                        return *serValue;
                    }
                }
            }
        }
    }

    
    auto* desc = result->find("description");
    if (desc && desc->isString()) {
        return *desc;
    }

    
    auto* type = result->find("type");
    if (type && type->isString()) {
        std::string typeStr = type->asString();
        if (typeStr == "undefined") {
            return JsonValue();  
        }
    }

    return JsonValue();
}

std::string QuickPage::evalString(const std::string& expression) {
    JsonValue result = eval(expression);

    if (result.isString()) {
        return result.asString();
    } else if (result.isBool()) {
        return result.asBool() ? "true" : "false";
    } else if (result.isNumber()) {
        
        double num = result.getNumber();
        if (num == static_cast<int64_t>(num)) {
            return std::to_string(static_cast<int64_t>(num));
        }
        return std::to_string(num);
    } else if (result.isNull()) {
        return "null";
    } else if (result.isArray() || result.isObject()) {
        return result.serialize();
    }

    return "";
}

bool QuickPage::exec(const std::string& script) {
    auto resp = client_->Runtime.evaluate(script);
    if (resp.hasError) {
        lastError_ = resp.errorMessage;
        return false;
    }
    return true;
}


bool QuickPage::screenshot(const std::string& filePath) {
    auto bytes = screenshotBytes();
    if (bytes.empty()) {
        return false;
    }

    std::ofstream file(filePath, std::ios::binary);
    if (!file) {
        lastError_ = "Failed to open file for writing: " + filePath;
        return false;
    }

    file.write(reinterpret_cast<const char*>(bytes.data()), bytes.size());
    return true;
}

std::vector<uint8_t> QuickPage::screenshotBytes() {
    auto resp = client_->Page.captureScreenshot("png");
    if (resp.hasError) {
        lastError_ = resp.errorMessage;
        return {};
    }

    auto* data = resp.result.find("data");
    if (!data || !data->isString()) {
        lastError_ = "No screenshot data in response";
        return {};
    }

    return Base64::decode(data->asString());
}

bool QuickPage::screenshotElement(const std::string& selector, const std::string& filePath) {
    if (!waitFor(selector)) {
        lastError_ = "Element not found: " + selector;
        return false;
    }

    auto bounds = eval(
        "(function(){var e=document.querySelector('" + selector +
        "');var r=e.getBoundingClientRect();return {x:r.x,y:r.y,width:r.width,height:r.height}})()");

    if (!bounds.isObject()) {
        lastError_ = "Failed to get element bounds";
        return false;
    }

    Viewport clip;
    clip.x = bounds["x"].getNumber();
    clip.y = bounds["y"].getNumber();
    clip.width = bounds["width"].getNumber();
    clip.height = bounds["height"].getNumber();
    clip.scale = 1.0;

    auto resp = client_->Page.captureScreenshot("png", 100, &clip, true, false, false);
    if (resp.hasError) {
        lastError_ = resp.errorMessage;
        return false;
    }

    auto* data = resp.result.find("data");
    if (!data || !data->isString()) {
        lastError_ = "No screenshot data";
        return false;
    }

    auto bytes = Base64::decode(data->asString());
    std::ofstream file(filePath, std::ios::binary);
    file.write(reinterpret_cast<const char*>(bytes.data()), bytes.size());
    return true;
}

bool QuickPage::screenshotFullPage(const std::string& filePath) {
    auto dims = eval(
        "(function(){return {width:Math.max(document.body.scrollWidth,document.documentElement.scrollWidth),"
        "height:Math.max(document.body.scrollHeight,document.documentElement.scrollHeight)}})()");

    if (!dims.isObject()) {
        lastError_ = "Failed to get page dimensions";
        return false;
    }

    Viewport clip;
    clip.x = 0;
    clip.y = 0;
    clip.width = dims["width"].getNumber();
    clip.height = dims["height"].getNumber();
    clip.scale = 1.0;

    auto resp = client_->Page.captureScreenshot("png", 100, &clip, true, true, false);
    if (resp.hasError) {
        lastError_ = resp.errorMessage;
        return false;
    }

    auto* data = resp.result.find("data");
    if (!data || !data->isString()) {
        lastError_ = "No screenshot data";
        return false;
    }

    auto bytes = Base64::decode(data->asString());
    std::ofstream file(filePath, std::ios::binary);
    file.write(reinterpret_cast<const char*>(bytes.data()), bytes.size());
    return true;
}


bool QuickPage::pdf(const std::string& filePath) {
    auto resp = client_->Page.printToPDF();
    if (resp.hasError) {
        lastError_ = resp.errorMessage;
        return false;
    }

    auto* data = resp.result.find("data");
    if (!data || !data->isString()) {
        lastError_ = "No PDF data in response";
        return false;
    }

    auto bytes = Base64::decode(data->asString());
    std::ofstream file(filePath, std::ios::binary);
    if (!file) {
        lastError_ = "Failed to open file for writing: " + filePath;
        return false;
    }

    file.write(reinterpret_cast<const char*>(bytes.data()), bytes.size());
    return true;
}


bool QuickPage::waitFor(const std::string& selector, int timeoutMs) {
    auto startTime = std::chrono::steady_clock::now();

    while (true) {
        if (exists(selector)) {
            return true;
        }

        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - startTime).count();

        if (elapsed >= timeoutMs) {
            lastError_ = "Timeout waiting for element: " + selector;
            return false;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
}

bool QuickPage::waitVisible(const std::string& selector, int timeoutMs) {
    auto startTime = std::chrono::steady_clock::now();

    while (true) {
        auto visStr = evalString(
            "(function(){var e=document.querySelector('" + selector +
            "');if(!e)return'none';var s=getComputedStyle(e);return s.display!='none'&&s.visibility!='hidden'&&s.opacity!='0'?'visible':'hidden'})()");

        if (visStr == "visible") {
            return true;
        }

        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - startTime).count();

        if (elapsed >= timeoutMs) {
            lastError_ = "Timeout waiting for element to be visible: " + selector;
            return false;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
}

bool QuickPage::waitHidden(const std::string& selector, int timeoutMs) {
    auto startTime = std::chrono::steady_clock::now();

    while (true) {
        if (!exists(selector)) {
            return true;
        }

        auto visStr = evalString(
            "(function(){var e=document.querySelector('" + selector +
            "');if(!e)return'none';var s=getComputedStyle(e);return s.display=='none'||s.visibility=='hidden'||s.opacity=='0'?'hidden':'visible'})()");

        if (visStr == "hidden" || visStr == "none") {
            return true;
        }

        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - startTime).count();

        if (elapsed >= timeoutMs) {
            lastError_ = "Timeout waiting for element to be hidden: " + selector;
            return false;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
}

bool QuickPage::waitNavigation(int timeoutMs) {
    auto result = page_->waitForNavigation(timeoutMs);
    if (!result.ok()) {
        lastError_ = result.error().message;
        return false;
    }
    return true;
}

bool QuickPage::waitNetworkIdle(int idleDurationMs, int timeoutMs) {
    auto startTime = std::chrono::steady_clock::now();
    auto lastActivity = startTime;

    while (true) {
        auto state = evalString("document.readyState");

        if (state == "complete") {
            auto now = std::chrono::steady_clock::now();
            auto idleTime = std::chrono::duration_cast<std::chrono::milliseconds>(
                now - lastActivity).count();

            if (idleTime >= idleDurationMs) {
                return true;
            }
        } else {
            lastActivity = std::chrono::steady_clock::now();
        }

        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - startTime).count();

        if (elapsed >= timeoutMs) {
            lastError_ = "Timeout waiting for network idle";
            return false;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
}

void QuickPage::sleep(int ms) {
    std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}


bool QuickPage::setCookie(const std::string& name, const std::string& value,
                          const std::string& domain, const std::string& path) {
    std::string actualDomain = domain;
    if (actualDomain.empty()) {
        auto urlStr = url();
        size_t start = urlStr.find("://");
        if (start != std::string::npos) {
            start += 3;
            size_t end = urlStr.find('/', start);
            actualDomain = urlStr.substr(start, end - start);
            size_t portPos = actualDomain.find(':');
            if (portPos != std::string::npos) {
                actualDomain = actualDomain.substr(0, portPos);
            }
        }
    }

    CookieParam cookie;
    cookie.name = name;
    cookie.value = value;
    cookie.domain = actualDomain;
    cookie.path = path;

    auto resp = client_->Network.setCookie(cookie);
    if (resp.hasError) {
        lastError_ = resp.errorMessage;
        return false;
    }
    return true;
}

std::string QuickPage::getCookie(const std::string& name) {
    auto resp = client_->Network.getCookies();
    if (resp.hasError) {
        lastError_ = resp.errorMessage;
        return "";
    }

    auto* cookies = resp.result.find("cookies");
    if (!cookies || !cookies->isArray()) {
        return "";
    }

    for (size_t i = 0; i < cookies->size(); ++i) {
        auto& cookie = (*cookies)[i];
        if (cookie["name"].getString() == name) {
            return cookie["value"].getString();
        }
    }

    return "";
}

std::string QuickPage::getAllCookies() {
    auto resp = client_->Network.getAllCookies();
    if (resp.hasError) {
        lastError_ = resp.errorMessage;
        return "[]";
    }

    auto* cookies = resp.result.find("cookies");
    if (cookies) {
        return cookies->serialize();
    }
    return "[]";
}

bool QuickPage::deleteCookie(const std::string& name, const std::string& domain) {
    auto resp = client_->Network.deleteCookies(name, "", domain);
    if (resp.hasError) {
        lastError_ = resp.errorMessage;
        return false;
    }
    return true;
}

bool QuickPage::deleteAllCookies() {
    auto resp = client_->Network.clearBrowserCookies();
    if (resp.hasError) {
        lastError_ = resp.errorMessage;
        return false;
    }
    return true;
}


bool QuickPage::setViewport(int width, int height, double deviceScaleFactor) {
    auto resp = client_->Emulation.setDeviceMetricsOverride(width, height, deviceScaleFactor, false);
    if (resp.hasError) {
        lastError_ = resp.errorMessage;
        return false;
    }
    return true;
}

bool QuickPage::emulateDevice(const std::string& deviceName) {
    struct DevicePreset {
        int width, height;
        double scale;
        std::string userAgent;
        bool mobile;
    };

    std::map<std::string, DevicePreset> devices = {
        {"iPhone 12", {390, 844, 3.0,
            "Mozilla/5.0 (iPhone; CPU iPhone OS 14_0 like Mac OS X) AppleWebKit/605.1.15", true}},
        {"iPhone 14", {390, 844, 3.0,
            "Mozilla/5.0 (iPhone; CPU iPhone OS 16_0 like Mac OS X) AppleWebKit/605.1.15", true}},
        {"iPad", {768, 1024, 2.0,
            "Mozilla/5.0 (iPad; CPU OS 14_0 like Mac OS X) AppleWebKit/605.1.15", true}},
        {"Pixel 5", {393, 851, 2.75,
            "Mozilla/5.0 (Linux; Android 11; Pixel 5) AppleWebKit/537.36", true}},
        {"Galaxy S21", {360, 800, 3.0,
            "Mozilla/5.0 (Linux; Android 11; SM-G991B) AppleWebKit/537.36", true}},
    };

    auto it = devices.find(deviceName);
    if (it == devices.end()) {
        lastError_ = "Unknown device: " + deviceName;
        return false;
    }

    const auto& preset = it->second;

    auto resp = client_->Emulation.setDeviceMetricsOverride(
        preset.width, preset.height, preset.scale, preset.mobile);
    if (resp.hasError) {
        lastError_ = resp.errorMessage;
        return false;
    }

    return setUserAgent(preset.userAgent);
}

bool QuickPage::setUserAgent(const std::string& userAgent) {
    auto resp = client_->Emulation.setUserAgentOverride(userAgent);
    if (resp.hasError) {
        lastError_ = resp.errorMessage;
        return false;
    }
    return true;
}


bool QuickPage::blockUrls(const std::vector<std::string>& patterns) {
    auto resp = client_->Network.setBlockedURLs(patterns);
    if (resp.hasError) {
        lastError_ = resp.errorMessage;
        return false;
    }
    return true;
}

bool QuickPage::setHeaders(const std::map<std::string, std::string>& headers) {
    auto resp = client_->Network.setExtraHTTPHeaders(headers);
    if (resp.hasError) {
        lastError_ = resp.errorMessage;
        return false;
    }
    return true;
}

bool QuickPage::enableInterception(std::function<void(const std::string&,
                                                      const std::string&,
                                                      const std::string&)> handler) {
    client_->Fetch.enable({}, false);

    client_->Fetch.onRequestPaused([this, handler](const std::string& requestId,
                                                    const JsonValue& request,
                                                    const std::string& frameId,
                                                    const std::string& resourceType,
                                                    const JsonValue& responseErrorReason,
                                                    int responseStatusCode,
                                                    const std::string& responseStatusText,
                                                    const JsonValue& responseHeaders,
                                                    const std::string& networkId) {
        std::string url = request["url"].getString();
        std::string method = request["method"].getString();

        if (handler) {
            handler(requestId, url, method);
        }

        client_->Fetch.continueRequestAsync(requestId);
    });

    return true;
}


void QuickPage::onConsole(std::function<void(const std::string&, const std::string&)> handler) {
    client_->Console.enable();
    client_->Runtime.enable();

    client_->Runtime.onConsoleAPICalled([handler](const std::string& type,
                                                    const JsonValue& args,
                                                    int executionContextId,
                                                    double timestamp,
                                                    const JsonValue& stackTrace) {
        if (!handler) return;

        std::string text;
        if (args.isArray() && args.size() > 0) {
            const auto& firstArg = args[0];
            auto* value = firstArg.find("value");
            if (value) {
                if (value->isString()) text = value->asString();
                else text = value->serialize();
            }
        }

        handler(type, text);
    });
}


void QuickPage::autoAcceptDialogs(bool accept, const std::string& promptText) {
    autoAccept_ = accept;
    promptText_ = promptText;

    client_->Page.onJavascriptDialogOpening([this](const std::string& url,
                                                    const std::string& message,
                                                    const std::string& type,
                                                    bool hasBrowserHandler,
                                                    const std::string& defaultPrompt) {
        client_->Page.handleJavaScriptDialog(autoAccept_, promptText_);
    });
}


bool QuickPage::isConnected() const {
    return !closed_ && client_ && client_->isConnected();
}

bool QuickPage::close() {
    if (closed_) {
        return true;  
    }

    if (!client_ || !client_->isConnected()) {
        closed_ = true;
        return true;
    }

    if (browser_) {
        auto resp = browser_->browserClient().Target.closeTarget(targetId_);
        if (resp.hasError) {
            lastError_ = resp.errorMessage;
            return false;
        }
    }

    client_->disconnect();
    closed_ = true;
    return true;
}

bool QuickPage::bringToFront() {
    if (!client_ || !client_->isConnected()) {
        lastError_ = "Page not connected";
        return false;
    }

    auto resp = client_->Page.bringToFront();
    if (resp.hasError) {
        lastError_ = resp.errorMessage;
        return false;
    }
    return true;
}


QuickContext::QuickContext(QuickBrowser* browser, const std::string& contextId)
    : browser_(browser), contextId_(contextId) {
}

QuickContext::~QuickContext() {
    std::lock_guard<std::mutex> lock(mutex_);
    pages_.clear();
}

PageResult QuickContext::newPage(const std::string& url) {
    return browser_->createPageInContext(url, contextId_);
}

std::vector<QuickPage*> QuickContext::pages() {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<QuickPage*> result;
    for (auto& page : pages_) {
        if (page && page->isConnected()) {
            result.push_back(page.get());
        }
    }
    return result;
}

bool QuickContext::close() {
    
    disableFetch();

    
    {
        std::lock_guard<std::mutex> lock(mutex_);
        for (auto& page : pages_) {
            if (page) {
                page->close();
            }
        }
        pages_.clear();
    }

    
    if (!isDefault()) {
        auto resp = browser_->browserClient().Target.disposeBrowserContext(contextId_);
        if (resp.hasError) {
            return false;
        }
    }

    return true;
}

bool QuickContext::enableFetch(FetchHandler handler,
                                const std::vector<RequestPattern>& patterns) {
    if (!handler) return false;

    std::lock_guard<std::mutex> lock(mutex_);

    
    fetchHandler_ = handler;
    fetchPatterns_ = patterns;
    fetchEnabled_ = true;

    
    for (auto& page : pages_) {
        if (page && page->isConnected()) {
            applyFetchToPage(page.get());
        }
    }

    return true;
}

void QuickContext::disableFetch() {
    std::lock_guard<std::mutex> lock(mutex_);

    if (!fetchEnabled_) return;

    
    for (auto& page : pages_) {
        if (page && page->isConnected()) {
            page->client().Fetch.disable();
        }
    }

    fetchEnabled_ = false;
    fetchHandler_ = nullptr;
    fetchPatterns_.clear();
}

void QuickContext::applyFetchToPage(QuickPage* page) {
    
    if (!page || !fetchEnabled_ || !fetchHandler_) return;

    
    page->client().Fetch.enable(fetchPatterns_, false);

    
    FetchHandler handler = fetchHandler_;

    
    page->client().Fetch.onRequestPaused([page, handler](
            const std::string& requestId,
            const JsonValue& request,
            const std::string& frameId,
            const std::string& resourceType,
            const JsonValue& responseErrorReason,
            int responseStatusCode,
            const std::string& responseStatusText,
            const JsonValue& responseHeaders,
            const std::string& networkId) {

        
        FetchRequest req;
        req.requestId = requestId;
        req.url = request["url"].getString();
        req.method = request["method"].getString();
        req.resourceType = resourceType;
        req.headers = request["headers"];
        req.postData = request["postData"].getString();
        req.page = page;

        
        FetchAction action(page, requestId);

        
        bool handled = handler(req, action);

        
        if (!handled) {
            action.continueRequest();
        }
    });
}


QuickBrowser::QuickBrowser(std::unique_ptr<ChromeLauncher> launcher, const CDPClientConfig& config)
    : launcher_(std::move(launcher))
    , config_(config)
    , browserClient_(config) {
}

QuickBrowser::~QuickBrowser() {
    
    std::lock_guard<std::mutex> lock(mutex_);
    incognitoContexts_.clear();
    defaultContext_.reset();

    browserClient_.disconnect();

    
}

PageResult QuickBrowser::newPage(const std::string& url) {
    return defaultContext_->newPage(url);
}

std::vector<QuickPage*> QuickBrowser::pages() {
    std::vector<QuickPage*> result;

    
    auto defaultPages = defaultContext_->pages();
    result.insert(result.end(), defaultPages.begin(), defaultPages.end());

    
    std::lock_guard<std::mutex> lock(mutex_);
    for (auto& ctx : incognitoContexts_) {
        auto ctxPages = ctx->pages();
        result.insert(result.end(), ctxPages.begin(), ctxPages.end());
    }

    return result;
}

std::vector<CDPTarget> QuickBrowser::listPages() {
    auto targets = browserClient_.listTargets();
    std::vector<CDPTarget> pages;
    for (const auto& target : targets) {
        if (target.type == "page") {
            pages.push_back(target);
        }
    }
    return pages;
}

PageResult QuickBrowser::connectToPage(const CDPTarget& target) {
    PageResult result;

    if (target.webSocketDebuggerUrl.empty()) {
        result.error = "Target has no WebSocket URL";
        return result;
    }

    
    auto pageClient = std::make_unique<CDPClient>(config_);
    if (!pageClient->connect(target.webSocketDebuggerUrl)) {
        result.error = "Failed to connect to target: " + pageClient->lastError();
        return result;
    }

    auto page = std::unique_ptr<QuickPage>(new QuickPage(std::move(pageClient), target.id, this));

    std::lock_guard<std::mutex> lock(defaultContext_->mutex_);
    defaultContext_->pages_.push_back(std::move(page));
    result.page = defaultContext_->pages_.back().get();
    return result;
}

PageResult QuickBrowser::connectToPage(int index) {
    PageResult result;
    auto pages = listPages();
    if (index < 0 || index >= static_cast<int>(pages.size())) {
        result.error = "Page index out of range";
        return result;
    }
    return connectToPage(pages[index]);
}

ContextResult QuickBrowser::newContext(const ContextOptions& options) {
    ContextResult result;

    if (!isConnected()) {
        result.error = "Browser not connected";
        return result;
    }

    auto resp = browserClient_.Target.createBrowserContext(
        true,  
        options.proxyServer,
        options.proxyBypassList
    );

    if (resp.hasError) {
        result.error = resp.errorMessage;
        return result;
    }

    std::string contextId = resp.result["browserContextId"].getString();

    std::lock_guard<std::mutex> lock(mutex_);
    incognitoContexts_.push_back(std::unique_ptr<QuickContext>(new QuickContext(this, contextId)));
    result.context = incognitoContexts_.back().get();

    
    if (fetchEnabled_ && fetchHandler_) {
        result.context->enableFetch(fetchHandler_, fetchPatterns_);
    }

    return result;
}

std::vector<QuickContext*> QuickBrowser::contexts() {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<QuickContext*> result;
    result.push_back(defaultContext_.get());
    for (auto& ctx : incognitoContexts_) {
        result.push_back(ctx.get());
    }
    return result;
}

bool QuickBrowser::enableFetch(FetchHandler handler,
                                const std::vector<RequestPattern>& patterns) {
    if (!handler) return false;

    
    {
        std::lock_guard<std::mutex> lock(mutex_);
        fetchHandler_ = handler;
        fetchPatterns_ = patterns;
        fetchEnabled_ = true;
    }

    
    defaultContext_->enableFetch(handler, patterns);

    
    {
        std::lock_guard<std::mutex> lock(mutex_);
        for (auto& ctx : incognitoContexts_) {
            ctx->enableFetch(handler, patterns);
        }
    }

    return true;
}

void QuickBrowser::disableFetch() {
    
    {
        std::lock_guard<std::mutex> lock(mutex_);
        fetchEnabled_ = false;
        fetchHandler_ = nullptr;
        fetchPatterns_.clear();
    }

    
    defaultContext_->disableFetch();

    
    {
        std::lock_guard<std::mutex> lock(mutex_);
        for (auto& ctx : incognitoContexts_) {
            ctx->disableFetch();
        }
    }
}

std::string QuickBrowser::version() {
    if (!isConnected()) return "";

    auto resp = browserClient_.Browser.getVersion();
    if (resp.hasError) return "";

    return resp.result["product"].getString();
}

std::string QuickBrowser::userAgent() {
    if (!isConnected()) return "";

    auto resp = browserClient_.Browser.getVersion();
    if (resp.hasError) return "";

    return resp.result["userAgent"].getString();
}

bool QuickBrowser::isConnected() const {
    return browserClient_.isConnected();
}

bool QuickBrowser::close() {
    if (!isConnected()) {
        return true;
    }

    auto resp = browserClient_.Browser.close();
    if (resp.hasError) {
        lastError_ = resp.errorMessage;
        return false;
    }

    browserClient_.disconnect();
    return true;
}

int QuickBrowser::debuggingPort() const {
    if (launcher_) {
        return launcher_->debuggingPort();
    }
    return config_.port;
}

PageResult QuickBrowser::createPageInContext(const std::string& url, const std::string& contextId) {
    PageResult result;

    if (!isConnected()) {
        result.error = "Browser not connected";
        return result;
    }

    
    auto createResp = browserClient_.Target.createTarget(
        url.empty() ? "about:blank" : url,
        0,  
        0,  
        contextId,
        false,  
        false,  
        false   
    );

    if (createResp.hasError) {
        result.error = createResp.errorMessage;
        return result;
    }

    std::string targetId = createResp.result["targetId"].getString();

    
    auto targets = browserClient_.listTargets();
    std::string wsUrl;
    for (const auto& target : targets) {
        if (target.id == targetId) {
            wsUrl = target.webSocketDebuggerUrl;
            break;
        }
    }

    if (wsUrl.empty()) {
        browserClient_.Target.closeTarget(targetId);
        result.error = "Failed to find WebSocket URL for new target";
        return result;
    }

    
    auto pageClient = std::make_unique<CDPClient>(config_);
    if (!pageClient->connect(wsUrl)) {
        browserClient_.Target.closeTarget(targetId);
        result.error = "Failed to connect to new target: " + pageClient->lastError();
        return result;
    }

    auto page = std::unique_ptr<QuickPage>(new QuickPage(std::move(pageClient), targetId, this));

    
    QuickContext* ctx = nullptr;
    if (contextId.empty()) {
        ctx = defaultContext_.get();
    } else {
        std::lock_guard<std::mutex> lock(mutex_);
        for (auto& incognitoCtx : incognitoContexts_) {
            if (incognitoCtx->id() == contextId) {
                ctx = incognitoCtx.get();
                break;
            }
        }
    }

    if (!ctx) {
        result.error = "Context not found";
        return result;
    }

    std::lock_guard<std::mutex> ctxLock(ctx->mutex_);
    ctx->pages_.push_back(std::move(page));
    result.page = ctx->pages_.back().get();

    
    if (ctx->fetchEnabled_) {
        ctx->applyFetchToPage(result.page);
    }

    
    if (!url.empty() && url != "about:blank") {
        result.page->waitNavigation(30000);
    }

    return result;
}


LaunchResult launch(const ChromeLaunchOptions& options) {
    LaunchResult result;

    
    auto launcher = std::make_unique<ChromeLauncher>(options);
    if (!launcher->launch()) {
        result.error = launcher->lastError();
        return result;
    }

    
    CDPClientConfig config;
    config.host = launcher->options().host;
    config.port = launcher->debuggingPort();

    
    result.browser = std::unique_ptr<QuickBrowser>(new QuickBrowser(std::move(launcher), config));

    
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    
    if (!result.browser->browserClient_.connectToBrowser()) {
        result.error = "Failed to connect to browser endpoint";
        result.browser.reset();
        return result;
    }

    
    result.browser->defaultContext_ = std::unique_ptr<QuickContext>(new QuickContext(result.browser.get(), ""));

    return result;
}

LaunchResult launchHeadless() {
    return launch(ChromeLaunchOptions::headlessMode());
}

LaunchResult launchWithViewport(int width, int height) {
    ChromeLaunchOptions options;
    options.windowWidth = width;
    options.windowHeight = height;
    return launch(options);
}

LaunchResult connect(const std::string& host, int port) {
    LaunchResult result;

    CDPClientConfig config;
    config.host = host;
    config.port = port;

    result.browser = std::unique_ptr<QuickBrowser>(new QuickBrowser(nullptr, config));

    if (!result.browser->browserClient_.connectToBrowser()) {
        result.error = "Failed to connect to browser at " + host + ":" + std::to_string(port);
        result.browser.reset();
        return result;
    }

    result.browser->defaultContext_ = std::unique_ptr<QuickContext>(new QuickContext(result.browser.get(), ""));

    return result;
}

LaunchResult connectWs(const std::string& wsUrl) {
    LaunchResult result;

    CDPClientConfig config;
    result.browser = std::unique_ptr<QuickBrowser>(new QuickBrowser(nullptr, config));

    if (!result.browser->browserClient_.connect(wsUrl)) {
        result.error = "Failed to connect to WebSocket: " + wsUrl;
        result.browser.reset();
        return result;
    }

    result.browser->defaultContext_ = std::unique_ptr<QuickContext>(new QuickContext(result.browser.get(), ""));

    return result;
}

} 
} 
