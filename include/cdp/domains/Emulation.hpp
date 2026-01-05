#pragma once

#include "Domain.hpp"

namespace cdp {


struct ScreenOrientation {
    std::string type;  
    int angle = 0;

    JsonValue toJson() const {
        JsonObject obj;
        obj["type"] = type;
        obj["angle"] = angle;
        return obj;
    }
};


struct DisplayFeature {
    std::string orientation;  
    int offset = 0;
    int maskLength = 0;

    JsonValue toJson() const {
        JsonObject obj;
        obj["orientation"] = orientation;
        obj["offset"] = offset;
        obj["maskLength"] = maskLength;
        return obj;
    }
};


struct DevicePosture {
    std::string type;  

    JsonValue toJson() const {
        JsonObject obj;
        obj["type"] = type;
        return obj;
    }
};


struct UserAgentBrandVersion {
    std::string brand;
    std::string version;

    JsonValue toJson() const {
        JsonObject obj;
        obj["brand"] = brand;
        obj["version"] = version;
        return obj;
    }
};


struct UserAgentMetadata {
    std::vector<UserAgentBrandVersion> brands;
    std::vector<UserAgentBrandVersion> fullVersionList;
    std::string platform;
    std::string platformVersion;
    std::string architecture;
    std::string model;
    bool mobile = false;
    std::string bitness;
    bool wow64 = false;

    JsonValue toJson() const {
        JsonObject obj;
        if (!brands.empty()) {
            JsonArray arr;
            for (const auto& b : brands) arr.push_back(b.toJson());
            obj["brands"] = arr;
        }
        if (!fullVersionList.empty()) {
            JsonArray arr;
            for (const auto& b : fullVersionList) arr.push_back(b.toJson());
            obj["fullVersionList"] = arr;
        }
        if (!platform.empty()) obj["platform"] = platform;
        if (!platformVersion.empty()) obj["platformVersion"] = platformVersion;
        if (!architecture.empty()) obj["architecture"] = architecture;
        if (!model.empty()) obj["model"] = model;
        obj["mobile"] = mobile;
        if (!bitness.empty()) obj["bitness"] = bitness;
        if (wow64) obj["wow64"] = true;
        return obj;
    }
};


class Emulation : public Domain {
public:
    explicit Emulation(CDPConnection& connection) : Domain(connection, "Emulation") {}

    
    CDPResponse setDeviceMetricsOverride(int width, int height,
                                          double deviceScaleFactor,
                                          bool mobile,
                                          double scale = 0,
                                          int screenWidth = 0,
                                          int screenHeight = 0,
                                          int positionX = 0,
                                          int positionY = 0,
                                          bool dontSetVisibleSize = false,
                                          const ScreenOrientation* screenOrientation = nullptr,
                                          const JsonValue& viewport = JsonValue(),
                                          const DisplayFeature* displayFeature = nullptr,
                                          const DevicePosture* devicePosture = nullptr) {
        Params params;
        params.set("width", width);
        params.set("height", height);
        params.set("deviceScaleFactor", deviceScaleFactor);
        params.set("mobile", mobile);
        if (scale > 0) params.set("scale", scale);
        if (screenWidth > 0) params.set("screenWidth", screenWidth);
        if (screenHeight > 0) params.set("screenHeight", screenHeight);
        if (positionX != 0) params.set("positionX", positionX);
        if (positionY != 0) params.set("positionY", positionY);
        if (dontSetVisibleSize) params.set("dontSetVisibleSize", true);
        if (screenOrientation) params.set("screenOrientation", screenOrientation->toJson());
        if (!viewport.isNull()) params.set("viewport", viewport);
        if (displayFeature) params.set("displayFeature", displayFeature->toJson());
        if (devicePosture) params.set("devicePosture", devicePosture->toJson());
        return call("setDeviceMetricsOverride", params);
    }

    CDPResponse clearDeviceMetricsOverride() { return call("clearDeviceMetricsOverride"); }

    
    CDPResponse setUserAgentOverride(const std::string& userAgent,
                                      const std::string& acceptLanguage = "",
                                      const std::string& platform = "",
                                      const UserAgentMetadata* userAgentMetadata = nullptr) {
        Params params;
        params.set("userAgent", userAgent);
        if (!acceptLanguage.empty()) params.set("acceptLanguage", acceptLanguage);
        if (!platform.empty()) params.set("platform", platform);
        if (userAgentMetadata) params.set("userAgentMetadata", userAgentMetadata->toJson());
        return call("setUserAgentOverride", params);
    }

    
    CDPResponse setGeolocationOverride(double latitude = 0,
                                        double longitude = 0,
                                        double accuracy = 0,
                                        double altitude = 0,
                                        double altitudeAccuracy = 0,
                                        double heading = 0,
                                        double speed = 0) {
        Params params;
        if (latitude != 0 || longitude != 0 || accuracy != 0) {
            params.set("latitude", latitude);
            params.set("longitude", longitude);
            params.set("accuracy", accuracy);
        }
        if (altitude != 0) params.set("altitude", altitude);
        if (altitudeAccuracy != 0) params.set("altitudeAccuracy", altitudeAccuracy);
        if (heading != 0) params.set("heading", heading);
        if (speed != 0) params.set("speed", speed);
        return call("setGeolocationOverride", params);
    }

    CDPResponse clearGeolocationOverride() { return call("clearGeolocationOverride"); }

    
    CDPResponse setTimezoneOverride(const std::string& timezoneId) {
        return call("setTimezoneOverride", Params().set("timezoneId", timezoneId));
    }

    
    CDPResponse setLocaleOverride(const std::string& locale = "") {
        Params params;
        if (!locale.empty()) params.set("locale", locale);
        return call("setLocaleOverride", params);
    }

    
    CDPResponse setTouchEmulationEnabled(bool enabled, int maxTouchPoints = 1) {
        Params params;
        params.set("enabled", enabled);
        if (maxTouchPoints != 1) params.set("maxTouchPoints", maxTouchPoints);
        return call("setTouchEmulationEnabled", params);
    }

    
    CDPResponse setEmitTouchEventsForMouse(bool enabled,
                                            const std::string& configuration = "") {
        Params params;
        params.set("enabled", enabled);
        if (!configuration.empty()) params.set("configuration", configuration);
        return call("setEmitTouchEventsForMouse", params);
    }

    CDPResponse setEmulatedMedia(const std::string& media = "",
                                  const std::vector<std::pair<std::string, std::string>>& features = {}) {
        Params params;
        if (!media.empty()) params.set("media", media);
        if (!features.empty()) {
            JsonArray arr;
            for (const auto& [name, value] : features) {
                JsonObject f;
                f["name"] = name;
                f["value"] = value;
                arr.push_back(f);
            }
            params.set("features", arr);
        }
        return call("setEmulatedMedia", params);
    }

    
    CDPResponse setEmulatedVisionDeficiency(const std::string& type) {
        
        return call("setEmulatedVisionDeficiency", Params().set("type", type));
    }

    
    CDPResponse setCPUThrottlingRate(double rate) {
        return call("setCPUThrottlingRate", Params().set("rate", rate));
    }

    
    CDPResponse setFocusEmulationEnabled(bool enabled) {
        return call("setFocusEmulationEnabled", Params().set("enabled", enabled));
    }

    
    CDPResponse setAutoDarkModeOverride(bool enabled = true) {
        Params params;
        if (!enabled) params.set("enabled", false);
        return call("setAutoDarkModeOverride", params);
    }

    
    CDPResponse setScrollbarsHidden(bool hidden) {
        return call("setScrollbarsHidden", Params().set("hidden", hidden));
    }

    
    CDPResponse setDocumentCookieDisabled(bool disabled) {
        return call("setDocumentCookieDisabled", Params().set("disabled", disabled));
    }

    
    CDPResponse setScriptExecutionDisabled(bool value) {
        return call("setScriptExecutionDisabled", Params().set("value", value));
    }

    
    CDPResponse setDefaultBackgroundColorOverride(int r = -1, int g = -1, int b = -1, double a = -1) {
        Params params;
        if (r >= 0 && g >= 0 && b >= 0) {
            JsonObject color;
            color["r"] = r;
            color["g"] = g;
            color["b"] = b;
            if (a >= 0) color["a"] = a;
            params.set("color", color);
        }
        return call("setDefaultBackgroundColorOverride", params);
    }

    
    CDPResponse setDevicePostureOverride(const DevicePosture& posture) {
        return call("setDevicePostureOverride", Params().set("posture", posture.toJson()));
    }

    CDPResponse clearDevicePostureOverride() { return call("clearDevicePostureOverride"); }

    
    CDPResponse setIdleOverride(bool isUserActive, bool isScreenUnlocked) {
        return call("setIdleOverride", Params()
            .set("isUserActive", isUserActive)
            .set("isScreenUnlocked", isScreenUnlocked));
    }

    CDPResponse clearIdleOverride() { return call("clearIdleOverride"); }

    
    CDPResponse setHardwareConcurrencyOverride(int hardwareConcurrency) {
        return call("setHardwareConcurrencyOverride",
            Params().set("hardwareConcurrency", hardwareConcurrency));
    }

    
    CDPResponse setSensorOverrideEnabled(bool enabled,
                                          const std::string& type,  
                                          const JsonValue& metadata = JsonValue()) {
        Params params;
        params.set("enabled", enabled);
        params.set("type", type);
        if (!metadata.isNull()) params.set("metadata", metadata);
        return call("setSensorOverrideEnabled", params);
    }

    CDPResponse setSensorOverrideReadings(const std::string& type, const JsonValue& reading) {
        return call("setSensorOverrideReadings", Params()
            .set("type", type)
            .set("reading", reading));
    }

    
    CDPResponse setPressureSourceOverrideEnabled(bool enabled,
                                                   const std::string& source,  
                                                   const std::string& state = "") {  
        Params params;
        params.set("enabled", enabled);
        params.set("source", source);
        if (!state.empty()) params.set("state", state);
        return call("setPressureSourceOverrideEnabled", params);
    }

    CDPResponse setPressureStateOverride(const std::string& source, const std::string& state) {
        return call("setPressureStateOverride", Params()
            .set("source", source)
            .set("state", state));
    }

    
    CDPResponse setVirtualTimePolicy(const std::string& policy,  
                                      double budget = 0,
                                      int maxVirtualTimeTaskStarvationCount = 0,
                                      double initialVirtualTime = 0) {
        Params params;
        params.set("policy", policy);
        if (budget > 0) params.set("budget", budget);
        if (maxVirtualTimeTaskStarvationCount > 0)
            params.set("maxVirtualTimeTaskStarvationCount", maxVirtualTimeTaskStarvationCount);
        if (initialVirtualTime > 0) params.set("initialVirtualTime", initialVirtualTime);
        return call("setVirtualTimePolicy", params);
    }

    
    CDPResponse setPageScaleFactor(double pageScaleFactor) {
        return call("setPageScaleFactor", Params().set("pageScaleFactor", pageScaleFactor));
    }

    
    CDPResponse setVisibleSize(int width, int height) {
        return call("setVisibleSize", Params()
            .set("width", width)
            .set("height", height));
    }

    
    CDPResponse setDisabledImageTypes(const std::vector<std::string>& imageTypes) {
        JsonArray arr;
        for (const auto& t : imageTypes) arr.push_back(t);
        return call("setDisabledImageTypes", Params().set("imageTypes", arr));
    }

    
    CDPResponse setAutomationOverride(bool enabled) {
        return call("setAutomationOverride", Params().set("enabled", enabled));
    }

    
    CDPResponse setEmulatedOSTextScale(double scale) {
        return call("setEmulatedOSTextScale", Params().set("scale", scale));
    }

    
    CDPResponse setDataSaverOverride(bool dataSaverEnabled) {
        return call("setDataSaverOverride", Params().set("dataSaverEnabled", dataSaverEnabled));
    }

    
    CDPResponse setDisplayFeaturesOverride(const std::vector<DisplayFeature>& features) {
        JsonArray arr;
        for (const auto& f : features) arr.push_back(f.toJson());
        return call("setDisplayFeaturesOverride", Params().set("features", arr));
    }

    CDPResponse clearDisplayFeaturesOverride() { return call("clearDisplayFeaturesOverride"); }

    
    CDPResponse setSafeAreaInsetsOverride(int top = 0, int left = 0, int bottom = 0, int right = 0) {
        JsonObject insets;
        insets["top"] = top;
        insets["left"] = left;
        insets["bottom"] = bottom;
        insets["right"] = right;
        return call("setSafeAreaInsetsOverride", Params().set("insets", insets));
    }

    
    CDPResponse setSmallViewportHeightDifferenceOverride(int difference) {
        return call("setSmallViewportHeightDifferenceOverride", Params().set("difference", difference));
    }

    
    CDPResponse canEmulate() { return call("canEmulate"); }

    
    CDPResponse resetPageScaleFactor() { return call("resetPageScaleFactor"); }

    
    CDPResponse getScreenInfos() { return call("getScreenInfos"); }

    CDPResponse addScreen(int left, int top, int width, int height,
                           int devicePixelRatio = 1,
                           int rotation = 0,
                           int colorDepth = 24,
                           const std::string& label = "",
                           bool isInternal = false) {
        Params params;
        params.set("left", left);
        params.set("top", top);
        params.set("width", width);
        params.set("height", height);
        if (devicePixelRatio != 1) params.set("devicePixelRatio", devicePixelRatio);
        if (rotation != 0) params.set("rotation", rotation);
        if (colorDepth != 24) params.set("colorDepth", colorDepth);
        if (!label.empty()) params.set("label", label);
        if (isInternal) params.set("isInternal", true);
        return call("addScreen", params);
    }

    CDPResponse removeScreen(const std::string& screenId) {
        return call("removeScreen", Params().set("screenId", screenId));
    }

    
    CDPResponse getOverriddenSensorInformation(const std::string& type) {
        return call("getOverriddenSensorInformation", Params().set("type", type));
    }

    
    CDPResponse setNavigatorOverrides(const std::string& platform) {
        return call("setNavigatorOverrides", Params().set("platform", platform));
    }

    
    CDPResponse setPressureDataOverride(const std::string& source, double pressure, double timestamp) {
        return call("setPressureDataOverride",
                   Params().set("source", source).set("pressure", pressure).set("timestamp", timestamp));
    }

    
    void onVirtualTimeBudgetExpired(std::function<void()> callback) {
        on("virtualTimeBudgetExpired", [callback](const CDPEvent&) {
            callback();
        });
    }
};

} 
