#pragma once

#include "Domain.hpp"

namespace cdp {


class DOMDebugger : public Domain {
public:
    explicit DOMDebugger(CDPConnection& connection) : Domain(connection, "DOMDebugger") {}

    
    CDPResponse getEventListeners(const std::string& objectId,
                                   int depth = -1,
                                   bool pierce = false) {
        Params params;
        params.set("objectId", objectId);
        if (depth >= 0) params.set("depth", depth);
        if (pierce) params.set("pierce", true);
        return call("getEventListeners", params);
    }

    
    CDPResponse removeDOMBreakpoint(int nodeId, const std::string& type) {
        return call("removeDOMBreakpoint", Params()
            .set("nodeId", nodeId)
            .set("type", type));  
    }

    
    CDPResponse removeEventListenerBreakpoint(const std::string& eventName,
                                               const std::string& targetName = "") {
        Params params;
        params.set("eventName", eventName);
        if (!targetName.empty()) params.set("targetName", targetName);
        return call("removeEventListenerBreakpoint", params);
    }

    
    CDPResponse removeInstrumentationBreakpoint(const std::string& eventName) {
        return call("removeInstrumentationBreakpoint", Params().set("eventName", eventName));
    }

    
    CDPResponse removeXHRBreakpoint(const std::string& url) {
        return call("removeXHRBreakpoint", Params().set("url", url));
    }

    
    CDPResponse setBreakOnCSPViolation(const std::vector<std::string>& violationTypes) {
        JsonArray arr;
        for (const auto& v : violationTypes) arr.push_back(v);
        return call("setBreakOnCSPViolation", Params().set("violationTypes", arr));
    }

    
    CDPResponse setDOMBreakpoint(int nodeId, const std::string& type) {
        return call("setDOMBreakpoint", Params()
            .set("nodeId", nodeId)
            .set("type", type));  
    }

    
    CDPResponse setEventListenerBreakpoint(const std::string& eventName,
                                            const std::string& targetName = "") {
        Params params;
        params.set("eventName", eventName);
        if (!targetName.empty()) params.set("targetName", targetName);
        return call("setEventListenerBreakpoint", params);
    }

    
    CDPResponse setInstrumentationBreakpoint(const std::string& eventName) {
        return call("setInstrumentationBreakpoint", Params().set("eventName", eventName));
    }

    
    CDPResponse setXHRBreakpoint(const std::string& url) {
        return call("setXHRBreakpoint", Params().set("url", url));
    }
};

} 
