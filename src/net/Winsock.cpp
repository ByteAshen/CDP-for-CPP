

#include "cdp/net/Winsock.hpp"
#include <sstream>

namespace cdp {


WinsockInit& WinsockInit::instance() {
    static WinsockInit instance;
    return instance;
}

WinsockInit::WinsockInit() {
    WSADATA wsaData;
    int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (result != 0) {
        throw SocketError("WSAStartup failed", result);
    }
    initialized_ = true;
}

WinsockInit::~WinsockInit() {
    if (initialized_) {
        WSACleanup();
    }
}


TcpSocket::TcpSocket() {
    WinsockInit::instance();
    socket_ = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (socket_ == INVALID_SOCKET) {
        throw SocketError("Failed to create socket", WSAGetLastError());
    }
}

TcpSocket::TcpSocket(SOCKET sock) : socket_(sock), connected_(sock != INVALID_SOCKET) {
    WinsockInit::instance();
}

TcpSocket::~TcpSocket() {
    close();
}

TcpSocket::TcpSocket(TcpSocket&& other) noexcept
    : socket_(other.socket_), connected_(other.connected_.load()) {
    other.socket_ = INVALID_SOCKET;
    other.connected_ = false;
}

TcpSocket& TcpSocket::operator=(TcpSocket&& other) noexcept {
    if (this != &other) {
        close();
        socket_ = other.socket_;
        connected_ = other.connected_.load();
        other.socket_ = INVALID_SOCKET;
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
    addr.sin_port = htons(static_cast<u_short>(port));
    inet_pton(AF_INET, ip.c_str(), &addr.sin_addr);

    int result = ::connect(socket_, reinterpret_cast<sockaddr*>(&addr), sizeof(addr));
    if (result == SOCKET_ERROR) {
        return false;
    }

    connected_ = true;
    return true;
}

void TcpSocket::disconnect() {
    if (connected_) {
        shutdown(socket_, SD_BOTH);
        connected_ = false;
    }
}

bool TcpSocket::isConnected() const {
    return connected_;
}

void TcpSocket::close() {
    if (socket_ != INVALID_SOCKET) {
        disconnect();
        closesocket(socket_);
        socket_ = INVALID_SOCKET;
    }
}

int TcpSocket::send(const char* data, int len) {
    if (!isConnected()) return -1;

    std::lock_guard<std::mutex> lock(sendMutex_);

    int totalSent = 0;
    while (totalSent < len) {
        int sent = ::send(socket_, data + totalSent, len - totalSent, 0);
        if (sent == SOCKET_ERROR) {
            int err = WSAGetLastError();
            if (err == WSAEWOULDBLOCK) continue;
            connected_ = false;
            return -1;
        }
        if (sent == 0) {
            connected_ = false;
            return totalSent;
        }
        totalSent += sent;
    }
    return totalSent;
}

int TcpSocket::send(const std::string& data) {
    return send(data.c_str(), static_cast<int>(data.size()));
}

int TcpSocket::recv(char* buffer, int len) {
    if (!isConnected()) return -1;

    std::lock_guard<std::mutex> lock(recvMutex_);

    int received = ::recv(socket_, buffer, len, 0);
    if (received == SOCKET_ERROR) {
        int err = WSAGetLastError();
        if (err == WSAEWOULDBLOCK) return 0;
        connected_ = false;
        return -1;
    }
    if (received == 0) {
        connected_ = false;
    }
    return received;
}

std::string TcpSocket::recvAll(int timeoutMs) {
    std::lock_guard<std::mutex> lock(recvMutex_);

    std::string result;
    char buffer[8192];

    fd_set readSet;
    timeval timeout;
    timeout.tv_sec = timeoutMs / 1000;
    timeout.tv_usec = (timeoutMs % 1000) * 1000;

    while (true) {
        FD_ZERO(&readSet);
        FD_SET(socket_, &readSet);

        int selectResult = select(0, &readSet, nullptr, nullptr, &timeout);
        if (selectResult == 0) break; 
        if (selectResult == SOCKET_ERROR) break;

        
        int received = ::recv(socket_, buffer, sizeof(buffer), 0);
        if (received == SOCKET_ERROR || received == 0) {
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
        FD_SET(socket_, &readSet);

        int selectResult = select(0, &readSet, nullptr, nullptr, &timeout);
        if (selectResult == 0) break; 
        if (selectResult == SOCKET_ERROR) break;

        
        int received = ::recv(socket_, buffer, sizeof(buffer), MSG_PEEK);
        if (received == SOCKET_ERROR || received == 0) {
            if (received == 0) connected_ = false;
            break;
        }

        
        int consumeBytes = received;
        for (int i = 0; i <= received - static_cast<int>(delimLen); i++) {
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

        
        received = ::recv(socket_, buffer, consumeBytes, 0);
        if (received == SOCKET_ERROR || received == 0) {
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
        FD_SET(socket_, &readSet);

        int selectResult = select(0, &readSet, nullptr, nullptr, &timeout);
        if (selectResult == 0) break; 
        if (selectResult == SOCKET_ERROR) break;

        int toRead = (remaining < static_cast<int>(sizeof(buffer))) ? remaining : static_cast<int>(sizeof(buffer));
        
        int received = ::recv(socket_, buffer, toRead, 0);
        if (received == SOCKET_ERROR || received == 0) {
            if (received == 0) connected_ = false;
            break;
        }
        result.append(buffer, received);
        remaining -= received;
    }

    return result;
}

void TcpSocket::setBlocking(bool blocking) {
    std::lock_guard<std::mutex> lock(optionsMutex_);
    u_long mode = blocking ? 0 : 1;
    ioctlsocket(socket_, FIONBIO, &mode);
}

void TcpSocket::setTimeout(int sendMs, int recvMs) {
    std::lock_guard<std::mutex> lock(optionsMutex_);
    DWORD sendTimeout = sendMs;
    DWORD recvTimeout = recvMs;
    setsockopt(socket_, SOL_SOCKET, SO_SNDTIMEO, reinterpret_cast<char*>(&sendTimeout), sizeof(sendTimeout));
    setsockopt(socket_, SOL_SOCKET, SO_RCVTIMEO, reinterpret_cast<char*>(&recvTimeout), sizeof(recvTimeout));
}

void TcpSocket::setNoDelay(bool noDelay) {
    std::lock_guard<std::mutex> lock(optionsMutex_);
    int flag = noDelay ? 1 : 0;
    setsockopt(socket_, IPPROTO_TCP, TCP_NODELAY, reinterpret_cast<char*>(&flag), sizeof(flag));
}

bool TcpSocket::hasData(int timeoutMs) const {
    
    fd_set readSet;
    FD_ZERO(&readSet);
    FD_SET(socket_, &readSet);

    timeval timeout;
    timeout.tv_sec = timeoutMs / 1000;
    timeout.tv_usec = (timeoutMs % 1000) * 1000;

    int result = select(0, &readSet, nullptr, nullptr, &timeout);
    return result > 0;
}


SocketError::SocketError(const std::string& message)
    : std::runtime_error(message), errorCode_(0) {}

SocketError::SocketError(const std::string& message, int errorCode)
    : std::runtime_error(message + " (error code: " + std::to_string(errorCode) + ")"),
      errorCode_(errorCode) {}


std::string getHostByName(const std::string& hostname) {
    WinsockInit::instance();

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

std::string getLastErrorMessage() {
    int error = WSAGetLastError();
    char* msg = nullptr;
    FormatMessageA(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
        nullptr,
        error,
        0,
        reinterpret_cast<LPSTR>(&msg),
        0,
        nullptr
    );
    std::string result = msg ? msg : "Unknown error";
    if (msg) LocalFree(msg);
    return result;
}

} 
