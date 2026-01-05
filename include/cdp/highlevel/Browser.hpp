#pragma once

#include "Result.hpp"
#include "Page.hpp"
#include "../protocol/CDPClient.hpp"
#include <string>
#include <vector>
#include <memory>
#include <map>
#include <mutex>
#include <optional>
#include <functional>

namespace cdp {
namespace highlevel {


class Browser;
class BrowserContext;


struct ProxyCredentials {
    std::string username;
    std::string password;
};


struct BrowserContextOptions {
    bool disposeOnDetach = true;    
    std::string proxyServer;         
    std::string proxyBypassList;     
    std::optional<ProxyCredentials> proxyCredentials;  
};


struct NewPageOptions {
    std::string url = "about:blank";  
    int width = 0;                     
    int height = 0;                    
    bool background = false;           
};


class ManagedPage {
public:
    ManagedPage(std::unique_ptr<CDPClient> client, const std::string& targetId,
                const std::string& contextId = "");
    ~ManagedPage();

    
    ManagedPage(const ManagedPage&) = delete;
    ManagedPage& operator=(const ManagedPage&) = delete;
    ManagedPage(ManagedPage&&) noexcept;
    ManagedPage& operator=(ManagedPage&&) noexcept;

    
    Page& page() { return *page_; }
    const Page& page() const { return *page_; }

    
    CDPClient& client() { return *client_; }

    
    const std::string& targetId() const { return targetId_; }
    const std::string& contextId() const { return contextId_; }

    
    bool isConnected() const { return client_ && client_->isConnected(); }

    
    Result<void> close();

    
    Result<void> bringToFront();

private:
    std::unique_ptr<CDPClient> client_;
    std::unique_ptr<Page> page_;
    std::string targetId_;
    std::string contextId_;
};


class BrowserContext {
public:
    BrowserContext(Browser* browser, const std::string& contextId,
                   const BrowserContextOptions& options = {});
    ~BrowserContext();

    
    BrowserContext(const BrowserContext&) = delete;
    BrowserContext& operator=(const BrowserContext&) = delete;

    
    const std::string& id() const { return contextId_; }

    
    bool isDefault() const { return contextId_.empty(); }

    
    Result<ManagedPage*> newPage(const NewPageOptions& options = {});

    
    std::vector<ManagedPage*> pages();

    
    Result<void> close();

    
    Browser& browser() { return *browser_; }

    
    bool hasProxyCredentials() const { return options_.proxyCredentials.has_value(); }

    
    const std::optional<ProxyCredentials>& proxyCredentials() const { return options_.proxyCredentials; }

private:
    friend class Browser;

    
    void setupProxyAuth(ManagedPage* page);

    Browser* browser_;
    std::string contextId_;
    BrowserContextOptions options_;
    std::vector<std::unique_ptr<ManagedPage>> pages_;
    mutable std::mutex mutex_;
};


class Browser {
public:
    Browser();
    explicit Browser(const CDPClientConfig& config);
    ~Browser();

    
    Browser(const Browser&) = delete;
    Browser& operator=(const Browser&) = delete;

    
    Result<void> connect();

    
    void disconnect();

    
    bool isConnected() const;

    
    std::string version();

    
    std::string userAgent();

    
    BrowserContext& defaultContext();

    
    Result<BrowserContext*> createIncognitoContext(const BrowserContextOptions& options = {});

    
    std::vector<BrowserContext*> contexts();

    
    Result<ManagedPage*> newPage(const NewPageOptions& options = {});

    
    std::vector<ManagedPage*> pages();

    
    Result<void> close();

    
    CDPClient& browserClient() { return browserClient_; }

    
    const CDPClientConfig& config() const { return config_; }

private:
    friend class BrowserContext;

    
    Result<std::unique_ptr<ManagedPage>> createPage(const std::string& url,
                                                     int width, int height,
                                                     const std::string& browserContextId,
                                                     bool background);

    
    Result<void> closeTarget(const std::string& targetId);

    CDPClientConfig config_;
    CDPClient browserClient_;  
    std::unique_ptr<BrowserContext> defaultContext_;
    std::vector<std::unique_ptr<BrowserContext>> incognitoContexts_;
    mutable std::mutex mutex_;
    bool connected_ = false;
};


class ScopedPage {
public:
    
    explicit ScopedPage(Browser& browser, const std::string& url = "about:blank")
        : browser_(&browser), context_(nullptr), page_(nullptr) {
        auto result = browser.newPage(NewPageOptions{url});
        if (result) {
            page_ = result.value();
        }
    }

    
    ScopedPage(BrowserContext& context, const std::string& url = "about:blank")
        : browser_(nullptr), context_(&context), page_(nullptr) {
        auto result = context.newPage(NewPageOptions{url});
        if (result) {
            page_ = result.value();
        }
    }

    
    ScopedPage(const ScopedPage&) = delete;
    ScopedPage& operator=(const ScopedPage&) = delete;

    
    ScopedPage(ScopedPage&& other) noexcept
        : browser_(other.browser_), context_(other.context_), page_(other.page_) {
        other.page_ = nullptr;
    }

    ScopedPage& operator=(ScopedPage&& other) noexcept {
        if (this != &other) {
            close();
            browser_ = other.browser_;
            context_ = other.context_;
            page_ = other.page_;
            other.page_ = nullptr;
        }
        return *this;
    }

    
    ~ScopedPage() {
        close();
    }

    
    bool valid() const { return page_ != nullptr && page_->isConnected(); }
    explicit operator bool() const { return valid(); }

    
    ManagedPage* get() { return page_; }
    const ManagedPage* get() const { return page_; }
    ManagedPage* operator->() { return page_; }
    const ManagedPage* operator->() const { return page_; }
    ManagedPage& operator*() { return *page_; }
    const ManagedPage& operator*() const { return *page_; }

    
    Page* page() { return page_ ? &page_->page() : nullptr; }
    const Page* page() const { return page_ ? &page_->page() : nullptr; }

    
    CDPClient* client() { return page_ ? &page_->client() : nullptr; }
    const CDPClient* client() const { return page_ ? &page_->client() : nullptr; }

    
    void close() {
        if (page_) {
            page_->close();
            page_ = nullptr;
        }
    }

    
    ManagedPage* release() {
        ManagedPage* p = page_;
        page_ = nullptr;
        return p;
    }

private:
    Browser* browser_;
    BrowserContext* context_;
    ManagedPage* page_;
};


class ScopedContext {
public:
    explicit ScopedContext(Browser& browser, const BrowserContextOptions& options = {})
        : browser_(&browser), context_(nullptr) {
        auto result = browser.createIncognitoContext(options);
        if (result) {
            context_ = result.value();
        }
    }

    
    ScopedContext(const ScopedContext&) = delete;
    ScopedContext& operator=(const ScopedContext&) = delete;

    
    ScopedContext(ScopedContext&& other) noexcept
        : browser_(other.browser_), context_(other.context_) {
        other.context_ = nullptr;
    }

    ScopedContext& operator=(ScopedContext&& other) noexcept {
        if (this != &other) {
            close();
            browser_ = other.browser_;
            context_ = other.context_;
            other.context_ = nullptr;
        }
        return *this;
    }

    ~ScopedContext() {
        close();
    }

    bool valid() const { return context_ != nullptr; }
    explicit operator bool() const { return valid(); }

    BrowserContext* get() { return context_; }
    const BrowserContext* get() const { return context_; }
    BrowserContext* operator->() { return context_; }
    const BrowserContext* operator->() const { return context_; }
    BrowserContext& operator*() { return *context_; }
    const BrowserContext& operator*() const { return *context_; }

    void close() {
        if (context_) {
            context_->close();
            context_ = nullptr;
        }
    }

    BrowserContext* release() {
        BrowserContext* c = context_;
        context_ = nullptr;
        return c;
    }

private:
    Browser* browser_;
    BrowserContext* context_;
};

} 
} 
