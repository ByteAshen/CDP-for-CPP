

#pragma once

#include <string>
#include <stdexcept>
#include <mutex>
#include <atomic>

namespace cdp {


using SocketHandle = intptr_t;
constexpr SocketHandle InvalidSocket = -1;


class SocketInit {
public:
    static SocketInit& instance();
    ~SocketInit();

    SocketInit(const SocketInit&) = delete;
    SocketInit& operator=(const SocketInit&) = delete;

private:
    SocketInit();
    bool initialized_ = false;
};


class TcpSocket {
public:
    TcpSocket();
    explicit TcpSocket(SocketHandle sock);
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

    
    SocketHandle handle() const { return socket_; }
    bool isValid() const { return socket_ != InvalidSocket; }

    
    bool hasData(int timeoutMs = 0) const;

private:
    SocketHandle socket_ = InvalidSocket;
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
std::string getLastSocketErrorMessage();
int getLastSocketError();

} 
