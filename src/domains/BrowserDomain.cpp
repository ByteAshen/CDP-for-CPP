

#include "cdp/domains/BrowserDomain.hpp"

namespace cdp {

WindowBounds WindowBounds::fromJson(const JsonValue& json) {
    WindowBounds bounds;
    bounds.left = json["left"].getInt();
    bounds.top = json["top"].getInt();
    bounds.width = json["width"].getInt();
    bounds.height = json["height"].getInt();
    bounds.windowState = json["windowState"].getString("normal");
    return bounds;
}

JsonValue WindowBounds::toJson() const {
    JsonObject obj;
    obj["left"] = left;
    obj["top"] = top;
    obj["width"] = width;
    obj["height"] = height;
    obj["windowState"] = windowState;
    return obj;
}

Histogram Histogram::fromJson(const JsonValue& json) {
    Histogram hist;
    hist.name = json["name"].getString();
    hist.sum = json["sum"].getInt();
    hist.count = json["count"].getInt();

    if (json["buckets"].isArray()) {
        for (size_t i = 0; i < json["buckets"].size(); ++i) {
            const auto& bucket = json["buckets"][i];
            hist.buckets.push_back({
                bucket["low"].getInt(),
                bucket["high"].getInt()
            });
        }
    }

    return hist;
}

std::string BrowserDomain::getVersion() {
    auto response = sendCommand("Browser.getVersion");
    if (response.isSuccess()) {
        return response.result["product"].getString();
    }
    return "";
}

std::pair<int, WindowBounds> BrowserDomain::getWindowForTarget(const std::string& targetId) {
    JsonObject params;
    if (!targetId.empty()) params["targetId"] = targetId;

    auto response = sendCommand("Browser.getWindowForTarget", params);
    if (response.isSuccess()) {
        return {
            response.result["windowId"].getInt(),
            WindowBounds::fromJson(response.result["bounds"])
        };
    }
    return {0, {}};
}

WindowBounds BrowserDomain::getWindowBounds(int windowId) {
    JsonObject params;
    params["windowId"] = windowId;

    auto response = sendCommand("Browser.getWindowBounds", params);
    if (response.isSuccess()) {
        return WindowBounds::fromJson(response.result["bounds"]);
    }
    return {};
}

CDPResponse BrowserDomain::setWindowBounds(int windowId, const WindowBounds& bounds) {
    JsonObject params;
    params["windowId"] = windowId;
    params["bounds"] = bounds.toJson();
    return sendCommand("Browser.setWindowBounds", params);
}

CDPResponse BrowserDomain::minimizeWindow(int windowId) {
    WindowBounds bounds;
    bounds.windowState = "minimized";
    return setWindowBounds(windowId, bounds);
}

CDPResponse BrowserDomain::maximizeWindow(int windowId) {
    WindowBounds bounds;
    bounds.windowState = "maximized";
    return setWindowBounds(windowId, bounds);
}

CDPResponse BrowserDomain::setFullscreen(int windowId, bool fullscreen) {
    WindowBounds bounds;
    bounds.windowState = fullscreen ? "fullscreen" : "normal";
    return setWindowBounds(windowId, bounds);
}

CDPResponse BrowserDomain::restoreWindow(int windowId) {
    WindowBounds bounds;
    bounds.windowState = "normal";
    return setWindowBounds(windowId, bounds);
}

CDPResponse BrowserDomain::close() {
    return sendCommand("Browser.close");
}

CDPResponse BrowserDomain::crash() {
    return sendCommand("Browser.crash");
}

CDPResponse BrowserDomain::crashGpuProcess() {
    return sendCommand("Browser.crashGpuProcess");
}

CDPResponse BrowserDomain::setDownloadBehavior(const std::string& behavior,
                                                const std::string& browserContextId,
                                                const std::string& downloadPath,
                                                bool eventsEnabled) {
    JsonObject params;
    params["behavior"] = behavior;
    if (!browserContextId.empty()) params["browserContextId"] = browserContextId;
    if (!downloadPath.empty()) params["downloadPath"] = downloadPath;
    if (eventsEnabled) params["eventsEnabled"] = eventsEnabled;
    return sendCommand("Browser.setDownloadBehavior", params);
}

CDPResponse BrowserDomain::cancelDownload(const std::string& guid, const std::string& browserContextId) {
    JsonObject params;
    params["guid"] = guid;
    if (!browserContextId.empty()) params["browserContextId"] = browserContextId;
    return sendCommand("Browser.cancelDownload", params);
}

CDPResponse BrowserDomain::setPermission(const std::string& permission, const std::string& setting,
                                          const std::string& origin,
                                          const std::string& browserContextId) {
    JsonObject params;
    JsonObject permissionObj;
    permissionObj["name"] = permission;
    params["permission"] = permissionObj;
    params["setting"] = setting;
    if (!origin.empty()) params["origin"] = origin;
    if (!browserContextId.empty()) params["browserContextId"] = browserContextId;
    return sendCommand("Browser.setPermission", params);
}

CDPResponse BrowserDomain::grantPermissions(const std::vector<std::string>& permissions,
                                             const std::string& origin,
                                             const std::string& browserContextId) {
    JsonObject params;
    JsonArray permsArray;
    for (const auto& perm : permissions) {
        permsArray.push_back(perm);
    }
    params["permissions"] = permsArray;
    if (!origin.empty()) params["origin"] = origin;
    if (!browserContextId.empty()) params["browserContextId"] = browserContextId;
    return sendCommand("Browser.grantPermissions", params);
}

CDPResponse BrowserDomain::resetPermissions(const std::string& browserContextId) {
    JsonObject params;
    if (!browserContextId.empty()) params["browserContextId"] = browserContextId;
    return sendCommand("Browser.resetPermissions", params);
}

std::vector<Histogram> BrowserDomain::getHistograms(const std::string& query, bool delta) {
    JsonObject params;
    if (!query.empty()) params["query"] = query;
    if (delta) params["delta"] = delta;

    auto response = sendCommand("Browser.getHistograms", params);
    std::vector<Histogram> histograms;

    if (response.isSuccess() && response.result["histograms"].isArray()) {
        for (size_t i = 0; i < response.result["histograms"].size(); ++i) {
            histograms.push_back(Histogram::fromJson(response.result["histograms"][i]));
        }
    }

    return histograms;
}

Histogram BrowserDomain::getHistogram(const std::string& name, bool delta) {
    JsonObject params;
    params["name"] = name;
    if (delta) params["delta"] = delta;

    auto response = sendCommand("Browser.getHistogram", params);
    if (response.isSuccess()) {
        return Histogram::fromJson(response.result["histogram"]);
    }
    return {};
}

std::vector<std::string> BrowserDomain::getBrowserCommandLine() {
    auto response = sendCommand("Browser.getBrowserCommandLine");
    std::vector<std::string> args;

    if (response.isSuccess() && response.result["arguments"].isArray()) {
        for (size_t i = 0; i < response.result["arguments"].size(); ++i) {
            args.push_back(response.result["arguments"][i].getString());
        }
    }

    return args;
}

CDPResponse BrowserDomain::setDockTile(const std::string& badgeLabel, const std::string& image) {
    JsonObject params;
    if (!badgeLabel.empty()) params["badgeLabel"] = badgeLabel;
    if (!image.empty()) params["image"] = image;
    return sendCommand("Browser.setDockTile", params);
}

CDPResponse BrowserDomain::executeBrowserCommand(const std::string& commandId) {
    JsonObject params;
    params["commandId"] = commandId;
    return sendCommand("Browser.executeBrowserCommand", params);
}

void BrowserDomain::onDownloadWillBegin(
    std::function<void(const std::string&, const std::string&, const std::string&, const std::string&)> callback) {
    subscribeEvent("downloadWillBegin", [callback](const CDPEvent& event) {
        callback(
            event.params["frameId"].getString(),
            event.params["guid"].getString(),
            event.params["url"].getString(),
            event.params["suggestedFilename"].getString()
        );
    });
}

void BrowserDomain::onDownloadProgress(
    std::function<void(const std::string&, double, double, const std::string&)> callback) {
    subscribeEvent("downloadProgress", [callback](const CDPEvent& event) {
        callback(
            event.params["guid"].getString(),
            event.params["totalBytes"].getNumber(),
            event.params["receivedBytes"].getNumber(),
            event.params["state"].getString()
        );
    });
}

} 
