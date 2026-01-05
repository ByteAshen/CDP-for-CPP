#pragma once

#include "Domain.hpp"

namespace cdp {


struct ManufacturerData {
    int key;
    std::string data;  

    JsonValue toJson() const {
        JsonObject obj;
        obj["key"] = key;
        obj["data"] = data;
        return obj;
    }
};


struct ScanEntry {
    std::string deviceAddress;
    int rssi;
    JsonValue scanRecord;

    JsonValue toJson() const {
        JsonObject obj;
        obj["deviceAddress"] = deviceAddress;
        obj["rssi"] = rssi;
        if (!scanRecord.isNull()) obj["scanRecord"] = scanRecord;
        return obj;
    }
};


class BluetoothEmulation : public Domain {
public:
    explicit BluetoothEmulation(CDPConnection& connection) : Domain(connection, "BluetoothEmulation") {}

    
    CDPResponse enable(const std::string& state) {  
        return call("enable", Params().set("state", state));
    }

    
    CDPResponse disable() { return call("disable"); }

    
    CDPResponse simulateAdvertisement(const ScanEntry& entry) {
        return call("simulateAdvertisement", Params().set("entry", entry.toJson()));
    }

    
    CDPResponse simulatePreconnectedPeripheral(const std::string& address,
                                                 const std::string& name,
                                                 const std::vector<std::string>& manufacturerData,
                                                 const std::vector<std::string>& knownServiceUuids) {
        Params params;
        params.set("address", address);
        params.set("name", name);

        JsonArray mdArr;
        for (const auto& md : manufacturerData) mdArr.push_back(md);
        params.set("manufacturerData", mdArr);

        JsonArray uuids;
        for (const auto& uuid : knownServiceUuids) uuids.push_back(uuid);
        params.set("knownServiceUuids", uuids);

        return call("simulatePreconnectedPeripheral", params);
    }
};

} 
