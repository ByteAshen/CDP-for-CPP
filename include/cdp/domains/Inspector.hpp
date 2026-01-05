#pragma once

#include "Domain.hpp"

namespace cdp {


class Inspector : public Domain {
public:
    explicit Inspector(CDPConnection& connection) : Domain(connection, "Inspector") {}

    
    CDPResponse enable() { return call("enable"); }

    
    CDPResponse disable() { return call("disable"); }

    
    void onDetached(std::function<void(const std::string& reason)> callback) {
        on("detached", [callback](const CDPEvent& event) {
            callback(event.params["reason"].getString());
        });
    }

    void onTargetCrashed(std::function<void()> callback) {
        on("targetCrashed", [callback](const CDPEvent&) {
            callback();
        });
    }

    void onTargetReloadedAfterCrash(std::function<void()> callback) {
        on("targetReloadedAfterCrash", [callback](const CDPEvent&) {
            callback();
        });
    }
};

} 
