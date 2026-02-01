// CDP High-Level API Test
// Start Chrome with: chrome.exe --remote-debugging-port=9222

#include <cdp/CDP.hpp>
#include <iostream>
#include <fstream>
#include <thread>
#include <chrono>

// Use explicit namespace alias to avoid conflicts between cdp::Page (domain) and highlevel::Page
namespace hl = cdp::highlevel;

void printResult(const std::string& operation, bool success) {
    std::cout << (success ? "[PASS] " : "[FAIL] ") << operation << "\n";
}

template<typename T>
void printResult(const std::string& operation, const hl::Result<T>& result) {
    printResult(operation, result.ok());
    if (!result.ok()) {
        const auto& err = result.error();
        std::cout << "       Error [" << err.code << "]: " << err.fullMessage() << "\n";
    }
}

int main() {
    std::cout << "=== CDP High-Level API Test ===\n\n";

    // Create client with custom config
    // IMPORTANT: autoEnableDomains = false validates that the high-level API
    // is self-contained and enables domains only when needed
    cdp::CDPClientConfig config;
    config.host = "localhost";
    config.port = 9222;
    config.autoEnableDomains = false;  // High-level API should be self-sufficient

    std::cout << "Creating CDPClient...\n";
    cdp::CDPClient client(config);

    // List available targets
    std::cout << "Discovering targets...\n";
    auto targets = client.listTargets();

    if (targets.empty()) {
        std::cerr << "No targets found. Make sure Chrome is running with:\n";
        std::cerr << "  chrome.exe --remote-debugging-port=9222\n";
        return 1;
    }

    std::cout << "Found " << targets.size() << " target(s)\n";

    // Find a page target - use target object directly instead of index
    cdp::CDPTarget* pageTarget = nullptr;
    for (auto& target : targets) {
        if (target.type == "page" && !target.webSocketDebuggerUrl.empty()) {
            pageTarget = &target;
            std::cout << "Using page: " << target.title << " (id: " << target.id << ")\n";
            break;
        }
    }

    if (!pageTarget) {
        std::cerr << "No suitable page target found\n";
        return 1;
    }

    // Connect using target object directly (safer than index-based)
    if (!client.connectToTarget(*pageTarget)) {
        std::cerr << "Failed to connect to target: " << pageTarget->id << "\n";
        return 1;
    }

    std::cout << "Connected!\n\n";

    // Create high-level Page wrapper
    hl::Page page(client);

    // Test Navigation  
    std::cout << "--- Navigation Tests ---\n";

    auto navResult = page.navigate("https://example.com");
    printResult("Navigate to example.com", navResult);

    if (navResult) {
        std::cout << "Current URL: " << page.url() << "\n";
        std::cout << "Page title: " << page.title() << "\n";
    }

    // Test Selectors  
    std::cout << "\n--- Selector Tests ---\n";

    auto textResult = page.getText("h1");
    printResult("Get h1 text", textResult);
    if (textResult) {
        std::cout << "       h1 text: " << textResult.value() << "\n";
    }

    auto htmlResult = page.getHTML("body");
    printResult("Get body HTML", htmlResult);
    if (htmlResult) {
        std::cout << "       Body length: " << htmlResult.value().length() << " chars\n";
    }

    // Test Element Handle  
    std::cout << "\n--- ElementHandle Tests ---\n";

    auto elemResult = page.querySelector("h1");
    printResult("querySelector h1", elemResult);

    if (elemResult) {
        auto& elem = elemResult.value();

        auto contentResult = elem.textContent();
        printResult("ElementHandle.textContent()", contentResult);

        auto visibleResult = elem.isVisible();
        printResult("ElementHandle.isVisible()", visibleResult);
        if (visibleResult) {
            std::cout << "       Is visible: " << (visibleResult.value() ? "yes" : "no") << "\n";
        }

        auto boxResult = elem.boundingBox();
        printResult("ElementHandle.boundingBox()", boxResult);
        if (boxResult) {
            auto& box = boxResult.value();
            std::cout << "       Box: x=" << box.x << " y=" << box.y
                      << " w=" << box.width << " h=" << box.height << "\n";
        }
    }

    // Test Waiting  
    std::cout << "\n--- Waiting Tests ---\n";

    auto waitResult = page.waitForSelector("p", 5000);
    printResult("waitForSelector('p')", waitResult);

    // Test Screenshots  
    std::cout << "\n--- Screenshot Tests ---\n";

    // Use ScreenshotOptions struct for readable configuration
    hl::ScreenshotOptions screenshotOpts;
    screenshotOpts.format = "png";
    screenshotOpts.quality = 100;
    screenshotOpts.fullPage = false;
    screenshotOpts.optimizeForSpeed = true;
    screenshotOpts.timeoutMs = 15000;

    std::cout << "       Taking screenshot using high-level API...\n";
    auto screenshotStart = std::chrono::high_resolution_clock::now();
    auto screenshotResult = page.screenshot(screenshotOpts);
    auto screenshotEnd = std::chrono::high_resolution_clock::now();
    auto screenshotDuration = std::chrono::duration_cast<std::chrono::milliseconds>(screenshotEnd - screenshotStart).count();

    printResult("Take screenshot", screenshotResult);
    if (screenshotResult) {
        std::cout << "       Screenshot completed in " << screenshotDuration << "ms\n";
        std::cout << "       Screenshot size: " << screenshotResult.value().size() << " bytes\n";

        std::ofstream file("test_screenshot.png", std::ios::binary);
        if (file) {
            const auto& data = screenshotResult.value();
            file.write(reinterpret_cast<const char*>(data.data()), data.size());
            std::cout << "       Saved to test_screenshot.png\n";
        }
    } else {
        std::cout << "       Screenshot failed after " << screenshotDuration << "ms\n";
    }

    // Test JavaScript Evaluation  
    std::cout << "\n--- JavaScript Evaluation Tests ---\n";

    auto evalResult = page.evaluate("document.title");
    printResult("Evaluate document.title", evalResult);
    if (evalResult) {
        std::cout << "       Result: " << evalResult.value().getString() << "\n";
    }

    auto mathResult = page.evaluate("2 + 2");
    printResult("Evaluate 2 + 2", mathResult);
    if (mathResult) {
        std::cout << "       Result: " << mathResult.value().asNumber() << "\n";
    }

    // Test Fast Input Methods  
    std::cout << "\n--- Fast Input Methods ---\n";

    // Navigate to a data: URL with an input field for testing
    auto navResult2 = page.navigate("data:text/html,<html><body><input id='test' type='text'/><button id='btn'>Click</button></body></html>");
    printResult("Navigate to test page (data: URL)", navResult2);

    if (navResult2) {
        // Wait for input to be ready
        auto inputWait = page.waitForSelector("#test", 5000);
        printResult("Wait for input field", inputWait);

        if (inputWait) {
            // Test fast typing
            auto clickResult = page.click("#test");
            printResult("Click input field", clickResult);

            // Use fast type (insertText under the hood)
            auto typeResult = page.type("#test", "Hello, World!");
            printResult("Type text into input", typeResult);

            // Get the value back
            auto valueResult = page.getValue("#test");
            printResult("Get input value", valueResult);
            if (valueResult) {
                std::cout << "       Input value: " << valueResult.value() << "\n";
            }
        }
    }

    // Test Network Interceptor  
    std::cout << "\n--- Network Interceptor Tests ---\n";

    hl::NetworkInterceptor interceptor(client);

    auto enableResult = interceptor.enable();
    printResult("Enable network interceptor", enableResult);

    if (enableResult) {
        // Mock a specific URL
        auto mockHandle = interceptor.mockRequest("*/api/test*",
            hl::MockResponse::json(R"({"mocked": true, "message": "This is mocked data"})"));
        printResult("Add mock rule", mockHandle.isActive());

        // Block images
        auto blockHandle = interceptor.blockResourceType("Image");
        printResult("Add block images rule", blockHandle.isActive());

        // Navigate to trigger interception
        std::cout << "       (Network interception is now active)\n";

        // Clean up
        mockHandle.remove();
        blockHandle.remove();

        auto disableResult = interceptor.disable();
        printResult("Disable network interceptor", disableResult);
    }

    // Test Cookies  
    std::cout << "\n--- Cookie Tests ---\n";

    auto setCookieResult = page.setCookie("test_cookie", "test_value", "example.com");
    printResult("Set cookie", setCookieResult);

    auto cookiesResult = page.getCookies();
    printResult("Get cookies", cookiesResult);
    if (cookiesResult) {
        std::cout << "       Found " << cookiesResult.value().size() << " cookie(s)\n";
    }

    // Performance Test  
    std::cout << "\n--- Performance Test ---\n";

    // Test fast vs slow typing using TypeOptions API on a data: URL
    auto navResult3 = page.navigate("data:text/html,<html><body><input id='fast' type='text'/><input id='slow' type='text'/></body></html>");
    printResult("Navigate to perf test page (data: URL)", navResult3);

    if (navResult3) {
        page.waitForSelector("#fast", 5000);

        std::string testText = "Hello World!";

        // Test 1: Fast typing (insertText - default)
        hl::TypeOptions fastOptions;
        fastOptions.clearFirst = true;
        fastOptions.useKeyEvents = false;  // Fast mode (default)

        auto start = std::chrono::high_resolution_clock::now();
        auto fastResult = page.type("#fast", testText, fastOptions);
        auto end = std::chrono::high_resolution_clock::now();
        auto fastDuration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

        printResult("Fast typing (insertText)", fastResult);
        if (fastResult) {
            auto fastValue = page.getValue("#fast");
            std::cout << "       Value: " << fastValue.valueOr("(failed)") << " (" << fastDuration << "ms)\n";
        }

        // Test 2: Slow typing (individual key events)
        hl::TypeOptions slowOptions;
        slowOptions.clearFirst = true;
        slowOptions.useKeyEvents = true;   // Slow mode - individual key events
        slowOptions.delayMs = 10;          // 10ms delay between keystrokes

        start = std::chrono::high_resolution_clock::now();
        auto slowResult = page.type("#slow", testText, slowOptions);
        end = std::chrono::high_resolution_clock::now();
        auto slowDuration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

        printResult("Slow typing (key events)", slowResult);
        if (slowResult) {
            auto slowValue = page.getValue("#slow");
            std::cout << "       Value: " << slowValue.valueOr("(failed)") << " (" << slowDuration << "ms)\n";
        }

        std::cout << "       Speed comparison: Fast=" << fastDuration << "ms, Slow=" << slowDuration << "ms\n";
    }

    std::cout << "\n=== Test Complete ===\n";

    return 0;
}
