#pragma once

#include "Domain.hpp"

namespace cdp {


struct AnimationEffect {
    double delay;
    double endDelay;
    int iterationStart;
    int iterations;
    double duration;
    std::string direction;
    std::string fill;
    int backendNodeId;
    JsonValue keyframesRule;
    std::string easing;

    static AnimationEffect fromJson(const JsonValue& json) {
        AnimationEffect e;
        e.delay = json["delay"].getNumber();
        e.endDelay = json["endDelay"].getNumber();
        e.iterationStart = json["iterationStart"].getInt();
        e.iterations = json["iterations"].getInt();
        e.duration = json["duration"].getNumber();
        e.direction = json["direction"].getString();
        e.fill = json["fill"].getString();
        e.backendNodeId = json["backendNodeId"].getInt();
        e.keyframesRule = json["keyframesRule"];
        e.easing = json["easing"].getString();
        return e;
    }
};


struct AnimationInstance {
    std::string id;
    std::string name;
    bool pausedState;
    std::string playState;
    double playbackRate;
    double startTime;
    double currentTime;
    std::string type;  
    JsonValue source;
    std::string cssId;

    static AnimationInstance fromJson(const JsonValue& json) {
        AnimationInstance a;
        a.id = json["id"].getString();
        a.name = json["name"].getString();
        a.pausedState = json["pausedState"].getBool();
        a.playState = json["playState"].getString();
        a.playbackRate = json["playbackRate"].getNumber();
        a.startTime = json["startTime"].getNumber();
        a.currentTime = json["currentTime"].getNumber();
        a.type = json["type"].getString();
        a.source = json["source"];
        a.cssId = json["cssId"].getString();
        return a;
    }
};


class Animation : public Domain {
public:
    explicit Animation(CDPConnection& connection) : Domain(connection, "Animation") {}

    
    CDPResponse enable() { return call("enable"); }

    
    CDPResponse disable() { return call("disable"); }

    
    CDPResponse getCurrentTime(const std::string& id) {
        return call("getCurrentTime", Params().set("id", id));
    }

    
    CDPResponse getPlaybackRate() { return call("getPlaybackRate"); }

    
    CDPResponse releaseAnimations(const std::vector<std::string>& animations) {
        JsonArray arr;
        for (const auto& a : animations) arr.push_back(a);
        return call("releaseAnimations", Params().set("animations", arr));
    }

    
    CDPResponse resolveAnimation(const std::string& animationId) {
        return call("resolveAnimation", Params().set("animationId", animationId));
    }

    
    CDPResponse seekAnimations(const std::vector<std::string>& animations, double currentTime) {
        JsonArray arr;
        for (const auto& a : animations) arr.push_back(a);
        return call("seekAnimations", Params()
            .set("animations", arr)
            .set("currentTime", currentTime));
    }

    
    CDPResponse setPaused(const std::vector<std::string>& animations, bool paused) {
        JsonArray arr;
        for (const auto& a : animations) arr.push_back(a);
        return call("setPaused", Params()
            .set("animations", arr)
            .set("paused", paused));
    }

    
    CDPResponse setPlaybackRate(double playbackRate) {
        return call("setPlaybackRate", Params().set("playbackRate", playbackRate));
    }

    
    CDPResponse setTiming(const std::string& animationId, double duration, double delay) {
        return call("setTiming", Params()
            .set("animationId", animationId)
            .set("duration", duration)
            .set("delay", delay));
    }

    
    void onAnimationCanceled(std::function<void(const std::string& id)> callback) {
        on("animationCanceled", [callback](const CDPEvent& event) {
            callback(event.params["id"].getString());
        });
    }

    void onAnimationCreated(std::function<void(const std::string& id)> callback) {
        on("animationCreated", [callback](const CDPEvent& event) {
            callback(event.params["id"].getString());
        });
    }

    void onAnimationStarted(std::function<void(const JsonValue& animation)> callback) {
        on("animationStarted", [callback](const CDPEvent& event) {
            callback(event.params["animation"]);
        });
    }

    void onAnimationUpdated(std::function<void(const JsonValue& animation)> callback) {
        on("animationUpdated", [callback](const CDPEvent& event) {
            callback(event.params["animation"]);
        });
    }
};

} 
