#pragma once

#include "Domain.hpp"

namespace cdp {


struct OverlayHighlightConfig {
    bool showInfo = false;
    bool showStyles = false;
    bool showRulers = false;
    bool showAccessibilityInfo = false;
    bool showExtensionLines = false;
    JsonValue contentColor;
    JsonValue paddingColor;
    JsonValue borderColor;
    JsonValue marginColor;
    JsonValue eventTargetColor;
    JsonValue shapeColor;
    JsonValue shapeMarginColor;
    JsonValue cssGridColor;
    std::string colorFormat;  
    JsonValue gridHighlightConfig;
    JsonValue flexContainerHighlightConfig;
    JsonValue flexItemHighlightConfig;
    std::string contrastAlgorithm;  
    JsonValue containerQueryContainerHighlightConfig;

    JsonValue toJson() const {
        JsonObject obj;
        if (showInfo) obj["showInfo"] = true;
        if (showStyles) obj["showStyles"] = true;
        if (showRulers) obj["showRulers"] = true;
        if (showAccessibilityInfo) obj["showAccessibilityInfo"] = true;
        if (showExtensionLines) obj["showExtensionLines"] = true;
        if (!contentColor.isNull()) obj["contentColor"] = contentColor;
        if (!paddingColor.isNull()) obj["paddingColor"] = paddingColor;
        if (!borderColor.isNull()) obj["borderColor"] = borderColor;
        if (!marginColor.isNull()) obj["marginColor"] = marginColor;
        if (!colorFormat.empty()) obj["colorFormat"] = colorFormat;
        return obj;
    }
};


class Overlay : public Domain {
public:
    explicit Overlay(CDPConnection& connection) : Domain(connection, "Overlay") {}

    
    CDPResponse enable() { return call("enable"); }

    
    CDPResponse disable() { return call("disable"); }

    
    CDPResponse getHighlightObjectForTest(int nodeId,
                                           bool includeDistance = false,
                                           bool includeStyle = false,
                                           const std::string& colorFormat = "",
                                           bool showAccessibilityInfo = false) {
        Params params;
        params.set("nodeId", nodeId);
        if (includeDistance) params.set("includeDistance", true);
        if (includeStyle) params.set("includeStyle", true);
        if (!colorFormat.empty()) params.set("colorFormat", colorFormat);
        if (showAccessibilityInfo) params.set("showAccessibilityInfo", true);
        return call("getHighlightObjectForTest", params);
    }

    
    CDPResponse getGridHighlightObjectsForTest(const std::vector<int>& nodeIds) {
        JsonArray arr;
        for (int id : nodeIds) arr.push_back(id);
        return call("getGridHighlightObjectsForTest", Params().set("nodeIds", arr));
    }

    
    CDPResponse getSourceOrderHighlightObjectForTest(int nodeId) {
        return call("getSourceOrderHighlightObjectForTest", Params().set("nodeId", nodeId));
    }

    
    CDPResponse hideHighlight() { return call("hideHighlight"); }

    
    CDPResponse highlightFrame(const std::string& frameId,
                                const JsonValue& contentColor = JsonValue(),
                                const JsonValue& contentOutlineColor = JsonValue()) {
        Params params;
        params.set("frameId", frameId);
        if (!contentColor.isNull()) params.set("contentColor", contentColor);
        if (!contentOutlineColor.isNull()) params.set("contentOutlineColor", contentOutlineColor);
        return call("highlightFrame", params);
    }

    
    CDPResponse highlightNode(const OverlayHighlightConfig& highlightConfig,
                               int nodeId = -1,
                               int backendNodeId = -1,
                               const std::string& objectId = "",
                               const std::string& selector = "") {
        Params params;
        params.set("highlightConfig", highlightConfig.toJson());
        if (nodeId >= 0) params.set("nodeId", nodeId);
        if (backendNodeId >= 0) params.set("backendNodeId", backendNodeId);
        if (!objectId.empty()) params.set("objectId", objectId);
        if (!selector.empty()) params.set("selector", selector);
        return call("highlightNode", params);
    }

    
    CDPResponse highlightQuad(const JsonValue& quad,
                               const JsonValue& color = JsonValue(),
                               const JsonValue& outlineColor = JsonValue()) {
        Params params;
        params.set("quad", quad);
        if (!color.isNull()) params.set("color", color);
        if (!outlineColor.isNull()) params.set("outlineColor", outlineColor);
        return call("highlightQuad", params);
    }

    
    CDPResponse highlightRect(int x, int y, int width, int height,
                               const JsonValue& color = JsonValue(),
                               const JsonValue& outlineColor = JsonValue()) {
        Params params;
        params.set("x", x);
        params.set("y", y);
        params.set("width", width);
        params.set("height", height);
        if (!color.isNull()) params.set("color", color);
        if (!outlineColor.isNull()) params.set("outlineColor", outlineColor);
        return call("highlightRect", params);
    }

    
    CDPResponse highlightSourceOrder(const JsonValue& sourceOrderConfig,
                                       int nodeId = -1,
                                       int backendNodeId = -1,
                                       const std::string& objectId = "") {
        Params params;
        params.set("sourceOrderConfig", sourceOrderConfig);
        if (nodeId >= 0) params.set("nodeId", nodeId);
        if (backendNodeId >= 0) params.set("backendNodeId", backendNodeId);
        if (!objectId.empty()) params.set("objectId", objectId);
        return call("highlightSourceOrder", params);
    }

    
    CDPResponse setInspectMode(const std::string& mode,  
                                const OverlayHighlightConfig* highlightConfig = nullptr) {
        Params params;
        params.set("mode", mode);
        if (highlightConfig) params.set("highlightConfig", highlightConfig->toJson());
        return call("setInspectMode", params);
    }

    
    CDPResponse setPausedInDebuggerMessage(const std::string& message = "") {
        Params params;
        if (!message.empty()) params.set("message", message);
        return call("setPausedInDebuggerMessage", params);
    }

    
    CDPResponse setShowAdHighlights(bool show) {
        return call("setShowAdHighlights", Params().set("show", show));
    }

    
    CDPResponse setShowDebugBorders(bool show) {
        return call("setShowDebugBorders", Params().set("show", show));
    }

    
    CDPResponse setShowFPSCounter(bool show) {
        return call("setShowFPSCounter", Params().set("show", show));
    }

    
    CDPResponse setShowGridOverlays(const JsonValue& gridNodeHighlightConfigs) {
        return call("setShowGridOverlays", Params().set("gridNodeHighlightConfigs", gridNodeHighlightConfigs));
    }

    
    CDPResponse setShowFlexOverlays(const JsonValue& flexNodeHighlightConfigs) {
        return call("setShowFlexOverlays", Params().set("flexNodeHighlightConfigs", flexNodeHighlightConfigs));
    }

    
    CDPResponse setShowScrollSnapOverlays(const JsonValue& scrollSnapHighlightConfigs) {
        return call("setShowScrollSnapOverlays", Params().set("scrollSnapHighlightConfigs", scrollSnapHighlightConfigs));
    }

    
    CDPResponse setShowContainerQueryOverlays(const JsonValue& containerQueryHighlightConfigs) {
        return call("setShowContainerQueryOverlays", Params().set("containerQueryHighlightConfigs", containerQueryHighlightConfigs));
    }

    
    CDPResponse setShowPaintRects(bool result) {
        return call("setShowPaintRects", Params().set("result", result));
    }

    
    CDPResponse setShowLayoutShiftRegions(bool result) {
        return call("setShowLayoutShiftRegions", Params().set("result", result));
    }

    
    CDPResponse setShowScrollBottleneckRects(bool show) {
        return call("setShowScrollBottleneckRects", Params().set("show", show));
    }

    
    CDPResponse setShowHitTestBorders(bool show) {
        return call("setShowHitTestBorders", Params().set("show", show));
    }

    
    CDPResponse setShowWebVitals(bool show) {
        return call("setShowWebVitals", Params().set("show", show));
    }

    
    CDPResponse setShowViewportSizeOnResize(bool show) {
        return call("setShowViewportSizeOnResize", Params().set("show", show));
    }

    
    CDPResponse setShowHinge(const JsonValue& hingeConfig = JsonValue()) {
        Params params;
        if (!hingeConfig.isNull()) params.set("hingeConfig", hingeConfig);
        return call("setShowHinge", params);
    }

    
    CDPResponse setShowIsolatedElements(const JsonValue& isolatedElementHighlightConfigs) {
        return call("setShowIsolatedElements", Params().set("isolatedElementHighlightConfigs", isolatedElementHighlightConfigs));
    }

    
    void onInspectNodeRequested(std::function<void(int backendNodeId)> callback) {
        on("inspectNodeRequested", [callback](const CDPEvent& event) {
            callback(event.params["backendNodeId"].getInt());
        });
    }

    void onNodeHighlightRequested(std::function<void(int nodeId)> callback) {
        on("nodeHighlightRequested", [callback](const CDPEvent& event) {
            callback(event.params["nodeId"].getInt());
        });
    }

    void onScreenshotRequested(std::function<void(const JsonValue& viewport)> callback) {
        on("screenshotRequested", [callback](const CDPEvent& event) {
            callback(event.params["viewport"]);
        });
    }

    void onInspectModeCanceled(std::function<void()> callback) {
        on("inspectModeCanceled", [callback](const CDPEvent&) {
            callback();
        });
    }
};

} 
