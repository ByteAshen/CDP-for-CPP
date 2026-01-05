#pragma once

#include "Domain.hpp"

namespace cdp {


struct GPUInfo {
    JsonValue devices;
    JsonValue auxAttributes;
    JsonValue featureStatus;
    std::vector<std::string> driverBugWorkarounds;
    JsonValue videoDecoding;
    JsonValue videoEncoding;
    JsonValue imageDecoding;

    static GPUInfo fromJson(const JsonValue& json) {
        GPUInfo g;
        g.devices = json["devices"];
        g.auxAttributes = json["auxAttributes"];
        g.featureStatus = json["featureStatus"];
        g.videoDecoding = json["videoDecoding"];
        g.videoEncoding = json["videoEncoding"];
        g.imageDecoding = json["imageDecoding"];
        return g;
    }
};


struct ProcessInfo {
    std::string type;
    int id;
    double cpuTime;

    static ProcessInfo fromJson(const JsonValue& json) {
        ProcessInfo p;
        p.type = json["type"].getString();
        p.id = json["id"].getInt();
        p.cpuTime = json["cpuTime"].getNumber();
        return p;
    }
};


class SystemInfo : public Domain {
public:
    explicit SystemInfo(CDPConnection& connection) : Domain(connection, "SystemInfo") {}

    
    CDPResponse getInfo() { return call("getInfo"); }

    
    CDPResponse getFeatureState(const std::string& featureState) {
        return call("getFeatureState", Params().set("featureState", featureState));
    }

    
    CDPResponse getProcessInfo() { return call("getProcessInfo"); }
};

} 
