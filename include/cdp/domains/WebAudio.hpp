#pragma once

#include "Domain.hpp"

namespace cdp {


struct ContextRealtimeData {
    double currentTime;
    double renderCapacity;
    double callbackIntervalMean;
    double callbackIntervalVariance;

    static ContextRealtimeData fromJson(const JsonValue& json) {
        ContextRealtimeData d;
        d.currentTime = json["currentTime"].getNumber();
        d.renderCapacity = json["renderCapacity"].getNumber();
        d.callbackIntervalMean = json["callbackIntervalMean"].getNumber();
        d.callbackIntervalVariance = json["callbackIntervalVariance"].getNumber();
        return d;
    }
};


struct BaseAudioContext {
    std::string contextId;
    std::string contextType;  
    std::string contextState;  
    JsonValue realtimeData;
    double callbackBufferSize;
    double maxOutputChannelCount;
    double sampleRate;

    static BaseAudioContext fromJson(const JsonValue& json) {
        BaseAudioContext c;
        c.contextId = json["contextId"].getString();
        c.contextType = json["contextType"].getString();
        c.contextState = json["contextState"].getString();
        c.realtimeData = json["realtimeData"];
        c.callbackBufferSize = json["callbackBufferSize"].getNumber();
        c.maxOutputChannelCount = json["maxOutputChannelCount"].getNumber();
        c.sampleRate = json["sampleRate"].getNumber();
        return c;
    }
};


struct AudioListener {
    std::string listenerId;
    std::string contextId;

    static AudioListener fromJson(const JsonValue& json) {
        AudioListener l;
        l.listenerId = json["listenerId"].getString();
        l.contextId = json["contextId"].getString();
        return l;
    }
};


struct AudioNode {
    std::string nodeId;
    std::string contextId;
    std::string nodeType;
    int numberOfInputs;
    int numberOfOutputs;
    int channelCount;
    std::string channelCountMode;  
    std::string channelInterpretation;  

    static AudioNode fromJson(const JsonValue& json) {
        AudioNode n;
        n.nodeId = json["nodeId"].getString();
        n.contextId = json["contextId"].getString();
        n.nodeType = json["nodeType"].getString();
        n.numberOfInputs = json["numberOfInputs"].getInt();
        n.numberOfOutputs = json["numberOfOutputs"].getInt();
        n.channelCount = json["channelCount"].getInt();
        n.channelCountMode = json["channelCountMode"].getString();
        n.channelInterpretation = json["channelInterpretation"].getString();
        return n;
    }
};


struct AudioParam {
    std::string paramId;
    std::string nodeId;
    std::string contextId;
    std::string paramType;
    std::string rate;  
    double defaultValue;
    double minValue;
    double maxValue;

    static AudioParam fromJson(const JsonValue& json) {
        AudioParam p;
        p.paramId = json["paramId"].getString();
        p.nodeId = json["nodeId"].getString();
        p.contextId = json["contextId"].getString();
        p.paramType = json["paramType"].getString();
        p.rate = json["rate"].getString();
        p.defaultValue = json["defaultValue"].getNumber();
        p.minValue = json["minValue"].getNumber();
        p.maxValue = json["maxValue"].getNumber();
        return p;
    }
};


class WebAudio : public Domain {
public:
    explicit WebAudio(CDPConnection& connection) : Domain(connection, "WebAudio") {}

    
    CDPResponse enable() { return call("enable"); }

    
    CDPResponse disable() { return call("disable"); }

    
    CDPResponse getRealtimeData(const std::string& contextId) {
        return call("getRealtimeData", Params().set("contextId", contextId));
    }

    
    void onContextCreated(std::function<void(const JsonValue& context)> callback) {
        on("contextCreated", [callback](const CDPEvent& event) {
            callback(event.params["context"]);
        });
    }

    void onContextWillBeDestroyed(std::function<void(const std::string& contextId)> callback) {
        on("contextWillBeDestroyed", [callback](const CDPEvent& event) {
            callback(event.params["contextId"].getString());
        });
    }

    void onContextChanged(std::function<void(const JsonValue& context)> callback) {
        on("contextChanged", [callback](const CDPEvent& event) {
            callback(event.params["context"]);
        });
    }

    void onAudioListenerCreated(std::function<void(const JsonValue& listener)> callback) {
        on("audioListenerCreated", [callback](const CDPEvent& event) {
            callback(event.params["listener"]);
        });
    }

    void onAudioListenerWillBeDestroyed(std::function<void(const std::string& contextId,
                                                             const std::string& listenerId)> callback) {
        on("audioListenerWillBeDestroyed", [callback](const CDPEvent& event) {
            callback(
                event.params["contextId"].getString(),
                event.params["listenerId"].getString()
            );
        });
    }

    void onAudioNodeCreated(std::function<void(const JsonValue& node)> callback) {
        on("audioNodeCreated", [callback](const CDPEvent& event) {
            callback(event.params["node"]);
        });
    }

    void onAudioNodeWillBeDestroyed(std::function<void(const std::string& contextId,
                                                         const std::string& nodeId)> callback) {
        on("audioNodeWillBeDestroyed", [callback](const CDPEvent& event) {
            callback(
                event.params["contextId"].getString(),
                event.params["nodeId"].getString()
            );
        });
    }

    void onAudioParamCreated(std::function<void(const JsonValue& param)> callback) {
        on("audioParamCreated", [callback](const CDPEvent& event) {
            callback(event.params["param"]);
        });
    }

    void onAudioParamWillBeDestroyed(std::function<void(const std::string& contextId,
                                                          const std::string& nodeId,
                                                          const std::string& paramId)> callback) {
        on("audioParamWillBeDestroyed", [callback](const CDPEvent& event) {
            callback(
                event.params["contextId"].getString(),
                event.params["nodeId"].getString(),
                event.params["paramId"].getString()
            );
        });
    }

    void onNodesConnected(std::function<void(const std::string& contextId,
                                               const std::string& sourceId,
                                               const std::string& destinationId,
                                               int sourceOutputIndex,
                                               int destinationInputIndex)> callback) {
        on("nodesConnected", [callback](const CDPEvent& event) {
            callback(
                event.params["contextId"].getString(),
                event.params["sourceId"].getString(),
                event.params["destinationId"].getString(),
                event.params["sourceOutputIndex"].getInt(),
                event.params["destinationInputIndex"].getInt()
            );
        });
    }

    void onNodesDisconnected(std::function<void(const std::string& contextId,
                                                  const std::string& sourceId,
                                                  const std::string& destinationId,
                                                  int sourceOutputIndex,
                                                  int destinationInputIndex)> callback) {
        on("nodesDisconnected", [callback](const CDPEvent& event) {
            callback(
                event.params["contextId"].getString(),
                event.params["sourceId"].getString(),
                event.params["destinationId"].getString(),
                event.params["sourceOutputIndex"].getInt(),
                event.params["destinationInputIndex"].getInt()
            );
        });
    }

    void onNodeParamConnected(std::function<void(const std::string& contextId,
                                                   const std::string& sourceId,
                                                   const std::string& destinationId,
                                                   int sourceOutputIndex)> callback) {
        on("nodeParamConnected", [callback](const CDPEvent& event) {
            callback(
                event.params["contextId"].getString(),
                event.params["sourceId"].getString(),
                event.params["destinationId"].getString(),
                event.params["sourceOutputIndex"].getInt()
            );
        });
    }

    void onNodeParamDisconnected(std::function<void(const std::string& contextId,
                                                      const std::string& sourceId,
                                                      const std::string& destinationId,
                                                      int sourceOutputIndex)> callback) {
        on("nodeParamDisconnected", [callback](const CDPEvent& event) {
            callback(
                event.params["contextId"].getString(),
                event.params["sourceId"].getString(),
                event.params["destinationId"].getString(),
                event.params["sourceOutputIndex"].getInt()
            );
        });
    }
};

} 
