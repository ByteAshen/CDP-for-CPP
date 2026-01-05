#pragma once

#include "Domain.hpp"

namespace cdp {


class DeviceOrientation : public Domain {
public:
    explicit DeviceOrientation(CDPConnection& connection) : Domain(connection, "DeviceOrientation") {}

    
    CDPResponse clearDeviceOrientationOverride() {
        return call("clearDeviceOrientationOverride");
    }

    
    CDPResponse setDeviceOrientationOverride(double alpha, double beta, double gamma) {
        return call("setDeviceOrientationOverride", Params()
            .set("alpha", alpha)
            .set("beta", beta)
            .set("gamma", gamma));
    }
};

} 
