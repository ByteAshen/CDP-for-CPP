

#pragma once

#include "Domain.hpp"

namespace cdp {


class HeapProfiler : public Domain {
public:
    explicit HeapProfiler(CDPConnection& connection) : Domain(connection, "HeapProfiler") {}

    
    CDPResponse enable() { return call("enable"); }

    
    CDPResponse disable() { return call("disable"); }

    
    CDPResponse collectGarbage() { return call("collectGarbage"); }

    
    CDPResponse startTrackingHeapObjects(bool trackAllocations = false) {
        Params params;
        if (trackAllocations) params.set("trackAllocations", true);
        return call("startTrackingHeapObjects", params);
    }

    
    CDPResponse stopTrackingHeapObjects(bool reportProgress = false,
                                         bool treatGlobalObjectsAsRoots = false,
                                         bool captureNumericValue = false,
                                         bool exposeInternals = false) {
        Params params;
        if (reportProgress) params.set("reportProgress", true);
        if (treatGlobalObjectsAsRoots) params.set("treatGlobalObjectsAsRoots", true);
        if (captureNumericValue) params.set("captureNumericValue", true);
        if (exposeInternals) params.set("exposeInternals", true);
        return call("stopTrackingHeapObjects", params);
    }

    
    CDPResponse takeHeapSnapshot(bool reportProgress = false,
                                  bool treatGlobalObjectsAsRoots = false,
                                  bool captureNumericValue = false,
                                  bool exposeInternals = false) {
        Params params;
        if (reportProgress) params.set("reportProgress", true);
        if (treatGlobalObjectsAsRoots) params.set("treatGlobalObjectsAsRoots", true);
        if (captureNumericValue) params.set("captureNumericValue", true);
        if (exposeInternals) params.set("exposeInternals", true);
        return call("takeHeapSnapshot", params);
    }

    
    CDPResponse getHeapObjectId(const std::string& objectId) {
        return call("getHeapObjectId", Params().set("objectId", objectId));
    }

    
    CDPResponse getObjectByHeapObjectId(const std::string& objectId,
                                         const std::string& objectGroup = "") {
        Params params;
        params.set("objectId", objectId);
        if (!objectGroup.empty()) params.set("objectGroup", objectGroup);
        return call("getObjectByHeapObjectId", params);
    }

    
    CDPResponse addInspectedHeapObject(const std::string& heapObjectId) {
        return call("addInspectedHeapObject", Params().set("heapObjectId", heapObjectId));
    }

    
    CDPResponse getSamplingProfile() {
        return call("getSamplingProfile");
    }

    
    CDPResponse startSampling(double samplingInterval = 0,
                               bool includeObjectsCollectedByMajorGC = false,
                               bool includeObjectsCollectedByMinorGC = false) {
        Params params;
        if (samplingInterval > 0) params.set("samplingInterval", samplingInterval);
        if (includeObjectsCollectedByMajorGC) params.set("includeObjectsCollectedByMajorGC", true);
        if (includeObjectsCollectedByMinorGC) params.set("includeObjectsCollectedByMinorGC", true);
        return call("startSampling", params);
    }

    
    CDPResponse stopSampling() {
        return call("stopSampling");
    }

    
    void onAddHeapSnapshotChunk(std::function<void(const std::string& chunk)> callback) {
        on("addHeapSnapshotChunk", [callback](const CDPEvent& event) {
            callback(event.params["chunk"].asString());
        });
    }

    
    void onHeapStatsUpdate(std::function<void(const JsonValue& statsUpdate)> callback) {
        on("heapStatsUpdate", [callback](const CDPEvent& event) {
            callback(event.params["statsUpdate"]);
        });
    }

    
    void onLastSeenObjectId(std::function<void(int lastSeenObjectId, double timestamp)> callback) {
        on("lastSeenObjectId", [callback](const CDPEvent& event) {
            callback(event.params["lastSeenObjectId"].asInt(),
                    event.params["timestamp"].asDouble());
        });
    }

    
    void onReportHeapSnapshotProgress(std::function<void(int done, int total, bool finished)> callback) {
        on("reportHeapSnapshotProgress", [callback](const CDPEvent& event) {
            callback(event.params["done"].asInt(),
                    event.params["total"].asInt(),
                    event.params["finished"].asBool());
        });
    }

    
    void onResetProfiles(std::function<void()> callback) {
        on("resetProfiles", [callback](const CDPEvent& event) {
            (void)event;
            callback();
        });
    }
};

} 
