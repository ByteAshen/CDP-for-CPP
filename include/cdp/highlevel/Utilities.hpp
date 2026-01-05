

#pragma once

#include "Result.hpp"
#include "../protocol/CDPConnection.hpp"
#include "../core/Json.hpp"
#include <string>
#include <vector>
#include <map>
#include <chrono>
#include <thread>
#include <random>
#include <functional>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <cmath>
#include <atomic>
#include <mutex>

namespace cdp {
namespace highlevel {


struct RetryPolicy {
    int maxAttempts = 3;              
    int initialDelayMs = 100;         
    int maxDelayMs = 5000;            
    double backoffMultiplier = 2.0;   
    bool retryOnTimeout = true;       
    bool retryOnNetwork = true;       
    bool retryOnStale = true;         
    std::function<bool(const Error&)> shouldRetry;  

    
    static RetryPolicy none() {
        return RetryPolicy{.maxAttempts = 1};
    }

    static RetryPolicy quick() {
        return RetryPolicy{
            .maxAttempts = 3,
            .initialDelayMs = 50,
            .maxDelayMs = 500,
            .backoffMultiplier = 1.5
        };
    }

    static RetryPolicy standard() {
        return RetryPolicy{
            .maxAttempts = 3,
            .initialDelayMs = 100,
            .maxDelayMs = 2000,
            .backoffMultiplier = 2.0
        };
    }

    static RetryPolicy aggressive() {
        return RetryPolicy{
            .maxAttempts = 5,
            .initialDelayMs = 200,
            .maxDelayMs = 10000,
            .backoffMultiplier = 2.0
        };
    }

    
    bool canRetry(const Error& error, int attempt) const {
        if (attempt >= maxAttempts) return false;

        
        if (shouldRetry && shouldRetry(error)) return true;

        
        if (retryOnTimeout && error.isTimeout()) return true;
        if (retryOnNetwork && error.isNetwork()) return true;
        if (retryOnStale && error.isElementStale()) return true;

        return false;
    }

    
    int getDelayMs(int attempt) const {
        double delay = initialDelayMs * std::pow(backoffMultiplier, attempt - 1);
        return std::min(static_cast<int>(delay), maxDelayMs);
    }

    
    int getDelayWithJitter(int attempt) const {
        int baseDelay = getDelayMs(attempt);
        static thread_local std::mt19937 rng(std::random_device{}());
        std::uniform_int_distribution<int> dist(0, baseDelay / 4);
        return baseDelay + dist(rng);
    }
};


template<typename Func>
auto executeWithRetry(Func&& func, const RetryPolicy& policy) -> decltype(func()) {
    using ResultType = decltype(func());

    for (int attempt = 1; attempt <= policy.maxAttempts; ++attempt) {
        auto result = func();

        if (result.ok()) {
            return result;
        }

        const Error* err = result.error_or_null();
        if (!err || !policy.canRetry(*err, attempt)) {
            return result;
        }

        if (attempt < policy.maxAttempts) {
            int delayMs = policy.getDelayWithJitter(attempt);
            std::this_thread::sleep_for(std::chrono::milliseconds(delayMs));
        }
    }

    return func();  
}


struct CookieData {
    std::string name;
    std::string value;
    std::string domain;
    std::string path = "/";
    double expires = -1;           
    bool httpOnly = false;
    bool secure = false;
    std::string sameSite = "Lax";  
    int64_t size = 0;
    std::string priority = "Medium";

    
    static CookieData fromJson(const JsonValue& json) {
        CookieData c;
        c.name = json.getStringAt("name");
        c.value = json.getStringAt("value");
        c.domain = json.getStringAt("domain");
        c.path = json.getStringAt("path", "/");
        c.expires = json.getDoubleAt("expires", -1);
        c.httpOnly = json.getBoolAt("httpOnly", false);
        c.secure = json.getBoolAt("secure", false);
        c.sameSite = json.getStringAt("sameSite", "Lax");
        c.size = json.getInt64At("size", 0);
        c.priority = json.getStringAt("priority", "Medium");
        return c;
    }

    
    JsonValue toJson() const {
        JsonObject obj;
        obj["name"] = name;
        obj["value"] = value;
        obj["domain"] = domain;
        obj["path"] = path;
        if (expires >= 0) obj["expires"] = expires;
        obj["httpOnly"] = httpOnly;
        obj["secure"] = secure;
        obj["sameSite"] = sameSite;
        return obj;
    }

    
    bool isExpired() const {
        if (expires < 0) return false;  
        auto now = std::chrono::system_clock::now();
        auto expTime = std::chrono::system_clock::from_time_t(static_cast<time_t>(expires));
        return now > expTime;
    }

    
    bool matchesDomain(const std::string& checkDomain) const {
        if (domain.empty()) return true;
        if (domain[0] == '.') {
            
            return checkDomain.ends_with(domain) ||
                   checkDomain == domain.substr(1);
        }
        return checkDomain == domain;
    }
};

class CookieManager {
public:
    explicit CookieManager(CDPConnection& conn) : connection_(conn) {}

    
    Result<std::vector<CookieData>> getAll() {
        auto response = connection_.sendCommandSync("Network.getAllCookies", {});
        if (response.hasError) {
            return Error::fromCDPResponse(response);
        }

        std::vector<CookieData> cookies;
        const auto* arr = response.result.getPath("cookies");
        if (arr && arr->isArray()) {
            for (const auto& c : arr->asArray()) {
                cookies.push_back(CookieData::fromJson(c));
            }
        }
        return cookies;
    }

    
    Result<std::vector<CookieData>> getForUrl(const std::string& url) {
        auto response = connection_.sendCommandSync("Network.getCookies",
            Params().set("urls", JsonArray{url}).build());
        if (response.hasError) {
            return Error::fromCDPResponse(response);
        }

        std::vector<CookieData> cookies;
        const auto* arr = response.result.getPath("cookies");
        if (arr && arr->isArray()) {
            for (const auto& c : arr->asArray()) {
                cookies.push_back(CookieData::fromJson(c));
            }
        }
        return cookies;
    }

    
    Result<void> set(const CookieData& cookie) {
        auto params = Params()
            .set("name", cookie.name)
            .set("value", cookie.value)
            .set("domain", cookie.domain)
            .set("path", cookie.path)
            .set("httpOnly", cookie.httpOnly)
            .set("secure", cookie.secure)
            .set("sameSite", cookie.sameSite);

        if (cookie.expires >= 0) {
            params.set("expires", cookie.expires);
        }

        auto response = connection_.sendCommandSync("Network.setCookie", params.build());
        if (response.hasError) {
            return Error::fromCDPResponse(response);
        }
        return {};
    }

    
    Result<void> setAll(const std::vector<CookieData>& cookies) {
        JsonArray cookieArray;
        for (const auto& c : cookies) {
            cookieArray.push_back(c.toJson());
        }

        auto response = connection_.sendCommandSync("Network.setCookies",
            Params().set("cookies", cookieArray).build());
        if (response.hasError) {
            return Error::fromCDPResponse(response);
        }
        return {};
    }

    
    Result<void> remove(const std::string& name, const std::string& domain = "",
                        const std::string& path = "/") {
        auto params = Params().set("name", name);
        if (!domain.empty()) params.set("domain", domain);
        if (!path.empty()) params.set("path", path);

        auto response = connection_.sendCommandSync("Network.deleteCookies", params.build());
        if (response.hasError) {
            return Error::fromCDPResponse(response);
        }
        return {};
    }

    
    Result<void> clearAll() {
        auto response = connection_.sendCommandSync("Network.clearBrowserCookies", {});
        if (response.hasError) {
            return Error::fromCDPResponse(response);
        }
        return {};
    }

    
    Result<void> saveToFile(const std::string& filename) {
        auto cookies = getAll();
        if (!cookies) return cookies.error();

        JsonArray arr;
        for (const auto& c : cookies.value()) {
            arr.push_back(c.toJson());
        }

        std::ofstream file(filename);
        if (!file) {
            return Error(ErrorCode::Internal, "Failed to open file for writing: " + filename);
        }

        file << JsonValue(arr).serialize(true);
        return {};
    }

    
    Result<void> loadFromFile(const std::string& filename) {
        std::ifstream file(filename);
        if (!file) {
            return Error(ErrorCode::Internal, "Failed to open file for reading: " + filename);
        }

        std::stringstream buffer;
        buffer << file.rdbuf();

        try {
            auto json = JsonValue::parse(buffer.str());
            if (!json.isArray()) {
                return Error(ErrorCode::InvalidArgument, "Cookie file must contain a JSON array");
            }

            std::vector<CookieData> cookies;
            for (const auto& c : json.asArray()) {
                cookies.push_back(CookieData::fromJson(c));
            }

            return setAll(cookies);
        } catch (const std::exception& e) {
            return Error(ErrorCode::InvalidArgument, std::string("Failed to parse cookie file: ") + e.what());
        }
    }

private:
    CDPConnection& connection_;
};


struct PerformanceMetrics {
    
    double navigationStart = 0;
    double domContentLoaded = 0;
    double loadEvent = 0;
    double firstPaint = 0;
    double firstContentfulPaint = 0;
    double largestContentfulPaint = 0;

    
    double domainLookupTime = 0;
    double connectTime = 0;
    double responseTime = 0;
    double domInteractive = 0;
    double domComplete = 0;

    
    int64_t jsHeapSizeLimit = 0;
    int64_t jsHeapTotalSize = 0;
    int64_t jsHeapUsedSize = 0;

    
    int64_t documentNodeCount = 0;
    int64_t frameCount = 0;
    int64_t layoutCount = 0;
    int64_t styleRecalcCount = 0;

    
    int64_t totalRequests = 0;
    int64_t totalTransferSize = 0;
    int64_t totalResourceSize = 0;

    std::string toJson() const {
        JsonObject obj;
        obj["navigationStart"] = navigationStart;
        obj["domContentLoaded"] = domContentLoaded;
        obj["loadEvent"] = loadEvent;
        obj["firstPaint"] = firstPaint;
        obj["firstContentfulPaint"] = firstContentfulPaint;
        obj["largestContentfulPaint"] = largestContentfulPaint;
        obj["jsHeapUsedSize"] = jsHeapUsedSize;
        obj["jsHeapTotalSize"] = jsHeapTotalSize;
        obj["documentNodeCount"] = documentNodeCount;
        obj["totalRequests"] = totalRequests;
        obj["totalTransferSize"] = totalTransferSize;
        return JsonValue(obj).serialize(true);
    }
};

class PerformanceMonitor {
public:
    explicit PerformanceMonitor(CDPConnection& conn) : connection_(conn) {}

    
    Result<void> enable() {
        auto r1 = connection_.sendCommandSync("Performance.enable", {});
        if (r1.hasError) return Error::fromCDPResponse(r1);

        enabled_ = true;
        return {};
    }

    
    Result<void> disable() {
        auto r1 = connection_.sendCommandSync("Performance.disable", {});
        enabled_ = false;
        if (r1.hasError) return Error::fromCDPResponse(r1);
        return {};
    }

    
    Result<PerformanceMetrics> getMetrics() {
        if (!enabled_) {
            auto r = enable();
            if (!r) return r.error();
        }

        PerformanceMetrics metrics;

        
        auto response = connection_.sendCommandSync("Performance.getMetrics", {});
        if (!response.hasError) {
            const auto* arr = response.result.getPath("metrics");
            if (arr && arr->isArray()) {
                for (const auto& m : arr->asArray()) {
                    std::string name = m.getStringAt("name");
                    double value = m.getDoubleAt("value");

                    if (name == "Timestamp") metrics.navigationStart = value;
                    else if (name == "JSHeapUsedSize") metrics.jsHeapUsedSize = static_cast<int64_t>(value);
                    else if (name == "JSHeapTotalSize") metrics.jsHeapTotalSize = static_cast<int64_t>(value);
                    else if (name == "Documents") metrics.documentNodeCount = static_cast<int64_t>(value);
                    else if (name == "Frames") metrics.frameCount = static_cast<int64_t>(value);
                    else if (name == "LayoutCount") metrics.layoutCount = static_cast<int64_t>(value);
                    else if (name == "RecalcStyleCount") metrics.styleRecalcCount = static_cast<int64_t>(value);
                }
            }
        }

        
        auto evalResult = connection_.sendCommandSync("Runtime.evaluate",
            Params()
                .set("expression", R"(
                    JSON.stringify({
                        domContentLoaded: performance.timing.domContentLoadedEventEnd - performance.timing.navigationStart,
                        loadEvent: performance.timing.loadEventEnd - performance.timing.navigationStart,
                        domInteractive: performance.timing.domInteractive - performance.timing.navigationStart,
                        domComplete: performance.timing.domComplete - performance.timing.navigationStart,
                        firstPaint: performance.getEntriesByType('paint').find(e => e.name === 'first-paint')?.startTime || 0,
                        firstContentfulPaint: performance.getEntriesByType('paint').find(e => e.name === 'first-contentful-paint')?.startTime || 0
                    })
                )")
                .set("returnByValue", true)
                .build());

        if (!evalResult.hasError) {
            std::string jsonStr = evalResult.result.getStringAt("result/value");
            if (!jsonStr.empty()) {
                try {
                    auto timing = JsonValue::parse(jsonStr);
                    metrics.domContentLoaded = timing.getDoubleAt("domContentLoaded");
                    metrics.loadEvent = timing.getDoubleAt("loadEvent");
                    metrics.domInteractive = timing.getDoubleAt("domInteractive");
                    metrics.domComplete = timing.getDoubleAt("domComplete");
                    metrics.firstPaint = timing.getDoubleAt("firstPaint");
                    metrics.firstContentfulPaint = timing.getDoubleAt("firstContentfulPaint");
                } catch (...) {}
            }
        }

        return metrics;
    }

    
    Result<void> startTracing(const std::vector<std::string>& categories = {}) {
        std::string cats = categories.empty()
            ? "-*,devtools.timeline,v8.execute"
            : "";
        for (size_t i = 0; i < categories.size(); ++i) {
            if (i > 0) cats += ",";
            cats += categories[i];
        }

        auto response = connection_.sendCommandSync("Tracing.start",
            Params().set("categories", cats).build());
        if (response.hasError) return Error::fromCDPResponse(response);
        return {};
    }

    
    Result<std::string> stopTracing() {
        std::string traceData;
        std::mutex mutex;
        std::atomic<bool> done{false};

        
        connection_.onEvent("Tracing.dataCollected", [&](const CDPEvent& evt) {
            std::lock_guard<std::mutex> lock(mutex);
            const auto* arr = evt.params.getPath("value");
            if (arr && arr->isArray()) {
                for (const auto& item : arr->asArray()) {
                    if (!traceData.empty()) traceData += ",";
                    traceData += item.serialize();
                }
            }
        });

        connection_.onEvent("Tracing.tracingComplete", [&](const CDPEvent&) {
            done = true;
        });

        auto response = connection_.sendCommandSync("Tracing.end", {});
        if (response.hasError) return Error::fromCDPResponse(response);

        
        auto start = std::chrono::steady_clock::now();
        while (!done) {
            if (std::chrono::steady_clock::now() - start > std::chrono::seconds(30)) {
                return Error::timeout("stopTracing", 30000);
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }

        return "[" + traceData + "]";
    }

private:
    CDPConnection& connection_;
    bool enabled_ = false;
};


struct HAREntry {
    std::string url;
    std::string method;
    int status = 0;
    std::string statusText;
    std::string mimeType;
    int64_t requestSize = 0;
    int64_t responseSize = 0;
    double startTime = 0;
    double duration = 0;
    std::map<std::string, std::string> requestHeaders;
    std::map<std::string, std::string> responseHeaders;
    std::string requestBody;
    std::string responseBody;
};

class HARExporter {
public:
    explicit HARExporter(CDPConnection& conn) : connection_(conn) {}

    
    Result<void> startRecording() {
        entries_.clear();
        pendingRequests_.clear();

        
        auto r = connection_.sendCommandSync("Network.enable", {});
        if (r.hasError) return Error::fromCDPResponse(r);

        
        connection_.onEvent("Network.requestWillBeSent", [this](const CDPEvent& evt) {
            handleRequestWillBeSent(evt);
        });

        connection_.onEvent("Network.responseReceived", [this](const CDPEvent& evt) {
            handleResponseReceived(evt);
        });

        connection_.onEvent("Network.loadingFinished", [this](const CDPEvent& evt) {
            handleLoadingFinished(evt);
        });

        recording_ = true;
        return {};
    }

    
    void stopRecording() {
        recording_ = false;
    }

    
    const std::vector<HAREntry>& entries() const { return entries_; }

    
    std::string exportHAR() const {
        JsonObject har;
        JsonObject log;

        log["version"] = "1.2";
        log["creator"] = JsonObject{{"name", "CDP for C++"}, {"version", "1.0"}};

        JsonArray entriesArr;
        for (const auto& e : entries_) {
            JsonObject entry;

            entry["startedDateTime"] = formatTime(e.startTime);
            entry["time"] = e.duration;

            
            JsonObject request;
            request["method"] = e.method;
            request["url"] = e.url;
            request["httpVersion"] = "HTTP/1.1";

            JsonArray reqHeaders;
            for (const auto& [k, v] : e.requestHeaders) {
                reqHeaders.push_back(JsonObject{{"name", k}, {"value", v}});
            }
            request["headers"] = reqHeaders;
            request["headersSize"] = static_cast<int64_t>(e.requestSize);
            request["bodySize"] = static_cast<int64_t>(e.requestBody.size());
            entry["request"] = request;

            
            JsonObject response;
            response["status"] = e.status;
            response["statusText"] = e.statusText;
            response["httpVersion"] = "HTTP/1.1";

            JsonArray respHeaders;
            for (const auto& [k, v] : e.responseHeaders) {
                respHeaders.push_back(JsonObject{{"name", k}, {"value", v}});
            }
            response["headers"] = respHeaders;

            JsonObject content;
            content["size"] = e.responseSize;
            content["mimeType"] = e.mimeType;
            response["content"] = content;

            response["headersSize"] = -1;
            response["bodySize"] = e.responseSize;
            entry["response"] = response;

            entry["cache"] = JsonObject{};
            entry["timings"] = JsonObject{{"wait", 0}, {"receive", e.duration}};

            entriesArr.push_back(entry);
        }

        log["entries"] = entriesArr;
        har["log"] = log;

        return JsonValue(har).serialize(true);
    }

    
    Result<void> saveToFile(const std::string& filename) {
        std::ofstream file(filename);
        if (!file) {
            return Error(ErrorCode::Internal, "Failed to open HAR file: " + filename);
        }
        file << exportHAR();
        return {};
    }

private:
    void handleRequestWillBeSent(const CDPEvent& evt) {
        if (!recording_) return;

        std::lock_guard<std::mutex> lock(mutex_);

        std::string requestId = evt.params.getStringAt("requestId");
        HAREntry entry;
        entry.url = evt.params.getStringAt("request/url");
        entry.method = evt.params.getStringAt("request/method");
        entry.startTime = evt.params.getDoubleAt("wallTime", 0);

        const auto* headers = evt.params.getPath("request/headers");
        if (headers && headers->isObject()) {
            for (const auto& [k, v] : headers->asObject()) {
                entry.requestHeaders[k] = v.getString();
            }
        }

        pendingRequests_[requestId] = entry;
    }

    void handleResponseReceived(const CDPEvent& evt) {
        if (!recording_) return;

        std::lock_guard<std::mutex> lock(mutex_);

        std::string requestId = evt.params.getStringAt("requestId");
        auto it = pendingRequests_.find(requestId);
        if (it == pendingRequests_.end()) return;

        it->second.status = evt.params.getIntAt("response/status");
        it->second.statusText = evt.params.getStringAt("response/statusText");
        it->second.mimeType = evt.params.getStringAt("response/mimeType");

        const auto* headers = evt.params.getPath("response/headers");
        if (headers && headers->isObject()) {
            for (const auto& [k, v] : headers->asObject()) {
                it->second.responseHeaders[k] = v.getString();
            }
        }
    }

    void handleLoadingFinished(const CDPEvent& evt) {
        if (!recording_) return;

        std::lock_guard<std::mutex> lock(mutex_);

        std::string requestId = evt.params.getStringAt("requestId");
        auto it = pendingRequests_.find(requestId);
        if (it == pendingRequests_.end()) return;

        it->second.responseSize = evt.params.getInt64At("encodedDataLength");
        double endTime = evt.params.getDoubleAt("timestamp", 0);
        if (it->second.startTime > 0 && endTime > 0) {
            it->second.duration = (endTime - it->second.startTime) * 1000;
        }

        entries_.push_back(std::move(it->second));
        pendingRequests_.erase(it);
    }

    std::string formatTime(double timestamp) const {
        auto time = static_cast<time_t>(timestamp);
        std::ostringstream oss;
        oss << std::put_time(std::gmtime(&time), "%Y-%m-%dT%H:%M:%S.000Z");
        return oss.str();
    }

    CDPConnection& connection_;
    bool recording_ = false;
    std::vector<HAREntry> entries_;
    std::map<std::string, HAREntry> pendingRequests_;
    std::mutex mutex_;
};


class StealthHelpers {
public:
    explicit StealthHelpers(CDPConnection& conn) : connection_(conn) {}

    
    Result<void> apply() {
        auto results = {
            hideWebDriver(),
            hideAutomation(),
            mockPlugins(),
            mockLanguages(),
            fixPermissions(),
            fixWebGL(),
            fixChrome()
        };

        for (const auto& r : results) {
            if (!r) return r;
        }
        return {};
    }

    
    Result<void> hideWebDriver() {
        return evaluate(R"(
            Object.defineProperty(navigator, 'webdriver', {
                get: () => undefined
            });
        )");
    }

    
    Result<void> hideAutomation() {
        return evaluate(R"(
            // Remove automation indicator properties
            delete window.cdc_adoQpoasnfa76pfcZLmcfl_Array;
            delete window.cdc_adoQpoasnfa76pfcZLmcfl_Promise;
            delete window.cdc_adoQpoasnfa76pfcZLmcfl_Symbol;

            // Hide chrome.runtime (used by some detection scripts)
            if (window.chrome && window.chrome.runtime) {
                delete window.chrome.runtime;
            }
        )");
    }

    
    Result<void> mockPlugins() {
        return evaluate(R"(
            Object.defineProperty(navigator, 'plugins', {
                get: () => {
                    const plugins = [
                        { name: 'Chrome PDF Plugin', filename: 'internal-pdf-viewer', description: 'Portable Document Format' },
                        { name: 'Chrome PDF Viewer', filename: 'mhjfbmdgcfjbbpaeojofohoefgiehjai', description: '' },
                        { name: 'Native Client', filename: 'internal-nacl-plugin', description: '' }
                    ];
                    plugins.item = (i) => plugins[i];
                    plugins.namedItem = (name) => plugins.find(p => p.name === name);
                    plugins.refresh = () => {};
                    return plugins;
                }
            });
        )");
    }

    
    Result<void> mockLanguages() {
        return evaluate(R"(
            Object.defineProperty(navigator, 'languages', {
                get: () => ['en-US', 'en']
            });
        )");
    }

    
    Result<void> fixPermissions() {
        return evaluate(R"(
            const originalQuery = window.navigator.permissions.query;
            window.navigator.permissions.query = (parameters) => (
                parameters.name === 'notifications' ?
                    Promise.resolve({ state: Notification.permission }) :
                    originalQuery(parameters)
            );
        )");
    }

    
    Result<void> fixWebGL() {
        return evaluate(R"(
            const getParameter = WebGLRenderingContext.prototype.getParameter;
            WebGLRenderingContext.prototype.getParameter = function(parameter) {
                if (parameter === 37445) {
                    return 'Intel Inc.';
                }
                if (parameter === 37446) {
                    return 'Intel Iris OpenGL Engine';
                }
                return getParameter.apply(this, arguments);
            };
        )");
    }

    
    Result<void> fixChrome() {
        return evaluate(R"(
            if (!window.chrome) {
                window.chrome = {};
            }
            window.chrome.app = {
                isInstalled: false,
                InstallState: { DISABLED: 'disabled', INSTALLED: 'installed', NOT_INSTALLED: 'not_installed' },
                RunningState: { CANNOT_RUN: 'cannot_run', READY_TO_RUN: 'ready_to_run', RUNNING: 'running' }
            };
            window.chrome.csi = function() {};
            window.chrome.loadTimes = function() {};
        )");
    }

    
    static void humanDelay(int minMs = 50, int maxMs = 200) {
        static thread_local std::mt19937 rng(std::random_device{}());
        std::uniform_int_distribution<int> dist(minMs, maxMs);
        std::this_thread::sleep_for(std::chrono::milliseconds(dist(rng)));
    }

    
    static std::vector<std::pair<double, double>> humanMousePath(
        double startX, double startY, double endX, double endY, int steps = 10) {

        std::vector<std::pair<double, double>> path;
        static thread_local std::mt19937 rng(std::random_device{}());
        std::normal_distribution<double> noise(0, 2);

        for (int i = 0; i <= steps; ++i) {
            double t = static_cast<double>(i) / steps;
            
            double eased = t < 0.5 ? 2 * t * t : 1 - std::pow(-2 * t + 2, 2) / 2;

            double x = startX + (endX - startX) * eased + noise(rng);
            double y = startY + (endY - startY) * eased + noise(rng);
            path.emplace_back(x, y);
        }

        
        path.back() = {endX, endY};
        return path;
    }

private:
    Result<void> evaluate(const std::string& script) {
        auto response = connection_.sendCommandSync("Runtime.evaluate",
            Params().set("expression", script).build());
        if (response.hasError) {
            return Error::fromCDPResponse(response);
        }
        return {};
    }

    CDPConnection& connection_;
};


struct ResourceStats {
    int64_t pendingRequests = 0;
    int64_t completedRequests = 0;
    int64_t failedRequests = 0;
    int64_t activeConnections = 0;
    int64_t eventHandlerCount = 0;
    int64_t totalBytesReceived = 0;
    int64_t totalBytesSent = 0;

    std::string format() const {
        std::ostringstream oss;
        oss << "pending=" << pendingRequests
            << " completed=" << completedRequests
            << " failed=" << failedRequests
            << " active_connections=" << activeConnections
            << " handlers=" << eventHandlerCount
            << " rx=" << totalBytesReceived << "B"
            << " tx=" << totalBytesSent << "B";
        return oss.str();
    }
};

} 
} 
