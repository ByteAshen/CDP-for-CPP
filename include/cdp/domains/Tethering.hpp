#pragma once

#include "Domain.hpp"

namespace cdp {


class Tethering : public Domain {
public:
    explicit Tethering(CDPConnection& connection) : Domain(connection, "Tethering") {}

    
    CDPResponse bind(int port) {
        return call("bind", Params().set("port", port));
    }

    
    CDPResponse unbind(int port) {
        return call("unbind", Params().set("port", port));
    }

    
    void onAccepted(std::function<void(int port, const std::string& connectionId)> callback) {
        on("accepted", [callback](const CDPEvent& event) {
            callback(
                event.params["port"].getInt(),
                event.params["connectionId"].getString()
            );
        });
    }
};

} 
