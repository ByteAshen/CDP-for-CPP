#pragma once

#include "Result.hpp"
#include "../protocol/CDPClient.hpp"
#include <string>
#include <vector>
#include <optional>

namespace cdp {
namespace highlevel {


struct BoundingBox {
    double x = 0;
    double y = 0;
    double width = 0;
    double height = 0;

    double centerX() const { return x + width / 2; }
    double centerY() const { return y + height / 2; }
    bool isValid() const { return width > 0 && height > 0; }
};


class Page;


class ElementHandle {
public:
    ElementHandle(CDPClient& client, int nodeId, int backendNodeId = 0)
        : client_(client), nodeId_(nodeId), backendNodeId_(backendNodeId) {}

    
    bool isValid() const { return nodeId_ > 0; }
    explicit operator bool() const { return isValid(); }

    
    Result<bool> isAttached();

    
    Result<void> ensureAttached();

    
    int nodeId() const { return nodeId_; }
    int backendNodeId() const { return backendNodeId_; }

    
    Result<void> click();

    
    Result<void> type(const std::string& text);

    
    Result<void> focus();

    
    Result<void> hover();

    
    Result<std::string> textContent();

    
    Result<std::string> innerHTML();

    
    Result<std::string> outerHTML();

    
    Result<std::string> getAttribute(const std::string& name);

    
    Result<std::string> getProperty(const std::string& name);

    
    Result<std::string> value();

    
    Result<void> setValue(const std::string& value);

    
    Result<bool> isVisible();

    
    Result<bool> isEnabled();

    
    Result<bool> isChecked();

    
    Result<BoundingBox> boundingBox();

    
    Result<void> scrollIntoView();

    
    Result<ElementHandle> querySelector(const std::string& selector);

    
    Result<std::vector<ElementHandle>> querySelectorAll(const std::string& selector);

    
    Result<std::vector<uint8_t>> screenshot();

    
    Result<JsonValue> evaluate(const std::string& expression);

private:
    Result<BoundingBox> getClickablePoint();
    Result<std::string> resolveToRemoteObjectId();

    CDPClient& client_;
    int nodeId_;
    int backendNodeId_;
    std::string remoteObjectId_;  
};

} 
} 
