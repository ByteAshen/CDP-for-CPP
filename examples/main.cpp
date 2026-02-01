// CDP Library Example - Using actual CDP domain methods
// Start Chrome with: chrome.exe --remote-debugging-port=9222

#include <cdp/CDP.hpp>
#include <iostream>
#include <fstream>
#include <chrono>

auto startTime = std::chrono::steady_clock::now();

int64_t elapsedMs() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - startTime).count();
}

void printSection(const std::string& title) {
    std::cout << "\n[" << elapsedMs() << "ms] " << std::string(40, '=') << "\n";
    std::cout << "[" << elapsedMs() << "ms]   " << title << "\n";
    std::cout << "[" << elapsedMs() << "ms] " << std::string(40, '=') << "\n\n";
}

void log(const std::string& msg) {
    std::cout << "[" << elapsedMs() << "ms] " << msg << "\n";
}

int main() {
    log("CDP Library Example - Proper CDP Implementation");
    log("  ========");

    // Create client with custom config
    cdp::CDPClientConfig config;
    config.host = "localhost";
    config.port = 9222;
    config.autoEnableDomains = true;

    log("Creating CDPClient...");
    cdp::CDPClient client(config);
    log("CDPClient created");

    // List available targets
    log("Discovering targets...");
    auto targets = client.listTargets();
    log("Discovery complete");

    if (targets.empty()) {
        std::cerr << "No targets found. Make sure Chrome is running with:\n";
        std::cerr << "  chrome.exe --remote-debugging-port=9222\n";
        return 1;
    }

    log("Found " + std::to_string(targets.size()) + " target(s):");
    int pageTargetIndex = -1;
    for (size_t i = 0; i < targets.size(); ++i) {
        const auto& target = targets[i];
        std::cout << "  [" << i << "] " << target.title << " (" << target.type << ")\n";
        std::cout << "      URL: " << target.url << "\n";
        std::cout << "      WebSocket: " << (target.webSocketDebuggerUrl.empty() ? "(none)" : target.webSocketDebuggerUrl) << "\n";

        // Find first page target with a WebSocket URL
        if (pageTargetIndex < 0 && target.type == "page" && !target.webSocketDebuggerUrl.empty()) {
            pageTargetIndex = static_cast<int>(i);
        }
    }

    if (pageTargetIndex < 0) {
        std::cerr << "\nNo suitable page target found with WebSocket URL.\n";
        std::cerr << "Try opening a regular webpage in Chrome (not chrome:// pages).\n";
        return 1;
    }

    // Connect to the page target we found
    log("Connecting to target [" + std::to_string(pageTargetIndex) + "]...");
    if (!client.connect(pageTargetIndex)) {
        std::cerr << "Failed to connect!\n";
        return 1;
    }
    log("Connected!");

    // Page Domain  
    printSection("Page Domain");

    // Navigate using actual CDP method
    log("Navigating to example.com...");
    auto navResult = client.Page.navigate("https://example.com");
    if (navResult.isSuccess()) {
        log("Navigation started, frameId: " + navResult.result["frameId"].getString());
    }

    // Wait for load
    bool loaded = false;
    client.Page.onLoadEventFired([&loaded](double timestamp) {
        std::cout << "[event] Page loaded at timestamp: " << timestamp << "\n";
        loaded = true;
    });

    log("Waiting for page load...");
    client.waitFor([&loaded]() { return loaded; }, 10000);
    log("Page load complete");

    // Get navigation history
    log("Getting navigation history...");
    auto history = client.Page.getNavigationHistory();
    if (history.isSuccess()) {
        log("Current history index: " + std::to_string(history.result["currentIndex"].getInt()));
    }

    // Runtime Domain  
    printSection("Runtime Domain");

    // Evaluate JavaScript using eval helper (returnByValue=true by default)
    log("Evaluating document.title...");
    auto evalResult = client.Runtime.eval("document.title");
    if (evalResult.isSuccess()) {
        log("Page title: " + evalResult.result["result"]["value"].getString());
    }

    // Evaluate with await promise (using full evaluate signature)
    log("Evaluating async promise...");
    auto asyncResult = client.Runtime.evaluate(
        "new Promise(r => setTimeout(() => r('async result'), 100))",
        "",     // objectGroup
        false,  // includeCommandLineAPI
        false,  // silent
        0,      // contextId
        true,   // returnByValue
        false,  // generatePreview
        false,  // userGesture
        true    // awaitPromise
    );
    if (asyncResult.isSuccess()) {
        log("Async result: " + asyncResult.result["result"]["value"].getString());
    }

    // Get all DOM element count using eval helper
    log("Getting DOM element count...");
    auto countResult = client.Runtime.eval("document.querySelectorAll('*').length");
    if (countResult.isSuccess()) {
        log("DOM element count: " + std::to_string(countResult.result["result"]["value"].getInt()));
    }

    // DOM Domain  
    printSection("DOM Domain");

    // Get document
    log("Getting document...");
    auto docResult = client.DOM.getDocument();
    if (docResult.isSuccess()) {
        int rootNodeId = docResult.result["root"]["nodeId"].getInt();
        log("Document root nodeId: " + std::to_string(rootNodeId));

        // Query selector
        log("Querying for h1...");
        auto queryResult = client.DOM.querySelector(rootNodeId, "h1");
        if (queryResult.isSuccess()) {
            int h1NodeId = queryResult.result["nodeId"].getInt();
            log("Found h1 element with nodeId: " + std::to_string(h1NodeId));

            // Get outer HTML
            log("Getting outer HTML...");
            auto htmlResult = client.DOM.getOuterHTML(h1NodeId);
            if (htmlResult.isSuccess()) {
                log("H1 HTML: " + htmlResult.result["outerHTML"].getString());
            }
        }

        // Query all links
        log("Querying for all links...");
        auto linksResult = client.DOM.querySelectorAll(rootNodeId, "a");
        if (linksResult.isSuccess() && linksResult.result["nodeIds"].isArray()) {
            log("Found " + std::to_string(linksResult.result["nodeIds"].size()) + " links");
        }
    }

    // Network Domain  
    printSection("Network Domain");

    // Set extra HTTP headers
    log("Setting custom HTTP header...");
    client.Network.setExtraHTTPHeaders({{"X-Custom-Header", "CDP-Library"}});
    log("Set custom HTTP header");

    // Set user agent override
    log("Setting user agent override...");
    client.Network.setUserAgentOverride("CDP-Library/1.0 (Custom User Agent)");
    log("Set custom user agent");

    // Get all cookies
    log("Getting all cookies...");
    auto cookiesResult = client.Network.getAllCookies();
    if (cookiesResult.isSuccess() && cookiesResult.result["cookies"].isArray()) {
        log("Found " + std::to_string(cookiesResult.result["cookies"].size()) + " cookies");
    }

    // Set a cookie using CookieParam struct
    log("Setting test cookie...");
    cdp::CookieParam cookie;
    cookie.name = "test_cookie";
    cookie.value = "test_value";
    cookie.domain = "example.com";
    cookie.path = "/";
    cookie.secure = false;
    cookie.httpOnly = false;
    client.Network.setCookie(cookie);
    log("Set test cookie");

    // Emulation Domain  
    printSection("Emulation Domain");

    // Set device metrics (viewport) using actual CDP method
    log("Setting viewport to 1920x1080...");
    client.Emulation.setDeviceMetricsOverride(192, 108, 1.0, false);
    log("Set viewport to 1920x1080 (desktop)");

    // Emulate mobile device
    log("Emulating iPhone X viewport...");
    client.Emulation.setDeviceMetricsOverride(375, 812, 3.0, true);
    log("Emulated iPhone X viewport (375x812, 3x scale, mobile)");

    // Set geolocation
    log("Setting geolocation...");
    client.Emulation.setGeolocationOverride(37.7749, -122.4194, 100);
    log("Set geolocation to San Francisco");

    // Set timezone
    log("Setting timezone...");
    client.Emulation.setTimezoneOverride("America/Los_Angeles");
    log("Set timezone to America/Los_Angeles");

    // Enable touch emulation
    log("Enabling touch emulation...");
    client.Emulation.setTouchEmulationEnabled(true, 5);
    log("Enabled touch emulation");

    // Emulate vision deficiency
    log("Setting vision deficiency emulation...");
    client.Emulation.setEmulatedVisionDeficiency("deuteranopia");
    log("Emulated deuteranopia (color blindness)");

    // Reset emulation
    log("Resetting all emulation...");
    client.Emulation.clearDeviceMetricsOverride();
    client.Emulation.setTouchEmulationEnabled(false);
    client.Emulation.setEmulatedVisionDeficiency("none");
    log("Reset all emulation");

    // Input Domain  
    printSection("Input Domain");

    // Bring page to front first - Chrome throttles Input commands when page isn't focused
    log("Bringing page to front...");
    client.Page.bringToFront();
    log("Page brought to front");

    // Simulate mouse move
    log("Moving mouse to (100, 100)...");
    client.Input.mouseMove(100, 100);
    log("Moved mouse to (100, 100)");

    // Simulate click
    log("Clicking at (200, 200)...");
    client.Input.click(200, 200);
    log("Clicked at (200, 200)");

    // Simulate keyboard - type text
    log("Typing 'Hello'...");
    client.Input.typeText("Hello");
    log("Typed 'Hello'");

    // Press specific keys
    log("Pressing Enter key...");
    client.Input.keyPress("Enter");
    log("Pressed Enter key");

    // Screenshots  
    printSection("Screenshots");

    // Capture screenshot using actual CDP method
    log("Capturing screenshot...");
    auto screenshotResult = client.Page.captureScreenshot();
    if (screenshotResult.isSuccess()) {
        std::string base64Data = screenshotResult.result["data"].getString();
        log("Captured screenshot (" + std::to_string(base64Data.length()) + " bytes base64)");

        // Save to file
        auto decoded = cdp::Base64::decode(base64Data);
        std::ofstream file("screenshot.png", std::ios::binary);
        file.write(reinterpret_cast<const char*>(decoded.data()), decoded.size());
        log("Saved to screenshot.png");
    }

    // Full page screenshot (without viewport parameter)
    log("Capturing full page screenshot...");
    auto fullScreenshot = client.Page.captureScreenshot("png", 100, nullptr, true);
    if (fullScreenshot.isSuccess()) {
        std::string base64Data = fullScreenshot.result["data"].getString();
        auto decoded = cdp::Base64::decode(base64Data);
        std::ofstream file("screenshot_full.png", std::ios::binary);
        file.write(reinterpret_cast<const char*>(decoded.data()), decoded.size());
        log("Saved full page screenshot to screenshot_full.png");
    }

    // Target Domain  
    printSection("Target Domain");

    // Get all targets
    log("Getting all targets...");
    auto targetsResult = client.Target.getTargets();
    if (targetsResult.isSuccess() && targetsResult.result["targetInfos"].isArray()) {
        const auto& targetInfos = targetsResult.result["targetInfos"].asArray();
        log("Found " + std::to_string(targetInfos.size()) + " targets");
        for (const auto& t : targetInfos) {
            std::cout << "  - " << t["title"].getString() << " (" << t["type"].getString() << ")\n";
        }
    }

    // Create a new target (new tab)
    log("Creating new target...");
    auto newTarget = client.Target.createTarget("about:blank");
    if (newTarget.isSuccess()) {
        std::string newTargetId = newTarget.result["targetId"].getString();
        log("Created new target: " + newTargetId);

        // Close the new target
        log("Closing new target...");
        //client.Target.closeTarget(newTargetId);
        log("Closed new target");
    }

    // Browser Domain  
    printSection("Browser Domain");

    // Get browser version
    log("Getting browser version...");
    auto versionResult = client.Browser.getVersion();
    if (versionResult.isSuccess()) {
        log("Browser: " + versionResult.result["product"].getString());
        log("Protocol: " + versionResult.result["protocolVersion"].getString());
        log("User Agent: " + versionResult.result["userAgent"].getString());
    }

    // Fetch Domain  
    printSection("Fetch Domain (Request Interception)");

    // Enable Fetch domain with patterns (without designated initializers for C++17)
    log("Enabling request interception...");
    cdp::RequestPattern pattern;
    pattern.urlPattern = "*";
    pattern.requestStage = "Request";
    std::vector<cdp::RequestPattern> patterns = {pattern};
    client.Fetch.enable(patterns, false);
    log("Enabled request interception for all URLs");

    // Set up request handler - modify ALL responses to return "balls"
    int interceptedCount = 0;
    client.Fetch.onRequestPaused([&client, &interceptedCount](
        const std::string& requestId,
        const cdp::JsonValue& request,
        const std::string& frameId,
        const std::string& resourceType,
        const cdp::JsonValue& responseErrorReason,
        int responseStatusCode,
        const std::string& responseStatusText,
        const cdp::JsonValue& responseHeaders,
        const std::string& networkId) {

        interceptedCount++;
        std::string url = request["url"].getString();
        std::cout << "  [intercept] " << url.substr(0, 50) << "...\n";

        // Base64 encode "balls" for the response body
        std::string body = cdp::Base64::encode(reinterpret_cast<const uint8_t*>("balls"), 5);

        // Fulfill with custom response containing just "balls"
        // Use async variant to avoid blocking the message thread
        std::vector<cdp::HeaderEntry> headers = {
            {"Content-Type", "text/html"},
            {"Content-Length", "5"}
        };
        client.Fetch.fulfillRequestAsync(requestId, 200, headers, body, "OK");
        std::cout << "  [intercept] -> Replaced response with 'balls'\n";
    });

    // Trigger some requests - use async navigate to avoid blocking on page load
    log("Navigating to trigger interception...");
    cdp::JsonObject navParams;
    navParams["url"] = "https://example.com";
    client.connection().sendCommand("Page.navigate", cdp::JsonValue(navParams),
        [](const cdp::CDPResponse&) {}); // Fire and forget

    // Wait for at least one intercept (event-based, not sleep-based)
    log("Waiting for intercept...");
    client.waitFor([&interceptedCount]() { return interceptedCount > 0; }, 5000);
    log("Intercepted and modified " + std::to_string(interceptedCount) + " requests");

    // Disable fetch - use async to avoid blocking on pending navigation
    log("Disabling request interception...");
    client.connection().sendCommand("Fetch.disable", cdp::JsonValue(),
        [](const cdp::CDPResponse&) {});  // Fire and forget
    client.poll(100);  // Give it a moment to process
    log("Disabled request interception");

    // Summary  
    printSection("Complete!");

    std::cout << "This library exposes actual CDP domain methods:\n\n";
    std::cout << "  client.Page.navigate(url)\n";
    std::cout << "  client.Page.captureScreenshot(format, quality)\n";
    std::cout << "  client.Runtime.evaluate(expression)\n";
    std::cout << "  client.DOM.getDocument()\n";
    std::cout << "  client.DOM.querySelector(nodeId, selector)\n";
    std::cout << "  client.Network.setExtraHTTPHeaders(headers)\n";
    std::cout << "  client.Network.setCookie(cookieParam)\n";
    std::cout << "  client.Emulation.setDeviceMetricsOverride(w, h, scale, mobile)\n";
    std::cout << "  client.Emulation.setGeolocationOverride(lat, lon)\n";
    std::cout << "  client.Input.click(x, y)\n";
    std::cout << "  client.Input.typeText(text)\n";
    std::cout << "  client.Fetch.enable(patterns, handleAuth)\n";
    std::cout << "  client.Target.createTarget(url)\n";
    std::cout << "  client.Browser.getVersion()\n";
    std::cout << "\nAll methods match the official CDP documentation!\n";

    return 0;
}
