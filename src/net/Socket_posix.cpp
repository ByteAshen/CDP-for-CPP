

#include "cdp/net/Socket.hpp"

#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <cstring>
#include <sstream>

namespace cdp {


SocketInit& SocketInit::instance() {
    static SocketInit instance;
    return instance;
}

SocketInit::SocketInit() {
    
    initialized_ = true;
}

SocketInit::~SocketInit() {
    
}


TcpSocket::TcpSocket() {
    SocketInit::instance();
    socket_ = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (socket_ == InvalidSocket) {
        throw SocketError("Failed to create socket", errno);
    }
}

TcpSocket::TcpSocket(SocketHandle sock) : socket_(sock), connected_(sock != InvalidSocket) {
    SocketInit::instance();
}

TcpSocket::~TcpSocket() {
    close();
}

TcpSocket::TcpSocket(TcpSocket&& other) noexcept
    : socket_(other.socket_), connected_(other.connected_.load()) {
    other.socket_ = InvalidSocket;
    other.connected_ = false;
}

TcpSocket& TcpSocket::operator=(TcpSocket&& other) noexcept {
    if (this != &other) {
        close();
        socket_ = other.socket_;
        connected_ = other.connected_.load();
        other.socket_ = InvalidSocket;
        other.connected_ = false;
    }
    return *this;
}

bool TcpSocket::connect(const std::string& host, int port) {
    if (!isValid()) return false;

    std::string ip = getHostByName(host);
    if (ip.empty()) return false;

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(static_cast<uint16_t>(port));
    inet_pton(AF_INET, ip.c_str(), &addr.sin_addr);

    int result = ::connect(static_cast<int>(socket_), reinterpret_cast<sockaddr*>(&addr), sizeof(addr));
    if (result < 0) {
        return false;
    }

    connected_ = true;
    return true;
}

void TcpSocket::disconnect() {
    if (connected_) {
        shutdown(static_cast<int>(socket_), SHUT_RDWR);
        connected_ = false;
    }
}

bool TcpSocket::isConnected() const {
    return connected_;
}

void TcpSocket::close() {
    if (socket_ != InvalidSocket) {
        disconnect();
        ::close(static_cast<int>(socket_));
        socket_ = InvalidSocket;
    }
}

int TcpSocket::send(const char* data, int len) {
    if (!isConnected()) return -1;

    std::lock_guard<std::mutex> lock(sendMutex_);
    int sock = static_cast<int>(socket_);

    int totalSent = 0;
    while (totalSent < len) {
        ssize_t sent = ::send(sock, data + totalSent, len - totalSent, MSG_NOSIGNAL);
        if (sent < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) continue;
            connected_ = false;
            return -1;
        }
        if (sent == 0) {
            connected_ = false;
            return totalSent;
        }
        totalSent += static_cast<int>(sent);
    }
    return totalSent;
}

int TcpSocket::send(const std::string& data) {
    return send(data.c_str(), static_cast<int>(data.size()));
}

int TcpSocket::recv(char* buffer, int len) {
    if (!isConnected()) return -1;

    std::lock_guard<std::mutex> lock(recvMutex_);
    int sock = static_cast<int>(socket_);

    ssize_t received = ::recv(sock, buffer, len, 0);
    if (received < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) return 0;
        connected_ = false;
        return -1;
    }
    if (received == 0) {
        connected_ = false;
    }
    return static_cast<int>(received);
}

std::string TcpSocket::recvAll(int timeoutMs) {
    std::lock_guard<std::mutex> lock(recvMutex_);
    int sock = static_cast<int>(socket_);

    std::string result;
    char buffer[8192];

    fd_set readSet;
    timeval timeout;
    timeout.tv_sec = timeoutMs / 1000;
    timeout.tv_usec = (timeoutMs % 1000) * 1000;

    while (true) {
        FD_ZERO(&readSet);
        FD_SET(sock, &readSet);

        int selectResult = select(sock + 1, &readSet, nullptr, nullptr, &timeout);
        if (selectResult == 0) break; 
        if (selectResult < 0) break;  

        ssize_t received = ::recv(sock, buffer, sizeof(buffer), 0);
        if (received <= 0) {
            if (received == 0) connected_ = false;
            break;
        }
        result.append(buffer, received);

        
        timeout.tv_sec = 0;
        timeout.tv_usec = 100000; 
    }

    return result;
}

std::string TcpSocket::recvUntil(const std::string& delimiter, int timeoutMs) {
    std::lock_guard<std::mutex> lock(recvMutex_);
    int sock = static_cast<int>(socket_);

    std::string result;
    result.reserve(4096);
    char buffer[4096];
    const size_t delimLen = delimiter.size();

    fd_set readSet;
    timeval timeout;
    timeout.tv_sec = timeoutMs / 1000;
    timeout.tv_usec = (timeoutMs % 1000) * 1000;

    while (true) {
        FD_ZERO(&readSet);
        FD_SET(sock, &readSet);

        int selectResult = select(sock + 1, &readSet, nullptr, nullptr, &timeout);
        if (selectResult == 0) break;
        if (selectResult < 0) break;

        
        ssize_t received = ::recv(sock, buffer, sizeof(buffer), MSG_PEEK);
        if (received <= 0) {
            if (received == 0) connected_ = false;
            break;
        }

        
        int consumeBytes = static_cast<int>(received);
        for (int i = 0; i <= static_cast<int>(received) - static_cast<int>(delimLen); i++) {
            bool found = true;
            for (size_t j = 0; j < delimLen; j++) {
                if (buffer[i + j] != delimiter[j]) {
                    found = false;
                    break;
                }
            }
            if (found) {
                consumeBytes = i + static_cast<int>(delimLen);
                break;
            }
        }

        
        received = ::recv(sock, buffer, consumeBytes, 0);
        if (received <= 0) {
            if (received == 0) connected_ = false;
            break;
        }
        result.append(buffer, received);

        
        if (result.size() >= delimLen) {
            bool found = true;
            size_t start = result.size() - delimLen;
            for (size_t j = 0; j < delimLen; j++) {
                if (result[start + j] != delimiter[j]) {
                    found = false;
                    break;
                }
            }
            if (found) break;
        }

        timeout.tv_sec = 0;
        timeout.tv_usec = 100000;
    }

    return result;
}

std::string TcpSocket::recvExact(int bytes, int timeoutMs) {
    std::lock_guard<std::mutex> lock(recvMutex_);
    int sock = static_cast<int>(socket_);

    std::string result;
    result.reserve(bytes);
    char buffer[8192];

    fd_set readSet;
    timeval timeout;
    timeout.tv_sec = timeoutMs / 1000;
    timeout.tv_usec = (timeoutMs % 1000) * 1000;

    int remaining = bytes;
    while (remaining > 0) {
        FD_ZERO(&readSet);
        FD_SET(sock, &readSet);

        int selectResult = select(sock + 1, &readSet, nullptr, nullptr, &timeout);
        if (selectResult == 0) break;
        if (selectResult < 0) break;

        int toRead = (remaining < static_cast<int>(sizeof(buffer))) ? remaining : static_cast<int>(sizeof(buffer));
        ssize_t received = ::recv(sock, buffer, toRead, 0);
        if (received <= 0) {
            if (received == 0) connected_ = false;
            break;
        }
        result.append(buffer, received);
        remaining -= static_cast<int>(received);
    }

    return result;
}

void TcpSocket::setBlocking(bool blocking) {
    std::lock_guard<std::mutex> lock(optionsMutex_);
    int sock = static_cast<int>(socket_);
    int flags = fcntl(sock, F_GETFL, 0);
    if (blocking) {
        fcntl(sock, F_SETFL, flags & ~O_NONBLOCK);
    } else {
        fcntl(sock, F_SETFL, flags | O_NONBLOCK);
    }
}

void TcpSocket::setTimeout(int sendMs, int recvMs) {
    std::lock_guard<std::mutex> lock(optionsMutex_);
    int sock = static_cast<int>(socket_);

    timeval sendTimeout;
    sendTimeout.tv_sec = sendMs / 1000;
    sendTimeout.tv_usec = (sendMs % 1000) * 1000;

    timeval recvTimeout;
    recvTimeout.tv_sec = recvMs / 1000;
    recvTimeout.tv_usec = (recvMs % 1000) * 1000;

    setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, &sendTimeout, sizeof(sendTimeout));
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &recvTimeout, sizeof(recvTimeout));
}

void TcpSocket::setNoDelay(bool noDelay) {
    std::lock_guard<std::mutex> lock(optionsMutex_);
    int sock = static_cast<int>(socket_);
    int flag = noDelay ? 1 : 0;
    setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, &flag, sizeof(flag));
}

bool TcpSocket::hasData(int timeoutMs) const {
    int sock = static_cast<int>(socket_);
    fd_set readSet;
    FD_ZERO(&readSet);
    FD_SET(sock, &readSet);

    timeval timeout;
    timeout.tv_sec = timeoutMs / 1000;
    timeout.tv_usec = (timeoutMs % 1000) * 1000;

    int result = select(sock + 1, &readSet, nullptr, nullptr, &timeout);
    return result > 0;
}


SocketError::SocketError(const std::string& message)
    : std::runtime_error(message), errorCode_(0) {}

SocketError::SocketError(const std::string& message, int errorCode)
    : std::runtime_error(message + " (error code: " + std::to_string(errorCode) + ")"),
      errorCode_(errorCode) {}


std::string getHostByName(const std::string& hostname) {
    SocketInit::instance();

    addrinfo hints{};
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    addrinfo* result = nullptr;
    int status = getaddrinfo(hostname.c_str(), nullptr, &hints, &result);
    if (status != 0 || !result) {
        return "";
    }

    char ip[INET_ADDRSTRLEN];
    sockaddr_in* addr = reinterpret_cast<sockaddr_in*>(result->ai_addr);
    inet_ntop(AF_INET, &addr->sin_addr, ip, sizeof(ip));

    freeaddrinfo(result);
    return std::string(ip);
}

int getLastSocketError() {
    return errno;
}

std::string getLastSocketErrorMessage() {
    return std::string(strerror(errno));
}

} 
