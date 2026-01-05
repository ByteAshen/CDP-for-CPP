

#include "cdp/highlevel/ElementHandle.hpp"
#include "cdp/core/Base64.hpp"
#include <cmath>

namespace cdp {
namespace highlevel {


static bool isStaleElementError(const CDPResponse& resp) {
    if (!resp.hasError) return false;

    
    const std::string& msg = resp.errorMessage;
    return msg.find("Could not find node") != std::string::npos ||
           msg.find("No node with given id found") != std::string::npos ||
           msg.find("Node with given id does not belong") != std::string::npos ||
           msg.find("detached") != std::string::npos ||
           msg.find("not found") != std::string::npos;
}


static Error toTypedError(const CDPResponse& resp) {
    if (isStaleElementError(resp)) {
        return Error::elementStale(resp.errorMessage);
    }
    return Error(resp.errorCode, resp.errorMessage);
}

Result<bool> ElementHandle::isAttached() {
    
    auto resp = client_.DOM.describeNode(nodeId_);
    if (resp.hasError) {
        if (isStaleElementError(resp)) {
            return Result<bool>(false);
        }
        return Result<bool>::failure(toTypedError(resp));
    }
    return Result<bool>(true);
}

Result<void> ElementHandle::ensureAttached() {
    auto attached = isAttached();
    if (!attached) {
        return Result<void>::failure(attached.error());
    }
    if (!attached.value()) {
        return Result<void>::failure(Error::elementDetached());
    }
    return Result<void>::success();
}

Result<void> ElementHandle::click() {
    
    auto attachCheck = ensureAttached();
    if (!attachCheck) return attachCheck;

    
    auto scrollResult = scrollIntoView();
    if (!scrollResult) return scrollResult;

    
    auto boundsResult = getClickablePoint();
    if (!boundsResult) return Result<void>::failure(boundsResult.error());

    double x = boundsResult->centerX();
    double y = boundsResult->centerY();

    
    auto moveResp = client_.Input.dispatchMouseEvent("mouseMoved", x, y);
    if (moveResp.hasError) {
        return Result<void>::failure(moveResp.errorCode, moveResp.errorMessage);
    }

    auto downResp = client_.Input.dispatchMouseEvent("mousePressed", x, y, 0, 0, MouseButton::Left, 0, 1);
    if (downResp.hasError) {
        return Result<void>::failure(downResp.errorCode, downResp.errorMessage);
    }

    auto upResp = client_.Input.dispatchMouseEvent("mouseReleased", x, y, 0, 0, MouseButton::Left, 0, 1);
    if (upResp.hasError) {
        return Result<void>::failure(upResp.errorCode, upResp.errorMessage);
    }

    return Result<void>::success();
}

Result<void> ElementHandle::type(const std::string& text) {
    
    auto attachCheck = ensureAttached();
    if (!attachCheck) return attachCheck;

    
    auto focusResult = focus();
    if (!focusResult) return focusResult;

    
    auto resp = client_.Input.insertText(text);
    if (resp.hasError) {
        return Result<void>::failure(resp.errorCode, resp.errorMessage);
    }

    return Result<void>::success();
}

Result<void> ElementHandle::focus() {
    
    auto attachCheck = ensureAttached();
    if (!attachCheck) return attachCheck;

    auto resp = client_.DOM.focus(nodeId_);
    if (resp.hasError) {
        return Result<void>::failure(toTypedError(resp));
    }
    return Result<void>::success();
}

Result<void> ElementHandle::hover() {
    
    auto attachCheck = ensureAttached();
    if (!attachCheck) return attachCheck;

    auto scrollResult = scrollIntoView();
    if (!scrollResult) return scrollResult;

    auto boundsResult = getClickablePoint();
    if (!boundsResult) return Result<void>::failure(boundsResult.error());

    double x = boundsResult->centerX();
    double y = boundsResult->centerY();

    auto resp = client_.Input.dispatchMouseEvent("mouseMoved", x, y);
    if (resp.hasError) {
        return Result<void>::failure(resp.errorCode, resp.errorMessage);
    }

    return Result<void>::success();
}

Result<std::string> ElementHandle::textContent() {
    
    auto objResult = resolveToRemoteObjectId();
    if (!objResult) return Result<std::string>::failure(objResult.error());

    auto resp = client_.Runtime.callFunctionOn(
        "function() { return this.textContent || ''; }",
        objResult.value(),
        {},     
        false,  
        true    
    );

    if (resp.hasError) {
        return Result<std::string>::failure(resp.errorCode, resp.errorMessage);
    }

    auto& result = resp.result["result"];
    if (result["type"].getString() == "string") {
        return Result<std::string>(result["value"].getString());
    }

    return Result<std::string>("");
}

Result<std::string> ElementHandle::innerHTML() {
    auto objResult = resolveToRemoteObjectId();
    if (!objResult) return Result<std::string>::failure(objResult.error());

    auto resp = client_.Runtime.callFunctionOn(
        "function() { return this.innerHTML || ''; }",
        objResult.value(),
        {},     
        false,  
        true    
    );

    if (resp.hasError) {
        return Result<std::string>::failure(resp.errorCode, resp.errorMessage);
    }

    return Result<std::string>(resp.result["result"]["value"].getString());
}

Result<std::string> ElementHandle::outerHTML() {
    
    auto attachCheck = ensureAttached();
    if (!attachCheck) return Result<std::string>::failure(attachCheck.error());

    auto resp = client_.DOM.getOuterHTML(nodeId_);
    if (resp.hasError) {
        return Result<std::string>::failure(toTypedError(resp));
    }

    return Result<std::string>(resp.result["outerHTML"].getString());
}

Result<std::string> ElementHandle::getAttribute(const std::string& name) {
    
    auto attachCheck = ensureAttached();
    if (!attachCheck) return Result<std::string>::failure(attachCheck.error());

    auto resp = client_.DOM.getAttributes(nodeId_);
    if (resp.hasError) {
        return Result<std::string>::failure(toTypedError(resp));
    }

    auto& attrs = resp.result["attributes"];
    if (attrs.isArray()) {
        for (size_t i = 0; i + 1 < attrs.size(); i += 2) {
            if (attrs[i].getString() == name) {
                return Result<std::string>(attrs[i + 1].getString());
            }
        }
    }

    return Result<std::string>::failure("Attribute not found: " + name);
}

Result<std::string> ElementHandle::getProperty(const std::string& name) {
    auto objResult = resolveToRemoteObjectId();
    if (!objResult) return Result<std::string>::failure(objResult.error());

    std::string fn = "function() { return String(this." + name + " || ''); }";
    auto resp = client_.Runtime.callFunctionOn(fn, objResult.value(), {}, false, true);

    if (resp.hasError) {
        return Result<std::string>::failure(resp.errorCode, resp.errorMessage);
    }

    return Result<std::string>(resp.result["result"]["value"].getString());
}

Result<std::string> ElementHandle::value() {
    return getProperty("value");
}

Result<void> ElementHandle::setValue(const std::string& value) {
    auto objResult = resolveToRemoteObjectId();
    if (!objResult) return Result<void>::failure(objResult.error());

    
    std::string escaped;
    for (char c : value) {
        switch (c) {
            case '\\': escaped += "\\\\"; break;
            case '\'': escaped += "\\'"; break;
            case '\n': escaped += "\\n"; break;
            case '\r': escaped += "\\r"; break;
            case '\t': escaped += "\\t"; break;
            default: escaped += c;
        }
    }

    std::string fn = "function() { this.value = '" + escaped + "'; "
                     "this.dispatchEvent(new Event('input', {bubbles: true})); "
                     "this.dispatchEvent(new Event('change', {bubbles: true})); }";

    auto resp = client_.Runtime.callFunctionOn(fn, objResult.value());

    if (resp.hasError) {
        return Result<void>::failure(resp.errorCode, resp.errorMessage);
    }

    return Result<void>::success();
}

Result<bool> ElementHandle::isVisible() {
    auto objResult = resolveToRemoteObjectId();
    if (!objResult) return Result<bool>::failure(objResult.error());

    std::string fn = R"(function() {
        const rect = this.getBoundingClientRect();
        const style = window.getComputedStyle(this);
        return rect.width > 0 && rect.height > 0 &&
               style.visibility !== 'hidden' &&
               style.display !== 'none' &&
               style.opacity !== '0';
    })";

    auto resp = client_.Runtime.callFunctionOn(fn, objResult.value(), {}, false, true);

    if (resp.hasError) {
        return Result<bool>::failure(resp.errorCode, resp.errorMessage);
    }

    return Result<bool>(resp.result["result"]["value"].getBool());
}

Result<bool> ElementHandle::isEnabled() {
    auto objResult = resolveToRemoteObjectId();
    if (!objResult) return Result<bool>::failure(objResult.error());

    auto resp = client_.Runtime.callFunctionOn(
        "function() { return !this.disabled; }",
        objResult.value(),
        {},     
        false,  
        true    
    );

    if (resp.hasError) {
        return Result<bool>::failure(resp.errorCode, resp.errorMessage);
    }

    return Result<bool>(resp.result["result"]["value"].getBool());
}

Result<bool> ElementHandle::isChecked() {
    auto objResult = resolveToRemoteObjectId();
    if (!objResult) return Result<bool>::failure(objResult.error());

    auto resp = client_.Runtime.callFunctionOn(
        "function() { return !!this.checked; }",
        objResult.value(),
        {},     
        false,  
        true    
    );

    if (resp.hasError) {
        return Result<bool>::failure(resp.errorCode, resp.errorMessage);
    }

    return Result<bool>(resp.result["result"]["value"].getBool());
}

Result<BoundingBox> ElementHandle::boundingBox() {
    
    auto attachCheck = ensureAttached();
    if (!attachCheck) return Result<BoundingBox>::failure(attachCheck.error());

    return getClickablePoint();
}

Result<void> ElementHandle::scrollIntoView() {
    
    auto attachCheck = ensureAttached();
    if (!attachCheck) return attachCheck;

    auto resp = client_.DOM.scrollIntoViewIfNeeded(nodeId_);
    if (resp.hasError) {
        return Result<void>::failure(toTypedError(resp));
    }
    return Result<void>::success();
}

Result<ElementHandle> ElementHandle::querySelector(const std::string& selector) {
    
    auto attachCheck = ensureAttached();
    if (!attachCheck) return Result<ElementHandle>::failure(attachCheck.error());

    auto resp = client_.DOM.querySelector(nodeId_, selector);
    if (resp.hasError) {
        return Result<ElementHandle>::failure(toTypedError(resp));
    }

    int childNodeId = resp.result["nodeId"].getInt();
    if (childNodeId == 0) {
        return Result<ElementHandle>::failure(Error::elementNotFound(selector));
    }

    return Result<ElementHandle>(ElementHandle(client_, childNodeId));
}

Result<std::vector<ElementHandle>> ElementHandle::querySelectorAll(const std::string& selector) {
    
    auto attachCheck = ensureAttached();
    if (!attachCheck) return Result<std::vector<ElementHandle>>::failure(attachCheck.error());

    auto resp = client_.DOM.querySelectorAll(nodeId_, selector);
    if (resp.hasError) {
        return Result<std::vector<ElementHandle>>::failure(toTypedError(resp));
    }

    std::vector<ElementHandle> elements;
    auto& nodeIds = resp.result["nodeIds"];
    if (nodeIds.isArray()) {
        for (size_t i = 0; i < nodeIds.size(); ++i) {
            int childNodeId = nodeIds[i].getInt();
            if (childNodeId > 0) {
                elements.emplace_back(client_, childNodeId);
            }
        }
    }

    return Result<std::vector<ElementHandle>>(std::move(elements));
}

Result<std::vector<uint8_t>> ElementHandle::screenshot() {
    
    auto attachCheck = ensureAttached();
    if (!attachCheck) return Result<std::vector<uint8_t>>::failure(attachCheck.error());

    auto scrollResult = scrollIntoView();
    if (!scrollResult) return Result<std::vector<uint8_t>>::failure(scrollResult.error());

    auto boundsResult = getClickablePoint();
    if (!boundsResult) return Result<std::vector<uint8_t>>::failure(boundsResult.error());

    auto& bounds = boundsResult.value();

    
    cdp::Viewport viewport;
    viewport.x = bounds.x;
    viewport.y = bounds.y;
    viewport.width = bounds.width;
    viewport.height = bounds.height;
    viewport.scale = 1.0;

    auto resp = client_.Page.captureScreenshot("png", 100, &viewport);
    if (resp.hasError) {
        return Result<std::vector<uint8_t>>::failure(resp.errorCode, resp.errorMessage);
    }

    std::string base64Data = resp.result["data"].getString();
    return Result<std::vector<uint8_t>>(Base64::decode(base64Data));
}

Result<JsonValue> ElementHandle::evaluate(const std::string& expression) {
    
    auto attachCheck = ensureAttached();
    if (!attachCheck) return Result<JsonValue>::failure(attachCheck.error());

    auto objResult = resolveToRemoteObjectId();
    if (!objResult) return Result<JsonValue>::failure(objResult.error());

    std::string fn = "function() { return " + expression + "; }";
    auto resp = client_.Runtime.callFunctionOn(fn, objResult.value(), {}, false, true);

    if (resp.hasError) {
        return Result<JsonValue>::failure(resp.errorCode, resp.errorMessage);
    }

    return Result<JsonValue>(resp.result["result"]["value"]);
}

Result<BoundingBox> ElementHandle::getClickablePoint() {
    auto resp = client_.DOM.getContentQuads(nodeId_);
    if (resp.hasError) {
        return Result<BoundingBox>::failure(toTypedError(resp));
    }

    auto& quads = resp.result["quads"];
    if (!quads.isArray() || quads.size() == 0) {
        return Result<BoundingBox>::failure(
            Error(ErrorCode::ElementNotVisible, "Element has no visible quads"));
    }

    
    auto& quad = quads[0];
    if (!quad.isArray() || quad.size() < 8) {
        return Result<BoundingBox>::failure("Invalid quad data");
    }

    double x1 = quad[0].asNumber();
    double y1 = quad[1].asNumber();
    double x2 = quad[2].asNumber();
    double y2 = quad[3].asNumber();
    double x3 = quad[4].asNumber();
    double y3 = quad[5].asNumber();
    double x4 = quad[6].asNumber();
    double y4 = quad[7].asNumber();

    
    double minX = std::fmin(std::fmin(x1, x2), std::fmin(x3, x4));
    double maxX = std::fmax(std::fmax(x1, x2), std::fmax(x3, x4));
    double minY = std::fmin(std::fmin(y1, y2), std::fmin(y3, y4));
    double maxY = std::fmax(std::fmax(y1, y2), std::fmax(y3, y4));

    BoundingBox box;
    box.x = minX;
    box.y = minY;
    box.width = maxX - minX;
    box.height = maxY - minY;

    return Result<BoundingBox>(box);
}

Result<std::string> ElementHandle::resolveToRemoteObjectId() {
    if (!remoteObjectId_.empty()) {
        
        return Result<std::string>(remoteObjectId_);
    }

    auto resp = client_.DOM.resolveNode(nodeId_);
    if (resp.hasError) {
        return Result<std::string>::failure(toTypedError(resp));
    }

    remoteObjectId_ = resp.result["object"]["objectId"].getString();
    if (remoteObjectId_.empty()) {
        return Result<std::string>::failure("Failed to resolve node to remote object");
    }

    return Result<std::string>(remoteObjectId_);
}

} 
} 
