#pragma once

#include "Domain.hpp"

namespace cdp {


struct RGBA {
    int r = 0;
    int g = 0;
    int b = 0;
    double a = 1.0;

    JsonValue toJson() const {
        JsonObject obj;
        obj["r"] = r;
        obj["g"] = g;
        obj["b"] = b;
        obj["a"] = a;
        return obj;
    }
};


struct HighlightConfig {
    bool showInfo = true;
    bool showStyles = false;
    bool showRulers = false;
    bool showAccessibilityInfo = false;
    bool showExtensionLines = false;
    RGBA contentColor = {255, 0, 0, 50};
    RGBA paddingColor = {0, 255, 0, 50};
    RGBA borderColor = {0, 0, 255, 50};
    RGBA marginColor = {255, 255, 0, 50};

    JsonValue toJson() const {
        JsonObject obj;
        obj["showInfo"] = showInfo;
        obj["showStyles"] = showStyles;
        obj["showRulers"] = showRulers;
        obj["showAccessibilityInfo"] = showAccessibilityInfo;
        obj["showExtensionLines"] = showExtensionLines;
        obj["contentColor"] = contentColor.toJson();
        obj["paddingColor"] = paddingColor.toJson();
        obj["borderColor"] = borderColor.toJson();
        obj["marginColor"] = marginColor.toJson();
        return obj;
    }
};


class DOM : public Domain {
public:
    explicit DOM(CDPConnection& connection) : Domain(connection, "DOM") {}

    
    CDPResponse enable(const std::string& includeWhitespace = "") {
        
        Params params;
        if (!includeWhitespace.empty()) params.set("includeWhitespace", includeWhitespace);
        return call("enable", params);
    }

    
    CDPResponse getDocument(int depth = -1, bool pierce = false) {
        Params params;
        if (depth >= 0) params.set("depth", depth);
        if (pierce) params.set("pierce", true);
        return call("getDocument", params);
    }

    
    CDPResponse getFlattenedDocument(int depth = -1, bool pierce = false) {
        Params params;
        if (depth >= 0) params.set("depth", depth);
        if (pierce) params.set("pierce", true);
        return call("getFlattenedDocument", params);
    }

    
    CDPResponse querySelector(int nodeId, const std::string& selector) {
        return call("querySelector", Params()
            .set("nodeId", nodeId)
            .set("selector", selector));
    }

    CDPResponse querySelectorAll(int nodeId, const std::string& selector) {
        return call("querySelectorAll", Params()
            .set("nodeId", nodeId)
            .set("selector", selector));
    }

    
    int getRootNodeId() {
        auto doc = getDocument();
        if (doc.hasError) return 0;
        return doc.result.getIntAt("root/nodeId", 0);
    }

    
    int findElement(const std::string& selector) {
        int rootId = getRootNodeId();
        if (rootId == 0) return 0;
        auto result = querySelector(rootId, selector);
        if (result.hasError) return 0;
        return result.result.getIntAt("nodeId", 0);
    }

    
    std::vector<int> findElements(const std::string& selector) {
        std::vector<int> nodes;
        int rootId = getRootNodeId();
        if (rootId == 0) return nodes;
        auto result = querySelectorAll(rootId, selector);
        if (result.hasError) return nodes;
        const auto* nodeIds = result.result.find("nodeIds");
        if (nodeIds && nodeIds->isArray()) {
            for (size_t i = 0; i < nodeIds->size(); ++i) {
                nodes.push_back((*nodeIds)[i].getInt(0));
            }
        }
        return nodes;
    }

    
    bool elementExists(const std::string& selector) {
        return findElement(selector) != 0;
    }

    
    std::string getElementHTML(const std::string& selector) {
        int nodeId = findElement(selector);
        if (nodeId == 0) return "";
        auto result = getOuterHTML(nodeId);
        if (result.hasError) return "";
        return result.result.getStringAt("outerHTML", "");
    }

    
    std::string getElementText(const std::string& selector) {
        std::string js = "document.querySelector('" + selector + "')?.textContent || ''";
        auto result = connection_.sendCommandSync("Runtime.evaluate",
            Params().set("expression", js).set("returnByValue", true).build());
        if (result.hasError) return "";
        return result.result.getStringAt("result/value", "");
    }

    
    std::string getElementAttribute(const std::string& selector, const std::string& attribute) {
        std::string js = "document.querySelector('" + selector + "')?.getAttribute('" + attribute + "') || ''";
        auto result = connection_.sendCommandSync("Runtime.evaluate",
            Params().set("expression", js).set("returnByValue", true).build());
        if (result.hasError) return "";
        return result.result.getStringAt("result/value", "");
    }

    
    struct BoundingBox {
        double x = 0, y = 0, width = 0, height = 0;
        bool valid = false;
        double centerX() const { return x + width / 2; }
        double centerY() const { return y + height / 2; }
    };

    BoundingBox getElementBounds(const std::string& selector) {
        BoundingBox box;
        int nodeId = findElement(selector);
        if (nodeId == 0) return box;

        auto result = getBoxModel(nodeId);
        if (result.hasError) return box;

        
        const auto* model = result.result.find("model");
        if (!model) return box;

        const auto* content = model->find("content");
        if (!content || !content->isArray() || content->size() < 8) return box;

        
        double x1 = (*content)[0].getNumber();
        double y1 = (*content)[1].getNumber();
        double x2 = (*content)[2].getNumber();
        double y3 = (*content)[5].getNumber();

        box.x = x1;
        box.y = y1;
        box.width = x2 - x1;
        box.height = y3 - y1;
        box.valid = true;
        return box;
    }

    
    CDPResponse describeNode(int nodeId = 0,
                              int backendNodeId = 0,
                              const std::string& objectId = "",
                              int depth = 1,
                              bool pierce = false) {
        Params params;
        if (nodeId > 0) params.set("nodeId", nodeId);
        if (backendNodeId > 0) params.set("backendNodeId", backendNodeId);
        if (!objectId.empty()) params.set("objectId", objectId);
        params.set("depth", depth);
        if (pierce) params.set("pierce", true);
        return call("describeNode", params);
    }

    CDPResponse getOuterHTML(int nodeId = 0,
                              int backendNodeId = 0,
                              const std::string& objectId = "",
                              bool includeShadowDOM = false) {
        Params params;
        if (nodeId > 0) params.set("nodeId", nodeId);
        if (backendNodeId > 0) params.set("backendNodeId", backendNodeId);
        if (!objectId.empty()) params.set("objectId", objectId);
        if (includeShadowDOM) params.set("includeShadowDOM", true);
        return call("getOuterHTML", params);
    }

    CDPResponse setOuterHTML(int nodeId, const std::string& outerHTML) {
        return call("setOuterHTML", Params()
            .set("nodeId", nodeId)
            .set("outerHTML", outerHTML));
    }

    CDPResponse setNodeValue(int nodeId, const std::string& value) {
        return call("setNodeValue", Params()
            .set("nodeId", nodeId)
            .set("value", value));
    }

    CDPResponse setNodeName(int nodeId, const std::string& name) {
        return call("setNodeName", Params()
            .set("nodeId", nodeId)
            .set("name", name));
    }

    CDPResponse removeNode(int nodeId) {
        return call("removeNode", Params().set("nodeId", nodeId));
    }

    
    CDPResponse getAttributes(int nodeId) {
        return call("getAttributes", Params().set("nodeId", nodeId));
    }

    CDPResponse setAttributeValue(int nodeId, const std::string& name, const std::string& value) {
        return call("setAttributeValue", Params()
            .set("nodeId", nodeId)
            .set("name", name)
            .set("value", value));
    }

    CDPResponse setAttributesAsText(int nodeId, const std::string& text, const std::string& name = "") {
        Params params;
        params.set("nodeId", nodeId);
        params.set("text", text);
        if (!name.empty()) params.set("name", name);
        return call("setAttributesAsText", params);
    }

    CDPResponse removeAttribute(int nodeId, const std::string& name) {
        return call("removeAttribute", Params()
            .set("nodeId", nodeId)
            .set("name", name));
    }

    
    CDPResponse requestChildNodes(int nodeId, int depth = 1, bool pierce = false) {
        Params params;
        params.set("nodeId", nodeId);
        params.set("depth", depth);
        if (pierce) params.set("pierce", true);
        return call("requestChildNodes", params);
    }

    CDPResponse moveTo(int nodeId, int targetNodeId, int insertBeforeNodeId = 0) {
        Params params;
        params.set("nodeId", nodeId);
        params.set("targetNodeId", targetNodeId);
        if (insertBeforeNodeId > 0) params.set("insertBeforeNodeId", insertBeforeNodeId);
        return call("moveTo", params);
    }

    CDPResponse copyTo(int nodeId, int targetNodeId, int insertBeforeNodeId = 0) {
        Params params;
        params.set("nodeId", nodeId);
        params.set("targetNodeId", targetNodeId);
        if (insertBeforeNodeId > 0) params.set("insertBeforeNodeId", insertBeforeNodeId);
        return call("copyTo", params);
    }

    
    CDPResponse getBoxModel(int nodeId = 0, int backendNodeId = 0, const std::string& objectId = "") {
        Params params;
        if (nodeId > 0) params.set("nodeId", nodeId);
        if (backendNodeId > 0) params.set("backendNodeId", backendNodeId);
        if (!objectId.empty()) params.set("objectId", objectId);
        return call("getBoxModel", params);
    }

    CDPResponse getContentQuads(int nodeId = 0, int backendNodeId = 0, const std::string& objectId = "") {
        Params params;
        if (nodeId > 0) params.set("nodeId", nodeId);
        if (backendNodeId > 0) params.set("backendNodeId", backendNodeId);
        if (!objectId.empty()) params.set("objectId", objectId);
        return call("getContentQuads", params);
    }

    
    CDPResponse getNodeForLocation(int x, int y,
                                    bool includeUserAgentShadowDOM = false,
                                    bool ignorePointerEventsNone = false) {
        Params params;
        params.set("x", x);
        params.set("y", y);
        if (includeUserAgentShadowDOM) params.set("includeUserAgentShadowDOM", true);
        if (ignorePointerEventsNone) params.set("ignorePointerEventsNone", true);
        return call("getNodeForLocation", params);
    }

    
    CDPResponse focus(int nodeId = 0, int backendNodeId = 0, const std::string& objectId = "") {
        Params params;
        if (nodeId > 0) params.set("nodeId", nodeId);
        if (backendNodeId > 0) params.set("backendNodeId", backendNodeId);
        if (!objectId.empty()) params.set("objectId", objectId);
        return call("focus", params);
    }

    
    CDPResponse setFileInputFiles(const std::vector<std::string>& files,
                                   int nodeId = 0,
                                   int backendNodeId = 0,
                                   const std::string& objectId = "") {
        Params params;
        JsonArray filesArray;
        for (const auto& f : files) filesArray.push_back(f);
        params.set("files", filesArray);
        if (nodeId > 0) params.set("nodeId", nodeId);
        if (backendNodeId > 0) params.set("backendNodeId", backendNodeId);
        if (!objectId.empty()) params.set("objectId", objectId);
        return call("setFileInputFiles", params);
    }

    
    CDPResponse scrollIntoViewIfNeeded(int nodeId = 0,
                                        int backendNodeId = 0,
                                        const std::string& objectId = "",
                                        const JsonValue& rect = JsonValue()) {
        Params params;
        if (nodeId > 0) params.set("nodeId", nodeId);
        if (backendNodeId > 0) params.set("backendNodeId", backendNodeId);
        if (!objectId.empty()) params.set("objectId", objectId);
        if (!rect.isNull()) params.set("rect", rect);
        return call("scrollIntoViewIfNeeded", params);
    }

    
    CDPResponse getFrameOwner(const std::string& frameId) {
        return call("getFrameOwner", Params().set("frameId", frameId));
    }

    
    CDPResponse resolveNode(int nodeId = 0,
                             int backendNodeId = 0,
                             const std::string& objectGroup = "",
                             int executionContextId = 0) {
        Params params;
        if (nodeId > 0) params.set("nodeId", nodeId);
        if (backendNodeId > 0) params.set("backendNodeId", backendNodeId);
        if (!objectGroup.empty()) params.set("objectGroup", objectGroup);
        if (executionContextId > 0) params.set("executionContextId", executionContextId);
        return call("resolveNode", params);
    }

    
    CDPResponse performSearch(const std::string& query, bool includeUserAgentShadowDOM = false) {
        Params params;
        params.set("query", query);
        if (includeUserAgentShadowDOM) params.set("includeUserAgentShadowDOM", true);
        return call("performSearch", params);
    }

    CDPResponse getSearchResults(const std::string& searchId, int fromIndex, int toIndex) {
        return call("getSearchResults", Params()
            .set("searchId", searchId)
            .set("fromIndex", fromIndex)
            .set("toIndex", toIndex));
    }

    CDPResponse discardSearchResults(const std::string& searchId) {
        return call("discardSearchResults", Params().set("searchId", searchId));
    }

    
    CDPResponse collectClassNamesFromSubtree(int nodeId) {
        return call("collectClassNamesFromSubtree", Params().set("nodeId", nodeId));
    }

    
    CDPResponse pushNodeByPathToFrontend(const std::string& path) {
        return call("pushNodeByPathToFrontend", Params().set("path", path));
    }

    
    CDPResponse pushNodesByBackendIdsToFrontend(const std::vector<int>& backendNodeIds) {
        Params params;
        JsonArray ids;
        for (int id : backendNodeIds) ids.push_back(static_cast<double>(id));
        params.set("backendNodeIds", ids);
        return call("pushNodesByBackendIdsToFrontend", params);
    }

    
    CDPResponse markUndoableState() { return call("markUndoableState"); }
    CDPResponse undo() { return call("undo"); }
    CDPResponse redo() { return call("redo"); }

    
    CDPResponse getRelayoutBoundary(int nodeId) {
        return call("getRelayoutBoundary", Params().set("nodeId", nodeId));
    }

    
    CDPResponse getElementByRelation(int nodeId, const std::string& relation) {
        
        return call("getElementByRelation", Params()
            .set("nodeId", nodeId)
            .set("relation", relation));
    }

    
    CDPResponse getAnchorElement(int nodeId, const std::string& anchorSpecifier = "") {
        Params params;
        params.set("nodeId", nodeId);
        if (!anchorSpecifier.empty()) params.set("anchorSpecifier", anchorSpecifier);
        return call("getAnchorElement", params);
    }

    
    CDPResponse getTopLayerElements() {
        return call("getTopLayerElements");
    }

    
    CDPResponse getDetachedDomNodes() {
        return call("getDetachedDomNodes");
    }

    
    CDPResponse getContainerForNode(int nodeId,
                                     const std::string& containerName = "",
                                     const std::string& physicalAxes = "",
                                     const std::string& logicalAxes = "",
                                     bool queriesScrollState = false,
                                     bool queriesAnchored = false) {
        Params params;
        params.set("nodeId", nodeId);
        if (!containerName.empty()) params.set("containerName", containerName);
        if (!physicalAxes.empty()) params.set("physicalAxes", physicalAxes);
        if (!logicalAxes.empty()) params.set("logicalAxes", logicalAxes);
        if (queriesScrollState) params.set("queriesScrollState", true);
        if (queriesAnchored) params.set("queriesAnchored", true);
        return call("getContainerForNode", params);
    }

    
    CDPResponse getQueryingDescendantsForContainer(int nodeId) {
        return call("getQueryingDescendantsForContainer", Params().set("nodeId", nodeId));
    }

    
    CDPResponse setNodeStackTracesEnabled(bool enable) {
        return call("setNodeStackTracesEnabled", Params().set("enable", enable));
    }

    
    CDPResponse getNodeStackTraces(int nodeId) {
        return call("getNodeStackTraces", Params().set("nodeId", nodeId));
    }

    
    CDPResponse getFileInfo(const std::string& objectId) {
        return call("getFileInfo", Params().set("objectId", objectId));
    }

    
    CDPResponse setInspectedNode(int nodeId) {
        return call("setInspectedNode", Params().set("nodeId", nodeId));
    }

    
    CDPResponse requestNode(const std::string& objectId) {
        return call("requestNode", Params().set("objectId", objectId));
    }

    
    CDPResponse getNodesForSubtreeByStyle(int nodeId,
                                           const JsonValue& computedStyles,
                                           bool pierce = false) {
        Params params;
        params.set("nodeId", nodeId);
        params.set("computedStyles", computedStyles);
        if (pierce) params.set("pierce", true);
        return call("getNodesForSubtreeByStyle", params);
    }

    
    CDPResponse highlightNode(const HighlightConfig& highlightConfig,
                               int nodeId = 0,
                               int backendNodeId = 0,
                               const std::string& objectId = "",
                               const std::string& selector = "") {
        Params params;
        params.set("highlightConfig", highlightConfig.toJson());
        if (nodeId > 0) params.set("nodeId", nodeId);
        if (backendNodeId > 0) params.set("backendNodeId", backendNodeId);
        if (!objectId.empty()) params.set("objectId", objectId);
        if (!selector.empty()) params.set("selector", selector);
        return call("highlightNode", params);
    }

    CDPResponse highlightRect(int x, int y, int width, int height,
                               const RGBA& color = RGBA(),
                               const RGBA& outlineColor = RGBA()) {
        Params params;
        params.set("x", x);
        params.set("y", y);
        params.set("width", width);
        params.set("height", height);
        params.set("color", color.toJson());
        params.set("outlineColor", outlineColor.toJson());
        return call("highlightRect", params);
    }

    CDPResponse hideHighlight() {
        return call("hideHighlight");
    }

    
    CDPResponse disable() { return call("disable"); }

    
    CDPResponse forceShowPopover(int nodeId = 0, int backendNodeId = 0, const std::string& objectId = "") {
        Params params;
        if (nodeId > 0) params.set("nodeId", nodeId);
        if (backendNodeId > 0) params.set("backendNodeId", backendNodeId);
        if (!objectId.empty()) params.set("objectId", objectId);
        return call("forceShowPopover", params);
    }

    
    void onDocumentUpdated(std::function<void()> callback) {
        on("documentUpdated", [callback](const CDPEvent&) { callback(); });
    }

    void onSetChildNodes(std::function<void(int parentId, const JsonValue& nodes)> callback) {
        on("setChildNodes", [callback](const CDPEvent& event) {
            callback(event.params["parentId"].getInt(), event.params["nodes"]);
        });
    }

    void onAttributeModified(std::function<void(int nodeId, const std::string& name, const std::string& value)> callback) {
        on("attributeModified", [callback](const CDPEvent& event) {
            callback(
                event.params["nodeId"].getInt(),
                event.params["name"].getString(),
                event.params["value"].getString()
            );
        });
    }

    void onAttributeRemoved(std::function<void(int nodeId, const std::string& name)> callback) {
        on("attributeRemoved", [callback](const CDPEvent& event) {
            callback(event.params["nodeId"].getInt(), event.params["name"].getString());
        });
    }

    void onChildNodeCountUpdated(std::function<void(int nodeId, int childNodeCount)> callback) {
        on("childNodeCountUpdated", [callback](const CDPEvent& event) {
            callback(event.params["nodeId"].getInt(), event.params["childNodeCount"].getInt());
        });
    }

    void onChildNodeInserted(std::function<void(int parentNodeId, int previousNodeId, const JsonValue& node)> callback) {
        on("childNodeInserted", [callback](const CDPEvent& event) {
            callback(
                event.params["parentNodeId"].getInt(),
                event.params["previousNodeId"].getInt(),
                event.params["node"]
            );
        });
    }

    void onChildNodeRemoved(std::function<void(int parentNodeId, int nodeId)> callback) {
        on("childNodeRemoved", [callback](const CDPEvent& event) {
            callback(event.params["parentNodeId"].getInt(), event.params["nodeId"].getInt());
        });
    }
};

} 
