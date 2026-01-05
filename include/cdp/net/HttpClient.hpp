

#pragma once

#include "../net/Socket.hpp"
#include "../core/Json.hpp"
#include <string>
#include <map>
#include <optional>

namespace cdp {


struct HttpResponse {
    int statusCode = 0;
    std::string statusMessage;
    std::map<std::string, std::string> headers;
    std::string body;

    bool isSuccess() const { return statusCode >= 200 && statusCode < 300; }
    bool isJson() const {
        auto it = headers.find("content-type");
        return it != headers.end() && it->second.find("application/json") != std::string::npos;
    }

    JsonValue json() const {
        return JsonValue::parse(body);
    }
};


class HttpRequest {
public:
    HttpRequest& setMethod(const std::string& method) { method_ = method; return *this; }
    HttpRequest& setPath(const std::string& path) { path_ = path; return *this; }
    HttpRequest& setHeader(const std::string& key, const std::string& value) {
        headers_[key] = value;
        return *this;
    }
    HttpRequest& setBody(const std::string& body) { body_ = body; return *this; }
    HttpRequest& setJsonBody(const JsonValue& json) {
        body_ = json.serialize();
        headers_["Content-Type"] = "application/json";
        return *this;
    }

    std::string build(const std::string& host, int port = 80) const;

private:
    std::string method_ = "GET";
    std::string path_ = "/";
    std::map<std::string, std::string> headers_;
    std::string body_;
};


class HttpClient {
public:
    HttpClient() = default;
    explicit HttpClient(const std::string& host, int port = 80);

    
    bool connect(const std::string& host, int port = 80);
    void disconnect();
    bool isConnected() const { return socket_.isConnected(); }

    
    HttpResponse get(const std::string& path);
    HttpResponse post(const std::string& path, const std::string& body,
                      const std::string& contentType = "application/json");
    HttpResponse put(const std::string& path, const std::string& body,
                     const std::string& contentType = "application/json");
    HttpResponse del(const std::string& path);

    
    HttpResponse request(const HttpRequest& req);

    
    void setTimeout(int ms) { timeoutMs_ = ms; }
    void setKeepAlive(bool keepAlive) { keepAlive_ = keepAlive; }

    
    TcpSocket& socket() { return socket_; }
    TcpSocket extractSocket() { return std::move(socket_); }

private:
    HttpResponse sendRequest(const std::string& request);
    HttpResponse parseResponse();
    std::string readLine();
    std::string readChunked();

    TcpSocket socket_;
    std::string host_;
    int port_ = 80;
    int timeoutMs_ = 30000;
    bool keepAlive_ = true;
};


struct Url {
    std::string scheme = "http";
    std::string host;
    int port = 80;
    std::string path = "/";
    std::string query;

    static std::optional<Url> parse(const std::string& url);
    std::string toString() const;
};

} 
