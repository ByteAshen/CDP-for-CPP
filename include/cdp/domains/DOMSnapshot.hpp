#pragma once

#include "Domain.hpp"

namespace cdp {


class DOMSnapshot : public Domain {
public:
    explicit DOMSnapshot(CDPConnection& connection) : Domain(connection, "DOMSnapshot") {}

    
    CDPResponse disable() { return call("disable"); }

    
    CDPResponse enable() { return call("enable"); }

    
    CDPResponse getSnapshot(const std::vector<std::string>& computedStyleWhitelist,
                             bool includeEventListeners = false,
                             bool includePaintOrder = false,
                             bool includeUserAgentShadowTree = false) {
        JsonArray styles;
        for (const auto& s : computedStyleWhitelist) styles.push_back(s);
        Params params;
        params.set("computedStyleWhitelist", styles);
        if (includeEventListeners) params.set("includeEventListeners", true);
        if (includePaintOrder) params.set("includePaintOrder", true);
        if (includeUserAgentShadowTree) params.set("includeUserAgentShadowTree", true);
        return call("getSnapshot", params);
    }

    
    CDPResponse captureSnapshot(const std::vector<std::string>& computedStyles,
                                 bool includePaintOrder = false,
                                 bool includeDOMRects = false,
                                 bool includeBlendedBackgroundColors = false,
                                 bool includeTextColorOpacities = false) {
        JsonArray styles;
        for (const auto& s : computedStyles) styles.push_back(s);
        Params params;
        params.set("computedStyles", styles);
        if (includePaintOrder) params.set("includePaintOrder", true);
        if (includeDOMRects) params.set("includeDOMRects", true);
        if (includeBlendedBackgroundColors) params.set("includeBlendedBackgroundColors", true);
        if (includeTextColorOpacities) params.set("includeTextColorOpacities", true);
        return call("captureSnapshot", params);
    }
};

} 
