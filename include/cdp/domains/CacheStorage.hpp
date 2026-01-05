#pragma once

#include "Domain.hpp"

namespace cdp {


struct CacheEntry {
    std::string requestURL;
    std::string requestMethod;
    JsonValue requestHeaders;
    double responseTime;
    int responseStatus;
    std::string responseStatusText;
    std::string responseType;
    JsonValue responseHeaders;

    static CacheEntry fromJson(const JsonValue& json) {
        CacheEntry e;
        e.requestURL = json["requestURL"].getString();
        e.requestMethod = json["requestMethod"].getString();
        e.requestHeaders = json["requestHeaders"];
        e.responseTime = json["responseTime"].getNumber();
        e.responseStatus = json["responseStatus"].getInt();
        e.responseStatusText = json["responseStatusText"].getString();
        e.responseType = json["responseType"].getString();
        e.responseHeaders = json["responseHeaders"];
        return e;
    }
};


struct Cache {
    std::string cacheId;
    std::string securityOrigin;
    std::string storageKey;
    std::string storageBucket;
    std::string cacheName;

    static Cache fromJson(const JsonValue& json) {
        Cache c;
        c.cacheId = json["cacheId"].getString();
        c.securityOrigin = json["securityOrigin"].getString();
        c.storageKey = json["storageKey"].getString();
        c.cacheName = json["cacheName"].getString();
        return c;
    }
};


class CacheStorage : public Domain {
public:
    explicit CacheStorage(CDPConnection& connection) : Domain(connection, "CacheStorage") {}

    
    CDPResponse deleteCache(const std::string& cacheId) {
        return call("deleteCache", Params().set("cacheId", cacheId));
    }

    
    CDPResponse deleteEntry(const std::string& cacheId, const std::string& request) {
        return call("deleteEntry", Params()
            .set("cacheId", cacheId)
            .set("request", request));
    }

    
    CDPResponse requestCacheNames(const std::string& securityOrigin = "",
                                   const std::string& storageKey = "",
                                   const std::string& storageBucket = "") {
        Params params;
        if (!securityOrigin.empty()) params.set("securityOrigin", securityOrigin);
        if (!storageKey.empty()) params.set("storageKey", storageKey);
        if (!storageBucket.empty()) params.set("storageBucket", storageBucket);
        return call("requestCacheNames", params);
    }

    
    CDPResponse requestCachedResponse(const std::string& cacheId,
                                        const std::string& requestURL,
                                        const JsonValue& requestHeaders) {
        return call("requestCachedResponse", Params()
            .set("cacheId", cacheId)
            .set("requestURL", requestURL)
            .set("requestHeaders", requestHeaders));
    }

    
    CDPResponse requestEntries(const std::string& cacheId,
                                int skipCount = 0,
                                int pageSize = 0,
                                const std::string& pathFilter = "") {
        Params params;
        params.set("cacheId", cacheId);
        if (skipCount > 0) params.set("skipCount", skipCount);
        if (pageSize > 0) params.set("pageSize", pageSize);
        if (!pathFilter.empty()) params.set("pathFilter", pathFilter);
        return call("requestEntries", params);
    }
};

} 
