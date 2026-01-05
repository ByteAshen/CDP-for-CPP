#pragma once

#include "Domain.hpp"
#include "../core/Enums.hpp"

namespace cdp {


class Input : public Domain {
public:
    explicit Input(CDPConnection& connection) : Domain(connection, "Input") {}

    
    CDPResponse dispatchMouseEvent(const std::string& type,  
                                    double x, double y,
                                    int modifiers = 0,
                                    double timestamp = 0,
                                    MouseButton button = MouseButton::None,
                                    int buttons = 0,
                                    int clickCount = 0,
                                    double force = 0,
                                    double tangentialPressure = 0,
                                    double tiltX = 0,
                                    double tiltY = 0,
                                    int twist = 0,
                                    double deltaX = 0,
                                    double deltaY = 0,
                                    PointerType pointerType = PointerType::Mouse) {
        Params params;
        params.set("type", type);
        params.set("x", x);
        params.set("y", y);
        if (modifiers != 0) params.set("modifiers", modifiers);
        if (timestamp > 0) params.set("timestamp", timestamp);

        if (button != MouseButton::None) {
            params.set("button", std::string(toString(button)));
        }

        if (buttons != 0) params.set("buttons", buttons);
        if (clickCount > 0) params.set("clickCount", clickCount);
        if (force > 0) params.set("force", force);
        if (tangentialPressure != 0) params.set("tangentialPressure", tangentialPressure);
        if (tiltX != 0) params.set("tiltX", tiltX);
        if (tiltY != 0) params.set("tiltY", tiltY);
        if (twist != 0) params.set("twist", twist);
        if (deltaX != 0) params.set("deltaX", deltaX);
        if (deltaY != 0) params.set("deltaY", deltaY);

        params.set("pointerType", std::string(toString(pointerType)));

        return call("dispatchMouseEvent", params);
    }

    
    CDPResponse dispatchMouseEvent(MouseEventType type,
                                    double x, double y,
                                    MouseButton button = MouseButton::None,
                                    int clickCount = 0,
                                    int modifiers = 0,
                                    PointerType pointerType = PointerType::Mouse) {
        Params params;
        params.set("type", std::string(toString(type)));
        params.set("x", x);
        params.set("y", y);
        if (button != MouseButton::None) {
            params.set("button", std::string(toString(button)));
        }
        if (clickCount > 0) params.set("clickCount", clickCount);
        if (modifiers != 0) params.set("modifiers", modifiers);
        params.set("pointerType", std::string(toString(pointerType)));
        return call("dispatchMouseEvent", params);
    }

    
    CDPResponse mouseMove(double x, double y) {
        return dispatchMouseEvent("mouseMoved", x, y);
    }

    
    CDPResponse click(double x, double y, MouseButton button = MouseButton::Left) {
        dispatchMouseEvent("mousePressed", x, y, 0, 0, button, 0, 1);
        return dispatchMouseEvent("mouseReleased", x, y, 0, 0, button, 0, 1);
    }

    
    CDPResponse doubleClick(double x, double y, MouseButton button = MouseButton::Left) {
        dispatchMouseEvent("mousePressed", x, y, 0, 0, button, 0, 1);
        dispatchMouseEvent("mouseReleased", x, y, 0, 0, button, 0, 1);
        dispatchMouseEvent("mousePressed", x, y, 0, 0, button, 0, 2);
        return dispatchMouseEvent("mouseReleased", x, y, 0, 0, button, 0, 2);
    }

    
    CDPResponse scroll(double x, double y, double deltaX, double deltaY) {
        return dispatchMouseEvent("mouseWheel", x, y, 0, 0, MouseButton::None, 0, 0, 0, 0, 0, 0, 0, deltaX, deltaY);
    }

    
    CDPResponse dispatchKeyEvent(const std::string& type,  
                                  int modifiers = 0,
                                  double timestamp = 0,
                                  const std::string& text = "",
                                  const std::string& unmodifiedText = "",
                                  const std::string& keyIdentifier = "",
                                  const std::string& code = "",
                                  const std::string& key = "",
                                  int windowsVirtualKeyCode = 0,
                                  int nativeVirtualKeyCode = 0,
                                  bool autoRepeat = false,
                                  bool isKeypad = false,
                                  bool isSystemKey = false,
                                  int location = 0,
                                  const std::vector<std::string>& commands = {}) {
        Params params;
        params.set("type", type);
        if (modifiers != 0) params.set("modifiers", modifiers);
        if (timestamp > 0) params.set("timestamp", timestamp);
        if (!text.empty()) params.set("text", text);
        if (!unmodifiedText.empty()) params.set("unmodifiedText", unmodifiedText);
        if (!keyIdentifier.empty()) params.set("keyIdentifier", keyIdentifier);
        if (!code.empty()) params.set("code", code);
        if (!key.empty()) params.set("key", key);
        if (windowsVirtualKeyCode > 0) params.set("windowsVirtualKeyCode", windowsVirtualKeyCode);
        if (nativeVirtualKeyCode > 0) params.set("nativeVirtualKeyCode", nativeVirtualKeyCode);
        if (autoRepeat) params.set("autoRepeat", true);
        if (isKeypad) params.set("isKeypad", true);
        if (isSystemKey) params.set("isSystemKey", true);
        if (location > 0) params.set("location", location);
        if (!commands.empty()) {
            JsonArray arr;
            for (const auto& cmd : commands) arr.push_back(cmd);
            params.set("commands", arr);
        }
        return call("dispatchKeyEvent", params);
    }

    
    CDPResponse dispatchKeyEvent(KeyEventType type,
                                  const std::string& key,
                                  const std::string& code = "",
                                  int windowsVirtualKeyCode = 0,
                                  int modifiers = 0,
                                  const std::string& text = "") {
        Params params;
        params.set("type", std::string(toString(type)));
        if (!key.empty()) params.set("key", key);
        if (!code.empty()) params.set("code", code);
        if (windowsVirtualKeyCode > 0) params.set("windowsVirtualKeyCode", windowsVirtualKeyCode);
        if (modifiers != 0) params.set("modifiers", modifiers);
        if (!text.empty()) params.set("text", text);
        return call("dispatchKeyEvent", params);
    }

    
    CDPResponse keyPress(const std::string& key, int modifiers = 0) {
        
        int vkCode = 0;
        std::string code = key;

        if (key == "Enter") { vkCode = 13; code = "Enter"; }
        else if (key == "Tab") { vkCode = 9; code = "Tab"; }
        else if (key == "Backspace") { vkCode = 8; code = "Backspace"; }
        else if (key == "Escape") { vkCode = 27; code = "Escape"; }
        else if (key == "ArrowLeft") { vkCode = 37; code = "ArrowLeft"; }
        else if (key == "ArrowUp") { vkCode = 38; code = "ArrowUp"; }
        else if (key == "ArrowRight") { vkCode = 39; code = "ArrowRight"; }
        else if (key == "ArrowDown") { vkCode = 40; code = "ArrowDown"; }
        else if (key == "Delete") { vkCode = 46; code = "Delete"; }
        else if (key == "Home") { vkCode = 36; code = "Home"; }
        else if (key == "End") { vkCode = 35; code = "End"; }
        else if (key == "PageUp") { vkCode = 33; code = "PageUp"; }
        else if (key == "PageDown") { vkCode = 34; code = "PageDown"; }
        else if (key.length() == 1) {
            vkCode = static_cast<unsigned char>(key[0]);
            if (key[0] >= 'a' && key[0] <= 'z') vkCode -= 32;  
        }

        dispatchKeyEvent("keyDown", modifiers, 0, "", "", "", code, key, vkCode);
        return dispatchKeyEvent("keyUp", modifiers, 0, "", "", "", code, key, vkCode);
    }

    
    CDPResponse typeText(const std::string& text) {
        CDPResponse lastResponse;
        for (char c : text) {
            std::string s(1, c);
            lastResponse = dispatchKeyEvent("char", 0, 0, s);
        }
        return lastResponse;
    }

    
    CDPResponse insertText(const std::string& text) {
        return call("insertText", Params().set("text", text));
    }

    
    CDPResponse typeTextFast(const std::string& text) {
        return insertText(text);
    }

    
    void clickFast(double x, double y, MouseButton button = MouseButton::Left) {
        
        Params pressParams;
        pressParams.set("type", "mousePressed");
        pressParams.set("x", x);
        pressParams.set("y", y);
        static const char* buttonNames[] = {"none", "left", "middle", "right", "back", "forward"};
        pressParams.set("button", buttonNames[static_cast<int>(button)]);
        pressParams.set("clickCount", 1);
        pressParams.set("pointerType", "mouse");
        callAsync("dispatchMouseEvent", pressParams);

        Params releaseParams;
        releaseParams.set("type", "mouseReleased");
        releaseParams.set("x", x);
        releaseParams.set("y", y);
        releaseParams.set("button", buttonNames[static_cast<int>(button)]);
        releaseParams.set("clickCount", 1);
        releaseParams.set("pointerType", "mouse");
        callAsync("dispatchMouseEvent", releaseParams);
    }

    
    struct Point {
        double x;
        double y;
    };

    
    void clickMultiple(const std::vector<Point>& points, MouseButton button = MouseButton::Left) {
        static const char* buttonNames[] = {"none", "left", "middle", "right", "back", "forward"};
        const char* btnName = buttonNames[static_cast<int>(button)];

        for (const auto& pt : points) {
            Params pressParams;
            pressParams.set("type", "mousePressed");
            pressParams.set("x", pt.x);
            pressParams.set("y", pt.y);
            pressParams.set("button", btnName);
            pressParams.set("clickCount", 1);
            pressParams.set("pointerType", "mouse");
            callAsync("dispatchMouseEvent", pressParams);

            Params releaseParams;
            releaseParams.set("type", "mouseReleased");
            releaseParams.set("x", pt.x);
            releaseParams.set("y", pt.y);
            releaseParams.set("button", btnName);
            releaseParams.set("clickCount", 1);
            releaseParams.set("pointerType", "mouse");
            callAsync("dispatchMouseEvent", releaseParams);
        }
    }

    
    void mousePathFast(const std::vector<Point>& path) {
        for (const auto& pt : path) {
            Params params;
            params.set("type", "mouseMoved");
            params.set("x", pt.x);
            params.set("y", pt.y);
            params.set("pointerType", "mouse");
            callAsync("dispatchMouseEvent", params);
        }
    }

    
    void dragFast(double startX, double startY, double endX, double endY,
                  MouseButton button = MouseButton::Left, int steps = 10) {
        static const char* buttonNames[] = {"none", "left", "middle", "right", "back", "forward"};
        const char* btnName = buttonNames[static_cast<int>(button)];

        
        Params pressParams;
        pressParams.set("type", "mousePressed");
        pressParams.set("x", startX);
        pressParams.set("y", startY);
        pressParams.set("button", btnName);
        pressParams.set("clickCount", 1);
        pressParams.set("pointerType", "mouse");
        callAsync("dispatchMouseEvent", pressParams);

        
        for (int i = 1; i <= steps; ++i) {
            double t = static_cast<double>(i) / steps;
            double x = startX + (endX - startX) * t;
            double y = startY + (endY - startY) * t;

            Params moveParams;
            moveParams.set("type", "mouseMoved");
            moveParams.set("x", x);
            moveParams.set("y", y);
            moveParams.set("pointerType", "mouse");
            callAsync("dispatchMouseEvent", moveParams);
        }

        
        Params releaseParams;
        releaseParams.set("type", "mouseReleased");
        releaseParams.set("x", endX);
        releaseParams.set("y", endY);
        releaseParams.set("button", btnName);
        releaseParams.set("clickCount", 1);
        releaseParams.set("pointerType", "mouse");
        callAsync("dispatchMouseEvent", releaseParams);
    }

    
    void scrollFast(double x, double y, double deltaX, double deltaY) {
        Params params;
        params.set("type", "mouseWheel");
        params.set("x", x);
        params.set("y", y);
        params.set("deltaX", deltaX);
        params.set("deltaY", deltaY);
        params.set("pointerType", "mouse");
        callAsync("dispatchMouseEvent", params);
    }

    
    void keyPressFast(const std::string& key, int modifiers = 0) {
        int vkCode = 0;
        std::string code = key;

        if (key == "Enter") { vkCode = 13; code = "Enter"; }
        else if (key == "Tab") { vkCode = 9; code = "Tab"; }
        else if (key == "Backspace") { vkCode = 8; code = "Backspace"; }
        else if (key == "Escape") { vkCode = 27; code = "Escape"; }
        else if (key == "ArrowLeft") { vkCode = 37; code = "ArrowLeft"; }
        else if (key == "ArrowUp") { vkCode = 38; code = "ArrowUp"; }
        else if (key == "ArrowRight") { vkCode = 39; code = "ArrowRight"; }
        else if (key == "ArrowDown") { vkCode = 40; code = "ArrowDown"; }
        else if (key == "Delete") { vkCode = 46; code = "Delete"; }
        else if (key == "Home") { vkCode = 36; code = "Home"; }
        else if (key == "End") { vkCode = 35; code = "End"; }
        else if (key == "PageUp") { vkCode = 33; code = "PageUp"; }
        else if (key == "PageDown") { vkCode = 34; code = "PageDown"; }
        else if (key.length() == 1) {
            vkCode = static_cast<unsigned char>(key[0]);
            if (key[0] >= 'a' && key[0] <= 'z') vkCode -= 32;
        }

        Params downParams;
        downParams.set("type", "keyDown");
        if (modifiers != 0) downParams.set("modifiers", modifiers);
        if (!code.empty()) downParams.set("code", code);
        if (!key.empty()) downParams.set("key", key);
        if (vkCode > 0) downParams.set("windowsVirtualKeyCode", vkCode);
        callAsync("dispatchKeyEvent", downParams);

        Params upParams;
        upParams.set("type", "keyUp");
        if (modifiers != 0) upParams.set("modifiers", modifiers);
        if (!code.empty()) upParams.set("code", code);
        if (!key.empty()) upParams.set("key", key);
        if (vkCode > 0) upParams.set("windowsVirtualKeyCode", vkCode);
        callAsync("dispatchKeyEvent", upParams);
    }

    
    CDPResponse imeSetComposition(const std::string& text,
                                   int selectionStart,
                                   int selectionEnd,
                                   int replacementStart = -1,
                                   int replacementEnd = -1) {
        Params params;
        params.set("text", text);
        params.set("selectionStart", selectionStart);
        params.set("selectionEnd", selectionEnd);
        if (replacementStart >= 0) params.set("replacementStart", replacementStart);
        if (replacementEnd >= 0) params.set("replacementEnd", replacementEnd);
        return call("imeSetComposition", params);
    }

    
    struct TouchPoint {
        double x;
        double y;
        double radiusX = 1;
        double radiusY = 1;
        double rotationAngle = 0;
        double force = 1;
        double tangentialPressure = 0;
        double tiltX = 0;
        double tiltY = 0;
        int twist = 0;
        int id = 0;

        JsonValue toJson() const {
            JsonObject obj;
            obj["x"] = x;
            obj["y"] = y;
            if (radiusX != 1) obj["radiusX"] = radiusX;
            if (radiusY != 1) obj["radiusY"] = radiusY;
            if (rotationAngle != 0) obj["rotationAngle"] = rotationAngle;
            if (force != 1) obj["force"] = force;
            if (tangentialPressure != 0) obj["tangentialPressure"] = tangentialPressure;
            if (tiltX != 0) obj["tiltX"] = tiltX;
            if (tiltY != 0) obj["tiltY"] = tiltY;
            if (twist != 0) obj["twist"] = twist;
            if (id != 0) obj["id"] = id;
            return obj;
        }
    };

    CDPResponse dispatchTouchEvent(const std::string& type,  
                                    const std::vector<TouchPoint>& touchPoints,
                                    int modifiers = 0,
                                    double timestamp = 0) {
        JsonArray points;
        for (const auto& tp : touchPoints) {
            points.push_back(tp.toJson());
        }
        Params params;
        params.set("type", type);
        params.set("touchPoints", points);
        if (modifiers != 0) params.set("modifiers", modifiers);
        if (timestamp > 0) params.set("timestamp", timestamp);
        return call("dispatchTouchEvent", params);
    }

    
    CDPResponse tap(double x, double y) {
        TouchPoint tp{x, y};
        dispatchTouchEvent("touchStart", {tp});
        return dispatchTouchEvent("touchEnd", {});
    }

    
    struct DragDataItem {
        std::string mimeType;
        std::string data;
        std::string title;
        std::string baseURL;

        JsonValue toJson() const {
            JsonObject obj;
            obj["mimeType"] = mimeType;
            obj["data"] = data;
            if (!title.empty()) obj["title"] = title;
            if (!baseURL.empty()) obj["baseURL"] = baseURL;
            return obj;
        }
    };

    CDPResponse dispatchDragEvent(const std::string& type,  
                                   double x, double y,
                                   const std::vector<DragDataItem>& items,
                                   const std::vector<std::string>& files = {},
                                   int modifiers = 0) {
        JsonObject data;
        JsonArray itemsArray;
        for (const auto& item : items) itemsArray.push_back(item.toJson());
        data["items"] = itemsArray;

        if (!files.empty()) {
            JsonArray filesArray;
            for (const auto& f : files) filesArray.push_back(f);
            data["files"] = filesArray;
        }
        data["dragOperationsMask"] = 1;  

        Params params;
        params.set("type", type);
        params.set("x", x);
        params.set("y", y);
        params.set("data", data);
        if (modifiers != 0) params.set("modifiers", modifiers);
        return call("dispatchDragEvent", params);
    }

    
    CDPResponse setIgnoreInputEvents(bool ignore) {
        return call("setIgnoreInputEvents", Params().set("ignore", ignore));
    }

    
    CDPResponse setInterceptDrags(bool enabled) {
        return call("setInterceptDrags", Params().set("enabled", enabled));
    }

    
    CDPResponse synthesizePinchGesture(double x, double y, double scaleFactor,
                                        int relativeSpeed = 0,
                                        const std::string& gestureSourceType = "") {
        Params params;
        params.set("x", x);
        params.set("y", y);
        params.set("scaleFactor", scaleFactor);
        if (relativeSpeed > 0) params.set("relativeSpeed", relativeSpeed);
        if (!gestureSourceType.empty()) params.set("gestureSourceType", gestureSourceType);
        return call("synthesizePinchGesture", params);
    }

    
    CDPResponse synthesizeScrollGesture(double x, double y,
                                         double xDistance = 0,
                                         double yDistance = 0,
                                         double xOverscroll = 0,
                                         double yOverscroll = 0,
                                         bool preventFling = true,
                                         int speed = 0,
                                         const std::string& gestureSourceType = "",
                                         int repeatCount = 0,
                                         int repeatDelayMs = 0,
                                         const std::string& interactionMarkerName = "") {
        Params params;
        params.set("x", x);
        params.set("y", y);
        if (xDistance != 0) params.set("xDistance", xDistance);
        if (yDistance != 0) params.set("yDistance", yDistance);
        if (xOverscroll != 0) params.set("xOverscroll", xOverscroll);
        if (yOverscroll != 0) params.set("yOverscroll", yOverscroll);
        if (!preventFling) params.set("preventFling", false);
        if (speed > 0) params.set("speed", speed);
        if (!gestureSourceType.empty()) params.set("gestureSourceType", gestureSourceType);
        if (repeatCount > 0) params.set("repeatCount", repeatCount);
        if (repeatDelayMs > 0) params.set("repeatDelayMs", repeatDelayMs);
        if (!interactionMarkerName.empty()) params.set("interactionMarkerName", interactionMarkerName);
        return call("synthesizeScrollGesture", params);
    }

    
    CDPResponse synthesizeTapGesture(double x, double y,
                                      int duration = 0,
                                      int tapCount = 1,
                                      const std::string& gestureSourceType = "") {
        Params params;
        params.set("x", x);
        params.set("y", y);
        if (duration > 0) params.set("duration", duration);
        if (tapCount != 1) params.set("tapCount", tapCount);
        if (!gestureSourceType.empty()) params.set("gestureSourceType", gestureSourceType);
        return call("synthesizeTapGesture", params);
    }

    
    CDPResponse cancelDragging() {
        return call("cancelDragging");
    }

    
    CDPResponse emulateTouchFromMouseEvent(const std::string& type,  
                                            int x, int y,
                                            MouseButton button,
                                            double timestamp = 0,
                                            double deltaX = 0,
                                            double deltaY = 0,
                                            int modifiers = 0,
                                            int clickCount = 0) {
        Params params;
        params.set("type", type);
        params.set("x", x);
        params.set("y", y);
        static const char* buttonNames[] = {"none", "left", "middle", "right", "back", "forward"};
        params.set("button", buttonNames[static_cast<int>(button)]);
        if (timestamp > 0) params.set("timestamp", timestamp);
        if (deltaX != 0) params.set("deltaX", deltaX);
        if (deltaY != 0) params.set("deltaY", deltaY);
        if (modifiers != 0) params.set("modifiers", modifiers);
        if (clickCount > 0) params.set("clickCount", clickCount);
        return call("emulateTouchFromMouseEvent", params);
    }

    
    void onDragIntercepted(std::function<void(const JsonValue& data)> callback) {
        on("dragIntercepted", [callback](const CDPEvent& event) {
            callback(event.params["data"]);
        });
    }
};

} 
