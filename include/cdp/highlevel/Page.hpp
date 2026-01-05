#pragma once

#include "Result.hpp"
#include "ElementHandle.hpp"
#include "../protocol/CDPClient.hpp"
#include <string>
#include <vector>
#include <map>
#include <chrono>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <thread>
#include <utility>

namespace cdp {
namespace highlevel {


class NetworkInterceptor;


constexpr int DEFAULT_TIMEOUT_MS = 30000;


class CancellationToken {
public:
    CancellationToken() : cancelled_(false) {}

    
    void cancel() {
        cancelled_.store(true, std::memory_order_release);
    }

    
    bool isCancelled() const {
        return cancelled_.load(std::memory_order_acquire);
    }

    
    void reset() {
        cancelled_.store(false, std::memory_order_release);
    }

private:
    std::atomic<bool> cancelled_;
};


struct WaitOptions {
    int timeoutMs = DEFAULT_TIMEOUT_MS;  
    int pollIntervalMs = 50;             
    int maxPollIntervalMs = 200;         
    bool visible = false;                
    CancellationToken* cancellationToken = nullptr;  

    
    static WaitOptions fast(int timeoutMs = 5000) {
        WaitOptions opts;
        opts.timeoutMs = timeoutMs;
        opts.pollIntervalMs = 10;
        opts.maxPollIntervalMs = 50;
        return opts;
    }

    static WaitOptions standard(int timeoutMs = DEFAULT_TIMEOUT_MS) {
        return WaitOptions{timeoutMs};
    }

    static WaitOptions slow(int timeoutMs = 60000) {
        WaitOptions opts;
        opts.timeoutMs = timeoutMs;
        opts.pollIntervalMs = 100;
        opts.maxPollIntervalMs = 500;
        return opts;
    }

    static WaitOptions withCancellation(CancellationToken* token, int timeoutMs = DEFAULT_TIMEOUT_MS) {
        WaitOptions opts;
        opts.timeoutMs = timeoutMs;
        opts.cancellationToken = token;
        return opts;
    }
};


enum class WaitUntil {
    Load,              
    DOMContentLoaded,  
    NetworkIdle0,      
    NetworkIdle2,      
    None               
};


struct NavigateOptions {
    int timeoutMs = 30000;
    WaitUntil waitUntil = WaitUntil::Load;
    std::string referer;        
};


struct TypeOptions {
    bool clearFirst = true;     
    bool useKeyEvents = false;  
    int delayMs = 0;            
};


struct ScreenshotOptions {
    std::string format = "png";  
    int quality = 80;            
    bool fullPage = false;       
    bool optimizeForSpeed = true; 
    int timeoutMs = 120000;      
    std::optional<BoundingBox> clip;  
};


class Page {
public:
    explicit Page(CDPClient& client);
    ~Page();

    
    Page(const Page&) = delete;
    Page& operator=(const Page&) = delete;

    
    Result<void> navigate(const std::string& url, int timeoutMs = 30000);

    
    Result<void> navigate(const std::string& url, const NavigateOptions& options);

    
    Result<void> navigateNoWait(const std::string& url);

    
    Result<void> reload(int timeoutMs = 30000);
    Result<void> reload(const NavigateOptions& options);

    
    Result<void> goBack(int timeoutMs = 30000);

    
    Result<void> goForward(int timeoutMs = 30000);

    
    std::string url();

    
    std::string title();

    
    Result<void> click(const std::string& selector, int timeoutMs = 30000);

    
    Result<void> type(const std::string& selector, const std::string& text, int timeoutMs = 30000);

    
    Result<void> type(const std::string& selector, const std::string& text, const TypeOptions& options, int timeoutMs = 30000);

    
    Result<void> typeAppend(const std::string& selector, const std::string& text, int timeoutMs = 30000);

    
    Result<std::string> getText(const std::string& selector, int timeoutMs = 30000);

    
    Result<std::string> getHTML(const std::string& selector, int timeoutMs = 30000);

    
    Result<std::string> getAttribute(const std::string& selector, const std::string& name, int timeoutMs = 30000);

    
    Result<std::string> getValue(const std::string& selector, int timeoutMs = 30000);

    
    Result<void> setValue(const std::string& selector, const std::string& value, int timeoutMs = 30000);

    
    Result<void> focus(const std::string& selector, int timeoutMs = 30000);

    
    Result<void> hover(const std::string& selector, int timeoutMs = 30000);

    
    Result<void> check(const std::string& selector, bool checked = true, int timeoutMs = 30000);

    
    Result<void> select(const std::string& selector, const std::string& value, int timeoutMs = 30000);

    
    Result<void> clear(const std::string& selector, int timeoutMs = 30000);

    
    Result<void> press(const std::string& key, int timeoutMs = 30000);

    
    Result<ElementHandle> querySelector(const std::string& selector);

    
    Result<std::vector<ElementHandle>> querySelectorAll(const std::string& selector);

    
    bool exists(const std::string& selector);

    
    int count(const std::string& selector);

    
    Result<ElementHandle> waitForSelector(const std::string& selector, int timeoutMs = DEFAULT_TIMEOUT_MS);
    Result<ElementHandle> waitForSelector(const std::string& selector, const WaitOptions& options);

    
    Result<ElementHandle> waitForVisible(const std::string& selector, int timeoutMs = DEFAULT_TIMEOUT_MS);
    Result<ElementHandle> waitForVisible(const std::string& selector, const WaitOptions& options);

    
    Result<void> waitForHidden(const std::string& selector, int timeoutMs = DEFAULT_TIMEOUT_MS);
    Result<void> waitForHidden(const std::string& selector, const WaitOptions& options);

    
    Result<void> waitForNavigation(int timeoutMs = DEFAULT_TIMEOUT_MS);
    Result<void> waitForNavigation(const WaitOptions& options);

    
    Result<void> waitForNetworkIdle(int idleTimeMs = 500, int timeoutMs = DEFAULT_TIMEOUT_MS);
    Result<void> waitForNetworkIdle(int idleTimeMs, const WaitOptions& options);

    
    Result<void> waitForFunction(const std::string& expression, int timeoutMs = DEFAULT_TIMEOUT_MS);
    Result<void> waitForFunction(const std::string& expression, const WaitOptions& options);

    
    Result<void> waitForURL(const std::string& urlPattern, int timeoutMs = DEFAULT_TIMEOUT_MS);
    Result<void> waitForURL(const std::string& urlPattern, const WaitOptions& options);

    
    Result<JsonValue> evaluate(const std::string& expression);

    
    Result<std::string> evaluateString(const std::string& expression);

    
    Result<int> evaluateInt(const std::string& expression);

    
    Result<bool> evaluateBool(const std::string& expression);

    
    Result<std::vector<uint8_t>> screenshot(const ScreenshotOptions& options = {});

    
    Result<void> screenshotToFile(const std::string& path, const ScreenshotOptions& options = {});

    
    Result<std::vector<uint8_t>> pdf();

    
    Result<void> pdfToFile(const std::string& path);

    
    Result<void> setViewport(int width, int height, double deviceScaleFactor = 1.0);

    
    Result<void> setUserAgent(const std::string& userAgent);

    
    Result<void> emulateDevice(const std::string& deviceName);

    
    Result<JsonArray> getCookies();

    
    Result<void> setCookie(const std::string& name, const std::string& value,
                           const std::string& domain = "", const std::string& path = "/");

    
    Result<void> deleteCookie(const std::string& name, const std::string& domain = "");

    
    Result<void> clearCookies();

    
    CDPClient& client() { return client_; }

    
    const std::string& frameId() const { return frameId_; }

    
    int rootNodeId();

    
    Result<void> fillForm(const std::map<std::string, std::string>& fields, int timeoutMs = 30000) {
        for (const auto& [selector, value] : fields) {
            auto result = type(selector, value, timeoutMs);
            if (!result) {
                return Error(result.error().code, result.error().message)
                    .withSelector(selector)
                    .withOperation("fillForm");
            }
        }
        return {};
    }

    
    Result<void> fillFormAndSubmit(const std::map<std::string, std::string>& fields,
                                   const std::string& submitSelector = "",
                                   int timeoutMs = 30000) {
        auto fill = fillForm(fields, timeoutMs);
        if (!fill) return fill;

        if (!submitSelector.empty()) {
            return click(submitSelector, timeoutMs);
        } else {
            return press("Enter", timeoutMs);
        }
    }

    
    Result<std::vector<std::string>> getTexts(const std::string& selector, int timeoutMs = 30000) {
        auto elements = querySelectorAll(selector);
        if (!elements) return elements.error();

        std::vector<std::string> texts;
        for (auto& el : elements.value()) {
            auto text = el.textContent();
            texts.push_back(text.valueOr(""));
        }
        return texts;
    }

    
    Result<std::pair<int, ElementHandle>> waitForAny(const std::vector<std::string>& selectors,
                                                      int timeoutMs = DEFAULT_TIMEOUT_MS) {
        auto start = std::chrono::steady_clock::now();
        while (true) {
            auto elapsed = std::chrono::steady_clock::now() - start;
            if (std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count() >= timeoutMs) {
                return Error::waitTimeout("any of " + std::to_string(selectors.size()) + " selectors", timeoutMs);
            }

            for (size_t i = 0; i < selectors.size(); ++i) {
                auto result = querySelector(selectors[i]);
                if (result) {
                    return std::make_pair(static_cast<int>(i), std::move(result.value()));
                }
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
    }

    
    Result<void> scrollToElement(const std::string& selector, int timeoutMs = 30000) {
        auto nodeId = findElement(selector);
        if (!nodeId) return nodeId.error();

        auto result = client_.sendCommand("DOM.scrollIntoViewIfNeeded",
            Params().set("nodeId", nodeId.value()).build());
        if (result.hasError) return Error::fromCDPResponse(result);
        return {};
    }

    
    Result<void> scrollBy(int x, int y) {
        std::string script = "window.scrollBy(" + std::to_string(x) + ", " + std::to_string(y) + ")";
        auto result = evaluate(script);
        if (!result) return result.error();
        return {};
    }

    
    Result<void> scrollTo(int x, int y) {
        std::string script = "window.scrollTo(" + std::to_string(x) + ", " + std::to_string(y) + ")";
        auto result = evaluate(script);
        if (!result) return result.error();
        return {};
    }

    
    Result<void> doubleClick(const std::string& selector, int timeoutMs = 30000) {
        auto elem = waitForSelector(selector, timeoutMs);
        if (!elem) return elem.error();

        auto bounds = getElementBounds(elem.value().nodeId());
        if (!bounds) return bounds.error();

        double x = bounds.value().x + bounds.value().width / 2;
        double y = bounds.value().y + bounds.value().height / 2;

        
        auto r1 = client_.sendCommand("Input.dispatchMouseEvent",
            Params()
                .set("type", "mousePressed")
                .set("x", x).set("y", y)
                .set("button", "left")
                .set("clickCount", 2)
                .build());
        if (r1.hasError) return Error::fromCDPResponse(r1);

        auto r2 = client_.sendCommand("Input.dispatchMouseEvent",
            Params()
                .set("type", "mouseReleased")
                .set("x", x).set("y", y)
                .set("button", "left")
                .set("clickCount", 2)
                .build());
        if (r2.hasError) return Error::fromCDPResponse(r2);

        return {};
    }

    
    Result<void> rightClick(const std::string& selector, int timeoutMs = 30000) {
        auto elem = waitForSelector(selector, timeoutMs);
        if (!elem) return elem.error();

        auto bounds = getElementBounds(elem.value().nodeId());
        if (!bounds) return bounds.error();

        double x = bounds.value().x + bounds.value().width / 2;
        double y = bounds.value().y + bounds.value().height / 2;

        auto r1 = client_.sendCommand("Input.dispatchMouseEvent",
            Params()
                .set("type", "mousePressed")
                .set("x", x).set("y", y)
                .set("button", "right")
                .set("clickCount", 1)
                .build());
        if (r1.hasError) return Error::fromCDPResponse(r1);

        auto r2 = client_.sendCommand("Input.dispatchMouseEvent",
            Params()
                .set("type", "mouseReleased")
                .set("x", x).set("y", y)
                .set("button", "right")
                .set("clickCount", 1)
                .build());
        if (r2.hasError) return Error::fromCDPResponse(r2);

        return {};
    }

    
    Result<bool> isVisible(const std::string& selector) {
        auto result = evaluate(
            "(function() {"
            "  var el = document.querySelector('" + selector + "');"
            "  if (!el) return false;"
            "  var style = window.getComputedStyle(el);"
            "  return style.display !== 'none' && style.visibility !== 'hidden' && style.opacity !== '0';"
            "})()");
        if (!result) return result.error();
        return result.value().getBool(false);
    }

    
    Result<bool> isEnabled(const std::string& selector) {
        auto result = evaluate(
            "(function() {"
            "  var el = document.querySelector('" + selector + "');"
            "  return el && !el.disabled;"
            "})()");
        if (!result) return result.error();
        return result.value().getBool(false);
    }

    
    Result<bool> isChecked(const std::string& selector) {
        auto result = evaluate(
            "(function() {"
            "  var el = document.querySelector('" + selector + "');"
            "  return el && el.checked === true;"
            "})()");
        if (!result) return result.error();
        return result.value().getBool(false);
    }

    
    Result<std::string> getComputedStyle(const std::string& selector, const std::string& property) {
        auto result = evaluate(
            "(function() {"
            "  var el = document.querySelector('" + selector + "');"
            "  if (!el) return '';"
            "  return window.getComputedStyle(el).getPropertyValue('" + property + "');"
            "})()");
        if (!result) return result.error();
        return result.value().getString("");
    }

    
    Result<void> dragAndDrop(const std::string& sourceSelector, const std::string& targetSelector,
                             int timeoutMs = 30000) {
        auto source = waitForSelector(sourceSelector, timeoutMs);
        if (!source) return source.error();
        auto target = waitForSelector(targetSelector, timeoutMs);
        if (!target) return target.error();

        auto sourceBounds = getElementBounds(source.value().nodeId());
        if (!sourceBounds) return sourceBounds.error();
        auto targetBounds = getElementBounds(target.value().nodeId());
        if (!targetBounds) return targetBounds.error();

        double sx = sourceBounds.value().x + sourceBounds.value().width / 2;
        double sy = sourceBounds.value().y + sourceBounds.value().height / 2;
        double tx = targetBounds.value().x + targetBounds.value().width / 2;
        double ty = targetBounds.value().y + targetBounds.value().height / 2;

        
        auto r1 = client_.sendCommand("Input.dispatchMouseEvent",
            Params().set("type", "mousePressed").set("x", sx).set("y", sy)
                    .set("button", "left").set("clickCount", 1).build());
        if (r1.hasError) return Error::fromCDPResponse(r1);

        
        int steps = 10;
        for (int i = 1; i <= steps; ++i) {
            double t = static_cast<double>(i) / steps;
            double x = sx + (tx - sx) * t;
            double y = sy + (ty - sy) * t;
            auto r = client_.sendCommand("Input.dispatchMouseEvent",
                Params().set("type", "mouseMoved").set("x", x).set("y", y)
                        .set("button", "left").build());
            if (r.hasError) return Error::fromCDPResponse(r);
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }

        
        auto r2 = client_.sendCommand("Input.dispatchMouseEvent",
            Params().set("type", "mouseReleased").set("x", tx).set("y", ty)
                    .set("button", "left").set("clickCount", 1).build());
        if (r2.hasError) return Error::fromCDPResponse(r2);

        return {};
    }

    
    Result<void> uploadFile(const std::string& selector, const std::vector<std::string>& filePaths,
                            int timeoutMs = 30000) {
        auto nodeId = findElement(selector);
        if (!nodeId) return nodeId.error();

        JsonArray files;
        for (const auto& path : filePaths) {
            files.push_back(path);
        }

        auto result = client_.sendCommand("DOM.setFileInputFiles",
            Params().set("files", files).set("nodeId", nodeId.value()).build());
        if (result.hasError) return Error::fromCDPResponse(result);
        return {};
    }

    
    Result<std::string> content() {
        return evaluateString("document.documentElement.outerHTML");
    }

    
    Result<void> setContent(const std::string& html) {
        auto result = client_.sendCommand("Page.setDocumentContent",
            Params().set("frameId", frameId_).set("html", html).build());
        if (result.hasError) return Error::fromCDPResponse(result);
        return {};
    }

    
    Result<void> bringToFront() {
        auto result = client_.sendCommand("Page.bringToFront", {});
        if (result.hasError) return Error::fromCDPResponse(result);
        return {};
    }

private:
    
    Result<int> ensureDocument();

    
    Result<int> findElement(const std::string& selector);

    
    Result<BoundingBox> getElementBounds(int nodeId);

    
    Result<void> clickAt(double x, double y);

    
    Result<std::string> evalString(const std::string& js);

    
    template<typename Func>
    Result<void> pollWithBackoff(const WaitOptions& options, const std::string& description, Func&& condition);

    CDPClient& client_;
    std::string frameId_;
    int rootNodeId_ = 0;
    bool domainEnabled_ = false;

    
    std::atomic<int> inflightRequests_{0};
    std::chrono::steady_clock::time_point lastNetworkActivity_;
    std::mutex networkMutex_;
};

} 
} 
