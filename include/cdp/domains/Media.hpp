#pragma once

#include "Domain.hpp"

namespace cdp {


struct PlayerProperty {
    std::string name;
    std::string value;

    static PlayerProperty fromJson(const JsonValue& json) {
        PlayerProperty p;
        p.name = json["name"].getString();
        p.value = json["value"].getString();
        return p;
    }
};


struct PlayerEvent {
    std::string timestamp;
    std::string value;

    static PlayerEvent fromJson(const JsonValue& json) {
        PlayerEvent e;
        e.timestamp = json["timestamp"].getString();
        e.value = json["value"].getString();
        return e;
    }
};


struct PlayerError {
    std::string errorType;
    int code;
    JsonValue stack;
    JsonValue cause;
    JsonValue data;

    static PlayerError fromJson(const JsonValue& json) {
        PlayerError e;
        e.errorType = json["errorType"].getString();
        e.code = json["code"].getInt();
        e.stack = json["stack"];
        e.cause = json["cause"];
        e.data = json["data"];
        return e;
    }
};


class Media : public Domain {
public:
    explicit Media(CDPConnection& connection) : Domain(connection, "Media") {}

    
    CDPResponse enable() { return call("enable"); }

    
    CDPResponse disable() { return call("disable"); }

    
    void onPlayerPropertiesChanged(std::function<void(const std::string& playerId,
                                                        const JsonValue& properties)> callback) {
        on("playerPropertiesChanged", [callback](const CDPEvent& event) {
            callback(
                event.params["playerId"].getString(),
                event.params["properties"]
            );
        });
    }

    void onPlayerEventsAdded(std::function<void(const std::string& playerId,
                                                  const JsonValue& events)> callback) {
        on("playerEventsAdded", [callback](const CDPEvent& event) {
            callback(
                event.params["playerId"].getString(),
                event.params["events"]
            );
        });
    }

    void onPlayerMessagesLogged(std::function<void(const std::string& playerId,
                                                     const JsonValue& messages)> callback) {
        on("playerMessagesLogged", [callback](const CDPEvent& event) {
            callback(
                event.params["playerId"].getString(),
                event.params["messages"]
            );
        });
    }

    void onPlayerErrorsRaised(std::function<void(const std::string& playerId,
                                                   const JsonValue& errors)> callback) {
        on("playerErrorsRaised", [callback](const CDPEvent& event) {
            callback(
                event.params["playerId"].getString(),
                event.params["errors"]
            );
        });
    }

    void onPlayersCreated(std::function<void(const JsonValue& players)> callback) {
        on("playersCreated", [callback](const CDPEvent& event) {
            callback(event.params["players"]);
        });
    }
};

} 
