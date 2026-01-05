#pragma once

#include "Domain.hpp"

namespace cdp {


struct FedCmAccount {
    std::string accountId;
    std::string email;
    std::string name;
    std::string givenName;
    std::string pictureUrl;
    std::string idpConfigUrl;
    std::string idpLoginUrl;
    std::string loginState;  
    std::string termsOfServiceUrl;
    std::string privacyPolicyUrl;

    static FedCmAccount fromJson(const JsonValue& json) {
        FedCmAccount a;
        a.accountId = json["accountId"].getString();
        a.email = json["email"].getString();
        a.name = json["name"].getString();
        a.givenName = json["givenName"].getString();
        a.pictureUrl = json["pictureUrl"].getString();
        a.idpConfigUrl = json["idpConfigUrl"].getString();
        a.idpLoginUrl = json["idpLoginUrl"].getString();
        a.loginState = json["loginState"].getString();
        a.termsOfServiceUrl = json["termsOfServiceUrl"].getString();
        a.privacyPolicyUrl = json["privacyPolicyUrl"].getString();
        return a;
    }
};


class FedCm : public Domain {
public:
    explicit FedCm(CDPConnection& connection) : Domain(connection, "FedCm") {}

    
    CDPResponse enable(bool disableRejectionDelay = false) {
        Params params;
        if (disableRejectionDelay) params.set("disableRejectionDelay", true);
        return call("enable", params);
    }

    
    CDPResponse disable() { return call("disable"); }

    
    CDPResponse selectAccount(const std::string& dialogId, int accountIndex) {
        return call("selectAccount", Params()
            .set("dialogId", dialogId)
            .set("accountIndex", accountIndex));
    }

    
    CDPResponse clickDialogButton(const std::string& dialogId,
                                   const std::string& dialogButton) {  
        return call("clickDialogButton", Params()
            .set("dialogId", dialogId)
            .set("dialogButton", dialogButton));
    }

    
    CDPResponse openUrl(const std::string& dialogId,
                         int accountIndex,
                         const std::string& accountUrlType) {  
        return call("openUrl", Params()
            .set("dialogId", dialogId)
            .set("accountIndex", accountIndex)
            .set("accountUrlType", accountUrlType));
    }

    
    CDPResponse dismissDialog(const std::string& dialogId, bool triggerCooldown = false) {
        Params params;
        params.set("dialogId", dialogId);
        if (triggerCooldown) params.set("triggerCooldown", true);
        return call("dismissDialog", params);
    }

    
    CDPResponse resetCooldown() { return call("resetCooldown"); }

    
    void onDialogShown(std::function<void(const std::string& dialogId,
                                            const std::string& dialogType,
                                            const JsonValue& accounts,
                                            const std::string& title,
                                            const std::string& subtitle)> callback) {
        on("dialogShown", [callback](const CDPEvent& event) {
            callback(
                event.params["dialogId"].getString(),
                event.params["dialogType"].getString(),
                event.params["accounts"],
                event.params["title"].getString(),
                event.params["subtitle"].getString()
            );
        });
    }

    void onDialogClosed(std::function<void(const std::string& dialogId)> callback) {
        on("dialogClosed", [callback](const CDPEvent& event) {
            callback(event.params["dialogId"].getString());
        });
    }
};

} 
