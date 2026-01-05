

#pragma once

#include "../protocol/CDPConnection.hpp"
#include "../domains/Domain.hpp"
#include <future>
#include <chrono>
#include <functional>
#include <vector>

namespace cdp {
namespace highlevel {


class AsyncCDPResponse {
public:
    AsyncCDPResponse() : future_(promise_.get_future()) {}

    
    void resolve(const CDPResponse& response) {
        try {
            promise_.set_value(response);
        } catch (const std::future_error&) {
            
        }
    }

    
    CDPResponse get() { return future_.get(); }

    
    template<typename Rep, typename Period>
    std::future_status wait_for(const std::chrono::duration<Rep, Period>& timeout) {
        return future_.wait_for(timeout);
    }

    
    bool isReady() const {
        return future_.wait_for(std::chrono::seconds(0)) == std::future_status::ready;
    }

    
    std::future<CDPResponse>& future() { return future_; }

private:
    std::promise<CDPResponse> promise_;
    std::future<CDPResponse> future_;
};


inline std::future<CDPResponse> sendAsync(CDPConnection& conn,
                                           const std::string& method,
                                           const JsonValue& params = JsonValue()) {
    auto promise = std::make_shared<std::promise<CDPResponse>>();
    auto future = promise->get_future();

    conn.sendCommand(method, params, [promise](const CDPResponse& response) {
        try {
            promise->set_value(response);
        } catch (const std::future_error&) {
            
        }
    });

    return future;
}


template<typename... Futures>
auto waitAll(Futures&&... futures) {
    return std::make_tuple(futures.get()...);
}


inline std::vector<CDPResponse> parallelExecute(
    CDPConnection& conn,
    const std::vector<std::pair<std::string, JsonValue>>& commands) {

    std::vector<std::future<CDPResponse>> futures;
    futures.reserve(commands.size());

    
    for (const auto& cmd : commands) {
        futures.push_back(sendAsync(conn, cmd.first, cmd.second));
    }

    
    std::vector<CDPResponse> results;
    results.reserve(futures.size());

    for (auto& future : futures) {
        results.push_back(future.get());
    }

    return results;
}


class AsyncBatch {
public:
    explicit AsyncBatch(CDPConnection& conn) : connection_(conn) {}

    
    AsyncBatch& add(const std::string& method, const JsonValue& params = JsonValue()) {
        commands_.emplace_back(method, params);
        return *this;
    }

    
    std::vector<CDPResponse> execute() {
        return parallelExecute(connection_, commands_);
    }

    
    std::vector<std::future<CDPResponse>> executeAsync() {
        std::vector<std::future<CDPResponse>> futures;
        futures.reserve(commands_.size());

        for (const auto& cmd : commands_) {
            futures.push_back(sendAsync(connection_, cmd.first, cmd.second));
        }

        return futures;
    }

    
    size_t size() const { return commands_.size(); }

    
    void clear() { commands_.clear(); }

private:
    CDPConnection& connection_;
    std::vector<std::pair<std::string, JsonValue>> commands_;
};


template<typename Func>
CDPResponse retryUntilSuccess(Func&& func, int maxAttempts = 3, int delayMs = 100) {
    CDPResponse lastResult;

    for (int i = 0; i < maxAttempts; ++i) {
        lastResult = func();
        if (!lastResult.hasError) return lastResult;

        if (i + 1 < maxAttempts) {
            std::this_thread::sleep_for(std::chrono::milliseconds(delayMs));
        }
    }

    return lastResult;
}


template<typename Func, typename Condition>
CDPResponse pollUntil(Func&& func, Condition&& condition, int timeoutMs = 30000, int intervalMs = 100) {
    auto startTime = std::chrono::steady_clock::now();
    CDPResponse lastResult;

    while (true) {
        lastResult = func();

        if (condition(lastResult)) return lastResult;

        auto elapsed = std::chrono::steady_clock::now() - startTime;
        if (std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count() >= timeoutMs) {
            lastResult.hasError = true;
            lastResult.errorMessage = "Poll timeout";
            return lastResult;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(intervalMs));
    }
}


template<typename T>
class Task {
public:
    Task(std::future<T>&& future) : future_(std::move(future)) {}

    
    T get() { return future_.get(); }

    
    template<typename Rep, typename Period>
    std::optional<T> get(const std::chrono::duration<Rep, Period>& timeout) {
        if (future_.wait_for(timeout) == std::future_status::ready) {
            return future_.get();
        }
        return std::nullopt;
    }

    
    bool isReady() const {
        return future_.wait_for(std::chrono::seconds(0)) == std::future_status::ready;
    }

    
    template<typename Func>
    auto then(Func&& func) -> Task<decltype(func(std::declval<T>()))> {
        using ResultType = decltype(func(std::declval<T>()));

        auto sharedFuture = std::make_shared<std::future<T>>(std::move(future_));
        auto func_ptr = std::make_shared<std::decay_t<Func>>(std::forward<Func>(func));

        auto promise = std::make_shared<std::promise<ResultType>>();
        auto resultFuture = promise->get_future();

        std::thread([sharedFuture, func_ptr, promise]() {
            try {
                T value = sharedFuture->get();
                if constexpr (std::is_void_v<ResultType>) {
                    (*func_ptr)(value);
                    promise->set_value();
                } else {
                    promise->set_value((*func_ptr)(value));
                }
            } catch (...) {
                promise->set_exception(std::current_exception());
            }
        }).detach();

        return Task<ResultType>(std::move(resultFuture));
    }

private:
    std::future<T> future_;
};


inline Task<CDPResponse> asyncCommand(CDPConnection& conn,
                                       const std::string& method,
                                       const JsonValue& params = JsonValue()) {
    return Task<CDPResponse>(sendAsync(conn, method, params));
}

} 
} 
