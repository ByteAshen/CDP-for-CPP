#pragma once

#include "Domain.hpp"

namespace cdp {


struct PromptDevice {
    std::string id;
    std::string name;

    static PromptDevice fromJson(const JsonValue& json) {
        PromptDevice d;
        d.id = json["id"].getString();
        d.name = json["name"].getString();
        return d;
    }
};


class DeviceAccess : public Domain {
public:
    explicit DeviceAccess(CDPConnection& connection) : Domain(connection, "DeviceAccess") {}

    
    CDPResponse enable() { return call("enable"); }

    
    CDPResponse disable() { return call("disable"); }

    
    CDPResponse selectPrompt(const std::string& id, const std::string& deviceId) {
        return call("selectPrompt", Params()
            .set("id", id)
            .set("deviceId", deviceId));
    }

    
    CDPResponse cancelPrompt(const std::string& id) {
        return call("cancelPrompt", Params().set("id", id));
    }

    
    void onDeviceRequestPrompted(std::function<void(const std::string& id,
                                                       const JsonValue& devices)> callback) {
        on("deviceRequestPrompted", [callback](const CDPEvent& event) {
            callback(
                event.params["id"].getString(),
                event.params["devices"]
            );
        });
    }
};

} 
