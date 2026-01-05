#pragma once

#include "Domain.hpp"

namespace cdp {


struct FileInfo {
    std::string name;
    std::string type;  

    static FileInfo fromJson(const JsonValue& json) {
        FileInfo f;
        f.name = json["name"].getString();
        f.type = json["type"].getString();
        return f;
    }
};


struct BucketFileSystemLocator {
    std::string storageKey;
    std::string bucketName;
    std::vector<std::string> pathComponents;

    JsonValue toJson() const {
        JsonObject obj;
        obj["storageKey"] = storageKey;
        if (!bucketName.empty()) obj["bucketName"] = bucketName;
        JsonArray arr;
        for (const auto& p : pathComponents) arr.push_back(p);
        obj["pathComponents"] = arr;
        return obj;
    }
};


class FileSystem : public Domain {
public:
    explicit FileSystem(CDPConnection& connection) : Domain(connection, "FileSystem") {}

    
    CDPResponse getDirectory(const BucketFileSystemLocator& bucketFileSystemLocator) {
        return call("getDirectory", Params().set("bucketFileSystemLocator", bucketFileSystemLocator.toJson()));
    }
};

} 
