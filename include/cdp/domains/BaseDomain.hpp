#pragma once

#include "../CDPConnection.hpp"
#include <string>
#include <functional>

namespace cdp {


class BaseDomain {
public:
    explicit BaseDomain(CDPConnection& connection) : connection_(connection) {}
    virtual ~BaseDomain() = default;

    
    virtual CDPResponse enable() { return sendCommand(domainName() + ".enable"); }
    virtual CDPResponse disable() { return sendCommand(domainName() + ".disable"); }

    
    virtual std::string domainName() const = 0;

protected:
    
    CDPResponse sendCommand(const std::string& method) {
        return connection_.sendCommandSync(method);
    }

    CDPResponse sendCommand(const std::string& method, const JsonValue& params) {
        return connection_.sendCommandSync(method, params);
    }

    
    int64_t sendCommandAsync(const std::string& method, ResponseCallback callback = nullptr) {
        return connection_.sendCommand(method, callback);
    }

    int64_t sendCommandAsync(const std::string& method, const JsonValue& params,
                              ResponseCallback callback = nullptr) {
        return connection_.sendCommand(method, params, callback);
    }

    
    void subscribeEvent(const std::string& eventName, EventCallback callback) {
        connection_.onEvent(domainName() + "." + eventName, callback);
    }

    CDPConnection& connection_;
};

} 
