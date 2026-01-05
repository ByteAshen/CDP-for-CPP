#pragma once

#include "Domain.hpp"

namespace cdp {


class Accessibility : public Domain {
public:
    explicit Accessibility(CDPConnection& connection) : Domain(connection, "Accessibility") {}

    
    CDPResponse enable() { return call("enable"); }

    
    CDPResponse disable() { return call("disable"); }

    
    CDPResponse getPartialAXTree(int nodeId = -1,
                                   int backendNodeId = -1,
                                   const std::string& objectId = "",
                                   bool fetchRelatives = false) {
        Params params;
        if (nodeId >= 0) params.set("nodeId", nodeId);
        if (backendNodeId >= 0) params.set("backendNodeId", backendNodeId);
        if (!objectId.empty()) params.set("objectId", objectId);
        if (fetchRelatives) params.set("fetchRelatives", true);
        return call("getPartialAXTree", params);
    }

    
    CDPResponse getFullAXTree(int depth = -1,
                               const std::string& frameId = "") {
        Params params;
        if (depth >= 0) params.set("depth", depth);
        if (!frameId.empty()) params.set("frameId", frameId);
        return call("getFullAXTree", params);
    }

    
    CDPResponse getRootAXNode(const std::string& frameId = "") {
        Params params;
        if (!frameId.empty()) params.set("frameId", frameId);
        return call("getRootAXNode", params);
    }

    
    CDPResponse getAXNodeAndAncestors(int nodeId = -1,
                                       int backendNodeId = -1,
                                       const std::string& objectId = "") {
        Params params;
        if (nodeId >= 0) params.set("nodeId", nodeId);
        if (backendNodeId >= 0) params.set("backendNodeId", backendNodeId);
        if (!objectId.empty()) params.set("objectId", objectId);
        return call("getAXNodeAndAncestors", params);
    }

    
    CDPResponse getChildAXNodes(const std::string& id,
                                 const std::string& frameId = "") {
        Params params;
        params.set("id", id);
        if (!frameId.empty()) params.set("frameId", frameId);
        return call("getChildAXNodes", params);
    }

    
    CDPResponse queryAXTree(int nodeId = -1,
                             int backendNodeId = -1,
                             const std::string& objectId = "",
                             const std::string& accessibleName = "",
                             const std::string& role = "") {
        Params params;
        if (nodeId >= 0) params.set("nodeId", nodeId);
        if (backendNodeId >= 0) params.set("backendNodeId", backendNodeId);
        if (!objectId.empty()) params.set("objectId", objectId);
        if (!accessibleName.empty()) params.set("accessibleName", accessibleName);
        if (!role.empty()) params.set("role", role);
        return call("queryAXTree", params);
    }

    
    void onLoadComplete(std::function<void(const JsonValue& root)> callback) {
        on("loadComplete", [callback](const CDPEvent& event) {
            callback(event.params["root"]);
        });
    }

    void onNodesUpdated(std::function<void(const JsonValue& nodes)> callback) {
        on("nodesUpdated", [callback](const CDPEvent& event) {
            callback(event.params["nodes"]);
        });
    }
};

} 
