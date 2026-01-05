#pragma once

#include "Domain.hpp"

namespace cdp {


struct TargetInfo {
    std::string targetId;
    std::string type;
    std::string title;
    std::string url;
    bool attached = false;
    std::string openerId;
    bool canAccessOpener = false;
    std::string openerFrameId;
    std::string browserContextId;
    std::string subtype;

    static TargetInfo fromJson(const JsonValue& json) {
        TargetInfo info;
        info.targetId = json["targetId"].getString();
        info.type = json["type"].getString();
        info.title = json["title"].getString();
        info.url = json["url"].getString();
        info.attached = json["attached"].getBool();
        info.openerId = json["openerId"].getString();
        info.canAccessOpener = json["canAccessOpener"].getBool();
        info.openerFrameId = json["openerFrameId"].getString();
        info.browserContextId = json["browserContextId"].getString();
        info.subtype = json["subtype"].getString();
        return info;
    }
};


struct FilterEntry {
    bool exclude = false;
    std::string type;

    JsonValue toJson() const {
        JsonObject obj;
        if (exclude) obj["exclude"] = true;
        if (!type.empty()) obj["type"] = type;
        return obj;
    }
};


struct RemoteLocation {
    std::string host;
    int port;

    JsonValue toJson() const {
        JsonObject obj;
        obj["host"] = host;
        obj["port"] = port;
        return obj;
    }
};


class Target : public Domain {
public:
    explicit Target(CDPConnection& connection) : Domain(connection, "Target") {}

    
    CDPResponse setDiscoverTargets(bool discover,
                                    const std::vector<FilterEntry>& filter = {}) {
        Params params;
        params.set("discover", discover);
        if (!filter.empty()) {
            JsonArray arr;
            for (const auto& f : filter) arr.push_back(f.toJson());
            params.set("filter", arr);
        }
        return call("setDiscoverTargets", params);
    }

    CDPResponse getTargets(const std::vector<FilterEntry>& filter = {}) {
        Params params;
        if (!filter.empty()) {
            JsonArray arr;
            for (const auto& f : filter) arr.push_back(f.toJson());
            params.set("filter", arr);
        }
        return call("getTargets", params);
    }

    CDPResponse getTargetInfo(const std::string& targetId = "") {
        Params params;
        if (!targetId.empty()) params.set("targetId", targetId);
        return call("getTargetInfo", params);
    }

    
    CDPResponse createTarget(const std::string& url,
                              int width = 0,
                              int height = 0,
                              const std::string& browserContextId = "",
                              bool enableBeginFrameControl = false,
                              bool newWindow = false,
                              bool background = false,
                              bool forTab = false) {
        Params params;
        params.set("url", url);
        if (width > 0) params.set("width", width);
        if (height > 0) params.set("height", height);
        if (!browserContextId.empty()) params.set("browserContextId", browserContextId);
        if (enableBeginFrameControl) params.set("enableBeginFrameControl", true);
        if (newWindow) params.set("newWindow", true);
        if (background) params.set("background", true);
        if (forTab) params.set("forTab", true);
        return call("createTarget", params);
    }

    CDPResponse closeTarget(const std::string& targetId) {
        return call("closeTarget", Params().set("targetId", targetId));
    }

    
    CDPResponse attachToTarget(const std::string& targetId, bool flatten = false) {
        return call("attachToTarget", Params()
            .set("targetId", targetId)
            .set("flatten", flatten));
    }

    CDPResponse detachFromTarget(const std::string& sessionId = "",
                                  const std::string& targetId = "") {
        Params params;
        if (!sessionId.empty()) params.set("sessionId", sessionId);
        if (!targetId.empty()) params.set("targetId", targetId);
        return call("detachFromTarget", params);
    }

    CDPResponse setAutoAttach(bool autoAttach,
                               bool waitForDebuggerOnStart,
                               bool flatten = false,
                               const std::vector<FilterEntry>& filter = {}) {
        Params params;
        params.set("autoAttach", autoAttach);
        params.set("waitForDebuggerOnStart", waitForDebuggerOnStart);
        if (flatten) params.set("flatten", true);
        if (!filter.empty()) {
            JsonArray arr;
            for (const auto& f : filter) arr.push_back(f.toJson());
            params.set("filter", arr);
        }
        return call("setAutoAttach", params);
    }

    CDPResponse autoAttachRelated(const std::string& targetId,
                                   bool waitForDebuggerOnStart,
                                   const std::vector<FilterEntry>& filter = {}) {
        Params params;
        params.set("targetId", targetId);
        params.set("waitForDebuggerOnStart", waitForDebuggerOnStart);
        if (!filter.empty()) {
            JsonArray arr;
            for (const auto& f : filter) arr.push_back(f.toJson());
            params.set("filter", arr);
        }
        return call("autoAttachRelated", params);
    }

    
    CDPResponse activateTarget(const std::string& targetId) {
        return call("activateTarget", Params().set("targetId", targetId));
    }

    
    CDPResponse createBrowserContext(bool disposeOnDetach = false,
                                      const std::string& proxyServer = "",
                                      const std::string& proxyBypassList = "",
                                      const std::vector<std::string>& originsWithUniversalNetworkAccess = {}) {
        Params params;
        if (disposeOnDetach) params.set("disposeOnDetach", true);
        if (!proxyServer.empty()) params.set("proxyServer", proxyServer);
        if (!proxyBypassList.empty()) params.set("proxyBypassList", proxyBypassList);
        if (!originsWithUniversalNetworkAccess.empty()) {
            JsonArray arr;
            for (const auto& o : originsWithUniversalNetworkAccess) arr.push_back(o);
            params.set("originsWithUniversalNetworkAccess", arr);
        }
        return call("createBrowserContext", params);
    }

    CDPResponse disposeBrowserContext(const std::string& browserContextId) {
        return call("disposeBrowserContext", Params().set("browserContextId", browserContextId));
    }

    CDPResponse getBrowserContexts() { return call("getBrowserContexts"); }

    
    CDPResponse sendMessageToTarget(const std::string& message,
                                     const std::string& sessionId = "",
                                     const std::string& targetId = "") {
        Params params;
        params.set("message", message);
        if (!sessionId.empty()) params.set("sessionId", sessionId);
        if (!targetId.empty()) params.set("targetId", targetId);
        return call("sendMessageToTarget", params);
    }

    
    CDPResponse exposeDevToolsProtocol(const std::string& targetId,
                                        const std::string& bindingName = "") {
        Params params;
        params.set("targetId", targetId);
        if (!bindingName.empty()) params.set("bindingName", bindingName);
        return call("exposeDevToolsProtocol", params);
    }

    
    CDPResponse setRemoteLocations(const std::vector<RemoteLocation>& locations) {
        JsonArray arr;
        for (const auto& loc : locations) arr.push_back(loc.toJson());
        return call("setRemoteLocations", Params().set("locations", arr));
    }

    
    CDPResponse attachToBrowserTarget() { return call("attachToBrowserTarget"); }

    
    CDPResponse openDevTools(const std::string& targetId, bool inspectWorkers = false) {
        Params params;
        params.set("targetId", targetId);
        if (inspectWorkers) params.set("inspectWorkers", true);
        return call("openDevTools", params);
    }

    
    CDPResponse getDevToolsTarget() { return call("getDevToolsTarget"); }

    
    void onTargetCreated(std::function<void(const TargetInfo& targetInfo)> callback) {
        on("targetCreated", [callback](const CDPEvent& event) {
            callback(TargetInfo::fromJson(event.params["targetInfo"]));
        });
    }

    void onTargetDestroyed(std::function<void(const std::string& targetId)> callback) {
        on("targetDestroyed", [callback](const CDPEvent& event) {
            callback(event.params["targetId"].getString());
        });
    }

    void onTargetInfoChanged(std::function<void(const TargetInfo& targetInfo)> callback) {
        on("targetInfoChanged", [callback](const CDPEvent& event) {
            callback(TargetInfo::fromJson(event.params["targetInfo"]));
        });
    }

    void onTargetCrashed(std::function<void(const std::string& targetId,
                                             const std::string& status,
                                             int errorCode)> callback) {
        on("targetCrashed", [callback](const CDPEvent& event) {
            callback(
                event.params["targetId"].getString(),
                event.params["status"].getString(),
                event.params["errorCode"].getInt()
            );
        });
    }

    void onAttachedToTarget(std::function<void(const std::string& sessionId,
                                                const TargetInfo& targetInfo,
                                                bool waitingForDebugger)> callback) {
        on("attachedToTarget", [callback](const CDPEvent& event) {
            callback(
                event.params["sessionId"].getString(),
                TargetInfo::fromJson(event.params["targetInfo"]),
                event.params["waitingForDebugger"].getBool()
            );
        });
    }

    void onDetachedFromTarget(std::function<void(const std::string& sessionId,
                                                   const std::string& targetId)> callback) {
        on("detachedFromTarget", [callback](const CDPEvent& event) {
            callback(
                event.params["sessionId"].getString(),
                event.params["targetId"].getString()
            );
        });
    }

    void onReceivedMessageFromTarget(std::function<void(const std::string& sessionId,
                                                          const std::string& message,
                                                          const std::string& targetId)> callback) {
        on("receivedMessageFromTarget", [callback](const CDPEvent& event) {
            callback(
                event.params["sessionId"].getString(),
                event.params["message"].getString(),
                event.params["targetId"].getString()
            );
        });
    }
};

} 
