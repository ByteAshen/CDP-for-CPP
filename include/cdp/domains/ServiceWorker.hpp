#pragma once

#include "Domain.hpp"

namespace cdp {


struct ServiceWorkerVersion {
    std::string versionId;
    std::string registrationId;
    std::string scriptURL;
    std::string runningStatus;  
    std::string status;  
    int scriptLastModified;
    int scriptResponseTime;
    std::vector<std::string> controlledClients;
    std::string targetId;

    static ServiceWorkerVersion fromJson(const JsonValue& json) {
        ServiceWorkerVersion v;
        v.versionId = json["versionId"].getString();
        v.registrationId = json["registrationId"].getString();
        v.scriptURL = json["scriptURL"].getString();
        v.runningStatus = json["runningStatus"].getString();
        v.status = json["status"].getString();
        return v;
    }
};


class ServiceWorker : public Domain {
public:
    explicit ServiceWorker(CDPConnection& connection) : Domain(connection, "ServiceWorker") {}

    
    CDPResponse enable() { return call("enable"); }

    
    CDPResponse disable() { return call("disable"); }

    
    CDPResponse deliverPushMessage(const std::string& origin,
                                    const std::string& registrationId,
                                    const std::string& data) {
        return call("deliverPushMessage", Params()
            .set("origin", origin)
            .set("registrationId", registrationId)
            .set("data", data));
    }

    
    CDPResponse dispatchPeriodicSyncEvent(const std::string& origin,
                                           const std::string& registrationId,
                                           const std::string& tag) {
        return call("dispatchPeriodicSyncEvent", Params()
            .set("origin", origin)
            .set("registrationId", registrationId)
            .set("tag", tag));
    }

    
    CDPResponse dispatchSyncEvent(const std::string& origin,
                                   const std::string& registrationId,
                                   const std::string& tag,
                                   bool lastChance = false) {
        Params params;
        params.set("origin", origin);
        params.set("registrationId", registrationId);
        params.set("tag", tag);
        if (lastChance) params.set("lastChance", true);
        return call("dispatchSyncEvent", params);
    }

    
    CDPResponse inspectWorker(const std::string& versionId) {
        return call("inspectWorker", Params().set("versionId", versionId));
    }

    
    CDPResponse setForceUpdateOnPageLoad(bool forceUpdateOnPageLoad) {
        return call("setForceUpdateOnPageLoad", Params().set("forceUpdateOnPageLoad", forceUpdateOnPageLoad));
    }

    
    CDPResponse skipWaiting(const std::string& scopeURL) {
        return call("skipWaiting", Params().set("scopeURL", scopeURL));
    }

    
    CDPResponse startWorker(const std::string& scopeURL) {
        return call("startWorker", Params().set("scopeURL", scopeURL));
    }

    
    CDPResponse stopAllWorkers() { return call("stopAllWorkers"); }

    
    CDPResponse stopWorker(const std::string& versionId) {
        return call("stopWorker", Params().set("versionId", versionId));
    }

    
    CDPResponse unregister(const std::string& scopeURL) {
        return call("unregister", Params().set("scopeURL", scopeURL));
    }

    
    CDPResponse updateRegistration(const std::string& scopeURL) {
        return call("updateRegistration", Params().set("scopeURL", scopeURL));
    }

    
    void onWorkerErrorReported(std::function<void(const JsonValue& errorMessage)> callback) {
        on("workerErrorReported", [callback](const CDPEvent& event) {
            callback(event.params["errorMessage"]);
        });
    }

    void onWorkerRegistrationUpdated(std::function<void(const JsonValue& registrations)> callback) {
        on("workerRegistrationUpdated", [callback](const CDPEvent& event) {
            callback(event.params["registrations"]);
        });
    }

    void onWorkerVersionUpdated(std::function<void(const JsonValue& versions)> callback) {
        on("workerVersionUpdated", [callback](const CDPEvent& event) {
            callback(event.params["versions"]);
        });
    }
};

} 
