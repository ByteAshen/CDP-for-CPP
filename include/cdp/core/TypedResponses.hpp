

#pragma once

#include "Json.hpp"
#include "../protocol/CDPConnection.hpp"
#include <string>
#include <vector>
#include <optional>

namespace cdp {


template<typename Derived>
struct TypedResponse {
    bool success = false;
    std::string error;

    explicit operator bool() const { return success; }
    bool hasError() const { return !success; }
};


struct NavigateResponse : TypedResponse<NavigateResponse> {
    std::string frameId;
    std::string loaderId;
    std::string errorText;

    static NavigateResponse from(const CDPResponse& response) {
        NavigateResponse r;
        r.success = !response.hasError;
        if (response.hasError) {
            r.error = response.errorMessage;
            return r;
        }
        r.frameId = response.result.getStringAt("frameId", "");
        r.loaderId = response.result.getStringAt("loaderId", "");
        r.errorText = response.result.getStringAt("errorText", "");
        if (!r.errorText.empty()) {
            r.success = false;
            r.error = r.errorText;
        }
        return r;
    }
};


struct ScreenshotResponse : TypedResponse<ScreenshotResponse> {
    std::string data;  

    static ScreenshotResponse from(const CDPResponse& response) {
        ScreenshotResponse r;
        r.success = !response.hasError;
        if (response.hasError) {
            r.error = response.errorMessage;
            return r;
        }
        r.data = response.result.getStringAt("data", "");
        return r;
    }

    
    size_t estimatedBytes() const {
        return (data.size() * 3) / 4;
    }
};


struct EvaluateResponse : TypedResponse<EvaluateResponse> {
    std::string type;        
    std::string subtype;     
    std::string objectId;    
    std::string className;
    std::string description;
    JsonValue value;         
    bool hasException = false;
    std::string exceptionText;

    static EvaluateResponse from(const CDPResponse& response) {
        EvaluateResponse r;
        r.success = !response.hasError;
        if (response.hasError) {
            r.error = response.errorMessage;
            return r;
        }

        
        if (response.result.contains("exceptionDetails")) {
            r.hasException = true;
            r.success = false;
            const auto* details = response.result.find("exceptionDetails");
            if (details) {
                r.exceptionText = details->getStringAt("text", "");
                const auto* exc = details->find("exception");
                if (exc) {
                    auto desc = exc->getStringAt("description", "");
                    if (!desc.empty()) r.exceptionText = desc;
                }
            }
            r.error = r.exceptionText;
            return r;
        }

        
        const auto* result = response.result.find("result");
        if (result) {
            r.type = result->getStringAt("type", "");
            r.subtype = result->getStringAt("subtype", "");
            r.objectId = result->getStringAt("objectId", "");
            r.className = result->getStringAt("className", "");
            r.description = result->getStringAt("description", "");
            const auto* val = result->find("value");
            if (val) r.value = *val;
        }
        return r;
    }

    
    bool isUndefined() const { return type == "undefined"; }
    bool isNull() const { return type == "object" && subtype == "null"; }
    bool isBoolean() const { return type == "boolean"; }
    bool isNumber() const { return type == "number"; }
    bool isString() const { return type == "string"; }
    bool isObject() const { return type == "object" && subtype != "null"; }
    bool isFunction() const { return type == "function"; }
    bool isArray() const { return type == "object" && subtype == "array"; }
    bool isNode() const { return type == "object" && subtype == "node"; }

    
    bool asBool(bool def = false) const { return value.getBool(def); }
    int64_t asInt(int64_t def = 0) const { return value.getInt64(def); }
    double asDouble(double def = 0.0) const { return value.getNumber(def); }
    std::string asString(const std::string& def = "") const { return value.getString(def); }
};


struct DocumentResponse : TypedResponse<DocumentResponse> {
    int nodeId = 0;
    int backendNodeId = 0;
    std::string documentURL;
    std::string baseURL;

    static DocumentResponse from(const CDPResponse& response) {
        DocumentResponse r;
        r.success = !response.hasError;
        if (response.hasError) {
            r.error = response.errorMessage;
            return r;
        }
        const auto* root = response.result.find("root");
        if (root) {
            r.nodeId = root->getIntAt("nodeId", 0);
            r.backendNodeId = root->getIntAt("backendNodeId", 0);
            r.documentURL = root->getStringAt("documentURL", "");
            r.baseURL = root->getStringAt("baseURL", "");
        }
        return r;
    }
};


struct QuerySelectorResponse : TypedResponse<QuerySelectorResponse> {
    int nodeId = 0;

    static QuerySelectorResponse from(const CDPResponse& response) {
        QuerySelectorResponse r;
        r.success = !response.hasError;
        if (response.hasError) {
            r.error = response.errorMessage;
            return r;
        }
        r.nodeId = response.result.getIntAt("nodeId", 0);
        if (r.nodeId == 0) {
            r.success = false;
            r.error = "Element not found";
        }
        return r;
    }

    bool found() const { return nodeId != 0; }
};


struct QuerySelectorAllResponse : TypedResponse<QuerySelectorAllResponse> {
    std::vector<int> nodeIds;

    static QuerySelectorAllResponse from(const CDPResponse& response) {
        QuerySelectorAllResponse r;
        r.success = !response.hasError;
        if (response.hasError) {
            r.error = response.errorMessage;
            return r;
        }
        const auto* ids = response.result.find("nodeIds");
        if (ids && ids->isArray()) {
            for (size_t i = 0; i < ids->size(); ++i) {
                r.nodeIds.push_back((*ids)[i].getInt(0));
            }
        }
        return r;
    }

    size_t count() const { return nodeIds.size(); }
    bool empty() const { return nodeIds.empty(); }
};


struct BoxModelResponse : TypedResponse<BoxModelResponse> {
    
    double contentX = 0, contentY = 0, contentWidth = 0, contentHeight = 0;
    
    double paddingX = 0, paddingY = 0, paddingWidth = 0, paddingHeight = 0;
    
    double borderX = 0, borderY = 0, borderWidth = 0, borderHeight = 0;
    
    double marginX = 0, marginY = 0, marginWidth = 0, marginHeight = 0;

    static BoxModelResponse from(const CDPResponse& response) {
        BoxModelResponse r;
        r.success = !response.hasError;
        if (response.hasError) {
            r.error = response.errorMessage;
            return r;
        }

        const auto* model = response.result.find("model");
        if (!model) {
            r.success = false;
            r.error = "No box model in response";
            return r;
        }

        auto extractBox = [](const JsonValue* quad, double& x, double& y, double& w, double& h) {
            if (!quad || !quad->isArray() || quad->size() < 8) return;
            x = (*quad)[0].getNumber();
            y = (*quad)[1].getNumber();
            double x2 = (*quad)[2].getNumber();
            double y2 = (*quad)[5].getNumber();
            w = x2 - x;
            h = y2 - y;
        };

        extractBox(model->find("content"), r.contentX, r.contentY, r.contentWidth, r.contentHeight);
        extractBox(model->find("padding"), r.paddingX, r.paddingY, r.paddingWidth, r.paddingHeight);
        extractBox(model->find("border"), r.borderX, r.borderY, r.borderWidth, r.borderHeight);
        extractBox(model->find("margin"), r.marginX, r.marginY, r.marginWidth, r.marginHeight);

        return r;
    }

    
    double centerX() const { return contentX + contentWidth / 2; }
    double centerY() const { return contentY + contentHeight / 2; }
};


struct Cookie {
    std::string name;
    std::string value;
    std::string domain;
    std::string path;
    double expires = -1;
    bool httpOnly = false;
    bool secure = false;
    std::string sameSite;

    static Cookie from(const JsonValue& json) {
        Cookie c;
        c.name = json.getStringAt("name", "");
        c.value = json.getStringAt("value", "");
        c.domain = json.getStringAt("domain", "");
        c.path = json.getStringAt("path", "");
        c.expires = json.getDoubleAt("expires", -1);
        c.httpOnly = json.getBoolAt("httpOnly", false);
        c.secure = json.getBoolAt("secure", false);
        c.sameSite = json.getStringAt("sameSite", "");
        return c;
    }
};

struct GetCookiesResponse : TypedResponse<GetCookiesResponse> {
    std::vector<Cookie> cookies;

    static GetCookiesResponse from(const CDPResponse& response) {
        GetCookiesResponse r;
        r.success = !response.hasError;
        if (response.hasError) {
            r.error = response.errorMessage;
            return r;
        }

        const auto* cookieList = response.result.find("cookies");
        if (cookieList && cookieList->isArray()) {
            for (size_t i = 0; i < cookieList->size(); ++i) {
                r.cookies.push_back(Cookie::from((*cookieList)[i]));
            }
        }
        return r;
    }

    
    std::optional<Cookie> find(const std::string& name) const {
        for (const auto& c : cookies) {
            if (c.name == name) return c;
        }
        return std::nullopt;
    }
};


struct Frame {
    std::string id;
    std::string parentId;
    std::string loaderId;
    std::string name;
    std::string url;
    std::string securityOrigin;
    std::string mimeType;

    static Frame from(const JsonValue& json) {
        Frame f;
        f.id = json.getStringAt("id", "");
        f.parentId = json.getStringAt("parentId", "");
        f.loaderId = json.getStringAt("loaderId", "");
        f.name = json.getStringAt("name", "");
        f.url = json.getStringAt("url", "");
        f.securityOrigin = json.getStringAt("securityOrigin", "");
        f.mimeType = json.getStringAt("mimeType", "");
        return f;
    }
};

struct FrameTreeResponse : TypedResponse<FrameTreeResponse> {
    Frame frame;
    std::vector<Frame> childFrames;

    static FrameTreeResponse from(const CDPResponse& response) {
        FrameTreeResponse r;
        r.success = !response.hasError;
        if (response.hasError) {
            r.error = response.errorMessage;
            return r;
        }

        const auto* frameTree = response.result.find("frameTree");
        if (frameTree) {
            const auto* frameJson = frameTree->find("frame");
            if (frameJson) {
                r.frame = Frame::from(*frameJson);
            }

            const auto* children = frameTree->find("childFrames");
            if (children && children->isArray()) {
                for (size_t i = 0; i < children->size(); ++i) {
                    const auto* childFrame = (*children)[i].find("frame");
                    if (childFrame) {
                        r.childFrames.push_back(Frame::from(*childFrame));
                    }
                }
            }
        }
        return r;
    }

    
    std::string mainFrameId() const { return frame.id; }
};

} 
