#pragma once

#include "Domain.hpp"

namespace cdp {


class Extensions : public Domain {
public:
    explicit Extensions(CDPConnection& connection) : Domain(connection, "Extensions") {}

    
    CDPResponse loadUnpacked(const std::string& path) {
        return call("loadUnpacked", Params().set("path", path));
    }

    
    CDPResponse getStorageItems(const std::string& id,
                                  const std::string& storageArea,  
                                  const std::vector<std::string>& keys = {}) {
        Params params;
        params.set("id", id);
        params.set("storageArea", storageArea);
        if (!keys.empty()) {
            JsonArray arr;
            for (const auto& k : keys) arr.push_back(k);
            params.set("keys", arr);
        }
        return call("getStorageItems", params);
    }

    
    CDPResponse setStorageItems(const std::string& id,
                                  const std::string& storageArea,
                                  const JsonValue& values) {
        return call("setStorageItems", Params()
            .set("id", id)
            .set("storageArea", storageArea)
            .set("values", values));
    }

    
    CDPResponse removeStorageItems(const std::string& id,
                                     const std::string& storageArea,
                                     const std::vector<std::string>& keys) {
        JsonArray arr;
        for (const auto& k : keys) arr.push_back(k);
        return call("removeStorageItems", Params()
            .set("id", id)
            .set("storageArea", storageArea)
            .set("keys", arr));
    }

    
    CDPResponse clearStorageItems(const std::string& id, const std::string& storageArea) {
        return call("clearStorageItems", Params()
            .set("id", id)
            .set("storageArea", storageArea));
    }
};

} 
