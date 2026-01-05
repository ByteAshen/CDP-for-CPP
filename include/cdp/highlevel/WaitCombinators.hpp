

#pragma once

#include "../protocol/CDPConnection.hpp"
#include "../domains/Domain.hpp"
#include "Result.hpp"
#include <future>
#include <chrono>
#include <variant>
#include <vector>
#include <string>
#include <functional>
#include <thread>
#include <atomic>
#include <mutex>
#include <memory>

namespace cdp {
namespace highlevel {


class WaitCondition {
public:
    enum class Type {
        Selector,       
        Navigation,     
        NetworkIdle,    
        Event,          
        Custom          
    };

    
    static WaitCondition selector(const std::string& sel) {
        WaitCondition c;
        c.type_ = Type::Selector;
        c.selector_ = sel;
        return c;
    }

    
    static WaitCondition navigation() {
        WaitCondition c;
        c.type_ = Type::Navigation;
        return c;
    }

    
    static WaitCondition networkIdle(int idleTimeMs = 500) {
        WaitCondition c;
        c.type_ = Type::NetworkIdle;
        c.networkIdleTimeMs_ = idleTimeMs;
        return c;
    }

    
    static WaitCondition event(const std::string& eventName) {
        WaitCondition c;
        c.type_ = Type::Event;
        c.eventName_ = eventName;
        return c;
    }

    
    static WaitCondition custom(std::function<bool()> predicate) {
        WaitCondition c;
        c.type_ = Type::Custom;
        c.customPredicate_ = std::move(predicate);
        return c;
    }

    Type type() const noexcept { return type_; }
    const std::string& selector() const noexcept { return selector_; }
    const std::string& eventName() const noexcept { return eventName_; }
    int networkIdleTimeMs() const noexcept { return networkIdleTimeMs_; }
    const std::function<bool()>& customPredicate() const noexcept { return customPredicate_; }

private:
    Type type_ = Type::Selector;
    std::string selector_;
    std::string eventName_;
    int networkIdleTimeMs_ = 500;
    std::function<bool()> customPredicate_;
};


struct WaitResult {
    bool success = false;
    int conditionIndex = -1;  
    WaitCondition::Type conditionType = WaitCondition::Type::Selector;
    std::string description;  
    CDPEvent event;           

    static WaitResult timeout() noexcept {
        WaitResult r;
        r.success = false;
        r.description = "Timeout";
        return r;
    }

    static WaitResult met(int index, WaitCondition::Type type, const std::string& desc = "") {
        WaitResult r;
        r.success = true;
        r.conditionIndex = index;
        r.conditionType = type;
        r.description = desc;
        return r;
    }
};


namespace detail {

struct WaitState {
    std::atomic<bool> done{false};
    WaitResult result;
    std::mutex mutex;

    WaitState() : result(WaitResult::timeout()) {}
};

struct WaitAllState {
    std::vector<bool> conditionsMet;
    std::mutex mutex;

    explicit WaitAllState(size_t count) : conditionsMet(count, false) {}
};

} 


class WaitAny {
public:
    explicit WaitAny(CDPConnection& conn) : connection_(conn) {}

    
    WaitAny& orSelector(const std::string& selector) {
        conditions_.push_back(WaitCondition::selector(selector));
        return *this;
    }

    
    WaitAny& orNavigation() {
        conditions_.push_back(WaitCondition::navigation());
        return *this;
    }

    
    WaitAny& orNetworkIdle(int idleTimeMs = 500) {
        conditions_.push_back(WaitCondition::networkIdle(idleTimeMs));
        return *this;
    }

    
    WaitAny& orEvent(const std::string& eventName) {
        conditions_.push_back(WaitCondition::event(eventName));
        return *this;
    }

    
    WaitAny& orCustom(std::function<bool()> predicate) {
        conditions_.push_back(WaitCondition::custom(std::move(predicate)));
        return *this;
    }

    
    WaitAny& withTimeout(int timeoutMs) {
        timeoutMs_ = timeoutMs;
        return *this;
    }

    
    WaitAny& withPollingInterval(int intervalMs) {
        pollingIntervalMs_ = intervalMs;
        return *this;
    }

    
    WaitResult wait() {
        if (conditions_.empty()) {
            return WaitResult::timeout();
        }

        auto startTime = std::chrono::steady_clock::now();

        
        auto state = std::make_shared<detail::WaitState>();

        
        std::vector<CDPConnection::EventToken> eventTokens;
        for (size_t i = 0; i < conditions_.size(); ++i) {
            const auto& cond = conditions_[i];
            if (cond.type() == WaitCondition::Type::Event) {
                
                auto token = connection_.onEventScoped(cond.eventName(),
                    [i, state, type = cond.type(), name = cond.eventName()](const CDPEvent& evt) {
                        std::lock_guard<std::mutex> lock(state->mutex);
                        if (!state->done.exchange(true)) {
                            state->result = WaitResult::met(static_cast<int>(i), type, name);
                            state->result.event = evt;
                        }
                    });
                eventTokens.push_back(std::move(token));
            } else if (cond.type() == WaitCondition::Type::Navigation) {
                auto token = connection_.onEventScoped("Page.loadEventFired",
                    [i, state](const CDPEvent& evt) {
                        std::lock_guard<std::mutex> lock(state->mutex);
                        if (!state->done.exchange(true)) {
                            state->result = WaitResult::met(static_cast<int>(i), WaitCondition::Type::Navigation, "Page loaded");
                            state->result.event = evt;
                        }
                    });
                eventTokens.push_back(std::move(token));
            }
        }

        
        while (!state->done.load()) {
            auto elapsed = std::chrono::steady_clock::now() - startTime;
            if (std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count() >= timeoutMs_) {
                break;
            }

            for (size_t i = 0; i < conditions_.size() && !state->done.load(); ++i) {
                const auto& cond = conditions_[i];
                bool condMet = false;
                std::string desc;

                switch (cond.type()) {
                    case WaitCondition::Type::Selector: {
                        condMet = checkSelector(cond.selector());
                        desc = "Selector: " + cond.selector();
                        break;
                    }
                    case WaitCondition::Type::Custom: {
                        if (cond.customPredicate()) {
                            condMet = cond.customPredicate()();
                            desc = "Custom condition";
                        }
                        break;
                    }
                    case WaitCondition::Type::NetworkIdle: {
                        condMet = checkNetworkIdle(cond.networkIdleTimeMs());
                        desc = "Network idle";
                        break;
                    }
                    default:
                        
                        break;
                }

                if (condMet) {
                    std::lock_guard<std::mutex> lock(state->mutex);
                    if (!state->done.exchange(true)) {
                        state->result = WaitResult::met(static_cast<int>(i), cond.type(), desc);
                    }
                    break;
                }
            }

            if (!state->done.load()) {
                std::this_thread::sleep_for(std::chrono::milliseconds(pollingIntervalMs_));
            }
        }

        
        eventTokens.clear();

        
        std::lock_guard<std::mutex> lock(state->mutex);
        return state->result;
    }

    
    std::future<WaitResult> waitAsync() {
        return std::async(std::launch::async, [this]() {
            return wait();
        });
    }

private:
    bool checkSelector(const std::string& selector) {
        
        auto result = connection_.sendCommandSync("Runtime.evaluate",
            Params()
                .set("expression", "!!document.querySelector('" + selector + "')")
                .set("returnByValue", true)
                .build(),
            1000);  

        if (result.hasError) return false;
        return result.result.getBoolAt("result/value", false);
    }

    bool checkNetworkIdle(int idleTimeMs) {
        
        
        (void)idleTimeMs;
        return false;  
    }

    CDPConnection& connection_;
    std::vector<WaitCondition> conditions_;
    int timeoutMs_ = 30000;
    int pollingIntervalMs_ = 100;
};


class WaitAll {
public:
    explicit WaitAll(CDPConnection& conn) : connection_(conn) {}

    
    WaitAll& andSelector(const std::string& selector) {
        conditions_.push_back(WaitCondition::selector(selector));
        return *this;
    }

    
    WaitAll& andEvent(const std::string& eventName) {
        conditions_.push_back(WaitCondition::event(eventName));
        return *this;
    }

    
    WaitAll& andCustom(std::function<bool()> predicate) {
        conditions_.push_back(WaitCondition::custom(std::move(predicate)));
        return *this;
    }

    
    WaitAll& withTimeout(int timeoutMs) {
        timeoutMs_ = timeoutMs;
        return *this;
    }

    
    WaitAll& withPollingInterval(int intervalMs) {
        pollingIntervalMs_ = intervalMs;
        return *this;
    }

    
    WaitResult wait() {
        if (conditions_.empty()) {
            return WaitResult::met(0, WaitCondition::Type::Custom, "No conditions");
        }

        auto startTime = std::chrono::steady_clock::now();

        
        auto state = std::make_shared<detail::WaitAllState>(conditions_.size());

        
        std::vector<CDPConnection::EventToken> eventTokens;
        for (size_t i = 0; i < conditions_.size(); ++i) {
            const auto& cond = conditions_[i];
            if (cond.type() == WaitCondition::Type::Event) {
                auto token = connection_.onEventScoped(cond.eventName(),
                    [i, state](const CDPEvent&) {
                        std::lock_guard<std::mutex> lock(state->mutex);
                        state->conditionsMet[i] = true;
                    });
                eventTokens.push_back(std::move(token));
            }
        }

        
        while (true) {
            auto elapsed = std::chrono::steady_clock::now() - startTime;
            if (std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count() >= timeoutMs_) {
                eventTokens.clear();
                return WaitResult::timeout();
            }

            
            bool allMet = true;
            {
                std::lock_guard<std::mutex> lock(state->mutex);
                for (size_t i = 0; i < conditions_.size(); ++i) {
                    if (state->conditionsMet[i]) continue;

                    const auto& cond = conditions_[i];
                    switch (cond.type()) {
                        case WaitCondition::Type::Selector:
                            state->conditionsMet[i] = checkSelector(cond.selector());
                            break;
                        case WaitCondition::Type::Custom:
                            if (cond.customPredicate()) {
                                state->conditionsMet[i] = cond.customPredicate()();
                            }
                            break;
                        default:
                            
                            break;
                    }
                }

                
                for (bool met : state->conditionsMet) {
                    if (!met) {
                        allMet = false;
                        break;
                    }
                }
            }

            if (allMet) {
                eventTokens.clear();
                return WaitResult::met(-1, WaitCondition::Type::Custom, "All conditions met");
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(pollingIntervalMs_));
        }
    }

    
    std::future<WaitResult> waitAsync() {
        return std::async(std::launch::async, [this]() {
            return wait();
        });
    }

private:
    bool checkSelector(const std::string& selector) {
        auto result = connection_.sendCommandSync("Runtime.evaluate",
            Params()
                .set("expression", "!!document.querySelector('" + selector + "')")
                .set("returnByValue", true)
                .build(),
            1000);

        if (result.hasError) return false;
        return result.result.getBoolAt("result/value", false);
    }

    CDPConnection& connection_;
    std::vector<WaitCondition> conditions_;
    int timeoutMs_ = 30000;
    int pollingIntervalMs_ = 100;
};

} 
} 
