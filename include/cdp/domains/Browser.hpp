#pragma once

#include "Domain.hpp"

namespace cdp {


struct Bounds {
    int left = 0;
    int top = 0;
    int width = 0;
    int height = 0;
    std::string windowState;  

    JsonValue toJson() const {
        JsonObject obj;
        if (left != 0) obj["left"] = left;
        if (top != 0) obj["top"] = top;
        if (width != 0) obj["width"] = width;
        if (height != 0) obj["height"] = height;
        if (!windowState.empty()) obj["windowState"] = windowState;
        return obj;
    }

    static Bounds fromJson(const JsonValue& json) {
        Bounds b;
        b.left = json["left"].getInt();
        b.top = json["top"].getInt();
        b.width = json["width"].getInt();
        b.height = json["height"].getInt();
        b.windowState = json["windowState"].getString();
        return b;
    }
};


struct PermissionDescriptor {
    std::string name;
    bool sysex = false;
    bool userVisibleOnly = false;
    bool allowWithoutSanitization = false;
    bool allowWithoutGesture = false;
    bool panTiltZoom = false;

    JsonValue toJson() const {
        JsonObject obj;
        obj["name"] = name;
        if (sysex) obj["sysex"] = true;
        if (userVisibleOnly) obj["userVisibleOnly"] = true;
        if (allowWithoutSanitization) obj["allowWithoutSanitization"] = true;
        if (allowWithoutGesture) obj["allowWithoutGesture"] = true;
        if (panTiltZoom) obj["panTiltZoom"] = true;
        return obj;
    }
};


struct HistogramEntry {
    int low;
    int high;
    int count;

    static HistogramEntry fromJson(const JsonValue& json) {
        HistogramEntry e;
        e.low = json["low"].getInt();
        e.high = json["high"].getInt();
        e.count = json["count"].getInt();
        return e;
    }
};


struct Histogram {
    std::string name;
    int sum;
    int count;
    std::vector<HistogramEntry> buckets;

    static Histogram fromJson(const JsonValue& json) {
        Histogram h;
        h.name = json["name"].getString();
        h.sum = json["sum"].getInt();
        h.count = json["count"].getInt();
        if (json["buckets"].isArray()) {
            for (const auto& b : json["buckets"].asArray()) {
                h.buckets.push_back(HistogramEntry::fromJson(b));
            }
        }
        return h;
    }
};


class Browser : public Domain {
public:
    explicit Browser(CDPConnection& connection) : Domain(connection, "Browser") {}

    
    CDPResponse getVersion() { return call("getVersion"); }

    CDPResponse getBrowserCommandLine() { return call("getBrowserCommandLine"); }

    
    CDPResponse getWindowBounds(int windowId) {
        return call("getWindowBounds", Params().set("windowId", windowId));
    }

    CDPResponse setWindowBounds(int windowId, const Bounds& bounds) {
        return call("setWindowBounds", Params()
            .set("windowId", windowId)
            .set("bounds", bounds.toJson()));
    }

    CDPResponse getWindowForTarget(const std::string& targetId = "") {
        Params params;
        if (!targetId.empty()) params.set("targetId", targetId);
        return call("getWindowForTarget", params);
    }

    
    CDPResponse setDownloadBehavior(const std::string& behavior,  
                                     const std::string& browserContextId = "",
                                     const std::string& downloadPath = "",
                                     bool eventsEnabled = false) {
        Params params;
        params.set("behavior", behavior);
        if (!browserContextId.empty()) params.set("browserContextId", browserContextId);
        if (!downloadPath.empty()) params.set("downloadPath", downloadPath);
        if (eventsEnabled) params.set("eventsEnabled", true);
        return call("setDownloadBehavior", params);
    }

    CDPResponse cancelDownload(const std::string& guid,
                                const std::string& browserContextId = "") {
        Params params;
        params.set("guid", guid);
        if (!browserContextId.empty()) params.set("browserContextId", browserContextId);
        return call("cancelDownload", params);
    }

    
    CDPResponse setPermission(const PermissionDescriptor& permission,
                               const std::string& setting,  
                               const std::string& origin = "",
                               const std::string& browserContextId = "") {
        Params params;
        params.set("permission", permission.toJson());
        params.set("setting", setting);
        if (!origin.empty()) params.set("origin", origin);
        if (!browserContextId.empty()) params.set("browserContextId", browserContextId);
        return call("setPermission", params);
    }

    CDPResponse grantPermissions(const std::vector<std::string>& permissions,
                                  const std::string& origin = "",
                                  const std::string& browserContextId = "") {
        JsonArray arr;
        for (const auto& p : permissions) arr.push_back(p);
        Params params;
        params.set("permissions", arr);
        if (!origin.empty()) params.set("origin", origin);
        if (!browserContextId.empty()) params.set("browserContextId", browserContextId);
        return call("grantPermissions", params);
    }

    CDPResponse resetPermissions(const std::string& browserContextId = "") {
        Params params;
        if (!browserContextId.empty()) params.set("browserContextId", browserContextId);
        return call("resetPermissions", params);
    }

    
    CDPResponse setDockTile(const std::string& badgeLabel = "",
                             const std::string& image = "") {
        Params params;
        if (!badgeLabel.empty()) params.set("badgeLabel", badgeLabel);
        if (!image.empty()) params.set("image", image);
        return call("setDockTile", params);
    }

    
    CDPResponse getHistograms(const std::string& query = "", bool delta = false) {
        Params params;
        if (!query.empty()) params.set("query", query);
        if (delta) params.set("delta", true);
        return call("getHistograms", params);
    }

    CDPResponse getHistogram(const std::string& name, bool delta = false) {
        Params params;
        params.set("name", name);
        if (delta) params.set("delta", true);
        return call("getHistogram", params);
    }

    
    CDPResponse crash() { return call("crash"); }

    CDPResponse crashGpuProcess() { return call("crashGpuProcess"); }

    
    CDPResponse close() { return call("close"); }

    
    CDPResponse executeBrowserCommand(const std::string& commandId) {
        return call("executeBrowserCommand", Params().set("commandId", commandId));
    }

    
    CDPResponse addPrivacySandboxEnrollmentOverride(const std::string& url) {
        return call("addPrivacySandboxEnrollmentOverride", Params().set("url", url));
    }

    
    CDPResponse addPrivacySandboxCoordinatorKeyConfig(const std::string& apiName,
                                                       const std::string& keyConfig) {
        return call("addPrivacySandboxCoordinatorKeyConfig",
                   Params().set("apiName", apiName).set("keyConfig", keyConfig));
    }

    
    CDPResponse setContentsSize(int windowId, int width, int height) {
        Params params;
        params.set("windowId", windowId);
        JsonObject size;
        size["width"] = width;
        size["height"] = height;
        params.set("size", size);
        return call("setContentsSize", params);
    }

    
    void onDownloadWillBegin(std::function<void(const std::string& frameId,
                                                  const std::string& guid,
                                                  const std::string& url,
                                                  const std::string& suggestedFilename)> callback) {
        on("downloadWillBegin", [callback](const CDPEvent& event) {
            callback(
                event.params["frameId"].getString(),
                event.params["guid"].getString(),
                event.params["url"].getString(),
                event.params["suggestedFilename"].getString()
            );
        });
    }

    void onDownloadProgress(std::function<void(const std::string& guid,
                                                 double totalBytes,
                                                 double receivedBytes,
                                                 const std::string& state)> callback) {
        on("downloadProgress", [callback](const CDPEvent& event) {
            callback(
                event.params["guid"].getString(),
                event.params["totalBytes"].getNumber(),
                event.params["receivedBytes"].getNumber(),
                event.params["state"].getString()
            );
        });
    }
};

} 
