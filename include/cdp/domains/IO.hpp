#pragma once

#include "Domain.hpp"

namespace cdp {


class IO : public Domain {
public:
    explicit IO(CDPConnection& connection) : Domain(connection, "IO") {}

    
    CDPResponse read(const std::string& handle, int offset = -1, int size = -1) {
        Params params;
        params.set("handle", handle);
        if (offset >= 0) params.set("offset", offset);
        if (size > 0) params.set("size", size);
        return call("read", params);
    }

    
    CDPResponse close(const std::string& handle) {
        return call("close", Params().set("handle", handle));
    }

    
    CDPResponse resolveBlob(const std::string& objectId) {
        return call("resolveBlob", Params().set("objectId", objectId));
    }
};

} 
