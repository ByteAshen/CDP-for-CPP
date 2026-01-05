

#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <winsock2.h>
#include <ws2tcpip.h>
#include <string>
#include <stdexcept>
#include <memory>
#include <mutex>
#include <atomic>

#pragma comment(lib, "ws2_32.lib")

namespace cdp {


class WinsockInit {
public:
    static WinsockInit& instance();
    ~WinsockInit();

    WinsockInit(const WinsockInit&) = delete;
    WinsockInit& operator=(const WinsockInit&) = delete;

private:
    WinsockInit();
    bool initialized_ = false;
};


class TcpSocket {
public:
    TcpSocket();
    explicit TcpSocket(SOCKET sock);
    ~TcpSocket();

    TcpSocket(TcpSocket&& other) noexcept;
    TcpSocket& operator=(TcpSocket&& other) noexcept;
    TcpSocket(const TcpSocket&) = delete;
    TcpSocket& operator=(const TcpSocket&) = delete;

    
    bool connect(const std::string& host, int port);
    void disconnect();
    bool isConnected() const;

    
    int send(const char* data, int len);
    int send(const std::string& data);
    int recv(char* buffer, int len);
    std::string recvAll(int timeoutMs = 5000);
    std::string recvUntil(const std::string& delimiter, int timeoutMs = 5000);
    std::string recvExact(int bytes, int timeoutMs = 5000);

    
    void setBlocking(bool blocking);
    void setTimeout(int sendMs, int recvMs);
    void setNoDelay(bool noDelay);

    
    SOCKET handle() const { return socket_; }
    bool isValid() const { return socket_ != INVALID_SOCKET; }

    
    bool hasData(int timeoutMs = 0) const;

private:
    SOCKET socket_ = INVALID_SOCKET;
    std::atomic<bool> connected_{false};

    
    mutable std::mutex sendMutex_;
    mutable std::mutex recvMutex_;
    mutable std::mutex optionsMutex_;

    void close();
};


class SocketError : public std::runtime_error {
public:
    explicit SocketError(const std::string& message);
    SocketError(const std::string& message, int errorCode);
    int errorCode() const { return errorCode_; }

private:
    int errorCode_;
};


std::string getHostByName(const std::string& hostname);
std::string getLastErrorMessage();

} 
