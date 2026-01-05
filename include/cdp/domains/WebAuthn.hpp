#pragma once

#include "Domain.hpp"

namespace cdp {


struct VirtualAuthenticatorOptions {
    std::string protocol;  
    std::string ctap2Version;  
    std::string transport;  
    bool hasResidentKey = false;
    bool hasUserVerification = false;
    bool hasLargeBlob = false;
    bool hasCredBlob = false;
    bool hasMinPinLength = false;
    bool hasPrf = false;
    bool automaticPresenceSimulation = false;
    bool isUserVerified = false;
    std::string defaultBackupEligibility;  
    std::string defaultBackupState;  

    JsonValue toJson() const {
        JsonObject obj;
        obj["protocol"] = protocol;
        if (!ctap2Version.empty()) obj["ctap2Version"] = ctap2Version;
        obj["transport"] = transport;
        if (hasResidentKey) obj["hasResidentKey"] = true;
        if (hasUserVerification) obj["hasUserVerification"] = true;
        if (hasLargeBlob) obj["hasLargeBlob"] = true;
        if (hasCredBlob) obj["hasCredBlob"] = true;
        if (hasMinPinLength) obj["hasMinPinLength"] = true;
        if (hasPrf) obj["hasPrf"] = true;
        if (automaticPresenceSimulation) obj["automaticPresenceSimulation"] = true;
        if (isUserVerified) obj["isUserVerified"] = true;
        return obj;
    }
};


struct Credential {
    std::string credentialId;  
    bool isResidentCredential;
    std::string rpId;
    std::string privateKey;  
    std::string userHandle;  
    int signCount;
    std::string largeBlob;  

    JsonValue toJson() const {
        JsonObject obj;
        obj["credentialId"] = credentialId;
        obj["isResidentCredential"] = isResidentCredential;
        if (!rpId.empty()) obj["rpId"] = rpId;
        obj["privateKey"] = privateKey;
        if (!userHandle.empty()) obj["userHandle"] = userHandle;
        obj["signCount"] = signCount;
        if (!largeBlob.empty()) obj["largeBlob"] = largeBlob;
        return obj;
    }

    static Credential fromJson(const JsonValue& json) {
        Credential c;
        c.credentialId = json["credentialId"].getString();
        c.isResidentCredential = json["isResidentCredential"].getBool();
        c.rpId = json["rpId"].getString();
        c.privateKey = json["privateKey"].getString();
        c.userHandle = json["userHandle"].getString();
        c.signCount = json["signCount"].getInt();
        c.largeBlob = json["largeBlob"].getString();
        return c;
    }
};


class WebAuthn : public Domain {
public:
    explicit WebAuthn(CDPConnection& connection) : Domain(connection, "WebAuthn") {}

    
    CDPResponse enable(bool enableUI = false) {
        Params params;
        if (enableUI) params.set("enableUI", true);
        return call("enable", params);
    }

    
    CDPResponse disable() { return call("disable"); }

    
    CDPResponse addVirtualAuthenticator(const VirtualAuthenticatorOptions& options) {
        return call("addVirtualAuthenticator", Params().set("options", options.toJson()));
    }

    
    CDPResponse setResponseOverrideBits(const std::string& authenticatorId,
                                          bool isBogusSignature = false,
                                          bool isBadUV = false,
                                          bool isBadUP = false) {
        Params params;
        params.set("authenticatorId", authenticatorId);
        if (isBogusSignature) params.set("isBogusSignature", true);
        if (isBadUV) params.set("isBadUV", true);
        if (isBadUP) params.set("isBadUP", true);
        return call("setResponseOverrideBits", params);
    }

    
    CDPResponse removeVirtualAuthenticator(const std::string& authenticatorId) {
        return call("removeVirtualAuthenticator", Params().set("authenticatorId", authenticatorId));
    }

    
    CDPResponse addCredential(const std::string& authenticatorId, const Credential& credential) {
        return call("addCredential", Params()
            .set("authenticatorId", authenticatorId)
            .set("credential", credential.toJson()));
    }

    
    CDPResponse getCredential(const std::string& authenticatorId, const std::string& credentialId) {
        return call("getCredential", Params()
            .set("authenticatorId", authenticatorId)
            .set("credentialId", credentialId));
    }

    
    CDPResponse getCredentials(const std::string& authenticatorId) {
        return call("getCredentials", Params().set("authenticatorId", authenticatorId));
    }

    
    CDPResponse removeCredential(const std::string& authenticatorId, const std::string& credentialId) {
        return call("removeCredential", Params()
            .set("authenticatorId", authenticatorId)
            .set("credentialId", credentialId));
    }

    
    CDPResponse clearCredentials(const std::string& authenticatorId) {
        return call("clearCredentials", Params().set("authenticatorId", authenticatorId));
    }

    
    CDPResponse setUserVerified(const std::string& authenticatorId, bool isUserVerified) {
        return call("setUserVerified", Params()
            .set("authenticatorId", authenticatorId)
            .set("isUserVerified", isUserVerified));
    }

    
    CDPResponse setAutomaticPresenceSimulation(const std::string& authenticatorId, bool enabled) {
        return call("setAutomaticPresenceSimulation", Params()
            .set("authenticatorId", authenticatorId)
            .set("enabled", enabled));
    }

    
    void onCredentialAdded(std::function<void(const std::string& authenticatorId,
                                                const JsonValue& credential)> callback) {
        on("credentialAdded", [callback](const CDPEvent& event) {
            callback(
                event.params["authenticatorId"].getString(),
                event.params["credential"]
            );
        });
    }

    void onCredentialAsserted(std::function<void(const std::string& authenticatorId,
                                                   const JsonValue& credential)> callback) {
        on("credentialAsserted", [callback](const CDPEvent& event) {
            callback(
                event.params["authenticatorId"].getString(),
                event.params["credential"]
            );
        });
    }
};

} 
