#pragma once

#include "Domain.hpp"

namespace cdp {


struct PreloadingAttemptKey {
    std::string loaderId;
    std::string action;  
    std::string url;
    std::string targetHint;  

    static PreloadingAttemptKey fromJson(const JsonValue& json) {
        PreloadingAttemptKey k;
        k.loaderId = json["loaderId"].getString();
        k.action = json["action"].getString();
        k.url = json["url"].getString();
        k.targetHint = json["targetHint"].getString();
        return k;
    }
};


struct PreloadingAttemptSource {
    JsonValue key;
    std::string ruleSetIds;
    std::vector<int> nodeIds;

    static PreloadingAttemptSource fromJson(const JsonValue& json) {
        PreloadingAttemptSource s;
        s.key = json["key"];
        s.ruleSetIds = json["ruleSetIds"].getString();
        if (json["nodeIds"].isArray()) {
            for (const auto& n : json["nodeIds"].asArray()) {
                s.nodeIds.push_back(n.getInt());
            }
        }
        return s;
    }
};


class Preload : public Domain {
public:
    explicit Preload(CDPConnection& connection) : Domain(connection, "Preload") {}

    
    CDPResponse enable() { return call("enable"); }

    
    CDPResponse disable() { return call("disable"); }

    
    void onRuleSetUpdated(std::function<void(const JsonValue& ruleSet)> callback) {
        on("ruleSetUpdated", [callback](const CDPEvent& event) {
            callback(event.params["ruleSet"]);
        });
    }

    void onRuleSetRemoved(std::function<void(const std::string& id)> callback) {
        on("ruleSetRemoved", [callback](const CDPEvent& event) {
            callback(event.params["id"].getString());
        });
    }

    void onPreloadEnabledStateUpdated(std::function<void(bool disabledByPreference,
                                                           bool disabledByDataSaver,
                                                           bool disabledByBatterySaver,
                                                           bool disabledByHoldbackPrefetchSpeculationRules,
                                                           bool disabledByHoldbackPrerenderSpeculationRules)> callback) {
        on("preloadEnabledStateUpdated", [callback](const CDPEvent& event) {
            callback(
                event.params["disabledByPreference"].getBool(),
                event.params["disabledByDataSaver"].getBool(),
                event.params["disabledByBatterySaver"].getBool(),
                event.params["disabledByHoldbackPrefetchSpeculationRules"].getBool(),
                event.params["disabledByHoldbackPrerenderSpeculationRules"].getBool()
            );
        });
    }

    void onPrefetchStatusUpdated(std::function<void(const JsonValue& key,
                                                       const std::string& initiatingFrameId,
                                                       const std::string& prefetchUrl,
                                                       const std::string& status)> callback) {
        on("prefetchStatusUpdated", [callback](const CDPEvent& event) {
            callback(
                event.params["key"],
                event.params["initiatingFrameId"].getString(),
                event.params["prefetchUrl"].getString(),
                event.params["status"].getString()
            );
        });
    }

    void onPrerenderStatusUpdated(std::function<void(const JsonValue& key,
                                                        const std::string& status,
                                                        const std::string& prerenderStatus,
                                                        const std::string& disallowedMojoInterface,
                                                        const std::vector<std::string>& mismatchedHeaders)> callback) {
        on("prerenderStatusUpdated", [callback](const CDPEvent& event) {
            std::vector<std::string> headers;
            if (event.params["mismatchedHeaders"].isArray()) {
                for (const auto& h : event.params["mismatchedHeaders"].asArray()) {
                    headers.push_back(h.getString());
                }
            }
            callback(
                event.params["key"],
                event.params["status"].getString(),
                event.params["prerenderStatus"].getString(),
                event.params["disallowedMojoInterface"].getString(),
                headers
            );
        });
    }

    void onPreloadingAttemptSourcesUpdated(std::function<void(const std::string& loaderId,
                                                                 const JsonValue& preloadingAttemptSources)> callback) {
        on("preloadingAttemptSourcesUpdated", [callback](const CDPEvent& event) {
            callback(
                event.params["loaderId"].getString(),
                event.params["preloadingAttemptSources"]
            );
        });
    }
};

} 
