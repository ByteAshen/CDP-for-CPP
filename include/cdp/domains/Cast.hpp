#pragma once

#include "Domain.hpp"

namespace cdp {


struct Sink {
    std::string name;
    std::string id;
    std::string session;

    static Sink fromJson(const JsonValue& json) {
        Sink s;
        s.name = json["name"].getString();
        s.id = json["id"].getString();
        s.session = json["session"].getString();
        return s;
    }
};


class Cast : public Domain {
public:
    explicit Cast(CDPConnection& connection) : Domain(connection, "Cast") {}

    
    CDPResponse enable(const std::string& presentationUrl = "") {
        Params params;
        if (!presentationUrl.empty()) params.set("presentationUrl", presentationUrl);
        return call("enable", params);
    }

    
    CDPResponse disable() { return call("disable"); }

    
    CDPResponse setSinkToUse(const std::string& sinkName) {
        return call("setSinkToUse", Params().set("sinkName", sinkName));
    }

    
    CDPResponse startDesktopMirroring(const std::string& sinkName) {
        return call("startDesktopMirroring", Params().set("sinkName", sinkName));
    }

    
    CDPResponse startTabMirroring(const std::string& sinkName) {
        return call("startTabMirroring", Params().set("sinkName", sinkName));
    }

    
    CDPResponse stopCasting(const std::string& sinkName) {
        return call("stopCasting", Params().set("sinkName", sinkName));
    }

    
    void onSinksUpdated(std::function<void(const JsonValue& sinks)> callback) {
        on("sinksUpdated", [callback](const CDPEvent& event) {
            callback(event.params["sinks"]);
        });
    }

    void onIssueUpdated(std::function<void(const std::string& issueMessage)> callback) {
        on("issueUpdated", [callback](const CDPEvent& event) {
            callback(event.params["issueMessage"].getString());
        });
    }
};

} 
