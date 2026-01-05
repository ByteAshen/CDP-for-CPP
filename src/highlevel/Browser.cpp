

#include "cdp/highlevel/Browser.hpp"
#include <algorithm>

namespace cdp {
namespace highlevel {


ManagedPage::ManagedPage(std::unique_ptr<CDPClient> client, const std::string& targetId,
                         const std::string& contextId)
    : client_(std::move(client)), targetId_(targetId), contextId_(contextId) {
    if (client_) {
        page_ = std::make_unique<Page>(*client_);
    }
}

ManagedPage::~ManagedPage() {
    
}

ManagedPage::ManagedPage(ManagedPage&& other) noexcept
    : client_(std::move(other.client_))
    , page_(std::move(other.page_))
    , targetId_(std::move(other.targetId_))
    , contextId_(std::move(other.contextId_)) {
}

ManagedPage& ManagedPage::operator=(ManagedPage&& other) noexcept {
    if (this != &other) {
        client_ = std::move(other.client_);
        page_ = std::move(other.page_);
        targetId_ = std::move(other.targetId_);
        contextId_ = std::move(other.contextId_);
    }
    return *this;
}

Result<void> ManagedPage::close() {
    if (!client_ || !client_->isConnected()) {
        return Result<void>::success();
    }

    
    auto resp = client_->Target.closeTarget(targetId_);
    if (resp.hasError) {
        return Result<void>::failure(resp.errorCode, resp.errorMessage);
    }

    client_->disconnect();
    return Result<void>::success();
}

Result<void> ManagedPage::bringToFront() {
    if (!client_ || !client_->isConnected()) {
        return Result<void>::failure("Page not connected");
    }

    auto resp = client_->Page.bringToFront();
    if (resp.hasError) {
        return Result<void>::failure(resp.errorCode, resp.errorMessage);
    }
    return Result<void>::success();
}


BrowserContext::BrowserContext(Browser* browser, const std::string& contextId,
                               const BrowserContextOptions& options)
    : browser_(browser), contextId_(contextId), options_(options) {
}

BrowserContext::~BrowserContext() {
    
    std::lock_guard<std::mutex> lock(mutex_);
    pages_.clear();
}

Result<ManagedPage*> BrowserContext::newPage(const NewPageOptions& options) {
    auto result = browser_->createPage(options.url, options.width, options.height,
                                        contextId_, options.background);
    if (!result) {
        return Result<ManagedPage*>::failure(result.error());
    }

    std::lock_guard<std::mutex> lock(mutex_);
    pages_.push_back(std::move(result.value()));
    ManagedPage* page = pages_.back().get();

    
    if (options_.proxyCredentials.has_value()) {
        setupProxyAuth(page);
    }

    return Result<ManagedPage*>(page);
}

void BrowserContext::setupProxyAuth(ManagedPage* page) {
    if (!page || !options_.proxyCredentials.has_value()) {
        return;
    }

    
    CDPClient* clientPtr = &page->client();
    const auto creds = options_.proxyCredentials.value();  

    
    clientPtr->Fetch.enable({}, true);  

    
    clientPtr->Fetch.onAuthRequired([clientPtr, creds](const std::string& requestId,
                                                        const JsonValue&,
                                                        const std::string&,
                                                        const std::string&,
                                                        const JsonValue&) {
        
        AuthChallengeResponse authResponse = AuthChallengeResponse::credentials(
            creds.username, creds.password);
        clientPtr->Fetch.continueWithAuthAsync(requestId, authResponse);
    });

    
    clientPtr->Fetch.onRequestPaused([clientPtr](const std::string& requestId,
                                                  const JsonValue&,
                                                  const std::string&,
                                                  const std::string&,
                                                  const JsonValue&,
                                                  int,
                                                  const std::string&,
                                                  const JsonValue&,
                                                  const std::string&) {
        
        clientPtr->Fetch.continueRequestAsync(requestId);
    });
}

std::vector<ManagedPage*> BrowserContext::pages() {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<ManagedPage*> result;
    for (auto& page : pages_) {
        if (page && page->isConnected()) {
            result.push_back(page.get());
        }
    }
    return result;
}

Result<void> BrowserContext::close() {
    
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
            return Result<void>::failure(resp.errorCode, resp.errorMessage);
        }
    }

    return Result<void>::success();
}


Browser::Browser() : Browser(CDPClientConfig{}) {
}

Browser::Browser(const CDPClientConfig& config)
    : config_(config), browserClient_(config) {
}

Browser::~Browser() {
    disconnect();
}

Result<void> Browser::connect() {
    if (connected_) {
        return Result<void>::success();
    }

    
    if (!browserClient_.connectToBrowser()) {
        return Result<void>::failure("Failed to connect to browser endpoint");
    }

    
    defaultContext_ = std::make_unique<BrowserContext>(this, "");

    connected_ = true;
    return Result<void>::success();
}

void Browser::disconnect() {
    if (!connected_) {
        return;
    }

    std::lock_guard<std::mutex> lock(mutex_);

    
    for (auto& ctx : incognitoContexts_) {
        if (ctx) {
            ctx->close();
        }
    }
    incognitoContexts_.clear();

    
    if (defaultContext_) {
        std::lock_guard<std::mutex> ctxLock(defaultContext_->mutex_);
        defaultContext_->pages_.clear();
    }

    browserClient_.disconnect();
    connected_ = false;
}

bool Browser::isConnected() const {
    return connected_ && browserClient_.isConnected();
}

std::string Browser::version() {
    if (!isConnected()) return "";

    auto resp = browserClient_.Browser.getVersion();
    if (resp.hasError) return "";

    return resp.result["product"].getString();
}

std::string Browser::userAgent() {
    if (!isConnected()) return "";

    auto resp = browserClient_.Browser.getVersion();
    if (resp.hasError) return "";

    return resp.result["userAgent"].getString();
}

BrowserContext& Browser::defaultContext() {
    return *defaultContext_;
}

Result<BrowserContext*> Browser::createIncognitoContext(const BrowserContextOptions& options) {
    if (!isConnected()) {
        return Result<BrowserContext*>::failure("Browser not connected");
    }

    auto resp = browserClient_.Target.createBrowserContext(
        options.disposeOnDetach,
        options.proxyServer,
        options.proxyBypassList
    );

    if (resp.hasError) {
        return Result<BrowserContext*>::failure(resp.errorCode, resp.errorMessage);
    }

    std::string contextId = resp.result["browserContextId"].getString();

    std::lock_guard<std::mutex> lock(mutex_);
    incognitoContexts_.push_back(std::make_unique<BrowserContext>(this, contextId, options));
    return Result<BrowserContext*>(incognitoContexts_.back().get());
}

std::vector<BrowserContext*> Browser::contexts() {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<BrowserContext*> result;
    result.push_back(defaultContext_.get());
    for (auto& ctx : incognitoContexts_) {
        result.push_back(ctx.get());
    }
    return result;
}

Result<ManagedPage*> Browser::newPage(const NewPageOptions& options) {
    return defaultContext_->newPage(options);
}

std::vector<ManagedPage*> Browser::pages() {
    std::vector<ManagedPage*> result;

    
    auto defaultPages = defaultContext_->pages();
    result.insert(result.end(), defaultPages.begin(), defaultPages.end());

    
    std::lock_guard<std::mutex> lock(mutex_);
    for (auto& ctx : incognitoContexts_) {
        auto ctxPages = ctx->pages();
        result.insert(result.end(), ctxPages.begin(), ctxPages.end());
    }

    return result;
}

Result<void> Browser::close() {
    if (!isConnected()) {
        return Result<void>::success();
    }

    
    auto resp = browserClient_.Browser.close();
    if (resp.hasError) {
        return Result<void>::failure(resp.errorCode, resp.errorMessage);
    }

    disconnect();
    return Result<void>::success();
}

Result<std::unique_ptr<ManagedPage>> Browser::createPage(const std::string& url,
                                                          int width, int height,
                                                          const std::string& browserContextId,
                                                          bool background) {
    if (!isConnected()) {
        return Result<std::unique_ptr<ManagedPage>>::failure("Browser not connected");
    }

    
    auto createResp = browserClient_.Target.createTarget(
        url.empty() ? "about:blank" : url,
        width,
        height,
        browserContextId,
        false,  
        false,  
        background
    );

    if (createResp.hasError) {
        return Result<std::unique_ptr<ManagedPage>>::failure(
            createResp.errorCode, createResp.errorMessage);
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
        return Result<std::unique_ptr<ManagedPage>>::failure(
            "Failed to find WebSocket URL for new target");
    }

    
    auto pageClient = std::make_unique<CDPClient>(config_);
    if (!pageClient->connect(wsUrl)) {
        browserClient_.Target.closeTarget(targetId);
        return Result<std::unique_ptr<ManagedPage>>::failure(
            "Failed to connect to new target");
    }

    auto managedPage = std::make_unique<ManagedPage>(
        std::move(pageClient), targetId, browserContextId);

    return Result<std::unique_ptr<ManagedPage>>(std::move(managedPage));
}

Result<void> Browser::closeTarget(const std::string& targetId) {
    auto resp = browserClient_.Target.closeTarget(targetId);
    if (resp.hasError) {
        return Result<void>::failure(resp.errorCode, resp.errorMessage);
    }
    return Result<void>::success();
}

} 
} 
