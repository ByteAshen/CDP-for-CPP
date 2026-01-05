#pragma once

#include "Domain.hpp"

namespace cdp {


struct PerformanceMetric {
    std::string name;
    double value;

    static PerformanceMetric fromJson(const JsonValue& json) {
        PerformanceMetric m;
        m.name = json["name"].getString();
        m.value = json["value"].getNumber();
        return m;
    }
};


class Performance : public Domain {
public:
    explicit Performance(CDPConnection& connection) : Domain(connection, "Performance") {}

    
    CDPResponse enable(const std::string& timeDomain = "") {
        Params params;
        if (!timeDomain.empty()) params.set("timeDomain", timeDomain);  
        return call("enable", params);
    }

    
    CDPResponse disable() { return call("disable"); }

    
    CDPResponse getMetrics() { return call("getMetrics"); }

    
    CDPResponse setTimeDomain(const std::string& timeDomain) {
        return call("setTimeDomain", Params().set("timeDomain", timeDomain));
    }

    
    void onMetrics(std::function<void(const std::vector<PerformanceMetric>& metrics, const std::string& title)> callback) {
        on("metrics", [callback](const CDPEvent& event) {
            std::vector<PerformanceMetric> metrics;
            if (event.params["metrics"].isArray()) {
                for (const auto& m : event.params["metrics"].asArray()) {
                    metrics.push_back(PerformanceMetric::fromJson(m));
                }
            }
            callback(metrics, event.params["title"].getString());
        });
    }
};

} 
