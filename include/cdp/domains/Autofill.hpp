#pragma once

#include "Domain.hpp"

namespace cdp {


struct CreditCard {
    std::string number;
    std::string name;
    std::string expiryMonth;
    std::string expiryYear;
    std::string cvc;

    JsonValue toJson() const {
        JsonObject obj;
        obj["number"] = number;
        obj["name"] = name;
        obj["expiryMonth"] = expiryMonth;
        obj["expiryYear"] = expiryYear;
        obj["cvc"] = cvc;
        return obj;
    }
};


struct AddressField {
    std::string name;
    std::string value;

    JsonValue toJson() const {
        JsonObject obj;
        obj["name"] = name;
        obj["value"] = value;
        return obj;
    }
};


struct Address {
    std::vector<AddressField> fields;

    JsonValue toJson() const {
        JsonArray arr;
        for (const auto& f : fields) arr.push_back(f.toJson());
        JsonObject obj;
        obj["fields"] = arr;
        return obj;
    }
};


struct FilledField {
    std::string htmlType;
    std::string id;
    std::string name;
    std::string value;
    std::string autofillType;
    std::string fillingStrategy;  
    std::string frameId;
    int fieldId;

    static FilledField fromJson(const JsonValue& json) {
        FilledField f;
        f.htmlType = json["htmlType"].getString();
        f.id = json["id"].getString();
        f.name = json["name"].getString();
        f.value = json["value"].getString();
        f.autofillType = json["autofillType"].getString();
        f.fillingStrategy = json["fillingStrategy"].getString();
        f.frameId = json["frameId"].getString();
        f.fieldId = json["fieldId"].getInt();
        return f;
    }
};


class Autofill : public Domain {
public:
    explicit Autofill(CDPConnection& connection) : Domain(connection, "Autofill") {}

    
    CDPResponse enable() { return call("enable"); }

    
    CDPResponse disable() { return call("disable"); }

    
    CDPResponse trigger(int fieldId, const std::string& frameId, const CreditCard& card) {
        return call("trigger", Params()
            .set("fieldId", fieldId)
            .set("frameId", frameId)
            .set("card", card.toJson()));
    }

    
    CDPResponse setAddresses(const std::vector<Address>& addresses) {
        JsonArray arr;
        for (const auto& a : addresses) arr.push_back(a.toJson());
        return call("setAddresses", Params().set("addresses", arr));
    }

    
    void onAddressFormFilled(std::function<void(const JsonValue& filledFields,
                                                  const JsonValue& addressUi)> callback) {
        on("addressFormFilled", [callback](const CDPEvent& event) {
            callback(event.params["filledFields"], event.params["addressUi"]);
        });
    }
};

} 
