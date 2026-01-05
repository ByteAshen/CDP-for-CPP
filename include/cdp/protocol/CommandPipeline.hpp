#pragma once

#include "CDPConnection.hpp"
#include <vector>
#include <string>
#include <future>
#include <atomic>

namespace cdp {


class CommandPipeline {
public:
    explicit CommandPipeline(CDPConnection& connection)
        : connection_(connection) {}

    
    CommandPipeline& add(const std::string& method) {
        commands_.push_back({method, JsonValue()});
        return *this;
    }

    CommandPipeline& add(const std::string& method, const JsonValue& params) {
        commands_.push_back({method, params});
        return *this;
    }

    
    std::vector<CDPResponse> execute(int timeoutMs = 30000) {
        if (commands_.empty()) return {};

        std::vector<CDPResponse> responses;
        responses.resize(commands_.size());

        std::atomic<int> completed{0};
        std::mutex responseMutex;
        std::condition_variable cv;

        
        for (size_t i = 0; i < commands_.size(); ++i) {
            connection_.sendCommand(commands_[i].method, commands_[i].params,
                [i, &responses, &completed, &responseMutex, &cv](const CDPResponse& response) {
                    {
                        std::lock_guard<std::mutex> lock(responseMutex);
                        responses[i] = response;
                    }
                    completed++;
                    cv.notify_one();
                });
        }

        
        std::unique_lock<std::mutex> lock(responseMutex);
        bool success = cv.wait_for(lock, std::chrono::milliseconds(timeoutMs),
            [&completed, size = commands_.size()]() {
                return completed.load() >= static_cast<int>(size);
            });

        if (!success) {
            
            for (size_t i = 0; i < responses.size(); ++i) {
                if (responses[i].id == 0 && !responses[i].hasError) {
                    responses[i].hasError = true;
                    responses[i].errorMessage = "Timeout waiting for response";
                }
            }
        }

        commands_.clear();
        return responses;
    }

    
    void executeAsync() {
        for (const auto& cmd : commands_) {
            connection_.sendCommand(cmd.method, cmd.params, nullptr);
        }
        commands_.clear();
    }

    
    void executeWithCallbacks(std::vector<ResponseCallback> callbacks) {
        if (callbacks.size() != commands_.size()) {
            throw std::invalid_argument("Callback count must match command count");
        }

        for (size_t i = 0; i < commands_.size(); ++i) {
            connection_.sendCommand(commands_[i].method, commands_[i].params, callbacks[i]);
        }
        commands_.clear();
    }

    
    size_t size() const { return commands_.size(); }

    
    void clear() { commands_.clear(); }

private:
    struct Command {
        std::string method;
        JsonValue params;
    };

    CDPConnection& connection_;
    std::vector<Command> commands_;
};

} 
