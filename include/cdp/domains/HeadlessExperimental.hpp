#pragma once

#include "Domain.hpp"

namespace cdp {


struct ScreenshotParams {
    std::string format;  
    int quality = -1;
    bool optimizeForSpeed = false;

    JsonValue toJson() const {
        JsonObject obj;
        if (!format.empty()) obj["format"] = format;
        if (quality >= 0) obj["quality"] = quality;
        if (optimizeForSpeed) obj["optimizeForSpeed"] = true;
        return obj;
    }
};


class HeadlessExperimental : public Domain {
public:
    explicit HeadlessExperimental(CDPConnection& connection) : Domain(connection, "HeadlessExperimental") {}

    
    CDPResponse beginFrame(double frameTimeTicks = 0,
                            double interval = 0,
                            bool noDisplayUpdates = false,
                            const ScreenshotParams* screenshot = nullptr) {
        Params params;
        if (frameTimeTicks > 0) params.set("frameTimeTicks", frameTimeTicks);
        if (interval > 0) params.set("interval", interval);
        if (noDisplayUpdates) params.set("noDisplayUpdates", true);
        if (screenshot) params.set("screenshot", screenshot->toJson());
        return call("beginFrame", params);
    }

    
    CDPResponse disable() { return call("disable"); }

    
    CDPResponse enable() { return call("enable"); }

    
    void onNeedsBeginFramesChanged(std::function<void(bool needsBeginFrames)> callback) {
        on("needsBeginFramesChanged", [callback](const CDPEvent& event) {
            callback(event.params["needsBeginFrames"].getBool());
        });
    }
};

} 
