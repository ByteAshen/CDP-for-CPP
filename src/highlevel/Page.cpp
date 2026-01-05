

#include "cdp/highlevel/Page.hpp"
#include "cdp/core/Base64.hpp"
#include <fstream>
#include <thread>
#include <regex>

namespace cdp {
namespace highlevel {

Page::Page(CDPClient& client) : client_(client) {
    
    if (!client_.Page.isEnabled()) {
        client_.Page.enable();
    }
    if (!client_.DOM.isEnabled()) {
        client_.DOM.enable();
    }
    if (!client_.Runtime.isEnabled()) {
        client_.Runtime.enable();
    }
    if (!client_.Input.isEnabled()) {
        
    }

    domainEnabled_ = true;
}

Page::~Page() {
    
}


Result<void> Page::navigate(const std::string& url, int timeoutMs) {
    NavigateOptions options;
    options.timeoutMs = timeoutMs;
    options.waitUntil = WaitUntil::Load;
    return navigate(url, options);
}

Result<void> Page::navigate(const std::string& url, const NavigateOptions& options) {
    
    std::future<CDPEvent> loadFuture;
    std::future<CDPEvent> domFuture;

    switch (options.waitUntil) {
        case WaitUntil::Load:
            loadFuture = client_.Page.once("loadEventFired");
            break;
        case WaitUntil::DOMContentLoaded:
            domFuture = client_.Page.once("domContentEventFired");
            break;
        case WaitUntil::NetworkIdle0:
        case WaitUntil::NetworkIdle2:
            
            loadFuture = client_.Page.once("loadEventFired");
            break;
        case WaitUntil::None:
            
            break;
    }

    
    CDPResponse resp;
    if (!options.referer.empty()) {
        resp = client_.Page.navigate(url, options.referer);
    } else {
        resp = client_.Page.navigate(url);
    }

    if (resp.hasError) {
        return Result<void>::failure(resp.errorCode, resp.errorMessage);
    }

    frameId_ = resp.result["frameId"].getString();

    
    switch (options.waitUntil) {
        case WaitUntil::Load: {
            auto status = loadFuture.wait_for(std::chrono::milliseconds(options.timeoutMs));
            if (status != std::future_status::ready) {
                return Result<void>::failure(Error::navigationTimeout(options.timeoutMs));
            }
            break;
        }
        case WaitUntil::DOMContentLoaded: {
            auto status = domFuture.wait_for(std::chrono::milliseconds(options.timeoutMs));
            if (status != std::future_status::ready) {
                return Result<void>::failure(Error::navigationTimeout(options.timeoutMs));
            }
            break;
        }
        case WaitUntil::NetworkIdle0: {
            
            auto status = loadFuture.wait_for(std::chrono::milliseconds(options.timeoutMs));
            if (status != std::future_status::ready) {
                return Result<void>::failure(Error::navigationTimeout(options.timeoutMs));
            }
            
            auto idleResult = waitForNetworkIdle(500, options.timeoutMs);
            if (!idleResult) {
                return idleResult;
            }
            break;
        }
        case WaitUntil::NetworkIdle2: {
            
            auto status = loadFuture.wait_for(std::chrono::milliseconds(options.timeoutMs));
            if (status != std::future_status::ready) {
                return Result<void>::failure(Error::navigationTimeout(options.timeoutMs));
            }
            
            auto idleResult = waitForNetworkIdle(200, options.timeoutMs);
            if (!idleResult) {
                
            }
            break;
        }
        case WaitUntil::None:
            
            break;
    }

    
    rootNodeId_ = 0;

    return Result<void>::success();
}

Result<void> Page::navigateNoWait(const std::string& url) {
    auto resp = client_.Page.navigate(url);
    if (resp.hasError) {
        return Result<void>::failure(resp.errorCode, resp.errorMessage);
    }
    frameId_ = resp.result["frameId"].getString();
    rootNodeId_ = 0;
    return Result<void>::success();
}

Result<void> Page::reload(int timeoutMs) {
    NavigateOptions options;
    options.timeoutMs = timeoutMs;
    return reload(options);
}

Result<void> Page::reload(const NavigateOptions& options) {
    std::future<CDPEvent> loadFuture;
    std::future<CDPEvent> domFuture;

    switch (options.waitUntil) {
        case WaitUntil::Load:
            loadFuture = client_.Page.once("loadEventFired");
            break;
        case WaitUntil::DOMContentLoaded:
            domFuture = client_.Page.once("domContentEventFired");
            break;
        case WaitUntil::NetworkIdle0:
        case WaitUntil::NetworkIdle2:
            loadFuture = client_.Page.once("loadEventFired");
            break;
        case WaitUntil::None:
            break;
    }

    auto resp = client_.Page.reload();
    if (resp.hasError) {
        return Result<void>::failure(resp.errorCode, resp.errorMessage);
    }

    switch (options.waitUntil) {
        case WaitUntil::Load: {
            auto status = loadFuture.wait_for(std::chrono::milliseconds(options.timeoutMs));
            if (status != std::future_status::ready) {
                return Result<void>::failure(Error::navigationTimeout(options.timeoutMs));
            }
            break;
        }
        case WaitUntil::DOMContentLoaded: {
            auto status = domFuture.wait_for(std::chrono::milliseconds(options.timeoutMs));
            if (status != std::future_status::ready) {
                return Result<void>::failure(Error::navigationTimeout(options.timeoutMs));
            }
            break;
        }
        case WaitUntil::NetworkIdle0:
        case WaitUntil::NetworkIdle2: {
            auto status = loadFuture.wait_for(std::chrono::milliseconds(options.timeoutMs));
            if (status != std::future_status::ready) {
                return Result<void>::failure(Error::navigationTimeout(options.timeoutMs));
            }
            waitForNetworkIdle(500, options.timeoutMs);
            break;
        }
        case WaitUntil::None:
            break;
    }

    rootNodeId_ = 0;
    return Result<void>::success();
}

Result<void> Page::goBack(int timeoutMs) {
    auto loadFuture = client_.Page.once("loadEventFired");

    auto histResp = client_.Page.getNavigationHistory();
    if (histResp.hasError) {
        return Result<void>::failure(histResp.errorCode, histResp.errorMessage);
    }

    int currentIndex = histResp.result["currentIndex"].getInt();
    if (currentIndex <= 0) {
        return Result<void>::failure(
            Error(ErrorCode::NavigationFailed, "Cannot go back - no previous page"));
    }

    auto& entries = histResp.result["entries"];
    int targetId = entries[currentIndex - 1]["id"].getInt();

    auto resp = client_.Page.navigateToHistoryEntry(targetId);
    if (resp.hasError) {
        return Result<void>::failure(resp.errorCode, resp.errorMessage);
    }

    auto status = loadFuture.wait_for(std::chrono::milliseconds(timeoutMs));
    if (status != std::future_status::ready) {
        return Result<void>::failure(Error::navigationTimeout(timeoutMs));
    }

    rootNodeId_ = 0;
    return Result<void>::success();
}

Result<void> Page::goForward(int timeoutMs) {
    auto loadFuture = client_.Page.once("loadEventFired");

    auto histResp = client_.Page.getNavigationHistory();
    if (histResp.hasError) {
        return Result<void>::failure(histResp.errorCode, histResp.errorMessage);
    }

    int currentIndex = histResp.result["currentIndex"].getInt();
    auto& entries = histResp.result["entries"];

    if (currentIndex >= static_cast<int>(entries.size()) - 1) {
        return Result<void>::failure(
            Error(ErrorCode::NavigationFailed, "Cannot go forward - no next page"));
    }

    int targetId = entries[currentIndex + 1]["id"].getInt();

    auto resp = client_.Page.navigateToHistoryEntry(targetId);
    if (resp.hasError) {
        return Result<void>::failure(resp.errorCode, resp.errorMessage);
    }

    auto status = loadFuture.wait_for(std::chrono::milliseconds(timeoutMs));
    if (status != std::future_status::ready) {
        return Result<void>::failure(Error::navigationTimeout(timeoutMs));
    }

    rootNodeId_ = 0;
    return Result<void>::success();
}

std::string Page::url() {
    auto resp = client_.Runtime.evaluate("window.location.href");
    if (resp.hasError) return "";
    return resp.result["result"]["value"].getString();
}

std::string Page::title() {
    auto resp = client_.Runtime.evaluate("document.title");
    if (resp.hasError) return "";
    return resp.result["result"]["value"].getString();
}


Result<void> Page::click(const std::string& selector, int timeoutMs) {
    auto elemResult = waitForSelector(selector, timeoutMs);
    if (!elemResult) return Result<void>::failure(elemResult.error());

    return elemResult->click();
}

Result<void> Page::type(const std::string& selector, const std::string& text, int timeoutMs) {
    auto elemResult = waitForSelector(selector, timeoutMs);
    if (!elemResult) return Result<void>::failure(elemResult.error());

    
    auto clearResult = clear(selector, timeoutMs);
    

    auto freshElem = waitForSelector(selector, timeoutMs);
    if (!freshElem) return Result<void>::failure(freshElem.error());

    return freshElem->type(text);
}

Result<void> Page::type(const std::string& selector, const std::string& text, const TypeOptions& options, int timeoutMs) {
    
    if (options.clearFirst) {
        auto clearResult = clear(selector, timeoutMs);
        
    }

    
    auto elemResult = waitForSelector(selector, timeoutMs);
    if (!elemResult) return Result<void>::failure(elemResult.error());

    
    auto focusResult = elemResult->focus();
    if (!focusResult) return Result<void>::failure(focusResult.error());

    if (options.useKeyEvents) {
        
        
        for (char c : text) {
            std::string charStr(1, c);

            
            int vkCode = static_cast<unsigned char>(c);
            if (c >= 'a' && c <= 'z') vkCode -= 32;  

            std::string keyCode;
            if (c >= 'a' && c <= 'z') {
                keyCode = std::string("Key") + static_cast<char>(c - 32);
            } else if (c >= 'A' && c <= 'Z') {
                keyCode = std::string("Key") + c;
            } else if (c >= '0' && c <= '9') {
                keyCode = std::string("Digit") + c;
            } else if (c == ' ') {
                keyCode = "Space";
            } else {
                keyCode = charStr;
            }

            
            client_.Input.dispatchKeyEvent("keyDown", 0, 0, "", "", "", keyCode, charStr, vkCode);

            
            client_.Input.dispatchKeyEvent("char", 0, 0, charStr, charStr, "", "", "", 0);

            
            client_.Input.dispatchKeyEvent("keyUp", 0, 0, "", "", "", keyCode, charStr, vkCode);

            
            if (options.delayMs > 0) {
                std::this_thread::sleep_for(std::chrono::milliseconds(options.delayMs));
            }
        }
        return Result<void>::success();
    } else {
        
        auto resp = client_.Input.insertText(text);
        if (resp.hasError) {
            return Result<void>::failure(resp.errorCode, resp.errorMessage);
        }
        return Result<void>::success();
    }
}

Result<void> Page::typeAppend(const std::string& selector, const std::string& text, int timeoutMs) {
    auto elemResult = waitForSelector(selector, timeoutMs);
    if (!elemResult) return Result<void>::failure(elemResult.error());

    return elemResult->type(text);
}

Result<std::string> Page::getText(const std::string& selector, int timeoutMs) {
    auto elemResult = waitForSelector(selector, timeoutMs);
    if (!elemResult) return Result<std::string>::failure(elemResult.error());

    return elemResult->textContent();
}

Result<std::string> Page::getHTML(const std::string& selector, int timeoutMs) {
    auto elemResult = waitForSelector(selector, timeoutMs);
    if (!elemResult) return Result<std::string>::failure(elemResult.error());

    return elemResult->innerHTML();
}

Result<std::string> Page::getAttribute(const std::string& selector, const std::string& name, int timeoutMs) {
    auto elemResult = waitForSelector(selector, timeoutMs);
    if (!elemResult) return Result<std::string>::failure(elemResult.error());

    return elemResult->getAttribute(name);
}

Result<std::string> Page::getValue(const std::string& selector, int timeoutMs) {
    auto elemResult = waitForSelector(selector, timeoutMs);
    if (!elemResult) return Result<std::string>::failure(elemResult.error());

    return elemResult->value();
}

Result<void> Page::setValue(const std::string& selector, const std::string& value, int timeoutMs) {
    auto elemResult = waitForSelector(selector, timeoutMs);
    if (!elemResult) return Result<void>::failure(elemResult.error());

    return elemResult->setValue(value);
}

Result<void> Page::focus(const std::string& selector, int timeoutMs) {
    auto elemResult = waitForSelector(selector, timeoutMs);
    if (!elemResult) return Result<void>::failure(elemResult.error());

    return elemResult->focus();
}

Result<void> Page::hover(const std::string& selector, int timeoutMs) {
    auto elemResult = waitForSelector(selector, timeoutMs);
    if (!elemResult) return Result<void>::failure(elemResult.error());

    return elemResult->hover();
}

Result<void> Page::check(const std::string& selector, bool checked, int timeoutMs) {
    auto elemResult = waitForSelector(selector, timeoutMs);
    if (!elemResult) return Result<void>::failure(elemResult.error());

    auto isCheckedResult = elemResult->isChecked();
    if (!isCheckedResult) return Result<void>::failure(isCheckedResult.error());

    if (isCheckedResult.value() != checked) {
        return elemResult->click();
    }

    return Result<void>::success();
}

Result<void> Page::select(const std::string& selector, const std::string& value, int timeoutMs) {
    auto elemResult = waitForSelector(selector, timeoutMs);
    if (!elemResult) return Result<void>::failure(elemResult.error());

    
    std::string escaped;
    for (char c : value) {
        if (c == '\\' || c == '\'') escaped += '\\';
        escaped += c;
    }

    auto evalResult = elemResult->evaluate(
        "(function() { "
        "  var opt = Array.from(this.options).find(o => o.value === '" + escaped + "' || o.text === '" + escaped + "'); "
        "  if (opt) { this.value = opt.value; this.dispatchEvent(new Event('change', {bubbles: true})); return true; } "
        "  return false; "
        "})()"
    );

    if (!evalResult) return Result<void>::failure(evalResult.error());

    if (!evalResult->getBool()) {
        return Result<void>::failure("Option not found: " + value);
    }

    return Result<void>::success();
}

Result<void> Page::clear(const std::string& selector, int timeoutMs) {
    auto elemResult = waitForSelector(selector, timeoutMs);
    if (!elemResult) return Result<void>::failure(elemResult.error());

    
    auto focusResult = elemResult->focus();
    if (!focusResult) return focusResult;

    
    client_.Input.dispatchKeyEvent("keyDown", 2, 0, "", "", "", "KeyA", "a", 65);  
    client_.Input.dispatchKeyEvent("keyUp", 2, 0, "", "", "", "KeyA", "a", 65);

    
    client_.Input.dispatchKeyEvent("keyDown", 0, 0, "", "", "", "Backspace", "Backspace", 8);
    client_.Input.dispatchKeyEvent("keyUp", 0, 0, "", "", "", "Backspace", "Backspace", 8);

    
    rootNodeId_ = 0;

    return Result<void>::success();
}

Result<void> Page::press(const std::string& key, int ) {
    
    auto resp = client_.Input.keyPress(key);
    if (resp.hasError) {
        return Result<void>::failure(resp.errorCode, resp.errorMessage);
    }

    return Result<void>::success();
}


Result<ElementHandle> Page::querySelector(const std::string& selector) {
    auto docResult = ensureDocument();
    if (!docResult) return Result<ElementHandle>::failure(docResult.error());

    auto resp = client_.DOM.querySelector(rootNodeId_, selector);
    if (resp.hasError) {
        return Result<ElementHandle>::failure(resp.errorCode, resp.errorMessage);
    }

    int nodeId = resp.result["nodeId"].getInt();
    if (nodeId == 0) {
        return Result<ElementHandle>::failure(Error::elementNotFound(selector));
    }

    return Result<ElementHandle>(ElementHandle(client_, nodeId));
}

Result<std::vector<ElementHandle>> Page::querySelectorAll(const std::string& selector) {
    auto docResult = ensureDocument();
    if (!docResult) return Result<std::vector<ElementHandle>>::failure(docResult.error());

    auto resp = client_.DOM.querySelectorAll(rootNodeId_, selector);
    if (resp.hasError) {
        return Result<std::vector<ElementHandle>>::failure(resp.errorCode, resp.errorMessage);
    }

    std::vector<ElementHandle> elements;
    auto& nodeIds = resp.result["nodeIds"];
    if (nodeIds.isArray()) {
        for (size_t i = 0; i < nodeIds.size(); ++i) {
            int nodeId = nodeIds[i].getInt();
            if (nodeId > 0) {
                elements.emplace_back(client_, nodeId);
            }
        }
    }

    return Result<std::vector<ElementHandle>>(std::move(elements));
}

bool Page::exists(const std::string& selector) {
    auto result = querySelector(selector);
    return result.ok();
}

int Page::count(const std::string& selector) {
    auto result = querySelectorAll(selector);
    if (!result) return 0;
    return static_cast<int>(result->size());
}


template<typename Func>
Result<void> Page::pollWithBackoff(const WaitOptions& options, const std::string& description, Func&& condition) {
    auto startTime = std::chrono::steady_clock::now();
    int currentPollInterval = options.pollIntervalMs;

    while (true) {
        
        if (options.cancellationToken && options.cancellationToken->isCancelled()) {
            return Result<void>::failure(Error::cancelled());
        }

        
        if (condition()) {
            return Result<void>::success();
        }

        
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - startTime).count();

        if (options.timeoutMs >= 0 && elapsed >= options.timeoutMs) {
            return Result<void>::failure(Error::waitTimeout(description, options.timeoutMs));
        }

        
        int sleepMs = currentPollInterval;
        if (options.timeoutMs >= 0) {
            auto remainingMs = static_cast<long long>(options.timeoutMs) - elapsed;
            sleepMs = static_cast<int>((std::min)(static_cast<long long>(currentPollInterval), remainingMs));
        }
        if (sleepMs < 1) sleepMs = 1;

        std::this_thread::sleep_for(std::chrono::milliseconds(sleepMs));

        
        currentPollInterval = (std::min)(currentPollInterval * 3 / 2, options.maxPollIntervalMs);
    }
}

Result<ElementHandle> Page::waitForSelector(const std::string& selector, int timeoutMs) {
    WaitOptions options;
    options.timeoutMs = timeoutMs;
    return waitForSelector(selector, options);
}

Result<ElementHandle> Page::waitForSelector(const std::string& selector, const WaitOptions& options) {
    auto startTime = std::chrono::steady_clock::now();
    int currentPollInterval = options.pollIntervalMs;

    while (true) {
        
        if (options.cancellationToken && options.cancellationToken->isCancelled()) {
            return Result<ElementHandle>::failure(Error::cancelled());
        }

        rootNodeId_ = 0;  
        auto result = querySelector(selector);
        if (result) {
            return result;
        }

        
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - startTime).count();

        if (options.timeoutMs >= 0 && elapsed >= options.timeoutMs) {
            return Result<ElementHandle>::failure(
                Error::waitTimeout("selector '" + selector + "'", options.timeoutMs));
        }

        
        int sleepMs = currentPollInterval;
        if (options.timeoutMs >= 0) {
            auto remainingMs = static_cast<long long>(options.timeoutMs) - elapsed;
            sleepMs = static_cast<int>((std::min)(static_cast<long long>(currentPollInterval), remainingMs));
        }
        if (sleepMs < 1) sleepMs = 1;

        std::this_thread::sleep_for(std::chrono::milliseconds(sleepMs));
        currentPollInterval = (std::min)(currentPollInterval * 3 / 2, options.maxPollIntervalMs);
    }
}

Result<ElementHandle> Page::waitForVisible(const std::string& selector, int timeoutMs) {
    WaitOptions options;
    options.timeoutMs = timeoutMs;
    options.visible = true;
    return waitForVisible(selector, options);
}

Result<ElementHandle> Page::waitForVisible(const std::string& selector, const WaitOptions& options) {
    auto startTime = std::chrono::steady_clock::now();
    int currentPollInterval = options.pollIntervalMs;

    while (true) {
        
        if (options.cancellationToken && options.cancellationToken->isCancelled()) {
            return Result<ElementHandle>::failure(Error::cancelled());
        }

        rootNodeId_ = 0;
        auto elemResult = querySelector(selector);
        if (elemResult) {
            auto visResult = elemResult->isVisible();
            if (visResult && visResult.value()) {
                return elemResult;
            }
        }

        
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - startTime).count();

        if (options.timeoutMs >= 0 && elapsed >= options.timeoutMs) {
            return Result<ElementHandle>::failure(
                Error::waitTimeout("visible '" + selector + "'", options.timeoutMs));
        }

        
        int sleepMs = currentPollInterval;
        if (options.timeoutMs >= 0) {
            auto remainingMs = static_cast<long long>(options.timeoutMs) - elapsed;
            sleepMs = static_cast<int>((std::min)(static_cast<long long>(currentPollInterval), remainingMs));
        }
        if (sleepMs < 1) sleepMs = 1;

        std::this_thread::sleep_for(std::chrono::milliseconds(sleepMs));
        currentPollInterval = (std::min)(currentPollInterval * 3 / 2, options.maxPollIntervalMs);
    }
}

Result<void> Page::waitForHidden(const std::string& selector, int timeoutMs) {
    WaitOptions options;
    options.timeoutMs = timeoutMs;
    return waitForHidden(selector, options);
}

Result<void> Page::waitForHidden(const std::string& selector, const WaitOptions& options) {
    return pollWithBackoff(options, "hidden '" + selector + "'", [&]() {
        rootNodeId_ = 0;
        auto elemResult = querySelector(selector);
        if (!elemResult) {
            
            return true;
        }
        auto visResult = elemResult->isVisible();
        return !visResult || !visResult.value();
    });
}

Result<void> Page::waitForNavigation(int timeoutMs) {
    WaitOptions options;
    options.timeoutMs = timeoutMs;
    return waitForNavigation(options);
}

Result<void> Page::waitForNavigation(const WaitOptions& options) {
    auto loadFuture = client_.Page.once("loadEventFired");

    auto startTime = std::chrono::steady_clock::now();
    int pollInterval = options.pollIntervalMs;

    while (true) {
        
        if (options.cancellationToken && options.cancellationToken->isCancelled()) {
            return Result<void>::failure(Error::cancelled());
        }

        auto status = loadFuture.wait_for(std::chrono::milliseconds(pollInterval));
        if (status == std::future_status::ready) {
            rootNodeId_ = 0;
            return Result<void>::success();
        }

        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - startTime).count();

        if (options.timeoutMs >= 0 && elapsed >= options.timeoutMs) {
            return Result<void>::failure(Error::navigationTimeout(options.timeoutMs));
        }

        pollInterval = (std::min)(pollInterval * 3 / 2, options.maxPollIntervalMs);
    }
}

Result<void> Page::waitForNetworkIdle(int idleTimeMs, int timeoutMs) {
    WaitOptions options;
    options.timeoutMs = timeoutMs;
    return waitForNetworkIdle(idleTimeMs, options);
}

Result<void> Page::waitForNetworkIdle(int idleTimeMs, const WaitOptions& options) {
    
    client_.Network.enable();

    std::atomic<int> inflight{0};
    std::chrono::steady_clock::time_point lastActivity = std::chrono::steady_clock::now();
    std::mutex mtx;

    
    auto requestToken = client_.Network.onScoped("requestWillBeSent", [&](const CDPEvent&) {
        std::lock_guard<std::mutex> lock(mtx);
        inflight++;
        lastActivity = std::chrono::steady_clock::now();
    });

    
    auto responseToken = client_.Network.onScoped("loadingFinished", [&](const CDPEvent&) {
        std::lock_guard<std::mutex> lock(mtx);
        if (inflight > 0) inflight--;
        lastActivity = std::chrono::steady_clock::now();
    });

    auto failedToken = client_.Network.onScoped("loadingFailed", [&](const CDPEvent&) {
        std::lock_guard<std::mutex> lock(mtx);
        if (inflight > 0) inflight--;
        lastActivity = std::chrono::steady_clock::now();
    });

    auto startTime = std::chrono::steady_clock::now();

    while (true) {
        
        if (options.cancellationToken && options.cancellationToken->isCancelled()) {
            return Result<void>::failure(Error::cancelled());
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(options.pollIntervalMs));

        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - startTime).count();

        if (options.timeoutMs >= 0 && elapsed >= options.timeoutMs) {
            return Result<void>::failure(Error::waitTimeout("network idle", options.timeoutMs));
        }

        int currentInflight;
        std::chrono::steady_clock::time_point lastAct;
        {
            std::lock_guard<std::mutex> lock(mtx);
            currentInflight = inflight.load();
            lastAct = lastActivity;
        }

        if (currentInflight == 0) {
            auto idleTime = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastAct).count();
            if (idleTime >= idleTimeMs) {
                return Result<void>::success();
            }
        }
    }
}

Result<void> Page::waitForFunction(const std::string& expression, int timeoutMs) {
    WaitOptions options;
    options.timeoutMs = timeoutMs;
    return waitForFunction(expression, options);
}

Result<void> Page::waitForFunction(const std::string& expression, const WaitOptions& options) {
    return pollWithBackoff(options, "function", [&]() {
        auto resp = client_.Runtime.evaluate("!!(" + expression + ")");
        return !resp.hasError && resp.result["result"]["value"].getBool();
    });
}

Result<void> Page::waitForURL(const std::string& urlPattern, int timeoutMs) {
    WaitOptions options;
    options.timeoutMs = timeoutMs;
    return waitForURL(urlPattern, options);
}

Result<void> Page::waitForURL(const std::string& urlPattern, const WaitOptions& options) {
    
    std::string regexPattern;
    for (char c : urlPattern) {
        switch (c) {
            case '*': regexPattern += ".*"; break;
            case '?': regexPattern += "."; break;
            case '.': case '\\': case '/': case '+': case '^': case '$':
            case '|': case '{': case '}': case '[': case ']': case '(':
            case ')':
                regexPattern += '\\';
                regexPattern += c;
                break;
            default:
                regexPattern += c;
        }
    }

    std::regex pattern(regexPattern);

    return pollWithBackoff(options, "URL '" + urlPattern + "'", [&]() {
        std::string currentUrl = url();
        return std::regex_match(currentUrl, pattern);
    });
}


Result<JsonValue> Page::evaluate(const std::string& expression) {
    auto resp = client_.Runtime.evaluate(expression);
    if (resp.hasError) {
        return Result<JsonValue>::failure(resp.errorCode, resp.errorMessage);
    }

    auto& result = resp.result["result"];
    if (result.contains("exceptionDetails")) {
        return Result<JsonValue>::failure(
            Error(ErrorCode::JavaScriptException,
                  result["exceptionDetails"]["exception"]["description"].getString()));
    }

    return Result<JsonValue>(result["value"]);
}

Result<std::string> Page::evaluateString(const std::string& expression) {
    auto result = evaluate(expression);
    if (!result) return Result<std::string>::failure(result.error());
    return Result<std::string>(result->getString());
}

Result<int> Page::evaluateInt(const std::string& expression) {
    auto result = evaluate(expression);
    if (!result) return Result<int>::failure(result.error());
    return Result<int>(result->getInt());
}

Result<bool> Page::evaluateBool(const std::string& expression) {
    auto result = evaluate(expression);
    if (!result) return Result<bool>::failure(result.error());
    return Result<bool>(result->getBool());
}


Result<std::vector<uint8_t>> Page::screenshot(const ScreenshotOptions& options) {
    cdp::Viewport viewport;
    cdp::Viewport* clipPtr = nullptr;

    if (options.clip.has_value()) {
        viewport.x = options.clip->x;
        viewport.y = options.clip->y;
        viewport.width = options.clip->width;
        viewport.height = options.clip->height;
        viewport.scale = 1.0;
        clipPtr = &viewport;
    }

    auto resp = client_.Page.captureScreenshot(
        options.format,
        options.quality,
        clipPtr,
        true,                      
        options.fullPage,          
        options.optimizeForSpeed,  
        options.timeoutMs          
    );
    if (resp.hasError) {
        return Result<std::vector<uint8_t>>::failure(resp.errorCode, resp.errorMessage);
    }

    std::string base64Data = resp.result["data"].getString();
    return Result<std::vector<uint8_t>>(Base64::decode(base64Data));
}

Result<void> Page::screenshotToFile(const std::string& path, const ScreenshotOptions& options) {
    auto dataResult = screenshot(options);
    if (!dataResult) return Result<void>::failure(dataResult.error());

    std::ofstream file(path, std::ios::binary);
    if (!file) {
        return Result<void>::failure("Failed to open file for writing: " + path);
    }

    file.write(reinterpret_cast<const char*>(dataResult->data()), dataResult->size());
    if (!file) {
        return Result<void>::failure("Failed to write file: " + path);
    }

    return Result<void>::success();
}

Result<std::vector<uint8_t>> Page::pdf() {
    auto resp = client_.Page.printToPDF();
    if (resp.hasError) {
        return Result<std::vector<uint8_t>>::failure(resp.errorCode, resp.errorMessage);
    }

    std::string base64Data = resp.result["data"].getString();
    return Result<std::vector<uint8_t>>(Base64::decode(base64Data));
}

Result<void> Page::pdfToFile(const std::string& path) {
    auto dataResult = pdf();
    if (!dataResult) return Result<void>::failure(dataResult.error());

    std::ofstream file(path, std::ios::binary);
    if (!file) {
        return Result<void>::failure("Failed to open file for writing: " + path);
    }

    file.write(reinterpret_cast<const char*>(dataResult->data()), dataResult->size());
    if (!file) {
        return Result<void>::failure("Failed to write file: " + path);
    }

    return Result<void>::success();
}


Result<void> Page::setViewport(int width, int height, double deviceScaleFactor) {
    auto resp = client_.Emulation.setDeviceMetricsOverride(width, height, deviceScaleFactor, false);
    if (resp.hasError) {
        return Result<void>::failure(resp.errorCode, resp.errorMessage);
    }
    return Result<void>::success();
}

Result<void> Page::setUserAgent(const std::string& userAgent) {
    auto resp = client_.Emulation.setUserAgentOverride(userAgent);
    if (resp.hasError) {
        return Result<void>::failure(resp.errorCode, resp.errorMessage);
    }
    return Result<void>::success();
}

Result<void> Page::emulateDevice(const std::string& deviceName) {
    
    struct DevicePreset {
        int width;
        int height;
        double scale;
        std::string userAgent;
    };

    static const std::map<std::string, DevicePreset> devices = {
        {"iPhone 12", {390, 844, 3.0, "Mozilla/5.0 (iPhone; CPU iPhone OS 14_0 like Mac OS X) AppleWebKit/605.1.15"}},
        {"iPhone 14", {393, 852, 3.0, "Mozilla/5.0 (iPhone; CPU iPhone OS 16_0 like Mac OS X) AppleWebKit/605.1.15"}},
        {"Pixel 5", {393, 851, 2.75, "Mozilla/5.0 (Linux; Android 11; Pixel 5) AppleWebKit/537.36"}},
        {"iPad", {768, 1024, 2.0, "Mozilla/5.0 (iPad; CPU OS 14_0 like Mac OS X) AppleWebKit/605.1.15"}},
        {"Desktop", {1920, 1080, 1.0, "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36"}}
    };

    auto it = devices.find(deviceName);
    if (it == devices.end()) {
        return Result<void>::failure("Unknown device: " + deviceName);
    }

    const auto& preset = it->second;
    auto viewportResult = setViewport(preset.width, preset.height, preset.scale);
    if (!viewportResult) return viewportResult;

    return setUserAgent(preset.userAgent);
}


Result<JsonArray> Page::getCookies() {
    auto resp = client_.Network.getCookies();
    if (resp.hasError) {
        return Result<JsonArray>::failure(resp.errorCode, resp.errorMessage);
    }

    return Result<JsonArray>(resp.result["cookies"].asArray());
}

Result<void> Page::setCookie(const std::string& name, const std::string& value,
                              const std::string& domain, const std::string& path) {
    CookieParam cookie;
    cookie.name = name;
    cookie.value = value;
    cookie.domain = domain.empty() ? url() : domain;
    cookie.path = path;

    auto resp = client_.Network.setCookie(cookie);
    if (resp.hasError) {
        return Result<void>::failure(resp.errorCode, resp.errorMessage);
    }
    return Result<void>::success();
}

Result<void> Page::deleteCookie(const std::string& name, const std::string& domain) {
    std::string cookieDomain = domain.empty() ? url() : domain;

    auto resp = client_.Network.deleteCookies(name, cookieDomain);
    if (resp.hasError) {
        return Result<void>::failure(resp.errorCode, resp.errorMessage);
    }
    return Result<void>::success();
}

Result<void> Page::clearCookies() {
    auto resp = client_.Network.clearBrowserCookies();
    if (resp.hasError) {
        return Result<void>::failure(resp.errorCode, resp.errorMessage);
    }
    return Result<void>::success();
}


Result<int> Page::ensureDocument() {
    if (rootNodeId_ > 0) {
        return Result<int>(rootNodeId_);
    }

    auto resp = client_.DOM.getDocument(-1, false);
    if (resp.hasError) {
        return Result<int>::failure(resp.errorCode, resp.errorMessage);
    }

    rootNodeId_ = resp.result["root"]["nodeId"].getInt();
    if (rootNodeId_ == 0) {
        return Result<int>::failure("Failed to get document root");
    }

    return Result<int>(rootNodeId_);
}

Result<int> Page::findElement(const std::string& selector) {
    auto docResult = ensureDocument();
    if (!docResult) return docResult;

    auto resp = client_.DOM.querySelector(rootNodeId_, selector);
    if (resp.hasError) {
        return Result<int>::failure(resp.errorCode, resp.errorMessage);
    }

    int nodeId = resp.result["nodeId"].getInt();
    if (nodeId == 0) {
        return Result<int>::failure(Error::elementNotFound(selector));
    }

    return Result<int>(nodeId);
}

Result<BoundingBox> Page::getElementBounds(int nodeId) {
    auto resp = client_.DOM.getContentQuads(nodeId);
    if (resp.hasError) {
        return Result<BoundingBox>::failure(resp.errorCode, resp.errorMessage);
    }

    auto& quads = resp.result["quads"];
    if (!quads.isArray() || quads.size() == 0) {
        return Result<BoundingBox>::failure("Element has no visible quads");
    }

    auto& quad = quads[0];
    if (!quad.isArray() || quad.size() < 8) {
        return Result<BoundingBox>::failure("Invalid quad data");
    }

    double x1 = quad[0].asNumber(), y1 = quad[1].asNumber();
    double x2 = quad[2].asNumber(), y2 = quad[3].asNumber();
    double x3 = quad[4].asNumber(), y3 = quad[5].asNumber();
    double x4 = quad[6].asNumber(), y4 = quad[7].asNumber();

    double minX = std::fmin(std::fmin(x1, x2), std::fmin(x3, x4));
    double maxX = std::fmax(std::fmax(x1, x2), std::fmax(x3, x4));
    double minY = std::fmin(std::fmin(y1, y2), std::fmin(y3, y4));
    double maxY = std::fmax(std::fmax(y1, y2), std::fmax(y3, y4));

    BoundingBox box;
    box.x = minX;
    box.y = minY;
    box.width = maxX - minX;
    box.height = maxY - minY;

    return Result<BoundingBox>(box);
}

Result<void> Page::clickAt(double x, double y) {
    auto moveResp = client_.Input.dispatchMouseEvent("mouseMoved", x, y);
    if (moveResp.hasError) {
        return Result<void>::failure(moveResp.errorCode, moveResp.errorMessage);
    }

    auto downResp = client_.Input.dispatchMouseEvent("mousePressed", x, y, 0, 0, MouseButton::Left, 0, 1);
    if (downResp.hasError) {
        return Result<void>::failure(downResp.errorCode, downResp.errorMessage);
    }

    auto upResp = client_.Input.dispatchMouseEvent("mouseReleased", x, y, 0, 0, MouseButton::Left, 0, 1);
    if (upResp.hasError) {
        return Result<void>::failure(upResp.errorCode, upResp.errorMessage);
    }

    return Result<void>::success();
}

int Page::rootNodeId() {
    auto result = ensureDocument();
    return result.ok() ? result.value() : 0;
}

} 
} 
