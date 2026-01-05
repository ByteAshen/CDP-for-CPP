#pragma once

#include "Domain.hpp"

namespace cdp {


struct StorageBucket {
    std::string storageKey;
    std::string name;

    JsonValue toJson() const {
        JsonObject obj;
        obj["storageKey"] = storageKey;
        if (!name.empty()) obj["name"] = name;
        return obj;
    }
};


class Storage : public Domain {
public:
    explicit Storage(CDPConnection& connection) : Domain(connection, "Storage") {}

    
    CDPResponse getStorageKeyForFrame(const std::string& frameId) {
        return call("getStorageKeyForFrame", Params().set("frameId", frameId));
    }

    
    CDPResponse clearDataForOrigin(const std::string& origin, const std::string& storageTypes) {
        return call("clearDataForOrigin", Params()
            .set("origin", origin)
            .set("storageTypes", storageTypes));
    }

    
    CDPResponse clearDataForStorageKey(const std::string& storageKey, const std::string& storageTypes) {
        return call("clearDataForStorageKey", Params()
            .set("storageKey", storageKey)
            .set("storageTypes", storageTypes));
    }

    
    CDPResponse getCookies(const std::string& browserContextId = "") {
        Params params;
        if (!browserContextId.empty()) params.set("browserContextId", browserContextId);
        return call("getCookies", params);
    }

    
    CDPResponse setCookies(const JsonArray& cookies, const std::string& browserContextId = "") {
        Params params;
        params.set("cookies", cookies);
        if (!browserContextId.empty()) params.set("browserContextId", browserContextId);
        return call("setCookies", params);
    }

    
    CDPResponse clearCookies(const std::string& browserContextId = "") {
        Params params;
        if (!browserContextId.empty()) params.set("browserContextId", browserContextId);
        return call("clearCookies", params);
    }

    
    CDPResponse getUsageAndQuota(const std::string& origin) {
        return call("getUsageAndQuota", Params().set("origin", origin));
    }

    
    CDPResponse overrideQuotaForOrigin(const std::string& origin, double quotaSize = -1) {
        Params params;
        params.set("origin", origin);
        if (quotaSize >= 0) params.set("quotaSize", quotaSize);
        return call("overrideQuotaForOrigin", params);
    }

    
    CDPResponse trackCacheStorageForOrigin(const std::string& origin) {
        return call("trackCacheStorageForOrigin", Params().set("origin", origin));
    }

    
    CDPResponse trackCacheStorageForStorageKey(const std::string& storageKey) {
        return call("trackCacheStorageForStorageKey", Params().set("storageKey", storageKey));
    }

    
    CDPResponse trackIndexedDBForOrigin(const std::string& origin) {
        return call("trackIndexedDBForOrigin", Params().set("origin", origin));
    }

    
    CDPResponse trackIndexedDBForStorageKey(const std::string& storageKey) {
        return call("trackIndexedDBForStorageKey", Params().set("storageKey", storageKey));
    }

    
    CDPResponse untrackCacheStorageForOrigin(const std::string& origin) {
        return call("untrackCacheStorageForOrigin", Params().set("origin", origin));
    }

    
    CDPResponse untrackCacheStorageForStorageKey(const std::string& storageKey) {
        return call("untrackCacheStorageForStorageKey", Params().set("storageKey", storageKey));
    }

    
    CDPResponse untrackIndexedDBForOrigin(const std::string& origin) {
        return call("untrackIndexedDBForOrigin", Params().set("origin", origin));
    }

    
    CDPResponse untrackIndexedDBForStorageKey(const std::string& storageKey) {
        return call("untrackIndexedDBForStorageKey", Params().set("storageKey", storageKey));
    }

    
    CDPResponse getTrustTokens() { return call("getTrustTokens"); }

    
    CDPResponse clearTrustTokens(const std::string& issuerOrigin) {
        return call("clearTrustTokens", Params().set("issuerOrigin", issuerOrigin));
    }

    
    CDPResponse getInterestGroupDetails(const std::string& ownerOrigin, const std::string& name) {
        return call("getInterestGroupDetails", Params()
            .set("ownerOrigin", ownerOrigin)
            .set("name", name));
    }

    
    CDPResponse setInterestGroupTracking(bool enable) {
        return call("setInterestGroupTracking", Params().set("enable", enable));
    }

    
    CDPResponse setInterestGroupAuctionTracking(bool enable) {
        return call("setInterestGroupAuctionTracking", Params().set("enable", enable));
    }

    
    CDPResponse getSharedStorageMetadata(const std::string& ownerOrigin) {
        return call("getSharedStorageMetadata", Params().set("ownerOrigin", ownerOrigin));
    }

    
    CDPResponse getSharedStorageEntries(const std::string& ownerOrigin) {
        return call("getSharedStorageEntries", Params().set("ownerOrigin", ownerOrigin));
    }

    
    CDPResponse setSharedStorageEntry(const std::string& ownerOrigin,
                                        const std::string& key,
                                        const std::string& value,
                                        bool ignoreIfPresent = false) {
        Params params;
        params.set("ownerOrigin", ownerOrigin);
        params.set("key", key);
        params.set("value", value);
        if (ignoreIfPresent) params.set("ignoreIfPresent", true);
        return call("setSharedStorageEntry", params);
    }

    
    CDPResponse deleteSharedStorageEntry(const std::string& ownerOrigin, const std::string& key) {
        return call("deleteSharedStorageEntry", Params()
            .set("ownerOrigin", ownerOrigin)
            .set("key", key));
    }

    
    CDPResponse clearSharedStorageEntries(const std::string& ownerOrigin) {
        return call("clearSharedStorageEntries", Params().set("ownerOrigin", ownerOrigin));
    }

    
    CDPResponse resetSharedStorageBudget(const std::string& ownerOrigin) {
        return call("resetSharedStorageBudget", Params().set("ownerOrigin", ownerOrigin));
    }

    
    CDPResponse setSharedStorageTracking(bool enable) {
        return call("setSharedStorageTracking", Params().set("enable", enable));
    }

    
    CDPResponse setStorageBucketTracking(const std::string& storageKey, bool enable) {
        return call("setStorageBucketTracking", Params()
            .set("storageKey", storageKey)
            .set("enable", enable));
    }

    
    CDPResponse deleteStorageBucket(const StorageBucket& bucket) {
        return call("deleteStorageBucket", Params().set("bucket", bucket.toJson()));
    }

    
    CDPResponse runBounceTrackingMitigations() { return call("runBounceTrackingMitigations"); }

    
    CDPResponse setAttributionReportingLocalTestingMode(bool enabled) {
        return call("setAttributionReportingLocalTestingMode", Params().set("enabled", enabled));
    }

    
    CDPResponse setAttributionReportingTracking(bool enable) {
        return call("setAttributionReportingTracking", Params().set("enable", enable));
    }

    
    CDPResponse sendPendingAttributionReports() { return call("sendPendingAttributionReports"); }

    
    CDPResponse getRelatedWebsiteSets() { return call("getRelatedWebsiteSets"); }

    
    void onCacheStorageContentUpdated(std::function<void(const std::string& origin,
                                                           const std::string& storageKey,
                                                           const std::string& bucketId,
                                                           const std::string& cacheName)> callback) {
        on("cacheStorageContentUpdated", [callback](const CDPEvent& event) {
            callback(
                event.params["origin"].getString(),
                event.params["storageKey"].getString(),
                event.params["bucketId"].getString(),
                event.params["cacheName"].getString()
            );
        });
    }

    void onCacheStorageListUpdated(std::function<void(const std::string& origin,
                                                        const std::string& storageKey,
                                                        const std::string& bucketId)> callback) {
        on("cacheStorageListUpdated", [callback](const CDPEvent& event) {
            callback(
                event.params["origin"].getString(),
                event.params["storageKey"].getString(),
                event.params["bucketId"].getString()
            );
        });
    }

    void onIndexedDBContentUpdated(std::function<void(const std::string& origin,
                                                        const std::string& storageKey,
                                                        const std::string& bucketId,
                                                        const std::string& databaseName,
                                                        const std::string& objectStoreName)> callback) {
        on("indexedDBContentUpdated", [callback](const CDPEvent& event) {
            callback(
                event.params["origin"].getString(),
                event.params["storageKey"].getString(),
                event.params["bucketId"].getString(),
                event.params["databaseName"].getString(),
                event.params["objectStoreName"].getString()
            );
        });
    }

    void onIndexedDBListUpdated(std::function<void(const std::string& origin,
                                                     const std::string& storageKey,
                                                     const std::string& bucketId)> callback) {
        on("indexedDBListUpdated", [callback](const CDPEvent& event) {
            callback(
                event.params["origin"].getString(),
                event.params["storageKey"].getString(),
                event.params["bucketId"].getString()
            );
        });
    }

    void onInterestGroupAccessed(std::function<void(double accessTime,
                                                      const std::string& type,
                                                      const std::string& ownerOrigin,
                                                      const std::string& name)> callback) {
        on("interestGroupAccessed", [callback](const CDPEvent& event) {
            callback(
                event.params["accessTime"].getNumber(),
                event.params["type"].getString(),
                event.params["ownerOrigin"].getString(),
                event.params["name"].getString()
            );
        });
    }

    void onSharedStorageAccessed(std::function<void(double accessTime,
                                                       const std::string& type,
                                                       const std::string& mainFrameId,
                                                       const std::string& ownerOrigin,
                                                       const JsonValue& params)> callback) {
        on("sharedStorageAccessed", [callback](const CDPEvent& event) {
            callback(
                event.params["accessTime"].getNumber(),
                event.params["type"].getString(),
                event.params["mainFrameId"].getString(),
                event.params["ownerOrigin"].getString(),
                event.params["params"]
            );
        });
    }

    void onStorageBucketCreatedOrUpdated(std::function<void(const JsonValue& bucketInfo)> callback) {
        on("storageBucketCreatedOrUpdated", [callback](const CDPEvent& event) {
            callback(event.params["bucketInfo"]);
        });
    }

    void onStorageBucketDeleted(std::function<void(const std::string& bucketId)> callback) {
        on("storageBucketDeleted", [callback](const CDPEvent& event) {
            callback(event.params["bucketId"].getString());
        });
    }
};

} 
