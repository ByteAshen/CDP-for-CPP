

#include "cdp/net/HttpClient.hpp"
#include <algorithm>
#include <sstream>
#include <cctype>

namespace cdp {


std::string HttpRequest::build(const std::string& host, int port) const {
    std::ostringstream oss;

    oss << method_ << " " << path_ << " HTTP/1.1\r\n";
    oss << "Host: " << host;
    if (port != 80 && port != 443) {
        oss << ":" << port;
    }
    oss << "\r\n";

    auto headers = headers_;
    if (headers.find("Connection") == headers.end()) {
        headers["Connection"] = "keep-alive";
    }
    if (!body_.empty() && headers.find("Content-Length") == headers.end()) {
        headers["Content-Length"] = std::to_string(body_.size());
    }

    for (const auto& [key, value] : headers) {
        oss << key << ": " << value << "\r\n";
    }

    oss << "\r\n";
    if (!body_.empty()) {
        oss << body_;
    }

    return oss.str();
}


HttpClient::HttpClient(const std::string& host, int port) {
    connect(host, port);
}

bool HttpClient::connect(const std::string& host, int port) {
    host_ = host;
    port_ = port;

    socket_ = TcpSocket();
    socket_.setTimeout(timeoutMs_, timeoutMs_);
    socket_.setNoDelay(true);

    return socket_.connect(host, port);
}

void HttpClient::disconnect() {
    socket_.disconnect();
}

HttpResponse HttpClient::get(const std::string& path) {
    HttpRequest req;
    req.setMethod("GET").setPath(path);
    return request(req);
}

HttpResponse HttpClient::post(const std::string& path, const std::string& body,
                               const std::string& contentType) {
    HttpRequest req;
    req.setMethod("POST")
       .setPath(path)
       .setHeader("Content-Type", contentType)
       .setBody(body);
    return request(req);
}

HttpResponse HttpClient::put(const std::string& path, const std::string& body,
                              const std::string& contentType) {
    HttpRequest req;
    req.setMethod("PUT")
       .setPath(path)
       .setHeader("Content-Type", contentType)
       .setBody(body);
    return request(req);
}

HttpResponse HttpClient::del(const std::string& path) {
    HttpRequest req;
    req.setMethod("DELETE").setPath(path);
    return request(req);
}

HttpResponse HttpClient::request(const HttpRequest& req) {
    if (!isConnected()) {
        if (!connect(host_, port_)) {
            HttpResponse resp;
            resp.statusCode = 0;
            resp.statusMessage = "Connection failed";
            return resp;
        }
    }

    std::string requestStr = req.build(host_, port_);
    return sendRequest(requestStr);
}

HttpResponse HttpClient::sendRequest(const std::string& request) {
    if (socket_.send(request) < 0) {
        HttpResponse resp;
        resp.statusCode = 0;
        resp.statusMessage = "Send failed";
        return resp;
    }

    return parseResponse();
}

HttpResponse HttpClient::parseResponse() {
    HttpResponse resp;

    
    std::string statusLine = readLine();
    if (statusLine.empty()) {
        resp.statusMessage = "No response";
        return resp;
    }

    
    size_t firstSpace = statusLine.find(' ');
    if (firstSpace == std::string::npos) {
        resp.statusMessage = "Invalid status line";
        return resp;
    }

    size_t secondSpace = statusLine.find(' ', firstSpace + 1);
    std::string codeStr = statusLine.substr(firstSpace + 1,
        secondSpace == std::string::npos ? std::string::npos : secondSpace - firstSpace - 1);

    try {
        resp.statusCode = std::stoi(codeStr);
    } catch (const std::exception&) {
        resp.statusMessage = "Invalid status code: " + codeStr;
        return resp;
    }

    if (secondSpace != std::string::npos) {
        resp.statusMessage = statusLine.substr(secondSpace + 1);
        
        if (!resp.statusMessage.empty() && resp.statusMessage.back() == '\r') {
            resp.statusMessage.pop_back();
        }
    }

    
    int contentLength = -1;
    bool chunked = false;

    while (true) {
        std::string line = readLine();
        if (line.empty() || line == "\r" || line == "\r\n") break;

        size_t colonPos = line.find(':');
        if (colonPos == std::string::npos) continue;

        std::string key = line.substr(0, colonPos);
        std::string value = line.substr(colonPos + 1);

        
        while (!value.empty() && (value[0] == ' ' || value[0] == '\t')) {
            value.erase(0, 1);
        }
        while (!value.empty() && (value.back() == '\r' || value.back() == '\n')) {
            value.pop_back();
        }

        
        std::string keyLower = key;
        std::transform(keyLower.begin(), keyLower.end(), keyLower.begin(),
            [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

        resp.headers[keyLower] = value;

        if (keyLower == "content-length") {
            try {
                contentLength = std::stoi(value);
            } catch (const std::exception&) {
                
            }
        } else if (keyLower == "transfer-encoding") {
            std::string valueLower = value;
            std::transform(valueLower.begin(), valueLower.end(), valueLower.begin(),
                [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
            chunked = (valueLower.find("chunked") != std::string::npos);
        }
    }

    
    if (chunked) {
        resp.body = readChunked();
    } else if (contentLength > 0) {
        resp.body = socket_.recvExact(contentLength, timeoutMs_);
    } else if (contentLength == 0) {
        
    } else {
        
        resp.body = socket_.recvAll(1000);
    }

    
    auto connHeader = resp.headers.find("connection");
    if (connHeader != resp.headers.end()) {
        std::string connValue = connHeader->second;
        std::transform(connValue.begin(), connValue.end(), connValue.begin(),
            [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        if (connValue == "close") {
            disconnect();
        }
    }

    return resp;
}

std::string HttpClient::readLine() {
    return socket_.recvUntil("\n", timeoutMs_);
}

std::string HttpClient::readChunked() {
    std::string result;

    while (true) {
        std::string sizeLine = readLine();
        if (sizeLine.empty()) break;

        
        while (!sizeLine.empty() && (sizeLine.back() == '\r' || sizeLine.back() == '\n')) {
            sizeLine.pop_back();
        }

        
        int chunkSize = 0;
        try {
            chunkSize = std::stoi(sizeLine, nullptr, 16);
        } catch (const std::exception&) {
            
            break;
        }

        if (chunkSize == 0) {
            
            readLine();
            break;
        }

        
        std::string chunk = socket_.recvExact(chunkSize, timeoutMs_);
        result += chunk;

        
        readLine();
    }

    return result;
}


std::optional<Url> Url::parse(const std::string& urlStr) {
    Url url;

    size_t pos = 0;

    
    size_t schemeEnd = urlStr.find("://");
    if (schemeEnd != std::string::npos) {
        url.scheme = urlStr.substr(0, schemeEnd);
        pos = schemeEnd + 3;
    }

    
    if (url.scheme == "https" || url.scheme == "wss") {
        url.port = 443;
    } else {
        url.port = 80;
    }

    
    size_t pathStart = urlStr.find('/', pos);
    size_t hostEnd = pathStart != std::string::npos ? pathStart : urlStr.size();

    
    std::string hostPart = urlStr.substr(pos, hostEnd - pos);
    size_t colonPos = hostPart.rfind(':');

    if (colonPos != std::string::npos) {
        url.host = hostPart.substr(0, colonPos);
        try {
            url.port = std::stoi(hostPart.substr(colonPos + 1));
        } catch (const std::exception&) {
            
            return std::nullopt;
        }
    } else {
        url.host = hostPart;
    }

    if (url.host.empty()) {
        return std::nullopt;
    }

    
    if (pathStart != std::string::npos) {
        size_t queryStart = urlStr.find('?', pathStart);
        if (queryStart != std::string::npos) {
            url.path = urlStr.substr(pathStart, queryStart - pathStart);
            url.query = urlStr.substr(queryStart + 1);
        } else {
            url.path = urlStr.substr(pathStart);
        }
    }

    return url;
}

std::string Url::toString() const {
    std::ostringstream oss;
    oss << scheme << "://" << host;
    if ((scheme == "http" && port != 80) || (scheme == "https" && port != 443)) {
        oss << ":" << port;
    }
    oss << path;
    if (!query.empty()) {
        oss << "?" << query;
    }
    return oss.str();
}

} 
