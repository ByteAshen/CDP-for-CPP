#pragma once

#include "BaseDomain.hpp"
#include <string>
#include <vector>

namespace cdp {


enum class MouseButton {
    None,
    Left,
    Middle,
    Right,
    Back,
    Forward
};


enum KeyModifier {
    None = 0,
    Alt = 1,
    Ctrl = 2,
    Meta = 4,  
    Shift = 8
};


struct TouchPoint {
    double x = 0;
    double y = 0;
    double radiusX = 1;
    double radiusY = 1;
    double rotationAngle = 0;
    double force = 1;
    int id = 0;

    JsonValue toJson() const;
};


class InputDomain : public BaseDomain {
public:
    using BaseDomain::BaseDomain;
    std::string domainName() const override { return "Input"; }

    
    CDPResponse dispatchMouseEvent(const std::string& type, double x, double y,
                                    int modifiers = 0,
                                    MouseButton button = MouseButton::None,
                                    int clickCount = 0,
                                    double deltaX = 0, double deltaY = 0);

    
    CDPResponse mouseMove(double x, double y, int modifiers = 0);
    CDPResponse mouseDown(double x, double y, MouseButton button = MouseButton::Left,
                           int modifiers = 0, int clickCount = 1);
    CDPResponse mouseUp(double x, double y, MouseButton button = MouseButton::Left,
                         int modifiers = 0, int clickCount = 1);
    CDPResponse click(double x, double y, MouseButton button = MouseButton::Left,
                       int modifiers = 0, int clickCount = 1);
    CDPResponse doubleClick(double x, double y, MouseButton button = MouseButton::Left,
                             int modifiers = 0);
    CDPResponse scroll(double x, double y, double deltaX, double deltaY, int modifiers = 0);

    
    CDPResponse dispatchKeyEvent(const std::string& type,
                                  int modifiers = 0,
                                  const std::string& key = "",
                                  const std::string& code = "",
                                  int windowsVirtualKeyCode = 0,
                                  int nativeVirtualKeyCode = 0,
                                  bool autoRepeat = false,
                                  bool isKeypad = false,
                                  bool isSystemKey = false,
                                  const std::string& text = "",
                                  const std::string& unmodifiedText = "");

    
    CDPResponse keyDown(const std::string& key, int modifiers = 0);
    CDPResponse keyUp(const std::string& key, int modifiers = 0);
    CDPResponse keyPress(const std::string& key, int modifiers = 0);
    CDPResponse typeText(const std::string& text);
    CDPResponse insertText(const std::string& text);

    
    CDPResponse dispatchTouchEvent(const std::string& type,
                                    const std::vector<TouchPoint>& touchPoints,
                                    int modifiers = 0);

    
    CDPResponse touchStart(double x, double y);
    CDPResponse touchMove(double x, double y);
    CDPResponse touchEnd(double x, double y);
    CDPResponse tap(double x, double y);

    
    CDPResponse dispatchDragEvent(const std::string& type, double x, double y,
                                   const JsonValue& data, int modifiers = 0);

    
    CDPResponse synthesizePinchGesture(double x, double y, double scaleFactor,
                                        int relativeSpeed = 800, const std::string& gestureSourceType = "");

    
    CDPResponse synthesizeScrollGesture(double x, double y,
                                         double xDistance = 0, double yDistance = 0,
                                         double xOverscroll = 0, double yOverscroll = 0,
                                         bool preventFling = true,
                                         int speed = 800,
                                         const std::string& gestureSourceType = "",
                                         int repeatCount = 0,
                                         int repeatDelayMs = 250);

    
    CDPResponse synthesizeTapGesture(double x, double y, int duration = 50,
                                      int tapCount = 1, const std::string& gestureSourceType = "");

    
    CDPResponse setIgnoreInputEvents(bool ignore);
    CDPResponse imeSetComposition(const std::string& text, int selectionStart,
                                   int selectionEnd, int replacementStart = 0,
                                   int replacementEnd = 0);

private:
    std::string mouseButtonToString(MouseButton button);
    int getKeyCode(const std::string& key);
};

} 
