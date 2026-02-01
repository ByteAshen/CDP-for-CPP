//   
// Comprehensive CDP for C++ API Test
//   
// Demonstrates:
// - Extension loading with incognito support
// - Multiple browser contexts (incognito) with isolated storage
// - Multiple pages per context
// - Low-level vs context-level fetch interception
// - Concurrent operations across all pages
//   

#include <cdp/CDP.hpp>
#include <iostream>
#include <thread>
#include <atomic>
#include <mutex>
#include <filesystem>
#include <functional>

//   
// Test Utilities
//   

namespace {

std::mutex g_printMutex;

void printSection(const std::string& title) {
    std::lock_guard<std::mutex> lock(g_printMutex);
    std::cout << "\n   ==========\n"
              << "  " << title << "\n"
              << "   ==========\n";
}

void printSubsection(const std::string& title) {
    std::lock_guard<std::mutex> lock(g_printMutex);
    std::cout << "\n--- " << title << " ---\n";
}

void printResult(const std::string& test, bool success, const std::string& details = "") {
    std::lock_guard<std::mutex> lock(g_printMutex);
    std::cout << (success ? "[PASS] " : "[FAIL] ") << test;
    if (!details.empty()) std::cout << " - " << details;
    std::cout << "\n";
}

void printInfo(const std::string& info) {
    std::lock_guard<std::mutex> lock(g_printMutex);
    std::cout << "       " << info << "\n";
}

// Interception statistics
struct Stats {
    std::atomic<int> requests{0};
    std::atomic<int> handled{0};
};

// Find test extension relative to executable
std::string findTestExtension() {
    namespace fs = std::filesystem;
    fs::path cwd = fs::current_path();

    for (const auto& path : {
        cwd / "test_extension",
        cwd / ".." / "examples" / "test_extension",
        cwd / ".." / ".." / "examples" / "test_extension",
        cwd / ".." / ".." / ".." / "examples" / "test_extension",
        fs::path(__FILE__).parent_path() / "test_extension"
    }) {
        if (fs::exists(path / "manifest.json")) {
            return fs::absolute(path).string();
        }
    }
    return "";
}

// Setup low-level image blocking on a page
void setupImageBlocker(cdp::quick::QuickPage* page, Stats& stats) {
    auto& client = page->client();
    client.Fetch.enable({cdp::RequestPattern::type("Image")}, false);
    client.Fetch.onRequestPaused([&stats, &client](
            const std::string& requestId, const cdp::JsonValue&,
            const std::string&, const std::string& resourceType,
            const cdp::JsonValue&, int, const std::string&,
            const cdp::JsonValue&, const std::string&) {
        stats.requests++;
        if (resourceType == "Image") {
            stats.handled++;
            client.Fetch.failRequestAsync(requestId, "BlockedByClient");
        } else {
            client.Fetch.continueRequestAsync(requestId);
        }
    });
}

// Check if extension banner is present on page
bool checkExtensionLoaded(cdp::quick::QuickPage* page, const std::string& name) {
    page->navigate("https://example.com");

    for (int i = 0; i < 10; ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(300));
        if (page->evalString("document.getElementById('cdp-test-extension-banner') ? 'found' : ''") == "found") {
            std::string mode = page->evalString("document.getElementById('cdp-ext-mode')?.textContent || ''");
            printResult(name + " extension loaded", true, "Banner found");
            printInfo("  Mode: " + mode);
            return true;
        }
    }
    printResult(name + " extension loaded", false, "Banner not found");
    return false;
}

} // anonymous namespace

//   
// Main
//   

int main() {
    std::cout << "   ==========\n"
              << "     CDP for C++ - Comprehensive API Showcase\n"
              << "   ==========\n";


    // Extension Discovery

    printSection("Extension Discovery");

    std::string extensionPath = findTestExtension();
    bool hasExtension = !extensionPath.empty();
    printResult("Test extension found", hasExtension,
                hasExtension ? extensionPath : "Not found - tests will be skipped");


    // Browser Launch

    printSection("Browser Launch");

    cdp::ChromeLaunchOptions opts;
    opts.headless = false;
    opts.useTempProfile = true;
    opts.windowWidth = 1400;
    opts.windowHeight = 900;

    if (hasExtension) {
        opts.extensions = {extensionPath};
        opts.extensionIncognitoEnabled = true;
        opts.extensionFileAccessEnabled = true;
        opts.disableExtensions = false;
        printInfo("Extension configured with incognito=true");
    }

    auto browser = cdp::quick::launch(opts);
    if (!browser) {
        std::cerr << "Failed to launch: " << browser.error << "\n";
        return 1;
    }

    printResult("Browser launched", true, "port " + std::to_string(browser->debuggingPort()));
    printResult("Browser version", true, browser->version());


    // Create Contexts and Pages

    printSection("Creating Contexts and Pages");

    auto ctx1 = browser->newContext();
    auto ctx2 = browser->newContext();
    auto ctx3 = browser->newContext();

    if (!ctx1 || !ctx2 || !ctx3) {
        std::cerr << "Failed to create contexts\n";
        return 1;
    }

    printResult("Context 1 (Low-level API)", true, ctx1->id().substr(0, 12) + "...");
    printResult("Context 2 (Context-level API)", true, ctx2->id().substr(0, 12) + "...");
    printResult("Context 3 (Context-level API)", true, ctx3->id().substr(0, 12) + "...");

    // Create pages
    auto ctx1_page1 = ctx1->newPage("about:blank");
    auto ctx1_page2 = ctx1->newPage("about:blank");
    auto ctx2_page1 = ctx2->newPage("about:blank");
    auto ctx2_page2 = ctx2->newPage("about:blank");
    auto ctx3_page1 = ctx3->newPage("about:blank");
    auto ctx3_page2 = ctx3->newPage("about:blank");
    auto defaultPage1 = browser->newPage("about:blank");
    auto defaultPage2 = browser->newPage("about:blank");

    printInfo("Created 8 pages across 4 contexts");


    // Low-Level Fetch API (per-page)

    printSection("APPROACH 1: Low-Level API (per-page)");
    printInfo("Each page needs manual Fetch.enable() + onRequestPaused()");

    Stats stats1;
    setupImageBlocker(ctx1_page1.get(), stats1);
    setupImageBlocker(ctx1_page2.get(), stats1);
    printResult("Image blocking enabled", true, "2 pages configured");


    // Context-Level Fetch API

    printSection("APPROACH 2: Context-Level API");
    printInfo("Single call applies to ALL pages in context");

    Stats stats2, stats3;

    // Context 2: API Mocking
    ctx2->enableFetch([&stats2](const cdp::quick::FetchRequest& req,
                                 cdp::quick::FetchAction& action) {
        stats2.requests++;
        if (req.url.find("/get") != std::string::npos) {
            stats2.handled++;
            action.fulfillJson(200, R"({"mocked": true, "context": "2"})");
            printInfo("[CTX2] Mocked: " + req.url.substr(0, 40));
            return true;
        }
        return false;
    }, {cdp::RequestPattern::url("*/get*")});

    printResult("Context 2: API mocking enabled", true, "mocks */get* requests");

    // Context 3: Header Injection
    ctx3->enableFetch([&stats3](const cdp::quick::FetchRequest& req,
                                 cdp::quick::FetchAction& action) {
        stats3.requests++;
        stats3.handled++;
        auto headers = req.getHeaders();
        cdp::HeaderEntry::set(headers, "X-CDP-Injected", "true");
        action.continueRequest(headers);
        if (req.resourceType == "Document") {
            printInfo("[CTX3] Injected headers: " + req.url.substr(0, 40));
        }
        return true;
    });

    printResult("Context 3: Header injection enabled", true, "all requests modified");

    // New page gets handler automatically
    auto ctx2_page3 = ctx2->newPage("about:blank");
    printResult("New page in Context 2", true, "automatically has mocking handler");


    // Concurrent Navigation

    printSection("Concurrent Navigation");

    struct NavTask {
        cdp::quick::QuickPage* page;
        const char* url;
        const char* name;
    };

    NavTask tasks[] = {
        {ctx1_page1.get(), "https://en.wikipedia.org/wiki/Cat", "CTX1-P1"},
        {ctx1_page2.get(), "https://en.wikipedia.org/wiki/Dog", "CTX1-P2"},
        {ctx2_page1.get(), "https://httpbin.org/get", "CTX2-P1"},
        {ctx2_page2.get(), "https://httpbin.org/get?test=1", "CTX2-P2"},
        {ctx2_page3.get(), "https://httpbin.org/get?auto=true", "CTX2-P3"},
        {ctx3_page1.get(), "https://httpbin.org/headers", "CTX3-P1"},
        {ctx3_page2.get(), "https://example.com", "CTX3-P2"},
        {defaultPage1.get(), "https://httpbin.org/get", "DEFAULT-P1"},
        {defaultPage2.get(), "https://example.com", "DEFAULT-P2"},
    };

    std::atomic<int> completed{0};
    std::vector<std::thread> threads;
    threads.reserve(std::size(tasks));

    for (const auto& t : tasks) {
        threads.emplace_back([&completed, page = t.page, url = t.url, name = t.name] {
            page->navigate(url);
            completed++;
            printInfo(std::string(name) + " loaded: " + page->title());
        });
    }

    for (auto& t : threads) t.join();

    printResult("All pages navigated", completed == std::size(tasks),
                std::to_string(completed.load()) + "/" + std::to_string(std::size(tasks)));


    // Verify Results

    printSection("Interception Results");

    printSubsection("Context 1: Image Blocker");
    printInfo("Requests: " + std::to_string(stats1.requests.load()) +
              ", Blocked: " + std::to_string(stats1.handled.load()));
    printResult("Image blocking worked", stats1.handled > 0);

    printSubsection("Context 2: API Mocker");
    printInfo("Requests: " + std::to_string(stats2.requests.load()) +
              ", Mocked: " + std::to_string(stats2.handled.load()));
    printResult("API mocking worked", stats2.handled > 0);

    bool hasMocked = ctx2_page1->evalString("document.body.innerText").find("mocked") != std::string::npos;
    printResult("Mocked content visible", hasMocked);

    bool autoApplied = ctx2_page3->evalString("document.body.innerText").find("mocked") != std::string::npos;
    printResult("Auto-applied handler worked", autoApplied);

    printSubsection("Context 3: Header Injector");
    printInfo("Requests: " + std::to_string(stats3.requests.load()) +
              ", Modified: " + std::to_string(stats3.handled.load()));

    std::string headers = ctx3_page1->evalString("document.body.innerText");
    bool hasHeader = headers.find("X-CDP-Injected") != std::string::npos ||
                     headers.find("X-Cdp-Injected") != std::string::npos ||
                     headers.find("x-cdp-injected") != std::string::npos;
    printResult("Injected headers visible", hasHeader);


    // Cookie Isolation

    printSection("Cookie Isolation");

    ctx1_page1->setCookie("ctx", "1", "en.wikipedia.org");
    ctx2_page1->setCookie("ctx", "2", "httpbin.org");
    ctx3_page1->setCookie("ctx", "3", "httpbin.org");
    defaultPage1->setCookie("ctx", "default", "httpbin.org");

    printResult("Context 1 cookie", ctx1_page1->getCookie("ctx") == "1");
    printResult("Context 2 cookie", ctx2_page1->getCookie("ctx") == "2");
    printResult("Context 3 cookie", ctx3_page1->getCookie("ctx") == "3");
    printResult("Default cookie", defaultPage1->getCookie("ctx") == "default");


    // Extension Verification

    if (hasExtension) {
        printSection("Extension Verification");
        printInfo("Checking extension works in all contexts (including incognito)");

        int passed = 0;
        passed += checkExtensionLoaded(defaultPage1.get(), "Default context");
        passed += checkExtensionLoaded(ctx1_page1.get(), "Incognito context 1");
        passed += checkExtensionLoaded(ctx2_page1.get(), "Incognito context 2");
        passed += checkExtensionLoaded(ctx3_page1.get(), "Incognito context 3");

        printSubsection("Summary");
        printResult("Extension works in all contexts", passed == 4,
                    std::to_string(passed) + "/4");
        printResult("Extension works in incognito", passed >= 3,
                    passed >= 3 ? "Verified!" : "Failed");
    }


    // Final State

    printSection("Final State");

    for (auto* ctx : browser->contexts()) {
        std::string type = ctx->isDefault() ? "DEFAULT" : "INCOGNITO";
        std::string fetch = ctx->isFetchEnabled() ? " [FETCH]" : "";
        printInfo("[" + type + "] " +
                  (ctx->isDefault() ? "default" : ctx->id().substr(0, 8)) +
                  " - " + std::to_string(ctx->pages().size()) + " pages" + fetch);
    }


    // Cleanup

    printSection("Cleanup");

    ctx1_page1->client().Fetch.disable();
    ctx1_page2->client().Fetch.disable();
    printResult("Context 1 fetch disabled", true, "per-page");

    ctx2->disableFetch();
    ctx3->disableFetch();
    printResult("Context 2-3 fetch disabled", true, "context-level");

    ctx1->close();
    ctx2->close();
    ctx3->close();
    browser->close();
    printResult("All contexts closed", true);


    // Summary

    printSection("Complete!");
    std::cout << R"(
API Comparison:
  +------------------------+------------------------------------------+
  | Approach               | Use Case                                 |
  +------------------------+------------------------------------------+
  | Low-Level API          | Per-page control, different handlers     |
  | (Fetch.onRequestPaused)| Most verbose, most flexible              |
  +------------------------+------------------------------------------+
  | Context-Level API      | Context-wide rules, auto-applies to      |
  | (context->enableFetch) | new pages, cleaner FetchAction API       |
  +------------------------+------------------------------------------+
  | Extension Loading      | Pre-install extensions with permissions  |
  | (ChromeLaunchOptions)  | Works in incognito when enabled          |
  +------------------------+------------------------------------------+
)";

    return 0;
}
