#pragma once

#include "Domain.hpp"

namespace cdp {


struct StorageId {
    std::string securityOrigin;
    std::string storageKey;
    bool isLocalStorage;

    JsonValue toJson() const {
        JsonObject obj;
        if (!securityOrigin.empty()) obj["securityOrigin"] = securityOrigin;
        if (!storageKey.empty()) obj["storageKey"] = storageKey;
        obj["isLocalStorage"] = isLocalStorage;
        return obj;
    }

    static StorageId fromJson(const JsonValue& json) {
        StorageId s;
        s.securityOrigin = json["securityOrigin"].getString();
        s.storageKey = json["storageKey"].getString();
        s.isLocalStorage = json["isLocalStorage"].getBool();
        return s;
    }
};


class DOMStorage : public Domain {
public:
    explicit DOMStorage(CDPConnection& connection) : Domain(connection, "DOMStorage") {}

    
    CDPResponse enable() { return call("enable"); }

    
    CDPResponse disable() { return call("disable"); }

    
    CDPResponse clear(const StorageId& storageId) {
        return call("clear", Params().set("storageId", storageId.toJson()));
    }

    
    CDPResponse getDOMStorageItems(const StorageId& storageId) {
        return call("getDOMStorageItems", Params().set("storageId", storageId.toJson()));
    }

    
    CDPResponse removeDOMStorageItem(const StorageId& storageId, const std::string& key) {
        return call("removeDOMStorageItem", Params()
            .set("storageId", storageId.toJson())
            .set("key", key));
    }

    
    CDPResponse setDOMStorageItem(const StorageId& storageId,
                                    const std::string& key,
                                    const std::string& value) {
        return call("setDOMStorageItem", Params()
            .set("storageId", storageId.toJson())
            .set("key", key)
            .set("value", value));
    }

    
    void onDomStorageItemAdded(std::function<void(const JsonValue& storageId,
                                                    const std::string& key,
                                                    const std::string& newValue)> callback) {
        on("domStorageItemAdded", [callback](const CDPEvent& event) {
            callback(
                event.params["storageId"],
                event.params["key"].getString(),
                event.params["newValue"].getString()
            );
        });
    }

    void onDomStorageItemRemoved(std::function<void(const JsonValue& storageId,
                                                      const std::string& key)> callback) {
        on("domStorageItemRemoved", [callback](const CDPEvent& event) {
            callback(
                event.params["storageId"],
                event.params["key"].getString()
            );
        });
    }

    void onDomStorageItemUpdated(std::function<void(const JsonValue& storageId,
                                                      const std::string& key,
                                                      const std::string& oldValue,
                                                      const std::string& newValue)> callback) {
        on("domStorageItemUpdated", [callback](const CDPEvent& event) {
            callback(
                event.params["storageId"],
                event.params["key"].getString(),
                event.params["oldValue"].getString(),
                event.params["newValue"].getString()
            );
        });
    }

    void onDomStorageItemsCleared(std::function<void(const JsonValue& storageId)> callback) {
        on("domStorageItemsCleared", [callback](const CDPEvent& event) {
            callback(event.params["storageId"]);
        });
    }
};

} 
