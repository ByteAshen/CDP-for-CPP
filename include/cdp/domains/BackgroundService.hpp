#pragma once

#include "Domain.hpp"

namespace cdp {


struct BackgroundServiceEvent {
    double timestamp;
    std::string origin;
    std::string serviceWorkerRegistrationId;
    std::string service;  
    std::string eventName;
    std::string instanceId;
    JsonValue eventMetadata;
    std::string storageKey;

    static BackgroundServiceEvent fromJson(const JsonValue& json) {
        BackgroundServiceEvent e;
        e.timestamp = json["timestamp"].getNumber();
        e.origin = json["origin"].getString();
        e.serviceWorkerRegistrationId = json["serviceWorkerRegistrationId"].getString();
        e.service = json["service"].getString();
        e.eventName = json["eventName"].getString();
        e.instanceId = json["instanceId"].getString();
        e.eventMetadata = json["eventMetadata"];
        e.storageKey = json["storageKey"].getString();
        return e;
    }
};


class BackgroundService : public Domain {
public:
    explicit BackgroundService(CDPConnection& connection) : Domain(connection, "BackgroundService") {}

    
    CDPResponse startObserving(const std::string& service) {
        return call("startObserving", Params().set("service", service));
    }

    
    CDPResponse stopObserving(const std::string& service) {
        return call("stopObserving", Params().set("service", service));
    }

    
    CDPResponse setRecording(bool shouldRecord, const std::string& service) {
        return call("setRecording", Params()
            .set("shouldRecord", shouldRecord)
            .set("service", service));
    }

    
    CDPResponse clearEvents(const std::string& service) {
        return call("clearEvents", Params().set("service", service));
    }

    
    void onRecordingStateChanged(std::function<void(bool isRecording, const std::string& service)> callback) {
        on("recordingStateChanged", [callback](const CDPEvent& event) {
            callback(
                event.params["isRecording"].getBool(),
                event.params["service"].getString()
            );
        });
    }

    void onBackgroundServiceEventReceived(std::function<void(const JsonValue& backgroundServiceEvent)> callback) {
        on("backgroundServiceEventReceived", [callback](const CDPEvent& event) {
            callback(event.params["backgroundServiceEvent"]);
        });
    }
};

} 
