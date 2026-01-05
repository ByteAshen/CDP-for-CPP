#pragma once

#include "Domain.hpp"

namespace cdp {


struct ProfileNode {
    int id;
    JsonValue callFrame;
    int hitCount;
    std::vector<int> children;
    std::string deoptReason;
    std::vector<JsonValue> positionTicks;

    static ProfileNode fromJson(const JsonValue& json) {
        ProfileNode n;
        n.id = json["id"].getInt();
        n.callFrame = json["callFrame"];
        n.hitCount = json["hitCount"].getInt();
        n.deoptReason = json["deoptReason"].getString();
        if (json["children"].isArray()) {
            for (const auto& c : json["children"].asArray()) {
                n.children.push_back(c.getInt());
            }
        }
        return n;
    }
};


class Profiler : public Domain {
public:
    explicit Profiler(CDPConnection& connection) : Domain(connection, "Profiler") {}

    
    CDPResponse enable() { return call("enable"); }

    
    CDPResponse disable() { return call("disable"); }

    
    CDPResponse setSamplingInterval(int interval) {
        return call("setSamplingInterval", Params().set("interval", interval));
    }

    
    CDPResponse start() { return call("start"); }

    
    CDPResponse stop() { return call("stop"); }

    
    CDPResponse startPreciseCoverage(bool callCount = false,
                                       bool detailed = false,
                                       bool allowTriggeredUpdates = false) {
        Params params;
        if (callCount) params.set("callCount", true);
        if (detailed) params.set("detailed", true);
        if (allowTriggeredUpdates) params.set("allowTriggeredUpdates", true);
        return call("startPreciseCoverage", params);
    }

    
    CDPResponse stopPreciseCoverage() { return call("stopPreciseCoverage"); }

    
    CDPResponse takePreciseCoverage() { return call("takePreciseCoverage"); }

    
    CDPResponse getBestEffortCoverage() { return call("getBestEffortCoverage"); }

    
    void onConsoleProfileStarted(std::function<void(const std::string& id,
                                                       const JsonValue& location,
                                                       const std::string& title)> callback) {
        on("consoleProfileStarted", [callback](const CDPEvent& event) {
            callback(
                event.params["id"].getString(),
                event.params["location"],
                event.params["title"].getString()
            );
        });
    }

    void onConsoleProfileFinished(std::function<void(const std::string& id,
                                                        const JsonValue& location,
                                                        const JsonValue& profile,
                                                        const std::string& title)> callback) {
        on("consoleProfileFinished", [callback](const CDPEvent& event) {
            callback(
                event.params["id"].getString(),
                event.params["location"],
                event.params["profile"],
                event.params["title"].getString()
            );
        });
    }

    void onPreciseCoverageDeltaUpdate(std::function<void(double timestamp,
                                                            const std::string& occasion,
                                                            const JsonValue& result)> callback) {
        on("preciseCoverageDeltaUpdate", [callback](const CDPEvent& event) {
            callback(
                event.params["timestamp"].getNumber(),
                event.params["occasion"].getString(),
                event.params["result"]
            );
        });
    }
};

} 
