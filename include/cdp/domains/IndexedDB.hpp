#pragma once

#include "Domain.hpp"

namespace cdp {


struct KeyRange {
    JsonValue lower;
    JsonValue upper;
    bool lowerOpen = false;
    bool upperOpen = false;

    JsonValue toJson() const {
        JsonObject obj;
        if (!lower.isNull()) obj["lower"] = lower;
        if (!upper.isNull()) obj["upper"] = upper;
        if (lowerOpen) obj["lowerOpen"] = true;
        if (upperOpen) obj["upperOpen"] = true;
        return obj;
    }
};


class IndexedDB : public Domain {
public:
    explicit IndexedDB(CDPConnection& connection) : Domain(connection, "IndexedDB") {}

    
    CDPResponse enable() { return call("enable"); }

    
    CDPResponse disable() { return call("disable"); }

    
    CDPResponse clearObjectStore(const std::string& databaseName,
                                   const std::string& objectStoreName,
                                   const std::string& securityOrigin = "",
                                   const std::string& storageKey = "",
                                   const std::string& storageBucket = "") {
        Params params;
        params.set("databaseName", databaseName);
        params.set("objectStoreName", objectStoreName);
        if (!securityOrigin.empty()) params.set("securityOrigin", securityOrigin);
        if (!storageKey.empty()) params.set("storageKey", storageKey);
        if (!storageBucket.empty()) params.set("storageBucket", storageBucket);
        return call("clearObjectStore", params);
    }

    
    CDPResponse deleteDatabase(const std::string& databaseName,
                                const std::string& securityOrigin = "",
                                const std::string& storageKey = "",
                                const std::string& storageBucket = "") {
        Params params;
        params.set("databaseName", databaseName);
        if (!securityOrigin.empty()) params.set("securityOrigin", securityOrigin);
        if (!storageKey.empty()) params.set("storageKey", storageKey);
        if (!storageBucket.empty()) params.set("storageBucket", storageBucket);
        return call("deleteDatabase", params);
    }

    
    CDPResponse deleteObjectStoreEntries(const std::string& databaseName,
                                          const std::string& objectStoreName,
                                          const KeyRange& keyRange,
                                          const std::string& securityOrigin = "",
                                          const std::string& storageKey = "",
                                          const std::string& storageBucket = "") {
        Params params;
        params.set("databaseName", databaseName);
        params.set("objectStoreName", objectStoreName);
        params.set("keyRange", keyRange.toJson());
        if (!securityOrigin.empty()) params.set("securityOrigin", securityOrigin);
        if (!storageKey.empty()) params.set("storageKey", storageKey);
        if (!storageBucket.empty()) params.set("storageBucket", storageBucket);
        return call("deleteObjectStoreEntries", params);
    }

    
    CDPResponse requestData(const std::string& databaseName,
                             const std::string& objectStoreName,
                             const std::string& indexName,
                             int skipCount,
                             int pageSize,
                             const std::string& securityOrigin = "",
                             const std::string& storageKey = "",
                             const std::string& storageBucket = "",
                             const KeyRange* keyRange = nullptr) {
        Params params;
        params.set("databaseName", databaseName);
        params.set("objectStoreName", objectStoreName);
        params.set("indexName", indexName);
        params.set("skipCount", skipCount);
        params.set("pageSize", pageSize);
        if (!securityOrigin.empty()) params.set("securityOrigin", securityOrigin);
        if (!storageKey.empty()) params.set("storageKey", storageKey);
        if (!storageBucket.empty()) params.set("storageBucket", storageBucket);
        if (keyRange) params.set("keyRange", keyRange->toJson());
        return call("requestData", params);
    }

    
    CDPResponse getMetadata(const std::string& databaseName,
                             const std::string& objectStoreName,
                             const std::string& securityOrigin = "",
                             const std::string& storageKey = "",
                             const std::string& storageBucket = "") {
        Params params;
        params.set("databaseName", databaseName);
        params.set("objectStoreName", objectStoreName);
        if (!securityOrigin.empty()) params.set("securityOrigin", securityOrigin);
        if (!storageKey.empty()) params.set("storageKey", storageKey);
        if (!storageBucket.empty()) params.set("storageBucket", storageBucket);
        return call("getMetadata", params);
    }

    
    CDPResponse requestDatabase(const std::string& databaseName,
                                 const std::string& securityOrigin = "",
                                 const std::string& storageKey = "",
                                 const std::string& storageBucket = "") {
        Params params;
        params.set("databaseName", databaseName);
        if (!securityOrigin.empty()) params.set("securityOrigin", securityOrigin);
        if (!storageKey.empty()) params.set("storageKey", storageKey);
        if (!storageBucket.empty()) params.set("storageBucket", storageBucket);
        return call("requestDatabase", params);
    }

    
    CDPResponse requestDatabaseNames(const std::string& securityOrigin = "",
                                      const std::string& storageKey = "",
                                      const std::string& storageBucket = "") {
        Params params;
        if (!securityOrigin.empty()) params.set("securityOrigin", securityOrigin);
        if (!storageKey.empty()) params.set("storageKey", storageKey);
        if (!storageBucket.empty()) params.set("storageBucket", storageBucket);
        return call("requestDatabaseNames", params);
    }
};

} 
