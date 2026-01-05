

#include "cdp/domains/PageDomain.hpp"

namespace cdp {

FrameInfo FrameInfo::fromJson(const JsonValue& json) {
    FrameInfo frame;
    frame.id = json["id"].getString();
    frame.parentId = json["parentId"].getString();
    frame.loaderId = json["loaderId"].getString();
    frame.name = json["name"].getString();
    frame.url = json["url"].getString();
    frame.securityOrigin = json["securityOrigin"].getString();
    frame.mimeType = json["mimeType"].getString();
    return frame;
}

JsonValue Viewport::toJson() const {
    JsonObject obj;
    obj["x"] = x;
    obj["y"] = y;
    obj["width"] = width;
    obj["height"] = height;
    obj["scale"] = scale;
    return obj;
}

NavigationResult PageDomain::navigate(const std::string& url) {
    JsonObject params;
    params["url"] = url;

    auto response = sendCommand("Page.navigate", params);
    NavigationResult result;

    if (response.isSuccess()) {
        result.frameId = response.result["frameId"].getString();
        result.loaderId = response.result["loaderId"].getString();
        result.errorText = response.result["errorText"].getString();
    } else {
        result.errorText = response.errorMessage;
    }

    return result;
}

CDPResponse PageDomain::navigateToHistoryEntry(int entryId) {
    JsonObject params;
    params["entryId"] = entryId;
    return sendCommand("Page.navigateToHistoryEntry", params);
}

CDPResponse PageDomain::reload(bool ignoreCache, const std::string& scriptToEvaluateOnLoad) {
    JsonObject params;
    params["ignoreCache"] = ignoreCache;
    if (!scriptToEvaluateOnLoad.empty()) {
        params["scriptToEvaluateOnLoad"] = scriptToEvaluateOnLoad;
    }
    return sendCommand("Page.reload", params);
}

CDPResponse PageDomain::stopLoading() {
    return sendCommand("Page.stopLoading");
}

CDPResponse PageDomain::goBack() {
    auto history = getNavigationHistory();
    if (history.first > 0) {
        return navigateToHistoryEntry(history.second[history.first - 1].id);
    }
    CDPResponse resp;
    resp.hasError = true;
    resp.errorMessage = "No back history";
    return resp;
}

CDPResponse PageDomain::goForward() {
    auto history = getNavigationHistory();
    if (history.first < static_cast<int>(history.second.size()) - 1) {
        return navigateToHistoryEntry(history.second[history.first + 1].id);
    }
    CDPResponse resp;
    resp.hasError = true;
    resp.errorMessage = "No forward history";
    return resp;
}

std::pair<int, std::vector<PageDomain::NavigationEntry>> PageDomain::getNavigationHistory() {
    auto response = sendCommand("Page.getNavigationHistory");
    std::vector<NavigationEntry> entries;
    int currentIndex = 0;

    if (response.isSuccess()) {
        currentIndex = response.result["currentIndex"].getInt();
        const auto& entriesJson = response.result["entries"];
        if (entriesJson.isArray()) {
            for (size_t i = 0; i < entriesJson.size(); ++i) {
                NavigationEntry entry;
                entry.id = entriesJson[i]["id"].getInt();
                entry.url = entriesJson[i]["url"].getString();
                entry.title = entriesJson[i]["title"].getString();
                entry.transitionType = entriesJson[i]["transitionType"].getString();
                entries.push_back(entry);
            }
        }
    }

    return {currentIndex, entries};
}

FrameInfo PageDomain::getFrameTree() {
    auto response = sendCommand("Page.getFrameTree");
    if (response.isSuccess() && response.result.contains("frameTree")) {
        return FrameInfo::fromJson(response.result["frameTree"]["frame"]);
    }
    return {};
}

std::vector<FrameInfo> PageDomain::getAllFrames() {
    std::vector<FrameInfo> frames;
    auto response = sendCommand("Page.getFrameTree");

    if (response.isSuccess()) {
        std::function<void(const JsonValue&)> collectFrames = [&](const JsonValue& tree) {
            if (tree.contains("frame")) {
                frames.push_back(FrameInfo::fromJson(tree["frame"]));
            }
            if (tree.contains("childFrames") && tree["childFrames"].isArray()) {
                for (size_t i = 0; i < tree["childFrames"].size(); ++i) {
                    collectFrames(tree["childFrames"][i]);
                }
            }
        };

        if (response.result.contains("frameTree")) {
            collectFrames(response.result["frameTree"]);
        }
    }

    return frames;
}

std::string PageDomain::getResourceContent(const std::string& frameId, const std::string& url) {
    JsonObject params;
    params["frameId"] = frameId;
    params["url"] = url;

    auto response = sendCommand("Page.getResourceContent", params);
    if (response.isSuccess()) {
        return response.result["content"].getString();
    }
    return "";
}

CDPResponse PageDomain::setDocumentContent(const std::string& frameId, const std::string& html) {
    JsonObject params;
    params["frameId"] = frameId;
    params["html"] = html;
    return sendCommand("Page.setDocumentContent", params);
}

std::string PageDomain::captureScreenshot(ScreenshotFormat format, int quality,
                                           std::optional<Viewport> clip, bool fullPage) {
    JsonObject params;

    switch (format) {
        case ScreenshotFormat::Jpeg:
            params["format"] = "jpeg";
            break;
        case ScreenshotFormat::Png:
            params["format"] = "png";
            break;
        case ScreenshotFormat::Webp:
            params["format"] = "webp";
            break;
    }

    params["quality"] = quality;

    if (clip) {
        params["clip"] = clip->toJson();
    }

    if (fullPage) {
        params["captureBeyondViewport"] = true;
    }

    auto response = sendCommand("Page.captureScreenshot", params);
    if (response.isSuccess()) {
        return response.result["data"].getString();
    }
    return "";
}

std::string PageDomain::printToPDF(bool landscape, bool displayHeaderFooter,
                                    bool printBackground, double scale,
                                    double paperWidth, double paperHeight,
                                    double marginTop, double marginBottom,
                                    double marginLeft, double marginRight) {
    JsonObject params;
    params["landscape"] = landscape;
    params["displayHeaderFooter"] = displayHeaderFooter;
    params["printBackground"] = printBackground;
    params["scale"] = scale;
    params["paperWidth"] = paperWidth;
    params["paperHeight"] = paperHeight;
    params["marginTop"] = marginTop;
    params["marginBottom"] = marginBottom;
    params["marginLeft"] = marginLeft;
    params["marginRight"] = marginRight;

    auto response = sendCommand("Page.printToPDF", params);
    if (response.isSuccess()) {
        return response.result["data"].getString();
    }
    return "";
}

CDPResponse PageDomain::handleJavaScriptDialog(bool accept, const std::string& promptText) {
    JsonObject params;
    params["accept"] = accept;
    if (!promptText.empty()) {
        params["promptText"] = promptText;
    }
    return sendCommand("Page.handleJavaScriptDialog", params);
}

CDPResponse PageDomain::bringToFront() {
    return sendCommand("Page.bringToFront");
}

CDPResponse PageDomain::close() {
    return sendCommand("Page.close");
}

CDPResponse PageDomain::crash() {
    return sendCommand("Page.crash");
}

CDPResponse PageDomain::addScriptToEvaluateOnLoad(const std::string& scriptSource) {
    JsonObject params;
    params["scriptSource"] = scriptSource;
    return sendCommand("Page.addScriptToEvaluateOnLoad", params);
}

CDPResponse PageDomain::addScriptToEvaluateOnNewDocument(const std::string& source,
                                                          const std::string& worldName) {
    JsonObject params;
    params["source"] = source;
    if (!worldName.empty()) {
        params["worldName"] = worldName;
    }
    return sendCommand("Page.addScriptToEvaluateOnNewDocument", params);
}

CDPResponse PageDomain::removeScriptToEvaluateOnNewDocument(const std::string& identifier) {
    JsonObject params;
    params["identifier"] = identifier;
    return sendCommand("Page.removeScriptToEvaluateOnNewDocument", params);
}

CDPResponse PageDomain::setPermissionOverride(const std::string& origin,
                                               const std::string& permission,
                                               const std::string& setting) {
    JsonObject params;
    JsonObject permissionObj;
    permissionObj["name"] = permission;
    params["origin"] = origin;
    params["permission"] = permissionObj;
    params["setting"] = setting;
    return sendCommand("Browser.setPermission", params);
}

void PageDomain::onLoadEventFired(std::function<void(double)> callback) {
    subscribeEvent("loadEventFired", [callback](const CDPEvent& event) {
        callback(event.params["timestamp"].getNumber());
    });
}

void PageDomain::onDomContentEventFired(std::function<void(double)> callback) {
    subscribeEvent("domContentEventFired", [callback](const CDPEvent& event) {
        callback(event.params["timestamp"].getNumber());
    });
}

void PageDomain::onFrameNavigated(std::function<void(const FrameInfo&)> callback) {
    subscribeEvent("frameNavigated", [callback](const CDPEvent& event) {
        callback(FrameInfo::fromJson(event.params["frame"]));
    });
}

void PageDomain::onFrameStartedLoading(std::function<void(const std::string&)> callback) {
    subscribeEvent("frameStartedLoading", [callback](const CDPEvent& event) {
        callback(event.params["frameId"].getString());
    });
}

void PageDomain::onFrameStoppedLoading(std::function<void(const std::string&)> callback) {
    subscribeEvent("frameStoppedLoading", [callback](const CDPEvent& event) {
        callback(event.params["frameId"].getString());
    });
}

void PageDomain::onJavascriptDialogOpening(
    std::function<void(const std::string&, const std::string&, const std::string&,
                       bool, const std::string&)> callback) {
    subscribeEvent("javascriptDialogOpening", [callback](const CDPEvent& event) {
        callback(
            event.params["url"].getString(),
            event.params["message"].getString(),
            event.params["type"].getString(),
            event.params["hasBrowserHandler"].getBool(),
            event.params["defaultPrompt"].getString()
        );
    });
}

void PageDomain::onJavascriptDialogClosed(
    std::function<void(bool, const std::string&)> callback) {
    subscribeEvent("javascriptDialogClosed", [callback](const CDPEvent& event) {
        callback(
            event.params["result"].getBool(),
            event.params["userInput"].getString()
        );
    });
}

void PageDomain::onWindowOpen(
    std::function<void(const std::string&, const std::string&,
                       const std::vector<std::string>&, bool)> callback) {
    subscribeEvent("windowOpen", [callback](const CDPEvent& event) {
        std::vector<std::string> features;
        if (event.params["windowFeatures"].isArray()) {
            for (size_t i = 0; i < event.params["windowFeatures"].size(); ++i) {
                features.push_back(event.params["windowFeatures"][i].getString());
            }
        }
        callback(
            event.params["url"].getString(),
            event.params["windowName"].getString(),
            features,
            event.params["userGesture"].getBool()
        );
    });
}

} 
