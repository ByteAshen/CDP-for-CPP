#pragma once

#include "Domain.hpp"

namespace cdp {


struct FileHandler {
    std::string action;
    std::string name;
    JsonValue icons;
    JsonValue accepts;
    std::string launchType;  

    static FileHandler fromJson(const JsonValue& json) {
        FileHandler f;
        f.action = json["action"].getString();
        f.name = json["name"].getString();
        f.icons = json["icons"];
        f.accepts = json["accepts"];
        f.launchType = json["launchType"].getString();
        return f;
    }
};


struct DisplayMode {
    std::string display;  

    static DisplayMode fromJson(const JsonValue& json) {
        DisplayMode d;
        d.display = json["display"].getString();
        return d;
    }
};


class PWA : public Domain {
public:
    explicit PWA(CDPConnection& connection) : Domain(connection, "PWA") {}

    
    CDPResponse getOsAppState(const std::string& manifestId) {
        return call("getOsAppState", Params().set("manifestId", manifestId));
    }

    
    CDPResponse install(const std::string& manifestId,
                         const std::string& installUrlOrBundleUrl = "") {
        Params params;
        params.set("manifestId", manifestId);
        if (!installUrlOrBundleUrl.empty()) params.set("installUrlOrBundleUrl", installUrlOrBundleUrl);
        return call("install", params);
    }

    
    CDPResponse uninstall(const std::string& manifestId) {
        return call("uninstall", Params().set("manifestId", manifestId));
    }

    
    CDPResponse launch(const std::string& manifestId, const std::string& url = "") {
        Params params;
        params.set("manifestId", manifestId);
        if (!url.empty()) params.set("url", url);
        return call("launch", params);
    }

    
    CDPResponse launchFilesInApp(const std::string& manifestId, const std::vector<std::string>& files) {
        JsonArray arr;
        for (const auto& f : files) arr.push_back(f);
        return call("launchFilesInApp", Params()
            .set("manifestId", manifestId)
            .set("files", arr));
    }

    
    CDPResponse openCurrentPageInApp(const std::string& manifestId) {
        return call("openCurrentPageInApp", Params().set("manifestId", manifestId));
    }

    
    CDPResponse changeAppUserSettings(const std::string& manifestId,
                                        bool linkCapturing = false,
                                        const std::string& displayMode = "") {
        Params params;
        params.set("manifestId", manifestId);
        if (linkCapturing) params.set("linkCapturing", true);
        if (!displayMode.empty()) params.set("displayMode", displayMode);
        return call("changeAppUserSettings", params);
    }
};

} 
