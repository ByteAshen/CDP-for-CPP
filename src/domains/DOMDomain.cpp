

#include "cdp/domains/DOMDomain.hpp"

namespace cdp {

DOMNode DOMNode::fromJson(const JsonValue& json) {
    DOMNode node;
    node.nodeId = json["nodeId"].getInt();
    node.parentId = json["parentId"].getInt();
    node.backendNodeId = json["backendNodeId"].getInt();
    node.nodeType = json["nodeType"].getInt();
    node.nodeName = json["nodeName"].getString();
    node.localName = json["localName"].getString();
    node.nodeValue = json["nodeValue"].getString();
    node.childNodeCount = json["childNodeCount"].getInt();
    node.documentURL = json["documentURL"].getString();
    node.baseURL = json["baseURL"].getString();
    node.publicId = json["publicId"].getString();
    node.systemId = json["systemId"].getString();
    node.frameId = json["frameId"].getString();

    if (json["attributes"].isArray()) {
        for (size_t i = 0; i < json["attributes"].size(); ++i) {
            node.attributes.push_back(json["attributes"][i].getString());
        }
    }

    if (json["children"].isArray()) {
        for (size_t i = 0; i < json["children"].size(); ++i) {
            node.children.push_back(DOMNode::fromJson(json["children"][i]));
        }
    }

    return node;
}

std::string DOMNode::getAttribute(const std::string& name) const {
    for (size_t i = 0; i + 1 < attributes.size(); i += 2) {
        if (attributes[i] == name) {
            return attributes[i + 1];
        }
    }
    return "";
}

BoxModel BoxModel::fromJson(const JsonValue& json) {
    BoxModel model;

    auto parseQuad = [](const JsonValue& arr) -> std::vector<double> {
        std::vector<double> result;
        if (arr.isArray()) {
            for (size_t i = 0; i < arr.size(); ++i) {
                result.push_back(arr[i].getNumber());
            }
        }
        return result;
    };

    model.content = parseQuad(json["content"]);
    model.padding = parseQuad(json["padding"]);
    model.border = parseQuad(json["border"]);
    model.margin = parseQuad(json["margin"]);
    model.width = json["width"].getInt();
    model.height = json["height"].getInt();

    return model;
}

JsonValue RGBA::toJson() const {
    JsonObject obj;
    obj["r"] = r;
    obj["g"] = g;
    obj["b"] = b;
    obj["a"] = a;
    return obj;
}

DOMNode DOMDomain::getDocument(int depth, bool pierce) {
    JsonObject params;
    params["depth"] = depth;
    params["pierce"] = pierce;

    auto response = sendCommand("DOM.getDocument", params);
    if (response.isSuccess()) {
        return DOMNode::fromJson(response.result["root"]);
    }
    return {};
}

int DOMDomain::querySelector(int nodeId, const std::string& selector) {
    JsonObject params;
    params["nodeId"] = nodeId;
    params["selector"] = selector;

    auto response = sendCommand("DOM.querySelector", params);
    if (response.isSuccess()) {
        return response.result["nodeId"].getInt();
    }
    return 0;
}

std::vector<int> DOMDomain::querySelectorAll(int nodeId, const std::string& selector) {
    JsonObject params;
    params["nodeId"] = nodeId;
    params["selector"] = selector;

    auto response = sendCommand("DOM.querySelectorAll", params);
    std::vector<int> nodeIds;

    if (response.isSuccess() && response.result["nodeIds"].isArray()) {
        for (size_t i = 0; i < response.result["nodeIds"].size(); ++i) {
            nodeIds.push_back(response.result["nodeIds"][i].getInt());
        }
    }

    return nodeIds;
}

DOMDomain::XPathResult DOMDomain::performSearch(const std::string& query, bool includeUserAgentShadowDOM) {
    JsonObject params;
    params["query"] = query;
    params["includeUserAgentShadowDOM"] = includeUserAgentShadowDOM;

    auto response = sendCommand("DOM.performSearch", params);
    XPathResult result;
    
    return result;
}

std::vector<int> DOMDomain::getSearchResults(const std::string& searchId, int fromIndex, int toIndex) {
    JsonObject params;
    params["searchId"] = searchId;
    params["fromIndex"] = fromIndex;
    params["toIndex"] = toIndex;

    auto response = sendCommand("DOM.getSearchResults", params);
    std::vector<int> nodeIds;

    if (response.isSuccess() && response.result["nodeIds"].isArray()) {
        for (size_t i = 0; i < response.result["nodeIds"].size(); ++i) {
            nodeIds.push_back(response.result["nodeIds"][i].getInt());
        }
    }

    return nodeIds;
}

CDPResponse DOMDomain::discardSearchResults(const std::string& searchId) {
    JsonObject params;
    params["searchId"] = searchId;
    return sendCommand("DOM.discardSearchResults", params);
}

DOMNode DOMDomain::describeNode(int nodeId, int backendNodeId, const std::string& objectId,
                                 int depth, bool pierce) {
    JsonObject params;
    if (nodeId > 0) params["nodeId"] = nodeId;
    if (backendNodeId > 0) params["backendNodeId"] = backendNodeId;
    if (!objectId.empty()) params["objectId"] = objectId;
    params["depth"] = depth;
    params["pierce"] = pierce;

    auto response = sendCommand("DOM.describeNode", params);
    if (response.isSuccess()) {
        return DOMNode::fromJson(response.result["node"]);
    }
    return {};
}

std::string DOMDomain::getOuterHTML(int nodeId, int backendNodeId, const std::string& objectId) {
    JsonObject params;
    if (nodeId > 0) params["nodeId"] = nodeId;
    if (backendNodeId > 0) params["backendNodeId"] = backendNodeId;
    if (!objectId.empty()) params["objectId"] = objectId;

    auto response = sendCommand("DOM.getOuterHTML", params);
    if (response.isSuccess()) {
        return response.result["outerHTML"].getString();
    }
    return "";
}

CDPResponse DOMDomain::setOuterHTML(int nodeId, const std::string& outerHTML) {
    JsonObject params;
    params["nodeId"] = nodeId;
    params["outerHTML"] = outerHTML;
    return sendCommand("DOM.setOuterHTML", params);
}

int DOMDomain::setNodeValue(int nodeId, const std::string& value) {
    JsonObject params;
    params["nodeId"] = nodeId;
    params["value"] = value;
    sendCommand("DOM.setNodeValue", params);
    return nodeId;
}

CDPResponse DOMDomain::setNodeName(int nodeId, const std::string& name) {
    JsonObject params;
    params["nodeId"] = nodeId;
    params["name"] = name;
    return sendCommand("DOM.setNodeName", params);
}

CDPResponse DOMDomain::removeNode(int nodeId) {
    JsonObject params;
    params["nodeId"] = nodeId;
    return sendCommand("DOM.removeNode", params);
}

std::vector<std::string> DOMDomain::getAttributes(int nodeId) {
    JsonObject params;
    params["nodeId"] = nodeId;

    auto response = sendCommand("DOM.getAttributes", params);
    std::vector<std::string> attributes;

    if (response.isSuccess() && response.result["attributes"].isArray()) {
        for (size_t i = 0; i < response.result["attributes"].size(); ++i) {
            attributes.push_back(response.result["attributes"][i].getString());
        }
    }

    return attributes;
}

CDPResponse DOMDomain::setAttributeValue(int nodeId, const std::string& name, const std::string& value) {
    JsonObject params;
    params["nodeId"] = nodeId;
    params["name"] = name;
    params["value"] = value;
    return sendCommand("DOM.setAttributeValue", params);
}

CDPResponse DOMDomain::setAttributesAsText(int nodeId, const std::string& text, const std::string& name) {
    JsonObject params;
    params["nodeId"] = nodeId;
    params["text"] = text;
    if (!name.empty()) params["name"] = name;
    return sendCommand("DOM.setAttributesAsText", params);
}

CDPResponse DOMDomain::removeAttribute(int nodeId, const std::string& name) {
    JsonObject params;
    params["nodeId"] = nodeId;
    params["name"] = name;
    return sendCommand("DOM.removeAttribute", params);
}

CDPResponse DOMDomain::requestChildNodes(int nodeId, int depth, bool pierce) {
    JsonObject params;
    params["nodeId"] = nodeId;
    params["depth"] = depth;
    params["pierce"] = pierce;
    return sendCommand("DOM.requestChildNodes", params);
}

int DOMDomain::moveTo(int nodeId, int targetNodeId, int insertBeforeNodeId) {
    JsonObject params;
    params["nodeId"] = nodeId;
    params["targetNodeId"] = targetNodeId;
    if (insertBeforeNodeId > 0) params["insertBeforeNodeId"] = insertBeforeNodeId;

    auto response = sendCommand("DOM.moveTo", params);
    if (response.isSuccess()) {
        return response.result["nodeId"].getInt();
    }
    return 0;
}

int DOMDomain::copyTo(int nodeId, int targetNodeId, int insertBeforeNodeId) {
    JsonObject params;
    params["nodeId"] = nodeId;
    params["targetNodeId"] = targetNodeId;
    if (insertBeforeNodeId > 0) params["insertBeforeNodeId"] = insertBeforeNodeId;

    auto response = sendCommand("DOM.copyTo", params);
    if (response.isSuccess()) {
        return response.result["nodeId"].getInt();
    }
    return 0;
}

BoxModel DOMDomain::getBoxModel(int nodeId, int backendNodeId, const std::string& objectId) {
    JsonObject params;
    if (nodeId > 0) params["nodeId"] = nodeId;
    if (backendNodeId > 0) params["backendNodeId"] = backendNodeId;
    if (!objectId.empty()) params["objectId"] = objectId;

    auto response = sendCommand("DOM.getBoxModel", params);
    if (response.isSuccess()) {
        return BoxModel::fromJson(response.result["model"]);
    }
    return {};
}

std::vector<std::vector<double>> DOMDomain::getContentQuads(int nodeId, int backendNodeId,
                                                             const std::string& objectId) {
    JsonObject params;
    if (nodeId > 0) params["nodeId"] = nodeId;
    if (backendNodeId > 0) params["backendNodeId"] = backendNodeId;
    if (!objectId.empty()) params["objectId"] = objectId;

    auto response = sendCommand("DOM.getContentQuads", params);
    std::vector<std::vector<double>> quads;

    if (response.isSuccess() && response.result["quads"].isArray()) {
        for (size_t i = 0; i < response.result["quads"].size(); ++i) {
            std::vector<double> quad;
            const auto& quadJson = response.result["quads"][i];
            if (quadJson.isArray()) {
                for (size_t j = 0; j < quadJson.size(); ++j) {
                    quad.push_back(quadJson[j].getNumber());
                }
            }
            quads.push_back(quad);
        }
    }

    return quads;
}

int DOMDomain::getNodeForLocation(int x, int y, bool includeUserAgentShadowDOM, bool ignorePointerEventsNone) {
    JsonObject params;
    params["x"] = x;
    params["y"] = y;
    params["includeUserAgentShadowDOM"] = includeUserAgentShadowDOM;
    params["ignorePointerEventsNone"] = ignorePointerEventsNone;

    auto response = sendCommand("DOM.getNodeForLocation", params);
    if (response.isSuccess()) {
        return response.result["nodeId"].getInt();
    }
    return 0;
}

CDPResponse DOMDomain::focus(int nodeId, int backendNodeId, const std::string& objectId) {
    JsonObject params;
    if (nodeId > 0) params["nodeId"] = nodeId;
    if (backendNodeId > 0) params["backendNodeId"] = backendNodeId;
    if (!objectId.empty()) params["objectId"] = objectId;
    return sendCommand("DOM.focus", params);
}

CDPResponse DOMDomain::setFileInputFiles(const std::vector<std::string>& files, int nodeId,
                                          int backendNodeId, const std::string& objectId) {
    JsonObject params;
    JsonArray filesArray;
    for (const auto& file : files) {
        filesArray.push_back(file);
    }
    params["files"] = filesArray;
    if (nodeId > 0) params["nodeId"] = nodeId;
    if (backendNodeId > 0) params["backendNodeId"] = backendNodeId;
    if (!objectId.empty()) params["objectId"] = objectId;
    return sendCommand("DOM.setFileInputFiles", params);
}

CDPResponse DOMDomain::highlightNode(int nodeId, const RGBA& contentColor,
                                      const RGBA& paddingColor, const RGBA& borderColor,
                                      const RGBA& marginColor) {
    JsonObject params;
    JsonObject highlightConfig;
    highlightConfig["showInfo"] = true;
    highlightConfig["showRulers"] = false;
    highlightConfig["showExtensionLines"] = false;
    highlightConfig["contentColor"] = contentColor.toJson();
    highlightConfig["paddingColor"] = paddingColor.toJson();
    highlightConfig["borderColor"] = borderColor.toJson();
    highlightConfig["marginColor"] = marginColor.toJson();

    params["highlightConfig"] = highlightConfig;
    params["nodeId"] = nodeId;

    return sendCommand("Overlay.highlightNode", params);
}

CDPResponse DOMDomain::highlightRect(int x, int y, int width, int height,
                                      const RGBA& color, const RGBA& outlineColor) {
    JsonObject params;
    params["x"] = x;
    params["y"] = y;
    params["width"] = width;
    params["height"] = height;
    params["color"] = color.toJson();
    params["outlineColor"] = outlineColor.toJson();

    return sendCommand("Overlay.highlightRect", params);
}

CDPResponse DOMDomain::hideHighlight() {
    return sendCommand("Overlay.hideHighlight");
}

int DOMDomain::requestShadowRoot(int nodeId) {
    JsonObject params;
    params["nodeId"] = nodeId;

    
    auto node = describeNode(nodeId, 0, "", 1, true);
    
    return 0;
}

CDPResponse DOMDomain::scrollIntoViewIfNeeded(int nodeId, int backendNodeId, const std::string& objectId) {
    JsonObject params;
    if (nodeId > 0) params["nodeId"] = nodeId;
    if (backendNodeId > 0) params["backendNodeId"] = backendNodeId;
    if (!objectId.empty()) params["objectId"] = objectId;
    return sendCommand("DOM.scrollIntoViewIfNeeded", params);
}

std::pair<int, int> DOMDomain::getFrameOwner(const std::string& frameId) {
    JsonObject params;
    params["frameId"] = frameId;

    auto response = sendCommand("DOM.getFrameOwner", params);
    if (response.isSuccess()) {
        return {response.result["nodeId"].getInt(), response.result["backendNodeId"].getInt()};
    }
    return {0, 0};
}

std::string DOMDomain::resolveNode(int nodeId, int backendNodeId, const std::string& objectGroup) {
    JsonObject params;
    if (nodeId > 0) params["nodeId"] = nodeId;
    if (backendNodeId > 0) params["backendNodeId"] = backendNodeId;
    if (!objectGroup.empty()) params["objectGroup"] = objectGroup;

    auto response = sendCommand("DOM.resolveNode", params);
    if (response.isSuccess()) {
        return response.result["object"]["objectId"].getString();
    }
    return "";
}

void DOMDomain::onDocumentUpdated(std::function<void()> callback) {
    subscribeEvent("documentUpdated", [callback](const CDPEvent&) {
        callback();
    });
}

void DOMDomain::onSetChildNodes(std::function<void(int, const std::vector<DOMNode>&)> callback) {
    subscribeEvent("setChildNodes", [callback](const CDPEvent& event) {
        std::vector<DOMNode> nodes;
        if (event.params["nodes"].isArray()) {
            for (size_t i = 0; i < event.params["nodes"].size(); ++i) {
                nodes.push_back(DOMNode::fromJson(event.params["nodes"][i]));
            }
        }
        callback(event.params["parentId"].getInt(), nodes);
    });
}

void DOMDomain::onAttributeModified(std::function<void(int, const std::string&, const std::string&)> callback) {
    subscribeEvent("attributeModified", [callback](const CDPEvent& event) {
        callback(
            event.params["nodeId"].getInt(),
            event.params["name"].getString(),
            event.params["value"].getString()
        );
    });
}

void DOMDomain::onAttributeRemoved(std::function<void(int, const std::string&)> callback) {
    subscribeEvent("attributeRemoved", [callback](const CDPEvent& event) {
        callback(
            event.params["nodeId"].getInt(),
            event.params["name"].getString()
        );
    });
}

void DOMDomain::onChildNodeCountUpdated(std::function<void(int, int)> callback) {
    subscribeEvent("childNodeCountUpdated", [callback](const CDPEvent& event) {
        callback(
            event.params["nodeId"].getInt(),
            event.params["childNodeCount"].getInt()
        );
    });
}

void DOMDomain::onChildNodeInserted(std::function<void(int, int, const DOMNode&)> callback) {
    subscribeEvent("childNodeInserted", [callback](const CDPEvent& event) {
        callback(
            event.params["parentNodeId"].getInt(),
            event.params["previousNodeId"].getInt(),
            DOMNode::fromJson(event.params["node"])
        );
    });
}

void DOMDomain::onChildNodeRemoved(std::function<void(int, int)> callback) {
    subscribeEvent("childNodeRemoved", [callback](const CDPEvent& event) {
        callback(
            event.params["parentNodeId"].getInt(),
            event.params["nodeId"].getInt()
        );
    });
}

} 
