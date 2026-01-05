#pragma once

#include "BaseDomain.hpp"
#include <vector>
#include <optional>

namespace cdp {


struct DOMNode {
    int nodeId = 0;
    int parentId = 0;
    int backendNodeId = 0;
    int nodeType = 0;
    std::string nodeName;
    std::string localName;
    std::string nodeValue;
    int childNodeCount = 0;
    std::vector<DOMNode> children;
    std::vector<std::string> attributes;
    std::string documentURL;
    std::string baseURL;
    std::string publicId;
    std::string systemId;
    std::string frameId;

    static DOMNode fromJson(const JsonValue& json);
    std::string getAttribute(const std::string& name) const;
};


struct BoxModel {
    std::vector<double> content;
    std::vector<double> padding;
    std::vector<double> border;
    std::vector<double> margin;
    int width = 0;
    int height = 0;

    static BoxModel fromJson(const JsonValue& json);
};


struct RGBA {
    int r = 0;
    int g = 0;
    int b = 0;
    double a = 1.0;

    JsonValue toJson() const;
};


class DOMDomain : public BaseDomain {
public:
    using BaseDomain::BaseDomain;
    std::string domainName() const override { return "DOM"; }

    
    DOMNode getDocument(int depth = -1, bool pierce = false);

    
    int querySelector(int nodeId, const std::string& selector);
    std::vector<int> querySelectorAll(int nodeId, const std::string& selector);

    
    struct XPathResult {
        std::vector<int> nodeIds;
    };
    XPathResult performSearch(const std::string& query, bool includeUserAgentShadowDOM = false);
    std::vector<int> getSearchResults(const std::string& searchId, int fromIndex, int toIndex);
    CDPResponse discardSearchResults(const std::string& searchId);

    
    DOMNode describeNode(int nodeId = 0, int backendNodeId = 0, const std::string& objectId = "",
                          int depth = 1, bool pierce = false);
    std::string getOuterHTML(int nodeId = 0, int backendNodeId = 0, const std::string& objectId = "");
    CDPResponse setOuterHTML(int nodeId, const std::string& outerHTML);
    int setNodeValue(int nodeId, const std::string& value);
    CDPResponse setNodeName(int nodeId, const std::string& name);
    CDPResponse removeNode(int nodeId);

    
    std::vector<std::string> getAttributes(int nodeId);
    CDPResponse setAttributeValue(int nodeId, const std::string& name, const std::string& value);
    CDPResponse setAttributesAsText(int nodeId, const std::string& text, const std::string& name = "");
    CDPResponse removeAttribute(int nodeId, const std::string& name);

    
    CDPResponse requestChildNodes(int nodeId, int depth = 1, bool pierce = false);
    int moveTo(int nodeId, int targetNodeId, int insertBeforeNodeId = 0);
    int copyTo(int nodeId, int targetNodeId, int insertBeforeNodeId = 0);

    
    BoxModel getBoxModel(int nodeId = 0, int backendNodeId = 0, const std::string& objectId = "");

    
    std::vector<std::vector<double>> getContentQuads(int nodeId = 0, int backendNodeId = 0,
                                                       const std::string& objectId = "");

    
    int getNodeForLocation(int x, int y, bool includeUserAgentShadowDOM = false, bool ignorePointerEventsNone = false);

    
    CDPResponse focus(int nodeId = 0, int backendNodeId = 0, const std::string& objectId = "");

    
    CDPResponse setFileInputFiles(const std::vector<std::string>& files, int nodeId = 0,
                                   int backendNodeId = 0, const std::string& objectId = "");

    
    CDPResponse highlightNode(int nodeId, const RGBA& contentColor = {255, 0, 0, 50},
                               const RGBA& paddingColor = {0, 255, 0, 50},
                               const RGBA& borderColor = {0, 0, 255, 50},
                               const RGBA& marginColor = {255, 255, 0, 50});
    CDPResponse highlightRect(int x, int y, int width, int height,
                               const RGBA& color = {255, 0, 0, 50},
                               const RGBA& outlineColor = {255, 0, 0, 255});
    CDPResponse hideHighlight();

    
    int requestShadowRoot(int nodeId);

    
    CDPResponse scrollIntoViewIfNeeded(int nodeId = 0, int backendNodeId = 0,
                                        const std::string& objectId = "");

    
    std::pair<int, int> getFrameOwner(const std::string& frameId);

    
    std::string resolveNode(int nodeId = 0, int backendNodeId = 0,
                             const std::string& objectGroup = "");

    
    void onDocumentUpdated(std::function<void()> callback);
    void onSetChildNodes(std::function<void(int parentId, const std::vector<DOMNode>&)> callback);
    void onAttributeModified(std::function<void(int nodeId, const std::string& name,
                                                 const std::string& value)> callback);
    void onAttributeRemoved(std::function<void(int nodeId, const std::string& name)> callback);
    void onChildNodeCountUpdated(std::function<void(int nodeId, int childNodeCount)> callback);
    void onChildNodeInserted(std::function<void(int parentNodeId, int previousNodeId,
                                                 const DOMNode& node)> callback);
    void onChildNodeRemoved(std::function<void(int parentNodeId, int nodeId)> callback);
};

} 
