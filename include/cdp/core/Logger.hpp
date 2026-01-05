

#pragma once

#include <string>
#include <functional>
#include <memory>
#include <mutex>
#include <chrono>
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>

namespace cdp {


enum class LogLevel {
    Trace = 0,   
    Debug = 1,   
    Info = 2,    
    Warning = 3, 
    Error = 4,   
    None = 5     
};

inline const char* toString(LogLevel level) {
    switch (level) {
        case LogLevel::Trace: return "TRACE";
        case LogLevel::Debug: return "DEBUG";
        case LogLevel::Info: return "INFO";
        case LogLevel::Warning: return "WARN";
        case LogLevel::Error: return "ERROR";
        case LogLevel::None: return "NONE";
    }
    return "UNKNOWN";
}


struct LogMessage {
    LogLevel level;
    std::string component;  
    std::string text;
    std::chrono::system_clock::time_point timestamp;

    std::string format() const {
        auto time = std::chrono::system_clock::to_time_t(timestamp);
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            timestamp.time_since_epoch()) % 1000;

        std::ostringstream oss;
        oss << std::put_time(std::localtime(&time), "%Y-%m-%d %H:%M:%S")
            << '.' << std::setfill('0') << std::setw(3) << ms.count()
            << " [" << toString(level) << "] "
            << "[" << component << "] "
            << text;
        return oss.str();
    }
};


class ILogger {
public:
    virtual ~ILogger() = default;
    virtual void log(const LogMessage& entry) = 0;
    virtual void setMinLevel(LogLevel level) = 0;
    virtual LogLevel minLevel() const = 0;
};


class ConsoleLogger : public ILogger {
public:
    explicit ConsoleLogger(LogLevel minLevel = LogLevel::Info)
        : minLevel_(minLevel) {}

    void log(const LogMessage& entry) override {
        if (entry.level < minLevel_) return;

        std::lock_guard<std::mutex> lock(mutex_);
        std::cerr << entry.format() << "\n";
    }

    void setMinLevel(LogLevel level) override { minLevel_ = level; }
    LogLevel minLevel() const override { return minLevel_; }

private:
    LogLevel minLevel_;
    std::mutex mutex_;
};


class FileLogger : public ILogger {
public:
    explicit FileLogger(const std::string& filename, LogLevel minLevel = LogLevel::Debug)
        : minLevel_(minLevel), file_(filename, std::ios::app) {}

    void log(const LogMessage& entry) override {
        if (entry.level < minLevel_) return;
        if (!file_.is_open()) return;

        std::lock_guard<std::mutex> lock(mutex_);
        file_ << entry.format() << "\n";
        file_.flush();
    }

    void setMinLevel(LogLevel level) override { minLevel_ = level; }
    LogLevel minLevel() const override { return minLevel_; }

private:
    LogLevel minLevel_;
    std::ofstream file_;
    std::mutex mutex_;
};


class CallbackLogger : public ILogger {
public:
    using Callback = std::function<void(const LogMessage&)>;

    explicit CallbackLogger(Callback callback, LogLevel minLevel = LogLevel::Info)
        : callback_(std::move(callback)), minLevel_(minLevel) {}

    void log(const LogMessage& entry) override {
        if (entry.level < minLevel_) return;
        if (callback_) callback_(entry);
    }

    void setMinLevel(LogLevel level) override { minLevel_ = level; }
    LogLevel minLevel() const override { return minLevel_; }

private:
    Callback callback_;
    LogLevel minLevel_;
};


class MultiLogger : public ILogger {
public:
    void addLogger(std::shared_ptr<ILogger> logger) {
        loggers_.push_back(std::move(logger));
    }

    void log(const LogMessage& entry) override {
        for (auto& logger : loggers_) {
            logger->log(entry);
        }
    }

    void setMinLevel(LogLevel level) override {
        for (auto& logger : loggers_) {
            logger->setMinLevel(level);
        }
    }

    LogLevel minLevel() const override {
        LogLevel min = LogLevel::None;
        for (const auto& logger : loggers_) {
            if (logger->minLevel() < min) {
                min = logger->minLevel();
            }
        }
        return min;
    }

private:
    std::vector<std::shared_ptr<ILogger>> loggers_;
};


class Logger {
public:
    static Logger& instance() {
        static Logger logger;
        return logger;
    }

    
    void setLogger(std::shared_ptr<ILogger> logger) {
        std::lock_guard<std::mutex> lock(mutex_);
        logger_ = std::move(logger);
    }

    
    std::shared_ptr<ILogger> logger() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return logger_;
    }

    
    void log(LogLevel level, const std::string& component, const std::string& message) {
        std::shared_ptr<ILogger> lg;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            lg = logger_;
        }

        if (lg) {
            LogMessage entry{level, component, message, std::chrono::system_clock::now()};
            lg->log(entry);
        }
    }

    
    void trace(const std::string& component, const std::string& message) {
        log(LogLevel::Trace, component, message);
    }

    void debug(const std::string& component, const std::string& message) {
        log(LogLevel::Debug, component, message);
    }

    void info(const std::string& component, const std::string& message) {
        log(LogLevel::Info, component, message);
    }

    void warning(const std::string& component, const std::string& message) {
        log(LogLevel::Warning, component, message);
    }

    void error(const std::string& component, const std::string& message) {
        log(LogLevel::Error, component, message);
    }

    
    bool isEnabled(LogLevel level) const {
        std::lock_guard<std::mutex> lock(mutex_);
        return logger_ && logger_->minLevel() <= level;
    }

private:
    Logger() = default;

    std::shared_ptr<ILogger> logger_;
    mutable std::mutex mutex_;
};


#define CDP_LOG(level, component, message) \
    do { \
        if (::cdp::Logger::instance().isEnabled(level)) { \
            ::cdp::Logger::instance().log(level, component, message); \
        } \
    } while (0)

#define CDP_LOG_TRACE(component, message) CDP_LOG(::cdp::LogLevel::Trace, component, message)
#define CDP_LOG_DEBUG(component, message) CDP_LOG(::cdp::LogLevel::Debug, component, message)
#define CDP_LOG_INFO(component, message) CDP_LOG(::cdp::LogLevel::Info, component, message)
#define CDP_LOG_WARN(component, message) CDP_LOG(::cdp::LogLevel::Warning, component, message)
#define CDP_LOG_ERROR(component, message) CDP_LOG(::cdp::LogLevel::Error, component, message)


class ScopedLog {
public:
    ScopedLog(const std::string& component, const std::string& operation)
        : component_(component), operation_(operation),
          start_(std::chrono::steady_clock::now()) {
        CDP_LOG_DEBUG(component_, operation_ + " started");
    }

    ~ScopedLog() {
        auto elapsed = std::chrono::steady_clock::now() - start_;
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count();
        CDP_LOG_DEBUG(component_, operation_ + " completed in " + std::to_string(ms) + "ms");
    }

private:
    std::string component_;
    std::string operation_;
    std::chrono::steady_clock::time_point start_;
};

#define CDP_SCOPED_LOG(component, operation) \
    ::cdp::ScopedLog _scoped_log_##__LINE__(component, operation)

} 
