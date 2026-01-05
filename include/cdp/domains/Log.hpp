#pragma once

#include "Domain.hpp"

namespace cdp {


struct LogEntry {
    std::string source;  
    std::string level;   
    std::string text;
    double timestamp;
    std::string url;
    int lineNumber;
    std::string stackTrace;
    std::string networkRequestId;
    std::string workerId;
    JsonValue args;

    static LogEntry fromJson(const JsonValue& json) {
        LogEntry e;
        e.source = json["source"].getString();
        e.level = json["level"].getString();
        e.text = json["text"].getString();
        e.timestamp = json["timestamp"].getNumber();
        e.url = json["url"].getString();
        e.lineNumber = json["lineNumber"].getInt();
        e.networkRequestId = json["networkRequestId"].getString();
        e.workerId = json["workerId"].getString();
        e.args = json["args"];
        return e;
    }
};


struct ViolationSetting {
    std::string name;  
    double threshold;

    JsonValue toJson() const {
        JsonObject obj;
        obj["name"] = name;
        obj["threshold"] = threshold;
        return obj;
    }
};


class Log : public Domain {
public:
    explicit Log(CDPConnection& connection) : Domain(connection, "Log") {}

    
    CDPResponse enable() { return call("enable"); }

    
    CDPResponse disable() { return call("disable"); }

    
    CDPResponse clear() { return call("clear"); }

    
    CDPResponse startViolationsReport(const std::vector<ViolationSetting>& config) {
        JsonArray arr;
        for (const auto& v : config) arr.push_back(v.toJson());
        return call("startViolationsReport", Params().set("config", arr));
    }

    
    CDPResponse stopViolationsReport() { return call("stopViolationsReport"); }

    
    void onEntryAdded(std::function<void(const LogEntry& entry)> callback) {
        on("entryAdded", [callback](const CDPEvent& event) {
            callback(LogEntry::fromJson(event.params["entry"]));
        });
    }
};

} 
