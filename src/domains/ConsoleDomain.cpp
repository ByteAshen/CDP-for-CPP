

#include "cdp/domains/ConsoleDomain.hpp"

namespace cdp {

ConsoleMessage ConsoleMessage::fromJson(const JsonValue& json) {
    ConsoleMessage msg;
    msg.source = json["source"].getString();
    msg.level = json["level"].getString();
    msg.text = json["text"].getString();
    msg.url = json["url"].getString();
    msg.line = json["line"].getInt();
    msg.column = json["column"].getInt();
    return msg;
}

CDPResponse ConsoleDomain::clearMessages() {
    return sendCommand("Console.clearMessages");
}

void ConsoleDomain::onMessageAdded(std::function<void(const ConsoleMessage&)> callback) {
    subscribeEvent("messageAdded", [callback](const CDPEvent& event) {
        callback(ConsoleMessage::fromJson(event.params["message"]));
    });
}

} 
