//   
// CDP Torture Test - Intentionally Try to Break the API
//   
// This test attempts to find edge cases and race conditions by:
// - Racing context creation/destruction
// - Killing pages mid-request
// - Sending commands to dead targets
// - Overwhelming with concurrent operations
// - Testing error recovery
//   

#include <cdp/CDP.hpp>
#include <iostream>
#include <thread>
#include <atomic>
#include <mutex>
#include <random>
#include <chrono>
#include <vector>

namespace {

std::mutex g_printMutex;
std::atomic<int> g_passed{0};
std::atomic<int> g_failed{0};
std::atomic<int> g_crashed{0};

void result(const std::string& test, bool success, const std::string& detail = "") {
    std::lock_guard<std::mutex> lock(g_printMutex);
    if (success) {
        g_passed++;
        std::cout << "[PASS] " << test;
    } else {
        g_failed++;
        std::cout << "[FAIL] " << test;
    }
    if (!detail.empty()) std::cout << " - " << detail;
    std::cout << "\n";
}

void crash(const std::string& test, const std::string& what) {
    std::lock_guard<std::mutex> lock(g_printMutex);
    g_crashed++;
    std::cout << "[CRASH] " << test << " - " << what << "\n";
}

void info(const std::string& msg) {
    std::lock_guard<std::mutex> lock(g_printMutex);
    std::cout << "       " << msg << "\n";
}

void section(const std::string& title) {
    std::lock_guard<std::mutex> lock(g_printMutex);
    std::cout << "\n   ==========\n"
              << "  " << title << "\n"
              << "   ==========\n";
}

} // namespace

int main() {
    std::cout << "   ==========\n"
              << "     CDP Torture Test - Breaking the API\n"
              << "   ==========\n";


    // Launch browser

    section("Setup");

    cdp::ChromeLaunchOptions opts;
    opts.headless = false;
    opts.useTempProfile = true;

    auto browser = cdp::quick::launch(opts);
    if (!browser) {
        std::cerr << "Failed to launch: " << browser.error << "\n";
        return 1;
    }
    result("Browser launched", true, "port " + std::to_string(browser->debuggingPort()));

    //  
    // TEST 1: Rapid context creation/destruction
    //  
    section("Test 1: Rapid Context Create/Destroy");
    {
        std::atomic<int> created{0}, destroyed{0}, errors{0};
        std::vector<std::thread> threads;

        for (int i = 0; i < 10; ++i) {
            threads.emplace_back([&, i] {
                try {
                    for (int j = 0; j < 5; ++j) {
                        auto ctx = browser->newContext();
                        if (ctx) {
                            created++;
                            // Immediately destroy
                            ctx->close();
                            destroyed++;
                        } else {
                            errors++;
                        }
                    }
                } catch (const std::exception& e) {
                    crash("Context create/destroy thread " + std::to_string(i), e.what());
                }
            });
        }

        for (auto& t : threads) t.join();

        result("Rapid context cycling", errors == 0,
               "created=" + std::to_string(created.load()) +
               " destroyed=" + std::to_string(destroyed.load()) +
               " errors=" + std::to_string(errors.load()));
    }

    //  
    // TEST 2: Kill page mid-navigation
    //  
    section("Test 2: Kill Page Mid-Navigation");
    {
        std::atomic<int> killed{0}, navCompleted{0}, navFailed{0};

        for (int i = 0; i < 5; ++i) {
            try {
                auto ctx = browser->newContext();
                if (!ctx) continue;

                auto page = ctx->newPage("about:blank");
                if (!page) {
                    ctx->close();
                    continue;
                }

                // Start navigation in background
                std::thread navThread([&navCompleted, &navFailed, p = page.get()] {
                    try {
                        p->navigate("https://en.wikipedia.org/wiki/Main_Page");
                        navCompleted++;
                    } catch (...) {
                        navFailed++;
                    }
                });

                // Kill the page almost immediately
                std::this_thread::sleep_for(std::chrono::milliseconds(50));
                try {
                    page->close();
                    killed++;
                } catch (...) {}

                navThread.join();
                ctx->close();
            } catch (const std::exception& e) {
                crash("Kill mid-nav iteration " + std::to_string(i), e.what());
            }
        }

        result("Kill page mid-navigation", true,
               "killed=" + std::to_string(killed.load()) +
               " navCompleted=" + std::to_string(navCompleted.load()) +
               " navFailed=" + std::to_string(navFailed.load()));
    }

    //  
    // TEST 3: Commands to closed/dead targets
    //  
    section("Test 3: Commands to Dead Targets");
    {
        int handled = 0;
        int crashed = 0;

        try {
            auto ctx = browser->newContext();
            auto page = ctx->newPage("https://example.com");

            // Close the page
            page->close();

            // Now try to use it - should not crash, may return empty/error
            try {
                std::string title = page->title();
                // Empty or error title is acceptable for closed page
                handled++;
            } catch (...) {
                handled++;  // Exception is also acceptable
            }

            try {
                bool navOk = page->navigate("https://google.com");
                // navigate returns false or page errors - both acceptable
                handled++;
            } catch (...) {
                handled++;
            }

            try {
                std::string result = page->evalString("document.title");
                // Empty result is acceptable for closed page
                handled++;
            } catch (...) {
                handled++;
            }

            ctx->close();
        } catch (const std::exception& e) {
            crashed++;
            crash("Dead target test", e.what());
        }

        // Pass if we handled all 3 cases without crashing
        result("Commands to dead targets", handled == 3 && crashed == 0,
               "handled=" + std::to_string(handled) + " crashed=" + std::to_string(crashed));
    }

    //  
    // TEST 4: Race fetch enable/disable
    //  
    section("Test 4: Race Fetch Enable/Disable");
    {
        std::atomic<int> enables{0}, disables{0}, errors{0};

        try {
            auto ctx = browser->newContext();
            auto page = ctx->newPage("about:blank");

            std::vector<std::thread> threads;

            // Multiple threads enabling/disabling fetch
            for (int i = 0; i < 5; ++i) {
                threads.emplace_back([&ctx, &enables, &errors] {
                    try {
                        ctx->enableFetch([](const cdp::quick::FetchRequest&,
                                           cdp::quick::FetchAction& action) {
                            action.continueRequest();
                            return true;
                        });
                        enables++;
                    } catch (...) {
                        errors++;
                    }
                });

                threads.emplace_back([&ctx, &disables, &errors] {
                    try {
                        ctx->disableFetch();
                        disables++;
                    } catch (...) {
                        errors++;
                    }
                });
            }

            for (auto& t : threads) t.join();

            // Clean up
            ctx->disableFetch();
            ctx->close();
        } catch (const std::exception& e) {
            crash("Fetch race test", e.what());
        }

        result("Race fetch enable/disable", true,
               "enables=" + std::to_string(enables.load()) +
               " disables=" + std::to_string(disables.load()) +
               " errors=" + std::to_string(errors.load()));
    }

    //  
    // TEST 5: Massive concurrent page creation
    //  
    section("Test 5: Concurrent Page Flood");
    {
        std::atomic<int> created{0}, failed{0};

        auto ctx = browser->newContext();
        std::vector<std::thread> threads;

        for (int i = 0; i < 20; ++i) {
            threads.emplace_back([&] {
                try {
                    auto page = ctx->newPage("about:blank");
                    if (page) {
                        created++;
                    } else {
                        failed++;
                    }
                } catch (...) {
                    failed++;
                }
            });
        }

        for (auto& t : threads) t.join();

        result("Concurrent page flood", created > 0,
               "created=" + std::to_string(created.load()) +
               " failed=" + std::to_string(failed.load()));

        ctx->close();
    }

    //  
    // TEST 6: Eval during navigation
    //  
    section("Test 6: Eval During Navigation");
    {
        std::atomic<int> evalSuccess{0}, evalFail{0};

        try {
            auto ctx = browser->newContext();
            auto page = ctx->newPage("about:blank");

            // Start navigation
            std::thread navThread([p = page.get()] {
                try {
                    p->navigate("https://example.com");
                } catch (...) {}
            });

            // Spam eval calls during navigation
            for (int i = 0; i < 20; ++i) {
                try {
                    page->evalString("1 + 1");
                    evalSuccess++;
                } catch (...) {
                    evalFail++;
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }

            navThread.join();
            ctx->close();
        } catch (const std::exception& e) {
            crash("Eval during nav", e.what());
        }

        result("Eval during navigation", true,
               "success=" + std::to_string(evalSuccess.load()) +
               " fail=" + std::to_string(evalFail.load()));
    }

    //  
    // TEST 7: Close context with active fetch handlers
    //  
    section("Test 7: Close Context with Active Fetch");
    {
        std::atomic<int> requestsSeen{0};
        bool closedCleanly = false;

        try {
            auto ctx = browser->newContext();
            auto page = ctx->newPage("about:blank");

            ctx->enableFetch([&requestsSeen](const cdp::quick::FetchRequest& req,
                                              cdp::quick::FetchAction& action) {
                requestsSeen++;
                // Intentionally slow handler
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                action.continueRequest();
                return true;
            });

            // Start navigation that will trigger fetch
            std::thread navThread([p = page.get()] {
                try {
                    p->navigate("https://example.com");
                } catch (...) {}
            });

            // Close context while fetch might be in progress
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            ctx->close();
            closedCleanly = true;

            navThread.join();
        } catch (const std::exception& e) {
            crash("Close with active fetch", e.what());
        }

        result("Close context with active fetch", closedCleanly,
               "requests=" + std::to_string(requestsSeen.load()));
    }

    //  
    // TEST 8: Double close
    //  
    section("Test 8: Double Close Operations");
    {
        bool pageDoubleClose = false;
        bool contextDoubleClose = false;

        try {
            auto ctx = browser->newContext();
            auto page = ctx->newPage("about:blank");

            // Double close page
            page->close();
            try {
                page->close();  // Should not crash
                pageDoubleClose = true;
            } catch (...) {
                pageDoubleClose = true;  // Exception is also acceptable
            }

            // Double close context
            ctx->close();
            try {
                ctx->close();
                contextDoubleClose = true;
            } catch (...) {
                contextDoubleClose = true;
            }
        } catch (const std::exception& e) {
            crash("Double close", e.what());
        }

        result("Double close page", pageDoubleClose);
        result("Double close context", contextDoubleClose);
    }

    //  
    // TEST 9: Null/empty string handling
    //  
    section("Test 9: Edge Case Inputs");
    {
        bool emptyUrlHandled = false;
        bool emptyEvalHandled = false;
        bool nullishCookieHandled = false;

        try {
            auto ctx = browser->newContext();
            auto page = ctx->newPage("about:blank");

            // Empty URL
            try {
                page->navigate("");
                emptyUrlHandled = true;
            } catch (...) {
                emptyUrlHandled = true;  // Exception is fine
            }

            // Empty eval
            try {
                page->evalString("");
                emptyEvalHandled = true;
            } catch (...) {
                emptyEvalHandled = true;
            }

            // Empty cookie name
            try {
                page->setCookie("", "value", "example.com");
                nullishCookieHandled = true;
            } catch (...) {
                nullishCookieHandled = true;
            }

            ctx->close();
        } catch (const std::exception& e) {
            crash("Edge case inputs", e.what());
        }

        result("Empty URL handled", emptyUrlHandled);
        result("Empty eval handled", emptyEvalHandled);
        result("Empty cookie name handled", nullishCookieHandled);
    }

    //  
    // TEST 10: Stress the WebSocket connection
    //  
    section("Test 10: WebSocket Stress");
    {
        std::atomic<int> commands{0}, errors{0};

        try {
            auto ctx = browser->newContext();
            auto page = ctx->newPage("https://example.com");

            std::vector<std::thread> threads;

            // Hammer the connection with concurrent commands
            for (int i = 0; i < 10; ++i) {
                threads.emplace_back([&commands, &errors, p = page.get()] {
                    for (int j = 0; j < 50; ++j) {
                        try {
                            p->evalString("document.title");
                            commands++;
                        } catch (...) {
                            errors++;
                        }
                    }
                });
            }

            for (auto& t : threads) t.join();

            ctx->close();
        } catch (const std::exception& e) {
            crash("WebSocket stress", e.what());
        }

        result("WebSocket stress", commands > 0,
               "commands=" + std::to_string(commands.load()) +
               " errors=" + std::to_string(errors.load()));
    }

    //  
    // TEST 11: Interleaved context operations
    //  
    section("Test 11: Interleaved Context Operations");
    {
        std::atomic<int> ops{0}, errors{0};

        try {
            std::vector<cdp::quick::QuickContext*> contexts;
            std::mutex ctxMutex;

            std::vector<std::thread> threads;

            // Create contexts
            for (int i = 0; i < 5; ++i) {
                threads.emplace_back([&] {
                    try {
                        auto ctxResult = browser->newContext();
                        if (ctxResult) {
                            std::lock_guard<std::mutex> lock(ctxMutex);
                            contexts.push_back(ctxResult.get());
                            ops++;
                        }
                    } catch (...) {
                        errors++;
                    }
                });
            }

            for (auto& t : threads) t.join();
            threads.clear();

            // Operate on contexts concurrently
            for (size_t i = 0; i < contexts.size(); ++i) {
                threads.emplace_back([&ops, &errors, ctx = contexts[i]] {
                    try {
                        auto page = ctx->newPage("about:blank");
                        if (page) {
                            page->navigate("https://example.com");
                            ops++;
                        }
                    } catch (...) {
                        errors++;
                    }
                });
            }

            for (auto& t : threads) t.join();

            // Close all
            for (auto* ctx : contexts) {
                try {
                    ctx->close();
                    ops++;
                } catch (...) {
                    errors++;
                }
            }
        } catch (const std::exception& e) {
            crash("Interleaved ops", e.what());
        }

        result("Interleaved context operations", ops > 0,
               "ops=" + std::to_string(ops.load()) +
               " errors=" + std::to_string(errors.load()));
    }

    //  
    // TEST 12: Fetch handler that throws
    //  
    section("Test 12: Throwing Fetch Handler");
    {
        std::atomic<int> handlerCalls{0};
        bool survived = false;

        try {
            auto ctx = browser->newContext();
            auto page = ctx->newPage("about:blank");

            ctx->enableFetch([&handlerCalls](const cdp::quick::FetchRequest&,
                                              cdp::quick::FetchAction&) -> bool {
                handlerCalls++;
                throw std::runtime_error("Intentional handler crash!");
            });

            try {
                page->navigate("https://example.com");
            } catch (...) {}

            ctx->disableFetch();
            ctx->close();
            survived = true;
        } catch (const std::exception& e) {
            crash("Throwing handler", e.what());
        }

        result("Throwing fetch handler", survived,
               "handler calls=" + std::to_string(handlerCalls.load()));
    }

    //  
    // Summary
    //  
    section("Results");

    browser->close();

    int total = g_passed + g_failed;
    std::cout << "\n"
              << "  Passed:  " << g_passed.load() << "/" << total << "\n"
              << "  Failed:  " << g_failed.load() << "/" << total << "\n"
              << "  Crashed: " << g_crashed.load() << "\n"
              << "\n";

    if (g_crashed > 0) {
        std::cout << "  !!! CRASHES DETECTED - API needs hardening !!!\n";
    } else if (g_failed > 0) {
        std::cout << "  Some edge cases need attention.\n";
    } else {
        std::cout << "  API survived the torture test!\n";
    }

    return (g_crashed > 0) ? 2 : (g_failed > 0) ? 1 : 0;
}
