#pragma once

#include "BaseDomain.hpp"
#include <string>
#include <optional>

namespace cdp {


struct ScreenOrientation {
    std::string type = "portraitPrimary";  
    int angle = 0;

    JsonValue toJson() const;
};


struct DisplayFeature {
    std::string orientation = "vertical";  
    int offset = 0;
    int maskLength = 0;

    JsonValue toJson() const;
};


struct DeviceMetrics {
    int width = 1920;
    int height = 1080;
    double deviceScaleFactor = 1.0;
    bool mobile = false;
    double scale = 1.0;
    int screenWidth = 0;
    int screenHeight = 0;
    int positionX = 0;
    int positionY = 0;
    bool dontSetVisibleSize = false;
    std::optional<ScreenOrientation> screenOrientation;
    std::optional<DisplayFeature> displayFeature;

    JsonValue toJson() const;
};


struct MediaFeature {
    std::string name;
    std::string value;

    JsonValue toJson() const;
};


class EmulationDomain : public BaseDomain {
public:
    using BaseDomain::BaseDomain;
    std::string domainName() const override { return "Emulation"; }

    
    CDPResponse setDeviceMetricsOverride(const DeviceMetrics& metrics);
    CDPResponse setDeviceMetricsOverride(int width, int height, double deviceScaleFactor = 1.0,
                                          bool mobile = false);
    CDPResponse clearDeviceMetricsOverride();

    
    CDPResponse setScrollbarsHidden(bool hidden);
    CDPResponse setDocumentCookieDisabled(bool disabled);

    
    CDPResponse setTouchEmulationEnabled(bool enabled, int maxTouchPoints = 1);

    
    CDPResponse setEmulatedMedia(const std::string& media = "",
                                  const std::vector<MediaFeature>& features = {});

    
    CDPResponse setEmulatedVisionDeficiency(const std::string& type);
    

    CDPResponse setGeolocationOverride(double latitude, double longitude, double accuracy = 1.0);
    CDPResponse clearGeolocationOverride();

    
    CDPResponse setTimezoneOverride(const std::string& timezoneId);

    
    CDPResponse setLocaleOverride(const std::string& locale);

    
    CDPResponse setUserAgentOverride(const std::string& userAgent,
                                      const std::string& acceptLanguage = "",
                                      const std::string& platform = "",
                                      const std::string& userAgentMetadata = "");

    
    CDPResponse setCPUThrottlingRate(double rate);  

    
    CDPResponse setFocusEmulationEnabled(bool enabled);

    
    CDPResponse setAutoDarkModeOverride(bool enabled);

    
    CDPResponse setDisabledImageTypes(const std::vector<std::string>& imageTypes);
    

    CDPResponse setHardwareConcurrencyOverride(int hardwareConcurrency);

    
    CDPResponse setIdleOverride(bool isUserActive, bool isScreenUnlocked);
    CDPResponse clearIdleOverride();

    
    CDPResponse setPageScaleFactor(double pageScaleFactor);

    
    CDPResponse setScriptExecutionDisabled(bool disabled);

    
    CDPResponse setVirtualTimePolicy(const std::string& policy, double budget = 0,
                                      int maxVirtualTimeTaskStarvationCount = 0,
                                      double initialVirtualTime = 0);
    

    CDPResponse setPressureSourceOverrideEnabled(bool enabled, const std::string& source = "",
                                                  const std::string& state = "");

    
    CDPResponse setAutomationOverride(bool enabled);

    
    CDPResponse setDefaultBackgroundColorOverride(int r, int g, int b, double a = 1.0);
    CDPResponse clearDefaultBackgroundColorOverride();

    
    void onVirtualTimeBudgetExpired(std::function<void()> callback);
};

} 
