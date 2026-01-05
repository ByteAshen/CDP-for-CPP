#pragma once

#include "Domain.hpp"

namespace cdp {


class Security : public Domain {
public:
    explicit Security(CDPConnection& connection) : Domain(connection, "Security") {}

    
    CDPResponse enable() { return call("enable"); }

    
    CDPResponse disable() { return call("disable"); }

    
    CDPResponse handleCertificateError(int eventId, const std::string& action) {
        return call("handleCertificateError", Params()
            .set("eventId", eventId)
            .set("action", action));  
    }

    
    CDPResponse setOverrideCertificateErrors(bool override) {
        return call("setOverrideCertificateErrors", Params().set("override", override));
    }

    
    CDPResponse setIgnoreCertificateErrors(bool ignore) {
        return call("setIgnoreCertificateErrors", Params().set("ignore", ignore));
    }

    
    void onSecurityStateChanged(std::function<void(const std::string& securityState,
                                                     bool schemeIsCryptographic,
                                                     const JsonValue& explanations,
                                                     const JsonValue& insecureContentStatus,
                                                     const std::string& summary)> callback) {
        on("securityStateChanged", [callback](const CDPEvent& event) {
            callback(
                event.params["securityState"].getString(),
                event.params["schemeIsCryptographic"].getBool(),
                event.params["explanations"],
                event.params["insecureContentStatus"],
                event.params["summary"].getString()
            );
        });
    }

    void onVisibleSecurityStateChanged(std::function<void(const JsonValue& visibleSecurityState)> callback) {
        on("visibleSecurityStateChanged", [callback](const CDPEvent& event) {
            callback(event.params["visibleSecurityState"]);
        });
    }

    void onCertificateError(std::function<void(int eventId,
                                                  const std::string& errorType,
                                                  const std::string& requestURL)> callback) {
        on("certificateError", [callback](const CDPEvent& event) {
            callback(
                event.params["eventId"].getInt(),
                event.params["errorType"].getString(),
                event.params["requestURL"].getString()
            );
        });
    }
};

} 
