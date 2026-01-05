#pragma once

#include "Domain.hpp"

namespace cdp {


struct AffectedCookie {
    std::string name;
    std::string path;
    std::string domain;

    JsonValue toJson() const {
        JsonObject obj;
        obj["name"] = name;
        obj["path"] = path;
        obj["domain"] = domain;
        return obj;
    }
};


struct AffectedRequest {
    std::string requestId;
    std::string url;

    JsonValue toJson() const {
        JsonObject obj;
        obj["requestId"] = requestId;
        if (!url.empty()) obj["url"] = url;
        return obj;
    }
};


class Audits : public Domain {
public:
    explicit Audits(CDPConnection& connection) : Domain(connection, "Audits") {}

    
    CDPResponse enable() { return call("enable"); }

    
    CDPResponse disable() { return call("disable"); }

    
    CDPResponse getEncodedResponse(const std::string& requestId,
                                     const std::string& encoding,  
                                     double quality = -1,
                                     bool sizeOnly = false) {
        Params params;
        params.set("requestId", requestId);
        params.set("encoding", encoding);
        if (quality >= 0) params.set("quality", quality);
        if (sizeOnly) params.set("sizeOnly", true);
        return call("getEncodedResponse", params);
    }

    
    CDPResponse checkContrast(bool reportAAA = false) {
        Params params;
        if (reportAAA) params.set("reportAAA", true);
        return call("checkContrast", params);
    }

    
    CDPResponse checkFormsIssues() { return call("checkFormsIssues"); }

    
    void onIssueAdded(std::function<void(const JsonValue& issue)> callback) {
        on("issueAdded", [callback](const CDPEvent& event) {
            callback(event.params["issue"]);
        });
    }
};

} 
