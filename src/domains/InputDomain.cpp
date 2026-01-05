

#include "cdp/domains/InputDomain.hpp"
#include <thread>
#include <chrono>

namespace cdp {

JsonValue TouchPoint::toJson() const {
    JsonObject obj;
    obj["x"] = x;
    obj["y"] = y;
    obj["radiusX"] = radiusX;
    obj["radiusY"] = radiusY;
    obj["rotationAngle"] = rotationAngle;
    obj["force"] = force;
    obj["id"] = id;
    return obj;
}

std::string InputDomain::mouseButtonToString(MouseButton button) {
    switch (button) {
        case MouseButton::Left: return "left";
        case MouseButton::Middle: return "middle";
        case MouseButton::Right: return "right";
        case MouseButton::Back: return "back";
        case MouseButton::Forward: return "forward";
        default: return "none";
    }
}

int InputDomain::getKeyCode(const std::string& key) {
    
    static const std::map<std::string, int> keyCodes = {
        {"Backspace", 8}, {"Tab", 9}, {"Enter", 13}, {"Shift", 16},
        {"Control", 17}, {"Alt", 18}, {"Escape", 27}, {"Space", 32},
        {"PageUp", 33}, {"PageDown", 34}, {"End", 35}, {"Home", 36},
        {"ArrowLeft", 37}, {"ArrowUp", 38}, {"ArrowRight", 39}, {"ArrowDown", 40},
        {"Delete", 46}, {"0", 48}, {"1", 49}, {"2", 50}, {"3", 51},
        {"4", 52}, {"5", 53}, {"6", 54}, {"7", 55}, {"8", 56}, {"9", 57},
        {"a", 65}, {"b", 66}, {"c", 67}, {"d", 68}, {"e", 69}, {"f", 70},
        {"g", 71}, {"h", 72}, {"i", 73}, {"j", 74}, {"k", 75}, {"l", 76},
        {"m", 77}, {"n", 78}, {"o", 79}, {"p", 80}, {"q", 81}, {"r", 82},
        {"s", 83}, {"t", 84}, {"u", 85}, {"v", 86}, {"w", 87}, {"x", 88},
        {"y", 89}, {"z", 90}, {"F1", 112}, {"F2", 113}, {"F3", 114},
        {"F4", 115}, {"F5", 116}, {"F6", 117}, {"F7", 118}, {"F8", 119},
        {"F9", 120}, {"F10", 121}, {"F11", 122}, {"F12", 123}
    };

    auto it = keyCodes.find(key);
    if (it != keyCodes.end()) {
        return it->second;
    }

    
    if (key.length() == 1) {
        char c = key[0];
        if (c >= 'a' && c <= 'z') {
            return 65 + (c - 'a');
        }
        if (c >= 'A' && c <= 'Z') {
            return 65 + (c - 'A');
        }
        return static_cast<int>(c);
    }

    return 0;
}

CDPResponse InputDomain::dispatchMouseEvent(const std::string& type, double x, double y,
                                             int modifiers, MouseButton button,
                                             int clickCount, double deltaX, double deltaY) {
    JsonObject params;
    params["type"] = type;
    params["x"] = x;
    params["y"] = y;
    params["modifiers"] = modifiers;
    params["button"] = mouseButtonToString(button);
    params["clickCount"] = clickCount;
    if (deltaX != 0) params["deltaX"] = deltaX;
    if (deltaY != 0) params["deltaY"] = deltaY;

    return sendCommand("Input.dispatchMouseEvent", params);
}

CDPResponse InputDomain::mouseMove(double x, double y, int modifiers) {
    return dispatchMouseEvent("mouseMoved", x, y, modifiers);
}

CDPResponse InputDomain::mouseDown(double x, double y, MouseButton button,
                                    int modifiers, int clickCount) {
    return dispatchMouseEvent("mousePressed", x, y, modifiers, button, clickCount);
}

CDPResponse InputDomain::mouseUp(double x, double y, MouseButton button,
                                  int modifiers, int clickCount) {
    return dispatchMouseEvent("mouseReleased", x, y, modifiers, button, clickCount);
}

CDPResponse InputDomain::click(double x, double y, MouseButton button,
                                int modifiers, int clickCount) {
    auto result = mouseDown(x, y, button, modifiers, clickCount);
    if (!result.isSuccess()) return result;
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    return mouseUp(x, y, button, modifiers, clickCount);
}

CDPResponse InputDomain::doubleClick(double x, double y, MouseButton button, int modifiers) {
    auto result = click(x, y, button, modifiers, 1);
    if (!result.isSuccess()) return result;
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    return click(x, y, button, modifiers, 2);
}

CDPResponse InputDomain::scroll(double x, double y, double deltaX, double deltaY, int modifiers) {
    return dispatchMouseEvent("mouseWheel", x, y, modifiers, MouseButton::None, 0, deltaX, deltaY);
}

CDPResponse InputDomain::dispatchKeyEvent(const std::string& type, int modifiers,
                                           const std::string& key, const std::string& code,
                                           int windowsVirtualKeyCode, int nativeVirtualKeyCode,
                                           bool autoRepeat, bool isKeypad, bool isSystemKey,
                                           const std::string& text, const std::string& unmodifiedText) {
    JsonObject params;
    params["type"] = type;
    params["modifiers"] = modifiers;

    if (!key.empty()) params["key"] = key;
    if (!code.empty()) params["code"] = code;
    if (windowsVirtualKeyCode > 0) params["windowsVirtualKeyCode"] = windowsVirtualKeyCode;
    if (nativeVirtualKeyCode > 0) params["nativeVirtualKeyCode"] = nativeVirtualKeyCode;
    if (autoRepeat) params["autoRepeat"] = autoRepeat;
    if (isKeypad) params["isKeypad"] = isKeypad;
    if (isSystemKey) params["isSystemKey"] = isSystemKey;
    if (!text.empty()) params["text"] = text;
    if (!unmodifiedText.empty()) params["unmodifiedText"] = unmodifiedText;

    return sendCommand("Input.dispatchKeyEvent", params);
}

CDPResponse InputDomain::keyDown(const std::string& key, int modifiers) {
    int keyCode = getKeyCode(key);
    return dispatchKeyEvent("keyDown", modifiers, key, "", keyCode, keyCode);
}

CDPResponse InputDomain::keyUp(const std::string& key, int modifiers) {
    int keyCode = getKeyCode(key);
    return dispatchKeyEvent("keyUp", modifiers, key, "", keyCode, keyCode);
}

CDPResponse InputDomain::keyPress(const std::string& key, int modifiers) {
    auto result = keyDown(key, modifiers);
    if (!result.isSuccess()) return result;
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    return keyUp(key, modifiers);
}

CDPResponse InputDomain::typeText(const std::string& text) {
    CDPResponse lastResult;
    for (char c : text) {
        std::string charStr(1, c);
        int keyCode = getKeyCode(charStr);

        
        lastResult = dispatchKeyEvent("keyDown", 0, charStr, "", keyCode, keyCode,
                                       false, false, false, charStr, charStr);
        if (!lastResult.isSuccess()) return lastResult;

        
        lastResult = dispatchKeyEvent("char", 0, charStr, "", keyCode, keyCode,
                                       false, false, false, charStr, charStr);
        if (!lastResult.isSuccess()) return lastResult;

        
        lastResult = dispatchKeyEvent("keyUp", 0, charStr, "", keyCode, keyCode);
        if (!lastResult.isSuccess()) return lastResult;

        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }
    return lastResult;
}

CDPResponse InputDomain::insertText(const std::string& text) {
    JsonObject params;
    params["text"] = text;
    return sendCommand("Input.insertText", params);
}

CDPResponse InputDomain::dispatchTouchEvent(const std::string& type,
                                             const std::vector<TouchPoint>& touchPoints,
                                             int modifiers) {
    JsonObject params;
    params["type"] = type;
    params["modifiers"] = modifiers;

    JsonArray points;
    for (const auto& point : touchPoints) {
        points.push_back(point.toJson());
    }
    params["touchPoints"] = points;

    return sendCommand("Input.dispatchTouchEvent", params);
}

CDPResponse InputDomain::touchStart(double x, double y) {
    TouchPoint point;
    point.x = x;
    point.y = y;
    return dispatchTouchEvent("touchStart", {point});
}

CDPResponse InputDomain::touchMove(double x, double y) {
    TouchPoint point;
    point.x = x;
    point.y = y;
    return dispatchTouchEvent("touchMove", {point});
}

CDPResponse InputDomain::touchEnd(double x, double y) {
    TouchPoint point;
    point.x = x;
    point.y = y;
    return dispatchTouchEvent("touchEnd", {point});
}

CDPResponse InputDomain::tap(double x, double y) {
    auto result = touchStart(x, y);
    if (!result.isSuccess()) return result;
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    return touchEnd(x, y);
}

CDPResponse InputDomain::dispatchDragEvent(const std::string& type, double x, double y,
                                            const JsonValue& data, int modifiers) {
    JsonObject params;
    params["type"] = type;
    params["x"] = x;
    params["y"] = y;
    params["data"] = data;
    params["modifiers"] = modifiers;
    return sendCommand("Input.dispatchDragEvent", params);
}

CDPResponse InputDomain::synthesizePinchGesture(double x, double y, double scaleFactor,
                                                 int relativeSpeed, const std::string& gestureSourceType) {
    JsonObject params;
    params["x"] = x;
    params["y"] = y;
    params["scaleFactor"] = scaleFactor;
    params["relativeSpeed"] = relativeSpeed;
    if (!gestureSourceType.empty()) params["gestureSourceType"] = gestureSourceType;
    return sendCommand("Input.synthesizePinchGesture", params);
}

CDPResponse InputDomain::synthesizeScrollGesture(double x, double y,
                                                  double xDistance, double yDistance,
                                                  double xOverscroll, double yOverscroll,
                                                  bool preventFling, int speed,
                                                  const std::string& gestureSourceType,
                                                  int repeatCount, int repeatDelayMs) {
    JsonObject params;
    params["x"] = x;
    params["y"] = y;
    if (xDistance != 0) params["xDistance"] = xDistance;
    if (yDistance != 0) params["yDistance"] = yDistance;
    if (xOverscroll != 0) params["xOverscroll"] = xOverscroll;
    if (yOverscroll != 0) params["yOverscroll"] = yOverscroll;
    params["preventFling"] = preventFling;
    params["speed"] = speed;
    if (!gestureSourceType.empty()) params["gestureSourceType"] = gestureSourceType;
    params["repeatCount"] = repeatCount;
    params["repeatDelayMs"] = repeatDelayMs;
    return sendCommand("Input.synthesizeScrollGesture", params);
}

CDPResponse InputDomain::synthesizeTapGesture(double x, double y, int duration,
                                               int tapCount, const std::string& gestureSourceType) {
    JsonObject params;
    params["x"] = x;
    params["y"] = y;
    params["duration"] = duration;
    params["tapCount"] = tapCount;
    if (!gestureSourceType.empty()) params["gestureSourceType"] = gestureSourceType;
    return sendCommand("Input.synthesizeTapGesture", params);
}

CDPResponse InputDomain::setIgnoreInputEvents(bool ignore) {
    JsonObject params;
    params["ignore"] = ignore;
    return sendCommand("Input.setIgnoreInputEvents", params);
}

CDPResponse InputDomain::imeSetComposition(const std::string& text, int selectionStart,
                                            int selectionEnd, int replacementStart,
                                            int replacementEnd) {
    JsonObject params;
    params["text"] = text;
    params["selectionStart"] = selectionStart;
    params["selectionEnd"] = selectionEnd;
    params["replacementStart"] = replacementStart;
    params["replacementEnd"] = replacementEnd;
    return sendCommand("Input.imeSetComposition", params);
}

} 
