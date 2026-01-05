#pragma once

#include "Domain.hpp"

namespace cdp {


struct SourceRange {
    int startLine = 0;
    int startColumn = 0;
    int endLine = 0;
    int endColumn = 0;

    JsonValue toJson() const {
        JsonObject obj;
        obj["startLine"] = startLine;
        obj["startColumn"] = startColumn;
        obj["endLine"] = endLine;
        obj["endColumn"] = endColumn;
        return obj;
    }

    static SourceRange fromJson(const JsonValue& json) {
        SourceRange r;
        r.startLine = json["startLine"].getInt();
        r.startColumn = json["startColumn"].getInt();
        r.endLine = json["endLine"].getInt();
        r.endColumn = json["endColumn"].getInt();
        return r;
    }
};


class CSS : public Domain {
public:
    explicit CSS(CDPConnection& connection) : Domain(connection, "CSS") {}

    
    CDPResponse enable() { return call("enable"); }

    
    CDPResponse disable() { return call("disable"); }

    
    CDPResponse getStyleSheetText(const std::string& styleSheetId) {
        return call("getStyleSheetText", Params().set("styleSheetId", styleSheetId));
    }

    
    CDPResponse setStyleSheetText(const std::string& styleSheetId, const std::string& text) {
        return call("setStyleSheetText", Params()
            .set("styleSheetId", styleSheetId)
            .set("text", text));
    }

    
    CDPResponse getComputedStyleForNode(int nodeId) {
        return call("getComputedStyleForNode", Params().set("nodeId", nodeId));
    }

    
    CDPResponse getInlineStylesForNode(int nodeId) {
        return call("getInlineStylesForNode", Params().set("nodeId", nodeId));
    }

    
    CDPResponse getMatchedStylesForNode(int nodeId) {
        return call("getMatchedStylesForNode", Params().set("nodeId", nodeId));
    }

    
    CDPResponse getPlatformFontsForNode(int nodeId) {
        return call("getPlatformFontsForNode", Params().set("nodeId", nodeId));
    }

    
    CDPResponse setEffectivePropertyValueForNode(int nodeId,
                                                   const std::string& propertyName,
                                                   const std::string& value) {
        return call("setEffectivePropertyValueForNode", Params()
            .set("nodeId", nodeId)
            .set("propertyName", propertyName)
            .set("value", value));
    }

    
    CDPResponse getBackgroundColors(int nodeId) {
        return call("getBackgroundColors", Params().set("nodeId", nodeId));
    }

    
    CDPResponse forcePseudoState(int nodeId, const std::vector<std::string>& forcedPseudoClasses) {
        JsonArray arr;
        for (const auto& c : forcedPseudoClasses) arr.push_back(c);
        return call("forcePseudoState", Params()
            .set("nodeId", nodeId)
            .set("forcedPseudoClasses", arr));
    }

    
    CDPResponse getMediaQueries() { return call("getMediaQueries"); }

    
    CDPResponse setMediaText(const std::string& styleSheetId,
                              const SourceRange& range,
                              const std::string& text) {
        return call("setMediaText", Params()
            .set("styleSheetId", styleSheetId)
            .set("range", range.toJson())
            .set("text", text));
    }

    
    CDPResponse createStyleSheet(const std::string& frameId) {
        return call("createStyleSheet", Params().set("frameId", frameId));
    }

    
    CDPResponse addRule(const std::string& styleSheetId,
                         const std::string& ruleText,
                         const SourceRange& location) {
        return call("addRule", Params()
            .set("styleSheetId", styleSheetId)
            .set("ruleText", ruleText)
            .set("location", location.toJson()));
    }

    
    CDPResponse setRuleSelector(const std::string& styleSheetId,
                                 const SourceRange& range,
                                 const std::string& selector) {
        return call("setRuleSelector", Params()
            .set("styleSheetId", styleSheetId)
            .set("range", range.toJson())
            .set("selector", selector));
    }

    
    CDPResponse setStyleTexts(const JsonArray& edits) {
        return call("setStyleTexts", Params().set("edits", edits));
    }

    
    CDPResponse startRuleUsageTracking() { return call("startRuleUsageTracking"); }

    
    CDPResponse stopRuleUsageTracking() { return call("stopRuleUsageTracking"); }

    
    CDPResponse takeCoverageDelta() { return call("takeCoverageDelta"); }

    
    CDPResponse setContainerQueryText(const std::string& styleSheetId,
                                        const SourceRange& range,
                                        const std::string& text) {
        return call("setContainerQueryText", Params()
            .set("styleSheetId", styleSheetId)
            .set("range", range.toJson())
            .set("text", text));
    }

    
    CDPResponse setSupportsText(const std::string& styleSheetId,
                                 const SourceRange& range,
                                 const std::string& text) {
        return call("setSupportsText", Params()
            .set("styleSheetId", styleSheetId)
            .set("range", range.toJson())
            .set("text", text));
    }

    
    CDPResponse setScopeText(const std::string& styleSheetId,
                              const SourceRange& range,
                              const std::string& text) {
        return call("setScopeText", Params()
            .set("styleSheetId", styleSheetId)
            .set("range", range.toJson())
            .set("text", text));
    }

    
    CDPResponse getLayersForNode(int nodeId) {
        return call("getLayersForNode", Params().set("nodeId", nodeId));
    }

    
    CDPResponse trackComputedStyleUpdates(const JsonArray& propertiesToTrack) {
        return call("trackComputedStyleUpdates", Params().set("propertiesToTrack", propertiesToTrack));
    }

    
    CDPResponse takeComputedStyleUpdates() { return call("takeComputedStyleUpdates"); }

    
    CDPResponse setLocalFontsEnabled(bool enabled) {
        return call("setLocalFontsEnabled", Params().set("enabled", enabled));
    }

    
    void onStyleSheetAdded(std::function<void(const JsonValue& header)> callback) {
        on("styleSheetAdded", [callback](const CDPEvent& event) {
            callback(event.params["header"]);
        });
    }

    void onStyleSheetChanged(std::function<void(const std::string& styleSheetId)> callback) {
        on("styleSheetChanged", [callback](const CDPEvent& event) {
            callback(event.params["styleSheetId"].getString());
        });
    }

    void onStyleSheetRemoved(std::function<void(const std::string& styleSheetId)> callback) {
        on("styleSheetRemoved", [callback](const CDPEvent& event) {
            callback(event.params["styleSheetId"].getString());
        });
    }

    void onFontsUpdated(std::function<void(const JsonValue& font)> callback) {
        on("fontsUpdated", [callback](const CDPEvent& event) {
            callback(event.params["font"]);
        });
    }

    void onMediaQueryResultChanged(std::function<void()> callback) {
        on("mediaQueryResultChanged", [callback](const CDPEvent&) {
            callback();
        });
    }

    void onComputedStyleUpdated(std::function<void(int nodeId)> callback) {
        on("computedStyleUpdated", [callback](const CDPEvent& event) {
            callback(event.params["nodeId"].getInt());
        });
    }
};

} 
