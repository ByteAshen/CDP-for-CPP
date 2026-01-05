#pragma once

#include "Domain.hpp"

namespace cdp {


struct LayoutShiftAttribution {
    JsonValue previousRect;
    JsonValue currentRect;
    int nodeId;

    static LayoutShiftAttribution fromJson(const JsonValue& json) {
        LayoutShiftAttribution l;
        l.previousRect = json["previousRect"];
        l.currentRect = json["currentRect"];
        l.nodeId = json["nodeId"].getInt();
        return l;
    }
};


struct TimelineEvent {
    std::string frameId;
    std::string type;
    std::string name;
    double time;
    double duration;
    JsonValue lcpDetails;
    JsonValue layoutShiftDetails;

    static TimelineEvent fromJson(const JsonValue& json) {
        TimelineEvent e;
        e.frameId = json["frameId"].getString();
        e.type = json["type"].getString();
        e.name = json["name"].getString();
        e.time = json["time"].getNumber();
        e.duration = json["duration"].getNumber();
        e.lcpDetails = json["lcpDetails"];
        e.layoutShiftDetails = json["layoutShiftDetails"];
        return e;
    }
};


class PerformanceTimeline : public Domain {
public:
    explicit PerformanceTimeline(CDPConnection& connection) : Domain(connection, "PerformanceTimeline") {}

    
    CDPResponse enable(const std::vector<std::string>& eventTypes) {
        JsonArray arr;
        for (const auto& e : eventTypes) arr.push_back(e);
        return call("enable", Params().set("eventTypes", arr));
    }

    
    void onTimelineEventAdded(std::function<void(const JsonValue& event)> callback) {
        on("timelineEventAdded", [callback](const CDPEvent& event) {
            callback(event.params["event"]);
        });
    }
};

} 
