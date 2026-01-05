#pragma once

#include "Domain.hpp"

namespace cdp {


struct ConsoleMessage {
    std::string source;  
    std::string level;   
    std::string text;
    std::string url;
    int line = 0;
    int column = 0;

    static ConsoleMessage fromJson(const JsonValue& json) {
        ConsoleMessage msg;
        msg.source = json["source"].getString();
        msg.level = json["level"].getString();
        msg.text = json["text"].getString();
        msg.url = json["url"].getString();
        msg.line = json["line"].getInt();
        msg.column = json["column"].getInt();
        return msg;
    }
};


class Console : public Domain {
public:
    explicit Console(CDPConnection& connection) : Domain(connection, "Console") {}

    
    CDPResponse enable() { return call("enable"); }

    
    CDPResponse disable() { return call("disable"); }

    
    CDPResponse clearMessages() { return call("clearMessages"); }

    
    void onMessageAdded(std::function<void(const ConsoleMessage& message)> callback) {
        on("messageAdded", [callback](const CDPEvent& event) {
            callback(ConsoleMessage::fromJson(event.params["message"]));
        });
    }
};

} 
