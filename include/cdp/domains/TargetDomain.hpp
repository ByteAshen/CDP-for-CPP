#pragma once

#include "BaseDomain.hpp"
#include <string>
#include <vector>

namespace cdp {


struct TargetInfo {
    std::string targetId;
    std::string type;
    std::string title;
    std::string url;
    bool attached = false;
    std::string openerId;
    std::string browserContextId;

    static TargetInfo fromJson(const JsonValue& json);
};


class TargetDomain : public BaseDomain {
public:
    using BaseDomain::BaseDomain;
    std::string domainName() const override { return "Target"; }

    
    CDPResponse setAutoAttach(bool autoAttach, bool waitForDebuggerOnStart = false,
                               bool flatten = true, const std::vector<std::string>& filter = {});

    
    CDPResponse setDiscoverTargets(bool discover,
                                    const std::vector<std::string>& filter = {});

    
    std::vector<TargetInfo> getTargets(const std::vector<std::string>& filter = {});

    
    std::string createTarget(const std::string& url, int width = 0, int height = 0,
                              const std::string& browserContextId = "",
                              bool enableBeginFrameControl = false,
                              bool newWindow = false, bool background = false);

    
    CDPResponse closeTarget(const std::string& targetId);

    
    std::string attachToTarget(const std::string& targetId, bool flatten = true);

    
    CDPResponse detachFromTarget(const std::string& sessionId = "",
                                  const std::string& targetId = "");

    
    CDPResponse activateTarget(const std::string& targetId);

    
    TargetInfo getTargetInfo(const std::string& targetId = "");

    
    std::string createBrowserContext(bool disposeOnDetach = false,
                                       const std::string& proxyServer = "",
                                       const std::string& proxyBypassList = "");
    CDPResponse disposeBrowserContext(const std::string& browserContextId);
    std::vector<std::string> getBrowserContexts();

    
    CDPResponse sendMessageToTarget(const std::string& message,
                                     const std::string& sessionId = "",
                                     const std::string& targetId = "");

    
    CDPResponse exposeDevToolsProtocol(const std::string& targetId,
                                        const std::string& bindingName = "cdp");

    
    void onTargetCreated(std::function<void(const TargetInfo&)> callback);
    void onTargetDestroyed(std::function<void(const std::string& targetId)> callback);
    void onTargetInfoChanged(std::function<void(const TargetInfo&)> callback);
    void onAttachedToTarget(std::function<void(const std::string& sessionId,
                                                const TargetInfo& targetInfo,
                                                bool waitingForDebugger)> callback);
    void onDetachedFromTarget(std::function<void(const std::string& sessionId,
                                                  const std::string& targetId)> callback);
    void onReceivedMessageFromTarget(std::function<void(const std::string& sessionId,
                                                         const std::string& message,
                                                         const std::string& targetId)> callback);
};

} 
