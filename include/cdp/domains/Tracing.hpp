#pragma once

#include "Domain.hpp"

namespace cdp {


struct TraceConfig {
    std::string recordMode;  
    int traceBufferSizeInKb = 0;
    bool enableSampling = false;
    bool enableSystrace = false;
    bool enableArgumentFilter = false;
    std::vector<std::string> includedCategories;
    std::vector<std::string> excludedCategories;
    std::vector<std::string> syntheticDelays;
    std::string memoryDumpConfig;

    JsonValue toJson() const {
        JsonObject obj;
        if (!recordMode.empty()) obj["recordMode"] = recordMode;
        if (traceBufferSizeInKb > 0) obj["traceBufferSizeInKb"] = traceBufferSizeInKb;
        if (enableSampling) obj["enableSampling"] = true;
        if (enableSystrace) obj["enableSystrace"] = true;
        if (enableArgumentFilter) obj["enableArgumentFilter"] = true;
        if (!includedCategories.empty()) {
            JsonArray arr;
            for (const auto& c : includedCategories) arr.push_back(c);
            obj["includedCategories"] = arr;
        }
        if (!excludedCategories.empty()) {
            JsonArray arr;
            for (const auto& c : excludedCategories) arr.push_back(c);
            obj["excludedCategories"] = arr;
        }
        return obj;
    }
};


class Tracing : public Domain {
public:
    explicit Tracing(CDPConnection& connection) : Domain(connection, "Tracing") {}

    
    CDPResponse start(const std::string& categories = "",
                       const std::string& options = "",
                       int bufferUsageReportingInterval = 0,
                       const std::string& transferMode = "",  
                       const std::string& streamFormat = "",  
                       const std::string& streamCompression = "",  
                       const TraceConfig* traceConfig = nullptr,
                       const std::string& perfettoConfig = "",
                       const std::string& tracingBackend = "") {  
        Params params;
        if (!categories.empty()) params.set("categories", categories);
        if (!options.empty()) params.set("options", options);
        if (bufferUsageReportingInterval > 0) params.set("bufferUsageReportingInterval", bufferUsageReportingInterval);
        if (!transferMode.empty()) params.set("transferMode", transferMode);
        if (!streamFormat.empty()) params.set("streamFormat", streamFormat);
        if (!streamCompression.empty()) params.set("streamCompression", streamCompression);
        if (traceConfig) params.set("traceConfig", traceConfig->toJson());
        if (!perfettoConfig.empty()) params.set("perfettoConfig", perfettoConfig);
        if (!tracingBackend.empty()) params.set("tracingBackend", tracingBackend);
        return call("start", params);
    }

    
    CDPResponse end() { return call("end"); }

    
    CDPResponse getCategories() { return call("getCategories"); }

    
    CDPResponse requestMemoryDump(bool deterministic = false,
                                   const std::string& levelOfDetail = "") {  
        Params params;
        if (deterministic) params.set("deterministic", true);
        if (!levelOfDetail.empty()) params.set("levelOfDetail", levelOfDetail);
        return call("requestMemoryDump", params);
    }

    
    CDPResponse recordClockSyncMarker(const std::string& syncId) {
        return call("recordClockSyncMarker", Params().set("syncId", syncId));
    }

    
    void onBufferUsage(std::function<void(double percentFull, int eventCount, double value)> callback) {
        on("bufferUsage", [callback](const CDPEvent& event) {
            callback(
                event.params["percentFull"].getNumber(),
                event.params["eventCount"].getInt(),
                event.params["value"].getNumber()
            );
        });
    }

    void onDataCollected(std::function<void(const JsonValue& value)> callback) {
        on("dataCollected", [callback](const CDPEvent& event) {
            callback(event.params["value"]);
        });
    }

    void onTracingComplete(std::function<void(bool dataLossOccurred,
                                                const std::string& stream,
                                                const std::string& traceFormat,
                                                const std::string& streamCompression)> callback) {
        on("tracingComplete", [callback](const CDPEvent& event) {
            callback(
                event.params["dataLossOccurred"].getBool(),
                event.params["stream"].getString(),
                event.params["traceFormat"].getString(),
                event.params["streamCompression"].getString()
            );
        });
    }
};

} 
