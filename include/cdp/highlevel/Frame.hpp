

#pragma once

#include "../protocol/CDPConnection.hpp"
#include "../domains/Domain.hpp"
#include "../core/TypedResponses.hpp"
#include <string>
#include <vector>
#include <memory>
#include <optional>
#include <chrono>
#include <thread>
#include <future>

namespace cdp {
namespace highlevel {


class Frame {
public:
    Frame(CDPConnection& conn, const std::string& frameId, const std::string& parentId = "")
        : connection_(conn)
        , frameId_(frameId)
        , parentId_(parentId)
    {}

    
    const std::string& id() const { return frameId_; }
    const std::string& parentId() const { return parentId_; }
    bool isMainFrame() const { return parentId_.empty(); }

    
    std::string url() const { return url_; }
    void setUrl(const std::string& url) { url_ = url; }

    
    std::string name() const { return name_; }
    void setName(const std::string& name) { name_ = name; }

    
    CDPResponse navigate(const std::string& url) {
        Params params;
        params.set("url", url);
        params.set("frameId", frameId_);
        return connection_.sendCommandSync("Page.navigate", params.build());
    }

    
    int findElement(const std::string& selector) {
        
        auto docResult = connection_.sendCommandSync("DOM.getDocument",
            Params().set("depth", -1).set("pierce", true).build());

        if (docResult.hasError) return 0;

        
        if (!isMainFrame()) {
            
            auto frameOwner = connection_.sendCommandSync("DOM.getFrameOwner",
                Params().set("frameId", frameId_).build());
            if (frameOwner.hasError) return 0;

            int backendNodeId = frameOwner.result.getIntAt("backendNodeId", 0);
            if (backendNodeId == 0) return 0;

            
            auto nodeResult = connection_.sendCommandSync("DOM.describeNode",
                Params().set("backendNodeId", backendNodeId).set("depth", 1).set("pierce", true).build());
            if (nodeResult.hasError) return 0;

            
            const auto* node = nodeResult.result.find("node");
            if (!node) return 0;

            const auto* contentDoc = node->find("contentDocument");
            if (!contentDoc) return 0;

            int docNodeId = contentDoc->getIntAt("nodeId", 0);
            if (docNodeId == 0) return 0;

            
            auto queryResult = connection_.sendCommandSync("DOM.querySelector",
                Params().set("nodeId", docNodeId).set("selector", selector).build());
            if (queryResult.hasError) return 0;
            return queryResult.result.getIntAt("nodeId", 0);
        }

        
        int rootId = docResult.result.getIntAt("root/nodeId", 0);
        if (rootId == 0) return 0;

        auto queryResult = connection_.sendCommandSync("DOM.querySelector",
            Params().set("nodeId", rootId).set("selector", selector).build());
        if (queryResult.hasError) return 0;
        return queryResult.result.getIntAt("nodeId", 0);
    }

    
    bool elementExists(const std::string& selector) {
        return findElement(selector) != 0;
    }

    
    std::string getElementText(const std::string& selector) {
        return evaluate("document.querySelector('" + selector + "')?.textContent || ''").asString();
    }

    
    std::string getElementAttribute(const std::string& selector, const std::string& attr) {
        return evaluate("document.querySelector('" + selector + "')?.getAttribute('" + attr + "') || ''").asString();
    }

    
    EvaluateResponse evaluate(const std::string& expression) {
        Params params;
        params.set("expression", expression);
        params.set("returnByValue", true);
        
        auto result = connection_.sendCommandSync("Runtime.evaluate", params.build());
        return EvaluateResponse::from(result);
    }

    
    std::string evalString(const std::string& expr, const std::string& def = "") {
        auto result = evaluate(expr);
        return result.success ? result.asString(def) : def;
    }

    
    int evalInt(const std::string& expr, int def = 0) {
        auto result = evaluate(expr);
        return result.success ? static_cast<int>(result.asInt(def)) : def;
    }

    
    bool evalBool(const std::string& expr, bool def = false) {
        auto result = evaluate(expr);
        return result.success ? result.asBool(def) : def;
    }

    
    std::string title() {
        return evalString("document.title", "");
    }

    
    std::string content() {
        return evalString("document.documentElement.outerHTML", "");
    }

    
    CDPResponse setContent(const std::string& html) {
        return connection_.sendCommandSync("Page.setDocumentContent",
            Params().set("frameId", frameId_).set("html", html).build());
    }

    
    std::vector<Frame> childFrames() {
        std::vector<Frame> children;

        auto result = connection_.sendCommandSync("Page.getFrameTree");
        if (result.hasError) return children;

        findChildFrames(result.result.find("frameTree"), children);
        return children;
    }

    
    std::optional<Frame> childFrame(const std::string& nameOrId) {
        for (auto& child : childFrames()) {
            if (child.name() == nameOrId || child.id() == nameOrId) {
                return child;
            }
        }
        return std::nullopt;
    }

    
    bool waitForSelector(const std::string& selector, int timeoutMs = 30000) {
        auto startTime = std::chrono::steady_clock::now();
        int interval = 100;

        while (true) {
            if (findElement(selector) != 0) return true;

            auto elapsed = std::chrono::steady_clock::now() - startTime;
            if (std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count() >= timeoutMs) {
                return false;
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(interval));
        }
    }

    
    bool waitForNavigation(int timeoutMs = 30000) {
        
        
        auto startUrl = url();
        auto startTime = std::chrono::steady_clock::now();

        while (true) {
            auto elapsed = std::chrono::steady_clock::now() - startTime;
            if (std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count() >= timeoutMs) {
                return false;
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(100));

            
            auto result = connection_.sendCommandSync("Page.getFrameTree");
            if (!result.hasError) {
                const auto* frameTree = result.result.find("frameTree");
                if (frameTree) {
                    const auto* frame = frameTree->find("frame");
                    if (frame && frame->getStringAt("id", "") == frameId_) {
                        std::string newUrl = frame->getStringAt("url", "");
                        if (!newUrl.empty() && newUrl != startUrl) {
                            return true;
                        }
                    }
                }
            }
        }
    }

    
    std::vector<int> findElements(const std::string& selector) {
        std::vector<int> nodeIds;

        auto docResult = connection_.sendCommandSync("DOM.getDocument",
            Params().set("depth", -1).set("pierce", true).build());
        if (docResult.hasError) return nodeIds;

        int rootId = docResult.result.getIntAt("root/nodeId", 0);
        if (rootId == 0) return nodeIds;

        auto queryResult = connection_.sendCommandSync("DOM.querySelectorAll",
            Params().set("nodeId", rootId).set("selector", selector).build());
        if (queryResult.hasError) return nodeIds;

        const auto* nodes = queryResult.result.find("nodeIds");
        if (nodes && nodes->isArray()) {
            for (size_t i = 0; i < nodes->size(); ++i) {
                nodeIds.push_back(static_cast<int>((*nodes)[i].getInt64(0)));
            }
        }
        return nodeIds;
    }

    
    int countElements(const std::string& selector) {
        return static_cast<int>(findElements(selector).size());
    }

    
    CDPResponse click(const std::string& selector) {
        int nodeId = findElement(selector);
        if (nodeId == 0) {
            CDPResponse err;
            err.hasError = true;
            err.errorMessage = "Element not found: " + selector;
            return err;
        }

        
        auto boxResult = connection_.sendCommandSync("DOM.getBoxModel",
            Params().set("nodeId", nodeId).build());
        if (boxResult.hasError) return boxResult;

        const auto* content = boxResult.result.getPath("model/content");
        if (!content || !content->isArray() || content->size() < 4) {
            CDPResponse err;
            err.hasError = true;
            err.errorMessage = "Could not get element bounds";
            return err;
        }

        double x = (content->asArray()[0].getNumber() + content->asArray()[2].getNumber()) / 2;
        double y = (content->asArray()[1].getNumber() + content->asArray()[5].getNumber()) / 2;

        
        connection_.sendCommandSync("Input.dispatchMouseEvent",
            Params().set("type", "mousePressed").set("x", x).set("y", y)
                    .set("button", "left").set("clickCount", 1).build());

        return connection_.sendCommandSync("Input.dispatchMouseEvent",
            Params().set("type", "mouseReleased").set("x", x).set("y", y)
                    .set("button", "left").set("clickCount", 1).build());
    }

    
    CDPResponse type(const std::string& selector, const std::string& text) {
        int nodeId = findElement(selector);
        if (nodeId == 0) {
            CDPResponse err;
            err.hasError = true;
            err.errorMessage = "Element not found: " + selector;
            return err;
        }

        
        auto focusResult = connection_.sendCommandSync("DOM.focus",
            Params().set("nodeId", nodeId).build());
        if (focusResult.hasError) return focusResult;

        
        return connection_.sendCommandSync("Input.insertText",
            Params().set("text", text).build());
    }

    
    CDPResponse setValue(const std::string& selector, const std::string& value) {
        
        auto result = evaluate("document.querySelector('" + selector + "').value = '" + value + "'");
        CDPResponse resp;
        resp.hasError = !result.success;
        resp.errorMessage = result.error;
        return resp;
    }

    
    std::string getValue(const std::string& selector) {
        return evalString("document.querySelector('" + selector + "')?.value || ''");
    }

    
    CDPResponse focus(const std::string& selector) {
        int nodeId = findElement(selector);
        if (nodeId == 0) {
            CDPResponse err;
            err.hasError = true;
            err.errorMessage = "Element not found: " + selector;
            return err;
        }
        return connection_.sendCommandSync("DOM.focus", Params().set("nodeId", nodeId).build());
    }

    
    bool isVisible(const std::string& selector) {
        return evalBool(
            "(function() {"
            "  var el = document.querySelector('" + selector + "');"
            "  if (!el) return false;"
            "  var style = window.getComputedStyle(el);"
            "  return style.display !== 'none' && style.visibility !== 'hidden';"
            "})()");
    }

private:
    void findChildFrames(const JsonValue* frameTree, std::vector<Frame>& out) {
        if (!frameTree) return;

        const auto* children = frameTree->find("childFrames");
        if (!children || !children->isArray()) return;

        for (size_t i = 0; i < children->size(); ++i) {
            const auto& child = (*children)[i];
            const auto* frame = child.find("frame");
            if (frame) {
                std::string id = frame->getStringAt("id", "");
                std::string parent = frame->getStringAt("parentId", "");
                std::string frameUrl = frame->getStringAt("url", "");
                std::string frameName = frame->getStringAt("name", "");

                if (!id.empty() && parent == frameId_) {
                    Frame f(connection_, id, parent);
                    f.setUrl(frameUrl);
                    f.setName(frameName);
                    out.push_back(std::move(f));
                }
            }
        }
    }

    CDPConnection& connection_;
    std::string frameId_;
    std::string parentId_;
    std::string url_;
    std::string name_;
};


class FrameManager {
public:
    explicit FrameManager(CDPConnection& conn)
        : connection_(conn) {}

    
    std::optional<Frame> mainFrame() {
        auto result = connection_.sendCommandSync("Page.getFrameTree");
        if (result.hasError) return std::nullopt;

        const auto* frameTree = result.result.find("frameTree");
        if (!frameTree) return std::nullopt;

        const auto* frame = frameTree->find("frame");
        if (!frame) return std::nullopt;

        std::string id = frame->getStringAt("id", "");
        if (id.empty()) return std::nullopt;

        Frame f(connection_, id);
        f.setUrl(frame->getStringAt("url", ""));
        f.setName(frame->getStringAt("name", ""));
        return f;
    }

    
    std::vector<Frame> frames() {
        std::vector<Frame> result;

        auto tree = connection_.sendCommandSync("Page.getFrameTree");
        if (tree.hasError) return result;

        collectFrames(tree.result.find("frameTree"), "", result);
        return result;
    }

    
    std::optional<Frame> frame(const std::string& nameOrId) {
        for (auto& f : frames()) {
            if (f.name() == nameOrId || f.id() == nameOrId) {
                return f;
            }
        }
        return std::nullopt;
    }

    
    std::optional<Frame> frameByUrl(const std::string& urlPattern) {
        for (auto& f : frames()) {
            if (f.url().find(urlPattern) != std::string::npos) {
                return f;
            }
        }
        return std::nullopt;
    }

    
    size_t frameCount() {
        return frames().size();
    }

private:
    void collectFrames(const JsonValue* frameTree, const std::string& parentId,
                       std::vector<Frame>& out) {
        if (!frameTree) return;

        const auto* frame = frameTree->find("frame");
        if (frame) {
            std::string id = frame->getStringAt("id", "");
            if (!id.empty()) {
                Frame f(connection_, id, parentId);
                f.setUrl(frame->getStringAt("url", ""));
                f.setName(frame->getStringAt("name", ""));
                out.push_back(std::move(f));

                
                const auto* children = frameTree->find("childFrames");
                if (children && children->isArray()) {
                    for (size_t i = 0; i < children->size(); ++i) {
                        collectFrames(&(*children)[i], id, out);
                    }
                }
            }
        }
    }

    CDPConnection& connection_;
};

} 
} 
