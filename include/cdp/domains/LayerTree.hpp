#pragma once

#include "Domain.hpp"

namespace cdp {


struct Layer {
    std::string layerId;
    std::string parentLayerId;
    int backendNodeId;
    double offsetX;
    double offsetY;
    double width;
    double height;
    std::vector<double> transform;
    double anchorX;
    double anchorY;
    double anchorZ;
    int paintCount;
    bool drawsContent;
    bool invisible;
    std::vector<JsonValue> scrollRects;
    JsonValue stickyPositionConstraint;

    static Layer fromJson(const JsonValue& json) {
        Layer l;
        l.layerId = json["layerId"].getString();
        l.parentLayerId = json["parentLayerId"].getString();
        l.backendNodeId = json["backendNodeId"].getInt();
        l.offsetX = json["offsetX"].getNumber();
        l.offsetY = json["offsetY"].getNumber();
        l.width = json["width"].getNumber();
        l.height = json["height"].getNumber();
        if (json["transform"].isArray()) {
            for (const auto& val : json["transform"].asArray()) {
                l.transform.push_back(val.getNumber());
            }
        }
        l.anchorX = json["anchorX"].getNumber();
        l.anchorY = json["anchorY"].getNumber();
        l.anchorZ = json["anchorZ"].getNumber();
        l.paintCount = json["paintCount"].getInt();
        l.drawsContent = json["drawsContent"].getBool();
        l.invisible = json["invisible"].getBool();
        if (json["scrollRects"].isArray()) {
            for (const auto& val : json["scrollRects"].asArray()) {
                l.scrollRects.push_back(val);
            }
        }
        l.stickyPositionConstraint = json["stickyPositionConstraint"];
        return l;
    }
};


class LayerTree : public Domain {
public:
    explicit LayerTree(CDPConnection& connection) : Domain(connection, "LayerTree") {}

    
    CDPResponse enable() { return call("enable"); }

    
    CDPResponse disable() { return call("disable"); }

    
    CDPResponse compositingReasons(const std::string& layerId) {
        return call("compositingReasons", Params().set("layerId", layerId));
    }

    
    CDPResponse loadSnapshot(const JsonValue& tiles) {
        return call("loadSnapshot", Params().set("tiles", tiles));
    }

    
    CDPResponse makeSnapshot(const std::string& layerId) {
        return call("makeSnapshot", Params().set("layerId", layerId));
    }

    
    CDPResponse profileSnapshot(const std::string& snapshotId,
                                 int minRepeatCount = 0,
                                 double minDuration = 0,
                                 const JsonValue* clipRect = nullptr) {
        Params params;
        params.set("snapshotId", snapshotId);
        if (minRepeatCount > 0) params.set("minRepeatCount", minRepeatCount);
        if (minDuration > 0) params.set("minDuration", minDuration);
        if (clipRect) params.set("clipRect", *clipRect);
        return call("profileSnapshot", params);
    }

    
    CDPResponse releaseSnapshot(const std::string& snapshotId) {
        return call("releaseSnapshot", Params().set("snapshotId", snapshotId));
    }

    
    CDPResponse replaySnapshot(const std::string& snapshotId,
                                int fromStep = -1,
                                int toStep = -1,
                                double scale = 0) {
        Params params;
        params.set("snapshotId", snapshotId);
        if (fromStep >= 0) params.set("fromStep", fromStep);
        if (toStep >= 0) params.set("toStep", toStep);
        if (scale > 0) params.set("scale", scale);
        return call("replaySnapshot", params);
    }

    
    CDPResponse snapshotCommandLog(const std::string& snapshotId) {
        return call("snapshotCommandLog", Params().set("snapshotId", snapshotId));
    }

    
    void onLayerPainted(std::function<void(const std::string& layerId, const JsonValue& clip)> callback) {
        on("layerPainted", [callback](const CDPEvent& event) {
            callback(event.params["layerId"].getString(), event.params["clip"]);
        });
    }

    void onLayerTreeDidChange(std::function<void(const JsonValue& layers)> callback) {
        on("layerTreeDidChange", [callback](const CDPEvent& event) {
            callback(event.params["layers"]);
        });
    }
};

} 
