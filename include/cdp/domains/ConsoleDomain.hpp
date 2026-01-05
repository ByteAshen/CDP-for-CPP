#pragma once

#include "BaseDomain.hpp"
#include <string>
#include <vector>

namespace cdp {


struct ConsoleMessage {
    std::string source;   
    std::string level;    
    std::string text;
    std::string url;
    int line = 0;
    int column = 0;

    static ConsoleMessage fromJson(const JsonValue& json);
};


class ConsoleDomain : public BaseDomain {
public:
    using BaseDomain::BaseDomain;
    std::string domainName() const override { return "Console"; }

    
    CDPResponse clearMessages();

    
    void onMessageAdded(std::function<void(const ConsoleMessage&)> callback);
};

} 
