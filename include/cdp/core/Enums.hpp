

#pragma once

#include <string>
#include <string_view>

namespace cdp {


enum class TransitionType {
    Link,           
    Typed,          
    AddressBar,     
    AutoBookmark,   
    AutoSubframe,   
    ManualSubframe, 
    Generated,      
    AutoToplevel,   
    FormSubmit,     
    Reload,         
    Keyword,        
    KeywordGenerated, 
    Other           
};

inline std::string_view toString(TransitionType type) {
    switch (type) {
        case TransitionType::Link: return "link";
        case TransitionType::Typed: return "typed";
        case TransitionType::AddressBar: return "address_bar";
        case TransitionType::AutoBookmark: return "auto_bookmark";
        case TransitionType::AutoSubframe: return "auto_subframe";
        case TransitionType::ManualSubframe: return "manual_subframe";
        case TransitionType::Generated: return "generated";
        case TransitionType::AutoToplevel: return "auto_toplevel";
        case TransitionType::FormSubmit: return "form_submit";
        case TransitionType::Reload: return "reload";
        case TransitionType::Keyword: return "keyword";
        case TransitionType::KeywordGenerated: return "keyword_generated";
        case TransitionType::Other: return "other";
    }
    return "other";
}


enum class ReferrerPolicy {
    NoReferrer,
    NoReferrerWhenDowngrade,
    Origin,
    OriginWhenCrossOrigin,
    SameOrigin,
    StrictOrigin,
    StrictOriginWhenCrossOrigin,
    UnsafeUrl
};

inline std::string_view toString(ReferrerPolicy policy) {
    switch (policy) {
        case ReferrerPolicy::NoReferrer: return "noReferrer";
        case ReferrerPolicy::NoReferrerWhenDowngrade: return "noReferrerWhenDowngrade";
        case ReferrerPolicy::Origin: return "origin";
        case ReferrerPolicy::OriginWhenCrossOrigin: return "originWhenCrossOrigin";
        case ReferrerPolicy::SameOrigin: return "sameOrigin";
        case ReferrerPolicy::StrictOrigin: return "strictOrigin";
        case ReferrerPolicy::StrictOriginWhenCrossOrigin: return "strictOriginWhenCrossOrigin";
        case ReferrerPolicy::UnsafeUrl: return "unsafeUrl";
    }
    return "noReferrerWhenDowngrade";
}


enum class TransferMode {
    ReturnAsBase64,
    ReturnAsStream
};

inline std::string_view toString(TransferMode mode) {
    switch (mode) {
        case TransferMode::ReturnAsBase64: return "ReturnAsBase64";
        case TransferMode::ReturnAsStream: return "ReturnAsStream";
    }
    return "ReturnAsBase64";
}


enum class KeyEventType {
    KeyDown,
    KeyUp,
    RawKeyDown,
    Char
};

inline std::string_view toString(KeyEventType type) {
    switch (type) {
        case KeyEventType::KeyDown: return "keyDown";
        case KeyEventType::KeyUp: return "keyUp";
        case KeyEventType::RawKeyDown: return "rawKeyDown";
        case KeyEventType::Char: return "char";
    }
    return "keyDown";
}


enum class MouseEventType {
    MousePressed,
    MouseReleased,
    MouseMoved,
    MouseWheel
};

inline std::string_view toString(MouseEventType type) {
    switch (type) {
        case MouseEventType::MousePressed: return "mousePressed";
        case MouseEventType::MouseReleased: return "mouseReleased";
        case MouseEventType::MouseMoved: return "mouseMoved";
        case MouseEventType::MouseWheel: return "mouseWheel";
    }
    return "mousePressed";
}


enum class MouseButton {
    None,
    Left,
    Middle,
    Right,
    Back,
    Forward
};

inline std::string_view toString(MouseButton button) {
    switch (button) {
        case MouseButton::None: return "none";
        case MouseButton::Left: return "left";
        case MouseButton::Middle: return "middle";
        case MouseButton::Right: return "right";
        case MouseButton::Back: return "back";
        case MouseButton::Forward: return "forward";
    }
    return "none";
}


enum class TouchEventType {
    TouchStart,
    TouchEnd,
    TouchMove,
    TouchCancel
};

inline std::string_view toString(TouchEventType type) {
    switch (type) {
        case TouchEventType::TouchStart: return "touchStart";
        case TouchEventType::TouchEnd: return "touchEnd";
        case TouchEventType::TouchMove: return "touchMove";
        case TouchEventType::TouchCancel: return "touchCancel";
    }
    return "touchStart";
}


enum class DragEventType {
    DragEnter,
    DragOver,
    Drop,
    DragCancel
};

inline std::string_view toString(DragEventType type) {
    switch (type) {
        case DragEventType::DragEnter: return "dragEnter";
        case DragEventType::DragOver: return "dragOver";
        case DragEventType::Drop: return "drop";
        case DragEventType::DragCancel: return "dragCancel";
    }
    return "dragEnter";
}


enum class PointerType {
    Mouse,
    Pen,
    Touch
};

inline std::string_view toString(PointerType type) {
    switch (type) {
        case PointerType::Mouse: return "mouse";
        case PointerType::Pen: return "pen";
        case PointerType::Touch: return "touch";
    }
    return "mouse";
}


enum class ResourceType {
    Document,
    Stylesheet,
    Image,
    Media,
    Font,
    Script,
    TextTrack,
    XHR,
    Fetch,
    Prefetch,
    EventSource,
    WebSocket,
    Manifest,
    SignedExchange,
    Ping,
    CSPViolationReport,
    Preflight,
    Other
};

inline std::string_view toString(ResourceType type) {
    switch (type) {
        case ResourceType::Document: return "Document";
        case ResourceType::Stylesheet: return "Stylesheet";
        case ResourceType::Image: return "Image";
        case ResourceType::Media: return "Media";
        case ResourceType::Font: return "Font";
        case ResourceType::Script: return "Script";
        case ResourceType::TextTrack: return "TextTrack";
        case ResourceType::XHR: return "XHR";
        case ResourceType::Fetch: return "Fetch";
        case ResourceType::Prefetch: return "Prefetch";
        case ResourceType::EventSource: return "EventSource";
        case ResourceType::WebSocket: return "WebSocket";
        case ResourceType::Manifest: return "Manifest";
        case ResourceType::SignedExchange: return "SignedExchange";
        case ResourceType::Ping: return "Ping";
        case ResourceType::CSPViolationReport: return "CSPViolationReport";
        case ResourceType::Preflight: return "Preflight";
        case ResourceType::Other: return "Other";
    }
    return "Other";
}


enum class CookieSameSite {
    Strict,
    Lax,
    None
};

inline std::string_view toString(CookieSameSite sameSite) {
    switch (sameSite) {
        case CookieSameSite::Strict: return "Strict";
        case CookieSameSite::Lax: return "Lax";
        case CookieSameSite::None: return "None";
    }
    return "None";
}


enum class CookiePriority {
    Low,
    Medium,
    High
};

inline std::string_view toString(CookiePriority priority) {
    switch (priority) {
        case CookiePriority::Low: return "Low";
        case CookiePriority::Medium: return "Medium";
        case CookiePriority::High: return "High";
    }
    return "Medium";
}


enum class ScreenOrientationType {
    PortraitPrimary,
    PortraitSecondary,
    LandscapePrimary,
    LandscapeSecondary
};

inline std::string_view toString(ScreenOrientationType type) {
    switch (type) {
        case ScreenOrientationType::PortraitPrimary: return "portraitPrimary";
        case ScreenOrientationType::PortraitSecondary: return "portraitSecondary";
        case ScreenOrientationType::LandscapePrimary: return "landscapePrimary";
        case ScreenOrientationType::LandscapeSecondary: return "landscapeSecondary";
    }
    return "portraitPrimary";
}


enum class DisplayFeatureOrientation {
    Vertical,
    Horizontal
};

inline std::string_view toString(DisplayFeatureOrientation orientation) {
    switch (orientation) {
        case DisplayFeatureOrientation::Vertical: return "vertical";
        case DisplayFeatureOrientation::Horizontal: return "horizontal";
    }
    return "vertical";
}


enum class VirtualTimePolicy {
    Advance,
    Pause,
    PauseIfNetworkFetchesPending
};

inline std::string_view toString(VirtualTimePolicy policy) {
    switch (policy) {
        case VirtualTimePolicy::Advance: return "advance";
        case VirtualTimePolicy::Pause: return "pause";
        case VirtualTimePolicy::PauseIfNetworkFetchesPending: return "pauseIfNetworkFetchesPending";
    }
    return "advance";
}


enum class PseudoType {
    FirstLine,
    FirstLetter,
    Before,
    After,
    Marker,
    Backdrop,
    Selection,
    SearchText,
    TargetText,
    SpellingError,
    GrammarError,
    Highlight,
    FirstLineInherited,
    ScrollMarker,
    ScrollMarkerGroup,
    Scrollbar,
    ScrollbarThumb,
    ScrollbarButton,
    ScrollbarTrack,
    ScrollbarTrackPiece,
    ScrollbarCorner,
    Resizer,
    InputListButton,
    ViewTransition,
    ViewTransitionGroup,
    ViewTransitionImagePair,
    ViewTransitionOld,
    ViewTransitionNew
};

inline std::string_view toString(PseudoType type) {
    switch (type) {
        case PseudoType::FirstLine: return "first-line";
        case PseudoType::FirstLetter: return "first-letter";
        case PseudoType::Before: return "before";
        case PseudoType::After: return "after";
        case PseudoType::Marker: return "marker";
        case PseudoType::Backdrop: return "backdrop";
        case PseudoType::Selection: return "selection";
        case PseudoType::SearchText: return "search-text";
        case PseudoType::TargetText: return "target-text";
        case PseudoType::SpellingError: return "spelling-error";
        case PseudoType::GrammarError: return "grammar-error";
        case PseudoType::Highlight: return "highlight";
        case PseudoType::FirstLineInherited: return "first-line-inherited";
        case PseudoType::ScrollMarker: return "scroll-marker";
        case PseudoType::ScrollMarkerGroup: return "scroll-marker-group";
        case PseudoType::Scrollbar: return "scrollbar";
        case PseudoType::ScrollbarThumb: return "scrollbar-thumb";
        case PseudoType::ScrollbarButton: return "scrollbar-button";
        case PseudoType::ScrollbarTrack: return "scrollbar-track";
        case PseudoType::ScrollbarTrackPiece: return "scrollbar-track-piece";
        case PseudoType::ScrollbarCorner: return "scrollbar-corner";
        case PseudoType::Resizer: return "resizer";
        case PseudoType::InputListButton: return "input-list-button";
        case PseudoType::ViewTransition: return "view-transition";
        case PseudoType::ViewTransitionGroup: return "view-transition-group";
        case PseudoType::ViewTransitionImagePair: return "view-transition-image-pair";
        case PseudoType::ViewTransitionOld: return "view-transition-old";
        case PseudoType::ViewTransitionNew: return "view-transition-new";
    }
    return "before";
}


enum class ObjectSubtype {
    Array,
    Null,
    Node,
    Regexp,
    Date,
    Map,
    Set,
    WeakMap,
    WeakSet,
    Iterator,
    Generator,
    Error,
    Proxy,
    Promise,
    TypedArray,
    ArrayBuffer,
    DataView,
    WebAssemblyMemory,
    WasmValue
};

inline std::string_view toString(ObjectSubtype subtype) {
    switch (subtype) {
        case ObjectSubtype::Array: return "array";
        case ObjectSubtype::Null: return "null";
        case ObjectSubtype::Node: return "node";
        case ObjectSubtype::Regexp: return "regexp";
        case ObjectSubtype::Date: return "date";
        case ObjectSubtype::Map: return "map";
        case ObjectSubtype::Set: return "set";
        case ObjectSubtype::WeakMap: return "weakmap";
        case ObjectSubtype::WeakSet: return "weakset";
        case ObjectSubtype::Iterator: return "iterator";
        case ObjectSubtype::Generator: return "generator";
        case ObjectSubtype::Error: return "error";
        case ObjectSubtype::Proxy: return "proxy";
        case ObjectSubtype::Promise: return "promise";
        case ObjectSubtype::TypedArray: return "typedarray";
        case ObjectSubtype::ArrayBuffer: return "arraybuffer";
        case ObjectSubtype::DataView: return "dataview";
        case ObjectSubtype::WebAssemblyMemory: return "webassemblymemory";
        case ObjectSubtype::WasmValue: return "wasmvalue";
    }
    return "null";
}


enum class LifecycleState {
    Load,
    DOMContentLoaded,
    NetworkAlmostIdle,
    NetworkIdle
};

inline std::string_view toString(LifecycleState state) {
    switch (state) {
        case LifecycleState::Load: return "load";
        case LifecycleState::DOMContentLoaded: return "DOMContentLoaded";
        case LifecycleState::NetworkAlmostIdle: return "networkAlmostIdle";
        case LifecycleState::NetworkIdle: return "networkIdle";
    }
    return "load";
}


enum class ImageFormat {
    Jpeg,
    Png,
    Webp
};

inline std::string_view toString(ImageFormat format) {
    switch (format) {
        case ImageFormat::Jpeg: return "jpeg";
        case ImageFormat::Png: return "png";
        case ImageFormat::Webp: return "webp";
    }
    return "png";
}


struct DeviceDescriptor {
    std::string_view name;
    int width;
    int height;
    double deviceScaleFactor;
    bool isMobile;
    bool hasTouch;
    std::string_view userAgent;
};

enum class DevicePreset {
    
    IPhoneSE,
    IPhone12,
    IPhone12Pro,
    IPhone12ProMax,
    IPhone13,
    IPhone13Pro,
    IPhone13ProMax,
    IPhone14,
    IPhone14Pro,
    IPhone14ProMax,

    
    IPad,
    IPadMini,
    IPadAir,
    IPadPro11,
    IPadPro12,

    
    Pixel5,
    Pixel6,
    Pixel7,
    SamsungGalaxyS20,
    SamsungGalaxyS21,
    SamsungGalaxyS22,

    
    SamsungGalaxyTabS7,

    
    Desktop1080p,
    Desktop1440p,
    Desktop4K
};

inline DeviceDescriptor getDeviceDescriptor(DevicePreset preset) {
    switch (preset) {
        case DevicePreset::IPhoneSE:
            return {"iPhone SE", 375, 667, 2.0, true, true, "Mozilla/5.0 (iPhone; CPU iPhone OS 15_0 like Mac OS X) AppleWebKit/605.1.15"};
        case DevicePreset::IPhone12:
            return {"iPhone 12", 390, 844, 3.0, true, true, "Mozilla/5.0 (iPhone; CPU iPhone OS 15_0 like Mac OS X) AppleWebKit/605.1.15"};
        case DevicePreset::IPhone12Pro:
            return {"iPhone 12 Pro", 390, 844, 3.0, true, true, "Mozilla/5.0 (iPhone; CPU iPhone OS 15_0 like Mac OS X) AppleWebKit/605.1.15"};
        case DevicePreset::IPhone12ProMax:
            return {"iPhone 12 Pro Max", 428, 926, 3.0, true, true, "Mozilla/5.0 (iPhone; CPU iPhone OS 15_0 like Mac OS X) AppleWebKit/605.1.15"};
        case DevicePreset::IPhone13:
            return {"iPhone 13", 390, 844, 3.0, true, true, "Mozilla/5.0 (iPhone; CPU iPhone OS 16_0 like Mac OS X) AppleWebKit/605.1.15"};
        case DevicePreset::IPhone13Pro:
            return {"iPhone 13 Pro", 390, 844, 3.0, true, true, "Mozilla/5.0 (iPhone; CPU iPhone OS 16_0 like Mac OS X) AppleWebKit/605.1.15"};
        case DevicePreset::IPhone13ProMax:
            return {"iPhone 13 Pro Max", 428, 926, 3.0, true, true, "Mozilla/5.0 (iPhone; CPU iPhone OS 16_0 like Mac OS X) AppleWebKit/605.1.15"};
        case DevicePreset::IPhone14:
            return {"iPhone 14", 390, 844, 3.0, true, true, "Mozilla/5.0 (iPhone; CPU iPhone OS 17_0 like Mac OS X) AppleWebKit/605.1.15"};
        case DevicePreset::IPhone14Pro:
            return {"iPhone 14 Pro", 393, 852, 3.0, true, true, "Mozilla/5.0 (iPhone; CPU iPhone OS 17_0 like Mac OS X) AppleWebKit/605.1.15"};
        case DevicePreset::IPhone14ProMax:
            return {"iPhone 14 Pro Max", 430, 932, 3.0, true, true, "Mozilla/5.0 (iPhone; CPU iPhone OS 17_0 like Mac OS X) AppleWebKit/605.1.15"};

        case DevicePreset::IPad:
            return {"iPad", 768, 1024, 2.0, true, true, "Mozilla/5.0 (iPad; CPU OS 15_0 like Mac OS X) AppleWebKit/605.1.15"};
        case DevicePreset::IPadMini:
            return {"iPad Mini", 768, 1024, 2.0, true, true, "Mozilla/5.0 (iPad; CPU OS 15_0 like Mac OS X) AppleWebKit/605.1.15"};
        case DevicePreset::IPadAir:
            return {"iPad Air", 820, 1180, 2.0, true, true, "Mozilla/5.0 (iPad; CPU OS 15_0 like Mac OS X) AppleWebKit/605.1.15"};
        case DevicePreset::IPadPro11:
            return {"iPad Pro 11", 834, 1194, 2.0, true, true, "Mozilla/5.0 (iPad; CPU OS 15_0 like Mac OS X) AppleWebKit/605.1.15"};
        case DevicePreset::IPadPro12:
            return {"iPad Pro 12.9", 1024, 1366, 2.0, true, true, "Mozilla/5.0 (iPad; CPU OS 15_0 like Mac OS X) AppleWebKit/605.1.15"};

        case DevicePreset::Pixel5:
            return {"Pixel 5", 393, 851, 2.75, true, true, "Mozilla/5.0 (Linux; Android 12; Pixel 5) AppleWebKit/537.36"};
        case DevicePreset::Pixel6:
            return {"Pixel 6", 412, 915, 2.625, true, true, "Mozilla/5.0 (Linux; Android 13; Pixel 6) AppleWebKit/537.36"};
        case DevicePreset::Pixel7:
            return {"Pixel 7", 412, 915, 2.625, true, true, "Mozilla/5.0 (Linux; Android 14; Pixel 7) AppleWebKit/537.36"};
        case DevicePreset::SamsungGalaxyS20:
            return {"Samsung Galaxy S20", 360, 800, 3.0, true, true, "Mozilla/5.0 (Linux; Android 12; SM-G981B) AppleWebKit/537.36"};
        case DevicePreset::SamsungGalaxyS21:
            return {"Samsung Galaxy S21", 360, 800, 3.0, true, true, "Mozilla/5.0 (Linux; Android 13; SM-G991B) AppleWebKit/537.36"};
        case DevicePreset::SamsungGalaxyS22:
            return {"Samsung Galaxy S22", 360, 780, 3.0, true, true, "Mozilla/5.0 (Linux; Android 14; SM-S901B) AppleWebKit/537.36"};

        case DevicePreset::SamsungGalaxyTabS7:
            return {"Samsung Galaxy Tab S7", 800, 1280, 2.25, true, true, "Mozilla/5.0 (Linux; Android 12; SM-T870) AppleWebKit/537.36"};

        case DevicePreset::Desktop1080p:
            return {"Desktop 1080p", 1920, 1080, 1.0, false, false, ""};
        case DevicePreset::Desktop1440p:
            return {"Desktop 1440p", 2560, 1440, 1.0, false, false, ""};
        case DevicePreset::Desktop4K:
            return {"Desktop 4K", 3840, 2160, 1.0, false, false, ""};
    }
    return {"Desktop 1080p", 1920, 1080, 1.0, false, false, ""};
}

} 
