

#pragma once

#include "../domains/Input.hpp"
#include "../domains/DOM.hpp"
#include "../protocol/CDPConnection.hpp"
#include <string>
#include <thread>
#include <chrono>

namespace cdp {
namespace highlevel {


class Keyboard {
public:
    explicit Keyboard(CDPConnection& conn) : connection_(conn), input_(conn) {}

    
    Keyboard& type(const std::string& text) {
        input_.insertText(text);
        return *this;
    }

    
    Keyboard& typeSlowly(const std::string& text, int delayMs = 50) {
        for (char c : text) {
            input_.dispatchKeyEvent("char", 0, 0, std::string(1, c));
            if (delayMs > 0) {
                std::this_thread::sleep_for(std::chrono::milliseconds(delayMs));
            }
        }
        return *this;
    }

    
    Keyboard& press(const std::string& key, int modifiers = 0) {
        input_.keyPress(key, modifiers);
        return *this;
    }

    
    Keyboard& enter() { return press("Enter"); }

    
    Keyboard& tab() { return press("Tab"); }

    
    Keyboard& escape() { return press("Escape"); }

    
    Keyboard& backspace() { return press("Backspace"); }

    
    Keyboard& del() { return press("Delete"); }

    
    Keyboard& up() { return press("ArrowUp"); }
    Keyboard& down() { return press("ArrowDown"); }
    Keyboard& left() { return press("ArrowLeft"); }
    Keyboard& right() { return press("ArrowRight"); }

    
    Keyboard& ctrl(const std::string& key) { return press(key, 2); }
    Keyboard& alt(const std::string& key) { return press(key, 1); }
    Keyboard& shift(const std::string& key) { return press(key, 8); }
    Keyboard& ctrlShift(const std::string& key) { return press(key, 2 | 8); }

    
    Keyboard& selectAll() { return ctrl("a"); }
    Keyboard& copy() { return ctrl("c"); }
    Keyboard& paste() { return ctrl("v"); }
    Keyboard& cut() { return ctrl("x"); }
    Keyboard& undo() { return ctrl("z"); }
    Keyboard& redo() { return ctrlShift("z"); }

    
    Keyboard& keyDown(const std::string& key, int modifiers = 0) {
        int vkCode = getVirtualKeyCode(key);
        input_.dispatchKeyEvent("keyDown", modifiers, 0, "", "", "", key, key, vkCode);
        return *this;
    }

    
    Keyboard& keyUp(const std::string& key, int modifiers = 0) {
        int vkCode = getVirtualKeyCode(key);
        input_.dispatchKeyEvent("keyUp", modifiers, 0, "", "", "", key, key, vkCode);
        return *this;
    }

    
    Keyboard& delay(int ms) {
        std::this_thread::sleep_for(std::chrono::milliseconds(ms));
        return *this;
    }

private:
    int getVirtualKeyCode(const std::string& key) {
        if (key == "Enter") return 13;
        if (key == "Tab") return 9;
        if (key == "Backspace") return 8;
        if (key == "Escape") return 27;
        if (key == "ArrowLeft") return 37;
        if (key == "ArrowUp") return 38;
        if (key == "ArrowRight") return 39;
        if (key == "ArrowDown") return 40;
        if (key == "Delete") return 46;
        if (key == "Home") return 36;
        if (key == "End") return 35;
        if (key == "PageUp") return 33;
        if (key == "PageDown") return 34;
        if (key == "Shift") return 16;
        if (key == "Control") return 17;
        if (key == "Alt") return 18;
        if (key.length() == 1) {
            int code = static_cast<unsigned char>(key[0]);
            if (key[0] >= 'a' && key[0] <= 'z') code -= 32;
            return code;
        }
        return 0;
    }

    CDPConnection& connection_;
    Input input_;
};


class Mouse {
public:
    explicit Mouse(CDPConnection& conn) : connection_(conn), input_(conn), dom_(conn) {}

    
    Mouse& moveTo(double x, double y) {
        currentX_ = x;
        currentY_ = y;
        input_.mouseMove(x, y);
        return *this;
    }

    
    Mouse& moveToElement(const std::string& selector) {
        auto bounds = dom_.getElementBounds(selector);
        if (bounds.valid) {
            moveTo(bounds.centerX(), bounds.centerY());
        }
        return *this;
    }

    
    Mouse& click(MouseButton button = MouseButton::Left) {
        input_.click(currentX_, currentY_, button);
        return *this;
    }

    
    Mouse& clickAt(double x, double y, MouseButton button = MouseButton::Left) {
        moveTo(x, y);
        return click(button);
    }

    
    Mouse& clickElement(const std::string& selector, MouseButton button = MouseButton::Left) {
        auto bounds = dom_.getElementBounds(selector);
        if (bounds.valid) {
            clickAt(bounds.centerX(), bounds.centerY(), button);
        }
        return *this;
    }

    
    Mouse& doubleClick(MouseButton button = MouseButton::Left) {
        input_.doubleClick(currentX_, currentY_, button);
        return *this;
    }

    
    Mouse& doubleClickElement(const std::string& selector, MouseButton button = MouseButton::Left) {
        auto bounds = dom_.getElementBounds(selector);
        if (bounds.valid) {
            moveTo(bounds.centerX(), bounds.centerY());
            doubleClick(button);
        }
        return *this;
    }

    
    Mouse& rightClick() { return click(MouseButton::Right); }
    Mouse& rightClickElement(const std::string& selector) {
        return clickElement(selector, MouseButton::Right);
    }

    
    Mouse& middleClick() { return click(MouseButton::Middle); }

    
    Mouse& scroll(double deltaX, double deltaY) {
        input_.scroll(currentX_, currentY_, deltaX, deltaY);
        return *this;
    }

    
    Mouse& scrollDown(double amount = 100) { return scroll(0, amount); }

    
    Mouse& scrollUp(double amount = 100) { return scroll(0, -amount); }

    
    Mouse& scrollLeft(double amount = 100) { return scroll(-amount, 0); }

    
    Mouse& scrollRight(double amount = 100) { return scroll(amount, 0); }

    
    Mouse& dragTo(double x, double y, MouseButton button = MouseButton::Left, int steps = 10) {
        double startX = currentX_;
        double startY = currentY_;

        
        input_.dispatchMouseEvent("mousePressed", startX, startY, 0, 0, button, 0, 1);

        
        for (int i = 1; i <= steps; ++i) {
            double t = static_cast<double>(i) / steps;
            double nx = startX + (x - startX) * t;
            double ny = startY + (y - startY) * t;
            input_.mouseMove(nx, ny);
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }

        
        input_.dispatchMouseEvent("mouseReleased", x, y, 0, 0, button, 0, 1);
        currentX_ = x;
        currentY_ = y;
        return *this;
    }

    
    Mouse& dragElementTo(const std::string& fromSelector, const std::string& toSelector,
                          MouseButton button = MouseButton::Left) {
        auto fromBounds = dom_.getElementBounds(fromSelector);
        auto toBounds = dom_.getElementBounds(toSelector);

        if (fromBounds.valid && toBounds.valid) {
            moveTo(fromBounds.centerX(), fromBounds.centerY());
            dragTo(toBounds.centerX(), toBounds.centerY(), button);
        }
        return *this;
    }

    
    Mouse& down(MouseButton button = MouseButton::Left) {
        input_.dispatchMouseEvent("mousePressed", currentX_, currentY_, 0, 0, button, 0, 1);
        return *this;
    }

    
    Mouse& up(MouseButton button = MouseButton::Left) {
        input_.dispatchMouseEvent("mouseReleased", currentX_, currentY_, 0, 0, button, 0, 1);
        return *this;
    }

    
    Mouse& delay(int ms) {
        std::this_thread::sleep_for(std::chrono::milliseconds(ms));
        return *this;
    }

    
    double x() const { return currentX_; }
    double y() const { return currentY_; }

private:
    CDPConnection& connection_;
    Input input_;
    DOM dom_;
    double currentX_ = 0;
    double currentY_ = 0;
};


class InputHelper {
public:
    explicit InputHelper(CDPConnection& conn)
        : connection_(conn), keyboard_(conn), mouse_(conn) {}

    Keyboard& keyboard() { return keyboard_; }
    Mouse& mouse() { return mouse_; }

    
    void clickAndType(const std::string& selector, const std::string& text) {
        mouse_.clickElement(selector);
        keyboard_.type(text);
    }

    
    void clearAndType(const std::string& selector, const std::string& text) {
        mouse_.clickElement(selector);
        keyboard_.selectAll().type(text);
    }

    
    void submitText(const std::string& selector, const std::string& text) {
        mouse_.clickElement(selector);
        keyboard_.type(text).enter();
    }

private:
    CDPConnection& connection_;
    Keyboard keyboard_;
    Mouse mouse_;
};

} 
} 
