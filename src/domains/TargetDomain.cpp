

#include "cdp/domains/TargetDomain.hpp"

namespace cdp {

TargetInfo TargetInfo::fromJson(const JsonValue& json) {
    TargetInfo info;
    info.targetId = json["targetId"].getString();
    info.type = json["type"].getString();
    info.title = json["title"].getString();
    info.url = json["url"].getString();
    info.attached = json["attached"].getBool();
    info.openerId = json["openerId"].getString();
    info.browserContextId = json["browserContextId"].getString();
    return info;
}

CDPResponse TargetDomain::setAutoAttach(bool autoAttach, bool waitForDebuggerOnStart,
                                         bool flatten, const std::vector<std::string>& filter) {
    JsonObject params;
    params["autoAttach"] = autoAttach;
    params["waitForDebuggerOnStart"] = waitForDebuggerOnStart;
    params["flatten"] = flatten;

    if (!filter.empty()) {
        JsonArray filterArray;
        for (const auto& f : filter) {
            JsonObject filterObj;
            filterObj["type"] = f;
            filterArray.push_back(filterObj);
        }
        params["filter"] = filterArray;
    }

    return sendCommand("Target.setAutoAttach", params);
}

CDPResponse TargetDomain::setDiscoverTargets(bool discover,
                                              const std::vector<std::string>& filter) {
    JsonObject params;
    params["discover"] = discover;

    if (!filter.empty()) {
        JsonArray filterArray;
        for (const auto& f : filter) {
            JsonObject filterObj;
            filterObj["type"] = f;
            filterArray.push_back(filterObj);
        }
        params["filter"] = filterArray;
    }

    return sendCommand("Target.setDiscoverTargets", params);
}

std::vector<TargetInfo> TargetDomain::getTargets(const std::vector<std::string>& filter) {
    JsonObject params;

    if (!filter.empty()) {
        JsonArray filterArray;
        for (const auto& f : filter) {
            JsonObject filterObj;
            filterObj["type"] = f;
            filterArray.push_back(filterObj);
        }
        params["filter"] = filterArray;
    }

    auto response = sendCommand("Target.getTargets", params);
    std::vector<TargetInfo> targets;

    if (response.isSuccess() && response.result["targetInfos"].isArray()) {
        for (size_t i = 0; i < response.result["targetInfos"].size(); ++i) {
            targets.push_back(TargetInfo::fromJson(response.result["targetInfos"][i]));
        }
    }

    return targets;
}

std::string TargetDomain::createTarget(const std::string& url, int width, int height,
                                        const std::string& browserContextId,
                                        bool enableBeginFrameControl,
                                        bool newWindow, bool background) {
    JsonObject params;
    params["url"] = url;
    if (width > 0) params["width"] = width;
    if (height > 0) params["height"] = height;
    if (!browserContextId.empty()) params["browserContextId"] = browserContextId;
    if (enableBeginFrameControl) params["enableBeginFrameControl"] = enableBeginFrameControl;
    if (newWindow) params["newWindow"] = newWindow;
    if (background) params["background"] = background;

    auto response = sendCommand("Target.createTarget", params);
    if (response.isSuccess()) {
        return response.result["targetId"].getString();
    }
    return "";
}

CDPResponse TargetDomain::closeTarget(const std::string& targetId) {
    JsonObject params;
    params["targetId"] = targetId;
    return sendCommand("Target.closeTarget", params);
}

std::string TargetDomain::attachToTarget(const std::string& targetId, bool flatten) {
    JsonObject params;
    params["targetId"] = targetId;
    params["flatten"] = flatten;

    auto response = sendCommand("Target.attachToTarget", params);
    if (response.isSuccess()) {
        return response.result["sessionId"].getString();
    }
    return "";
}

CDPResponse TargetDomain::detachFromTarget(const std::string& sessionId, const std::string& targetId) {
    JsonObject params;
    if (!sessionId.empty()) params["sessionId"] = sessionId;
    if (!targetId.empty()) params["targetId"] = targetId;
    return sendCommand("Target.detachFromTarget", params);
}

CDPResponse TargetDomain::activateTarget(const std::string& targetId) {
    JsonObject params;
    params["targetId"] = targetId;
    return sendCommand("Target.activateTarget", params);
}

TargetInfo TargetDomain::getTargetInfo(const std::string& targetId) {
    JsonObject params;
    if (!targetId.empty()) params["targetId"] = targetId;

    auto response = sendCommand("Target.getTargetInfo", params);
    if (response.isSuccess()) {
        return TargetInfo::fromJson(response.result["targetInfo"]);
    }
    return {};
}

std::string TargetDomain::createBrowserContext(bool disposeOnDetach,
                                                const std::string& proxyServer,
                                                const std::string& proxyBypassList) {
    JsonObject params;
    if (disposeOnDetach) params["disposeOnDetach"] = disposeOnDetach;
    if (!proxyServer.empty()) params["proxyServer"] = proxyServer;
    if (!proxyBypassList.empty()) params["proxyBypassList"] = proxyBypassList;

    auto response = sendCommand("Target.createBrowserContext", params);
    if (response.isSuccess()) {
        return response.result["browserContextId"].getString();
    }
    return "";
}

CDPResponse TargetDomain::disposeBrowserContext(const std::string& browserContextId) {
    JsonObject params;
    params["browserContextId"] = browserContextId;
    return sendCommand("Target.disposeBrowserContext", params);
}

std::vector<std::string> TargetDomain::getBrowserContexts() {
    auto response = sendCommand("Target.getBrowserContexts");
    std::vector<std::string> contexts;

    if (response.isSuccess() && response.result["browserContextIds"].isArray()) {
        for (size_t i = 0; i < response.result["browserContextIds"].size(); ++i) {
            contexts.push_back(response.result["browserContextIds"][i].getString());
        }
    }

    return contexts;
}

CDPResponse TargetDomain::sendMessageToTarget(const std::string& message,
                                               const std::string& sessionId,
                                               const std::string& targetId) {
    JsonObject params;
    params["message"] = message;
    if (!sessionId.empty()) params["sessionId"] = sessionId;
    if (!targetId.empty()) params["targetId"] = targetId;
    return sendCommand("Target.sendMessageToTarget", params);
}

CDPResponse TargetDomain::exposeDevToolsProtocol(const std::string& targetId,
                                                  const std::string& bindingName) {
    JsonObject params;
    params["targetId"] = targetId;
    if (!bindingName.empty()) params["bindingName"] = bindingName;
    return sendCommand("Target.exposeDevToolsProtocol", params);
}

void TargetDomain::onTargetCreated(std::function<void(const TargetInfo&)> callback) {
    subscribeEvent("targetCreated", [callback](const CDPEvent& event) {
        callback(TargetInfo::fromJson(event.params["targetInfo"]));
    });
}

void TargetDomain::onTargetDestroyed(std::function<void(const std::string&)> callback) {
    subscribeEvent("targetDestroyed", [callback](const CDPEvent& event) {
        callback(event.params["targetId"].getString());
    });
}

void TargetDomain::onTargetInfoChanged(std::function<void(const TargetInfo&)> callback) {
    subscribeEvent("targetInfoChanged", [callback](const CDPEvent& event) {
        callback(TargetInfo::fromJson(event.params["targetInfo"]));
    });
}

void TargetDomain::onAttachedToTarget(
    std::function<void(const std::string&, const TargetInfo&, bool)> callback) {
    subscribeEvent("attachedToTarget", [callback](const CDPEvent& event) {
        callback(
            event.params["sessionId"].getString(),
            TargetInfo::fromJson(event.params["targetInfo"]),
            event.params["waitingForDebugger"].getBool()
        );
    });
}

void TargetDomain::onDetachedFromTarget(
    std::function<void(const std::string&, const std::string&)> callback) {
    subscribeEvent("detachedFromTarget", [callback](const CDPEvent& event) {
        callback(
            event.params["sessionId"].getString(),
            event.params["targetId"].getString()
        );
    });
}

void TargetDomain::onReceivedMessageFromTarget(
    std::function<void(const std::string&, const std::string&, const std::string&)> callback) {
    subscribeEvent("receivedMessageFromTarget", [callback](const CDPEvent& event) {
        callback(
            event.params["sessionId"].getString(),
            event.params["message"].getString(),
            event.params["targetId"].getString()
        );
    });
}

} 
