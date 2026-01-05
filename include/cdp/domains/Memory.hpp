#pragma once

#include "Domain.hpp"

namespace cdp {


struct SamplingProfileNode {
    double size;
    double total;
    std::vector<std::string> stack;

    static SamplingProfileNode fromJson(const JsonValue& json) {
        SamplingProfileNode n;
        n.size = json["size"].getNumber();
        n.total = json["total"].getNumber();
        if (json["stack"].isArray()) {
            for (const auto& s : json["stack"].asArray()) {
                n.stack.push_back(s.getString());
            }
        }
        return n;
    }
};


class Memory : public Domain {
public:
    explicit Memory(CDPConnection& connection) : Domain(connection, "Memory") {}

    
    CDPResponse getDOMCounters() { return call("getDOMCounters"); }

    
    CDPResponse prepareForLeakDetection() { return call("prepareForLeakDetection"); }

    
    CDPResponse forciblyPurgeJavaScriptMemory() { return call("forciblyPurgeJavaScriptMemory"); }

    
    CDPResponse setPressureNotificationsSuppressed(bool suppressed) {
        return call("setPressureNotificationsSuppressed", Params().set("suppressed", suppressed));
    }

    
    CDPResponse simulatePressureNotification(const std::string& level) {  
        return call("simulatePressureNotification", Params().set("level", level));
    }

    
    CDPResponse startSampling(int samplingInterval = 0, bool suppressRandomness = false) {
        Params params;
        if (samplingInterval > 0) params.set("samplingInterval", samplingInterval);
        if (suppressRandomness) params.set("suppressRandomness", true);
        return call("startSampling", params);
    }

    
    CDPResponse stopSampling() { return call("stopSampling"); }

    
    CDPResponse getAllTimeSamplingProfile() { return call("getAllTimeSamplingProfile"); }

    
    CDPResponse getBrowserSamplingProfile() { return call("getBrowserSamplingProfile"); }

    
    CDPResponse getSamplingProfile() { return call("getSamplingProfile"); }
};

} 
