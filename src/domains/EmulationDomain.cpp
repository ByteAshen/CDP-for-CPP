

#include "cdp/domains/EmulationDomain.hpp"

namespace cdp {

JsonValue ScreenOrientation::toJson() const {
    JsonObject obj;
    obj["type"] = type;
    obj["angle"] = angle;
    return obj;
}

JsonValue DisplayFeature::toJson() const {
    JsonObject obj;
    obj["orientation"] = orientation;
    obj["offset"] = offset;
    obj["maskLength"] = maskLength;
    return obj;
}

JsonValue DeviceMetrics::toJson() const {
    JsonObject obj;
    obj["width"] = width;
    obj["height"] = height;
    obj["deviceScaleFactor"] = deviceScaleFactor;
    obj["mobile"] = mobile;
    if (scale != 1.0) obj["scale"] = scale;
    if (screenWidth > 0) obj["screenWidth"] = screenWidth;
    if (screenHeight > 0) obj["screenHeight"] = screenHeight;
    if (positionX != 0) obj["positionX"] = positionX;
    if (positionY != 0) obj["positionY"] = positionY;
    if (dontSetVisibleSize) obj["dontSetVisibleSize"] = dontSetVisibleSize;
    if (screenOrientation) obj["screenOrientation"] = screenOrientation->toJson();
    if (displayFeature) obj["displayFeature"] = displayFeature->toJson();
    return obj;
}

JsonValue MediaFeature::toJson() const {
    JsonObject obj;
    obj["name"] = name;
    obj["value"] = value;
    return obj;
}

CDPResponse EmulationDomain::setDeviceMetricsOverride(const DeviceMetrics& metrics) {
    return sendCommand("Emulation.setDeviceMetricsOverride", metrics.toJson());
}

CDPResponse EmulationDomain::setDeviceMetricsOverride(int width, int height,
                                                       double deviceScaleFactor, bool mobile) {
    DeviceMetrics metrics;
    metrics.width = width;
    metrics.height = height;
    metrics.deviceScaleFactor = deviceScaleFactor;
    metrics.mobile = mobile;
    return setDeviceMetricsOverride(metrics);
}

CDPResponse EmulationDomain::clearDeviceMetricsOverride() {
    return sendCommand("Emulation.clearDeviceMetricsOverride");
}

CDPResponse EmulationDomain::setScrollbarsHidden(bool hidden) {
    JsonObject params;
    params["hidden"] = hidden;
    return sendCommand("Emulation.setScrollbarsHidden", params);
}

CDPResponse EmulationDomain::setDocumentCookieDisabled(bool disabled) {
    JsonObject params;
    params["disabled"] = disabled;
    return sendCommand("Emulation.setDocumentCookieDisabled", params);
}

CDPResponse EmulationDomain::setTouchEmulationEnabled(bool enabled, int maxTouchPoints) {
    JsonObject params;
    params["enabled"] = enabled;
    params["maxTouchPoints"] = maxTouchPoints;
    return sendCommand("Emulation.setTouchEmulationEnabled", params);
}

CDPResponse EmulationDomain::setEmulatedMedia(const std::string& media,
                                               const std::vector<MediaFeature>& features) {
    JsonObject params;
    if (!media.empty()) params["media"] = media;

    if (!features.empty()) {
        JsonArray featuresArray;
        for (const auto& feature : features) {
            featuresArray.push_back(feature.toJson());
        }
        params["features"] = featuresArray;
    }

    return sendCommand("Emulation.setEmulatedMedia", params);
}

CDPResponse EmulationDomain::setEmulatedVisionDeficiency(const std::string& type) {
    JsonObject params;
    params["type"] = type;
    return sendCommand("Emulation.setEmulatedVisionDeficiency", params);
}

CDPResponse EmulationDomain::setGeolocationOverride(double latitude, double longitude, double accuracy) {
    JsonObject params;
    params["latitude"] = latitude;
    params["longitude"] = longitude;
    params["accuracy"] = accuracy;
    return sendCommand("Emulation.setGeolocationOverride", params);
}

CDPResponse EmulationDomain::clearGeolocationOverride() {
    return sendCommand("Emulation.clearGeolocationOverride");
}

CDPResponse EmulationDomain::setTimezoneOverride(const std::string& timezoneId) {
    JsonObject params;
    params["timezoneId"] = timezoneId;
    return sendCommand("Emulation.setTimezoneOverride", params);
}

CDPResponse EmulationDomain::setLocaleOverride(const std::string& locale) {
    JsonObject params;
    params["locale"] = locale;
    return sendCommand("Emulation.setLocaleOverride", params);
}

CDPResponse EmulationDomain::setUserAgentOverride(const std::string& userAgent,
                                                   const std::string& acceptLanguage,
                                                   const std::string& platform,
                                                   const std::string& userAgentMetadata) {
    JsonObject params;
    params["userAgent"] = userAgent;
    if (!acceptLanguage.empty()) params["acceptLanguage"] = acceptLanguage;
    if (!platform.empty()) params["platform"] = platform;
    
    return sendCommand("Emulation.setUserAgentOverride", params);
}

CDPResponse EmulationDomain::setCPUThrottlingRate(double rate) {
    JsonObject params;
    params["rate"] = rate;
    return sendCommand("Emulation.setCPUThrottlingRate", params);
}

CDPResponse EmulationDomain::setFocusEmulationEnabled(bool enabled) {
    JsonObject params;
    params["enabled"] = enabled;
    return sendCommand("Emulation.setFocusEmulationEnabled", params);
}

CDPResponse EmulationDomain::setAutoDarkModeOverride(bool enabled) {
    JsonObject params;
    params["enabled"] = enabled;
    return sendCommand("Emulation.setAutoDarkModeOverride", params);
}

CDPResponse EmulationDomain::setDisabledImageTypes(const std::vector<std::string>& imageTypes) {
    JsonObject params;
    JsonArray types;
    for (const auto& type : imageTypes) {
        types.push_back(type);
    }
    params["imageTypes"] = types;
    return sendCommand("Emulation.setDisabledImageTypes", params);
}

CDPResponse EmulationDomain::setHardwareConcurrencyOverride(int hardwareConcurrency) {
    JsonObject params;
    params["hardwareConcurrency"] = hardwareConcurrency;
    return sendCommand("Emulation.setHardwareConcurrencyOverride", params);
}

CDPResponse EmulationDomain::setIdleOverride(bool isUserActive, bool isScreenUnlocked) {
    JsonObject params;
    params["isUserActive"] = isUserActive;
    params["isScreenUnlocked"] = isScreenUnlocked;
    return sendCommand("Emulation.setIdleOverride", params);
}

CDPResponse EmulationDomain::clearIdleOverride() {
    return sendCommand("Emulation.clearIdleOverride");
}

CDPResponse EmulationDomain::setPageScaleFactor(double pageScaleFactor) {
    JsonObject params;
    params["pageScaleFactor"] = pageScaleFactor;
    return sendCommand("Emulation.setPageScaleFactor", params);
}

CDPResponse EmulationDomain::setScriptExecutionDisabled(bool disabled) {
    JsonObject params;
    params["value"] = disabled;
    return sendCommand("Emulation.setScriptExecutionDisabled", params);
}

CDPResponse EmulationDomain::setVirtualTimePolicy(const std::string& policy, double budget,
                                                   int maxVirtualTimeTaskStarvationCount,
                                                   double initialVirtualTime) {
    JsonObject params;
    params["policy"] = policy;
    if (budget > 0) params["budget"] = budget;
    if (maxVirtualTimeTaskStarvationCount > 0) {
        params["maxVirtualTimeTaskStarvationCount"] = maxVirtualTimeTaskStarvationCount;
    }
    if (initialVirtualTime > 0) params["initialVirtualTime"] = initialVirtualTime;
    return sendCommand("Emulation.setVirtualTimePolicy", params);
}

CDPResponse EmulationDomain::setPressureSourceOverrideEnabled(bool enabled,
                                                               const std::string& source,
                                                               const std::string& state) {
    JsonObject params;
    params["enabled"] = enabled;
    if (!source.empty()) params["source"] = source;
    if (!state.empty()) params["state"] = state;
    return sendCommand("Emulation.setPressureSourceOverrideEnabled", params);
}

CDPResponse EmulationDomain::setAutomationOverride(bool enabled) {
    JsonObject params;
    params["enabled"] = enabled;
    return sendCommand("Emulation.setAutomationOverride", params);
}

CDPResponse EmulationDomain::setDefaultBackgroundColorOverride(int r, int g, int b, double a) {
    JsonObject params;
    JsonObject color;
    color["r"] = r;
    color["g"] = g;
    color["b"] = b;
    color["a"] = a;
    params["color"] = color;
    return sendCommand("Emulation.setDefaultBackgroundColorOverride", params);
}

CDPResponse EmulationDomain::clearDefaultBackgroundColorOverride() {
    return sendCommand("Emulation.setDefaultBackgroundColorOverride");
}

void EmulationDomain::onVirtualTimeBudgetExpired(std::function<void()> callback) {
    subscribeEvent("virtualTimeBudgetExpired", [callback](const CDPEvent&) {
        callback();
    });
}

} 
