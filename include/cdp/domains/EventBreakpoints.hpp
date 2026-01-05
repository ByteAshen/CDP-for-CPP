#pragma once

#include "Domain.hpp"

namespace cdp {


class EventBreakpoints : public Domain {
public:
    explicit EventBreakpoints(CDPConnection& connection) : Domain(connection, "EventBreakpoints") {}

    
    CDPResponse setInstrumentationBreakpoint(const std::string& eventName) {
        return call("setInstrumentationBreakpoint", Params().set("eventName", eventName));
    }

    
    CDPResponse removeInstrumentationBreakpoint(const std::string& eventName) {
        return call("removeInstrumentationBreakpoint", Params().set("eventName", eventName));
    }

    
    CDPResponse disable() { return call("disable"); }
};

} 
