

#pragma once

#include "../protocol/CDPConnection.hpp"
#include "../domains/Domain.hpp"
#include <mutex>
#include <shared_mutex>
#include <queue>
#include <functional>
#include <thread>
#include <atomic>
#include <condition_variable>

namespace cdp {
namespace highlevel {


class SerializedExecutor {
public:
    SerializedExecutor() = default;

    
    SerializedExecutor(const SerializedExecutor&) = delete;
    SerializedExecutor& operator=(const SerializedExecutor&) = delete;

    
    template<typename Func>
    auto execute(Func&& func) -> decltype(func()) {
        std::lock_guard<std::mutex> lock(mutex_);
        return func();
    }

    
    template<typename Func>
    bool tryExecute(Func&& func) {
        std::unique_lock<std::mutex> lock(mutex_, std::try_to_lock);
        if (!lock.owns_lock()) return false;
        func();
        return true;
    }

private:
    std::mutex mutex_;
};


class ThreadSafeConnection {
public:
    explicit ThreadSafeConnection(CDPConnection& conn)
        : connection_(conn) {}

    
    CDPResponse sendCommandSerialized(const std::string& method,
                                        const JsonValue& params = JsonValue(),
                                        int timeoutMs = 30000) {
        return executor_.execute([&]() {
            return connection_.sendCommandSync(method, params, timeoutMs);
        });
    }

    
    template<typename Func>
    auto atomic(Func&& func) -> decltype(func()) {
        return executor_.execute(std::forward<Func>(func));
    }

    
    CDPConnection& connection() { return connection_; }

private:
    CDPConnection& connection_;
    SerializedExecutor executor_;
};


class ThreadSafeDomain {
public:
    ThreadSafeDomain(CDPConnection& conn, const std::string& name)
        : connection_(conn), name_(name), executor_(std::make_shared<SerializedExecutor>()) {}

    
    CDPResponse call(const std::string& method, const JsonValue& params = JsonValue()) {
        return executor_->execute([&]() {
            return connection_.sendCommandSync(name_ + "." + method, params);
        });
    }

    CDPResponse call(const std::string& method, const Params& params) {
        return call(method, params.build());
    }

    
    int64_t callAsync(const std::string& method, const JsonValue& params = JsonValue(),
                       ResponseCallback callback = nullptr) {
        return connection_.sendCommand(name_ + "." + method, params, callback);
    }

    
    void shareExecutorWith(ThreadSafeDomain& other) {
        other.executor_ = executor_;
    }

private:
    CDPConnection& connection_;
    std::string name_;
    std::shared_ptr<SerializedExecutor> executor_;
};


class OperationQueue {
public:
    using Operation = std::function<void()>;

    OperationQueue() : running_(false) {}

    ~OperationQueue() {
        stop();
    }

    
    void start() {
        if (running_.exchange(true)) return;

        worker_ = std::thread([this]() {
            while (running_) {
                Operation op;

                {
                    std::unique_lock<std::mutex> lock(mutex_);
                    cond_.wait(lock, [this]() {
                        return !queue_.empty() || !running_;
                    });

                    if (!running_ && queue_.empty()) break;

                    if (!queue_.empty()) {
                        op = std::move(queue_.front());
                        queue_.pop();
                    }
                }

                if (op) {
                    try {
                        op();
                    } catch (...) {
                        
                    }
                }
            }
        });
    }

    
    void stop() {
        running_ = false;
        cond_.notify_all();
        if (worker_.joinable()) {
            worker_.join();
        }
    }

    
    void enqueue(Operation op) {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            queue_.push(std::move(op));
        }
        cond_.notify_one();
    }

    
    template<typename Func>
    auto enqueueWithResult(Func&& func) -> std::future<decltype(func())> {
        using ResultType = decltype(func());
        auto promise = std::make_shared<std::promise<ResultType>>();
        auto future = promise->get_future();

        enqueue([func = std::forward<Func>(func), promise]() mutable {
            try {
                if constexpr (std::is_void_v<ResultType>) {
                    func();
                    promise->set_value();
                } else {
                    promise->set_value(func());
                }
            } catch (...) {
                promise->set_exception(std::current_exception());
            }
        });

        return future;
    }

    
    size_t size() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return queue_.size();
    }

    
    bool empty() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return queue_.empty();
    }

private:
    std::queue<Operation> queue_;
    mutable std::mutex mutex_;
    std::condition_variable cond_;
    std::thread worker_;
    std::atomic<bool> running_;
};


class ReadWriteLock {
public:
    
    class ReadGuard {
    public:
        explicit ReadGuard(std::shared_mutex& m) : mutex_(m) { mutex_.lock_shared(); }
        ~ReadGuard() { mutex_.unlock_shared(); }

        ReadGuard(const ReadGuard&) = delete;
        ReadGuard& operator=(const ReadGuard&) = delete;

    private:
        std::shared_mutex& mutex_;
    };

    
    class WriteGuard {
    public:
        explicit WriteGuard(std::shared_mutex& m) : mutex_(m) { mutex_.lock(); }
        ~WriteGuard() { mutex_.unlock(); }

        WriteGuard(const WriteGuard&) = delete;
        WriteGuard& operator=(const WriteGuard&) = delete;

    private:
        std::shared_mutex& mutex_;
    };

    ReadGuard readLock() { return ReadGuard(mutex_); }
    WriteGuard writeLock() { return WriteGuard(mutex_); }

    
    template<typename Func>
    auto withRead(Func&& func) -> decltype(func()) {
        std::shared_lock<std::shared_mutex> lock(mutex_);
        return func();
    }

    
    template<typename Func>
    auto withWrite(Func&& func) -> decltype(func()) {
        std::unique_lock<std::shared_mutex> lock(mutex_);
        return func();
    }

private:
    std::shared_mutex mutex_;
};


template<typename T>
class ThreadLocalCache {
public:
    
    T& get() {
        thread_local T value;
        return value;
    }

    
    template<typename Init>
    T& getOrInit(Init&& init) {
        thread_local T value = init();
        return value;
    }
};


class DeadlockDetector {
public:
    
    static bool isMessageThread(const CDPConnection& conn) {
        return conn.isMessageThread();
    }

    
    static void assertNotMessageThread(const CDPConnection& conn, const char* operation) {
        if (conn.isMessageThread()) {
            throw std::runtime_error(
                std::string("Deadlock risk: ") + operation +
                " called from message thread. Use async version instead.");
        }
    }

    
    static CDPResponse safeSyncCall(CDPConnection& conn,
                                     const std::string& method,
                                     const JsonValue& params = JsonValue(),
                                     int timeoutMs = 30000) {
        assertNotMessageThread(conn, method.c_str());
        return conn.sendCommandSync(method, params, timeoutMs);
    }
};

} 
} 
