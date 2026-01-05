#pragma once


#include "ChromeLauncher.hpp"
#include "../protocol/CDPClient.hpp"
#include "../highlevel/Page.hpp"
#include "../highlevel/Result.hpp"
#include <memory>
#include <fstream>
#include <vector>
#include <optional>

namespace cdp {
namespace quick {


class QuickBrowser;
class QuickPage;
class QuickContext;
struct LaunchResult;
struct PageResult;
struct ContextResult;
struct ContextOptions;


LaunchResult launch(const ChromeLaunchOptions& options);
LaunchResult connect(const std::string& host, int port);
LaunchResult connectWs(const std::string& wsUrl);


class QuickPage {
public:
    ~QuickPage();

    
    QuickPage(const QuickPage&) = delete;
    QuickPage& operator=(const QuickPage&) = delete;
    QuickPage(QuickPage&&) noexcept;
    QuickPage& operator=(QuickPage&&) noexcept;

    
    bool navigate(const std::string& url, int timeoutMs = 30000);

    
    bool back();
    bool forward();

    
    bool reload(bool ignoreCache = false);

    
    std::string url();

    
    std::string title();

    
    std::string html();

    
    std::string text(const std::string& selector = "");

    
    std::string attribute(const std::string& selector, const std::string& attr);

    
    bool exists(const std::string& selector);

    
    int count(const std::string& selector);

    
    bool click(const std::string& selector, int timeoutMs = 5000);

    
    bool doubleClick(const std::string& selector, int timeoutMs = 5000);

    
    bool type(const std::string& selector, const std::string& text, int timeoutMs = 5000);

    
    bool append(const std::string& selector, const std::string& text, int timeoutMs = 5000);

    
    bool clear(const std::string& selector, int timeoutMs = 5000);

    
    bool press(const std::string& key);

    
    bool select(const std::string& selector, const std::string& value, int timeoutMs = 5000);

    
    bool check(const std::string& selector, bool checked = true, int timeoutMs = 5000);

    
    bool focus(const std::string& selector, int timeoutMs = 5000);

    
    bool hover(const std::string& selector, int timeoutMs = 5000);

    
    bool scrollTo(const std::string& selector, int timeoutMs = 5000);

    
    bool scroll(int x, int y);

    
    JsonValue eval(const std::string& expression);

    
    std::string evalString(const std::string& expression);

    
    bool exec(const std::string& script);

    
    bool screenshot(const std::string& filePath);

    
    std::vector<uint8_t> screenshotBytes();

    
    bool screenshotElement(const std::string& selector, const std::string& filePath);

    
    bool screenshotFullPage(const std::string& filePath);

    
    bool pdf(const std::string& filePath);

    
    bool waitFor(const std::string& selector, int timeoutMs = 30000);

    
    bool waitVisible(const std::string& selector, int timeoutMs = 30000);

    
    bool waitHidden(const std::string& selector, int timeoutMs = 30000);

    
    bool waitNavigation(int timeoutMs = 30000);

    
    bool waitNetworkIdle(int idleDurationMs = 500, int timeoutMs = 30000);

    
    void sleep(int ms);

    
    bool setCookie(const std::string& name, const std::string& value,
                   const std::string& domain = "", const std::string& path = "/");

    
    std::string getCookie(const std::string& name);

    
    std::string getAllCookies();

    
    bool deleteCookie(const std::string& name, const std::string& domain = "");

    
    bool deleteAllCookies();

    
    bool setViewport(int width, int height, double deviceScaleFactor = 1.0);

    
    bool emulateDevice(const std::string& deviceName);

    
    bool setUserAgent(const std::string& userAgent);

    
    bool blockUrls(const std::vector<std::string>& patterns);

    
    bool setHeaders(const std::map<std::string, std::string>& headers);

    
    bool enableInterception(std::function<void(const std::string& requestId,
                                                const std::string& url,
                                                const std::string& method)> handler);

    
    void onConsole(std::function<void(const std::string& type, const std::string& text)> handler);

    
    void autoAcceptDialogs(bool accept = true, const std::string& promptText = "");

    
    bool isConnected() const;

    
    bool isClosed() const { return closed_; }

    
    const std::string& lastError() const { return lastError_; }

    
    const std::string& targetId() const { return targetId_; }

    
    bool close();

    
    bool bringToFront();

    
    CDPClient& client() { return *client_; }

private:
    friend class QuickBrowser;
    friend class QuickContext;

    QuickPage(std::unique_ptr<CDPClient> client, const std::string& targetId,
              QuickBrowser* browser);

    std::unique_ptr<CDPClient> client_;
    std::unique_ptr<highlevel::Page> page_;
    std::string targetId_;
    mutable std::string lastError_;
    QuickBrowser* browser_;
    bool autoAccept_ = false;
    std::string promptText_;
    bool closed_ = false;
};


struct FetchRequest {
    std::string requestId;
    std::string url;
    std::string method;
    std::string resourceType;      
    JsonValue headers;             
    std::string postData;          
    QuickPage* page;               

    
    std::vector<HeaderEntry> getHeaders() const {
        return HeaderEntry::fromRequest(JsonObject{{"headers", headers}});
    }
};


class FetchAction {
public:
    explicit FetchAction(QuickPage* page, const std::string& requestId)
        : page_(page), requestId_(requestId) {}

    
    void continueRequest();

    
    void continueRequest(const std::vector<HeaderEntry>& headers,
                         const std::string& url = "",
                         const std::string& method = "",
                         const std::string& postData = "");

    
    void block(const std::string& reason = "BlockedByClient");

    
    void fulfill(int statusCode,
                 const std::vector<HeaderEntry>& headers,
                 const std::string& body);

    
    void fulfillText(int statusCode, const std::string& text,
                     const std::string& contentType = "text/plain");

    
    void fulfillJson(int statusCode, const std::string& json);

private:
    QuickPage* page_;
    std::string requestId_;
};


using FetchHandler = std::function<bool(const FetchRequest& request, FetchAction& action)>;


class QuickContext {
public:
    ~QuickContext();

    
    QuickContext(const QuickContext&) = delete;
    QuickContext& operator=(const QuickContext&) = delete;

    
    PageResult newPage(const std::string& url = "about:blank");

    
    std::vector<QuickPage*> pages();

    
    bool close();

    
    const std::string& id() const { return contextId_; }

    
    bool isDefault() const { return contextId_.empty(); }

    
    bool enableFetch(FetchHandler handler,
                     const std::vector<RequestPattern>& patterns = {RequestPattern::all()});

    
    void disableFetch();

    
    bool isFetchEnabled() const { return fetchEnabled_; }

private:
    friend class QuickBrowser;
    friend LaunchResult launch(const ChromeLaunchOptions& options);
    friend LaunchResult connect(const std::string& host, int port);
    friend LaunchResult connectWs(const std::string& wsUrl);

    QuickContext(QuickBrowser* browser, const std::string& contextId);

    
    void applyFetchToPage(QuickPage* page);

    QuickBrowser* browser_;
    std::string contextId_;
    std::vector<std::unique_ptr<QuickPage>> pages_;
    mutable std::mutex mutex_;

    
    bool fetchEnabled_ = false;
    FetchHandler fetchHandler_;
    std::vector<RequestPattern> fetchPatterns_;
};


struct ContextOptions {
    std::string proxyServer;         
    std::string proxyBypassList;     
    std::string proxyUsername;       
    std::string proxyPassword;       
};


struct PageResult {
    QuickPage* page = nullptr;
    std::string error;

    bool ok() const { return page != nullptr; }
    explicit operator bool() const { return ok(); }
    QuickPage* operator->() { return page; }
    QuickPage& operator*() { return *page; }
    QuickPage* get() { return page; }
};


struct ContextResult {
    QuickContext* context = nullptr;
    std::string error;

    bool ok() const { return context != nullptr; }
    explicit operator bool() const { return ok(); }
    QuickContext* operator->() { return context; }
    QuickContext& operator*() { return *context; }
    QuickContext* get() { return context; }
};


struct LaunchResult {
    std::unique_ptr<QuickBrowser> browser;
    std::string error;

    bool ok() const { return browser != nullptr; }
    explicit operator bool() const { return ok(); }
    QuickBrowser* operator->() { return browser.get(); }
    QuickBrowser& operator*() { return *browser; }
};


class QuickBrowser {
public:
    ~QuickBrowser();

    
    QuickBrowser(const QuickBrowser&) = delete;
    QuickBrowser& operator=(const QuickBrowser&) = delete;

    
    PageResult newPage(const std::string& url = "about:blank");

    
    std::vector<QuickPage*> pages();

    
    std::vector<CDPTarget> listPages();

    
    PageResult connectToPage(const CDPTarget& target);

    
    PageResult connectToPage(int index);

    
    ContextResult newContext(const ContextOptions& options = {});

    
    QuickContext& defaultContext() { return *defaultContext_; }

    
    std::vector<QuickContext*> contexts();

    
    bool enableFetch(FetchHandler handler,
                     const std::vector<RequestPattern>& patterns = {RequestPattern::all()});

    
    void disableFetch();

    
    bool isFetchEnabled() const { return fetchEnabled_; }

    
    std::string version();

    
    std::string userAgent();

    
    bool isConnected() const;

    
    bool close();

    
    int debuggingPort() const;

    
    const std::string& lastError() const { return lastError_; }

    
    CDPClient& browserClient() { return browserClient_; }

    
    ChromeLauncher* launcher() { return launcher_.get(); }

private:
    friend LaunchResult launch(const ChromeLaunchOptions& options);
    friend LaunchResult connect(const std::string& host, int port);
    friend LaunchResult connectWs(const std::string& wsUrl);
    friend class QuickContext;

    QuickBrowser(std::unique_ptr<ChromeLauncher> launcher, const CDPClientConfig& config);

    
    PageResult createPageInContext(const std::string& url, const std::string& contextId);

    std::unique_ptr<ChromeLauncher> launcher_;
    CDPClientConfig config_;
    CDPClient browserClient_;
    std::unique_ptr<QuickContext> defaultContext_;
    std::vector<std::unique_ptr<QuickContext>> incognitoContexts_;
    std::string lastError_;
    mutable std::mutex mutex_;

    
    bool fetchEnabled_ = false;
    FetchHandler fetchHandler_;
    std::vector<RequestPattern> fetchPatterns_;
};


LaunchResult launch(const ChromeLaunchOptions& options = ChromeLaunchOptions{});


LaunchResult launchHeadless();


LaunchResult launchWithViewport(int width, int height);


LaunchResult connect(const std::string& host = "localhost", int port = 9222);


LaunchResult connectWs(const std::string& wsUrl);

} 
} 
