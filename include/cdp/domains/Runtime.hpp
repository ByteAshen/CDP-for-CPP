#pragma once

#include "Domain.hpp"

namespace cdp {


struct CallArgument {
    JsonValue value;
    std::string unserializableValue;
    std::string objectId;

    JsonValue toJson() const {
        JsonObject obj;
        if (!objectId.empty()) {
            obj["objectId"] = objectId;
        } else if (!unserializableValue.empty()) {
            obj["unserializableValue"] = unserializableValue;
        } else {
            obj["value"] = value;
        }
        return obj;
    }

    static CallArgument fromValue(const JsonValue& v) { return {v, "", ""}; }
    static CallArgument fromObjectId(const std::string& id) { return {JsonValue(), "", id}; }
    static CallArgument unserializable(const std::string& v) { return {JsonValue(), v, ""}; }
};


struct SerializationOptions {
    std::string serialization = "deep";  
    int maxDepth = 0;
    JsonValue additionalParameters;

    JsonValue toJson() const {
        JsonObject obj;
        obj["serialization"] = serialization;
        if (maxDepth > 0) obj["maxDepth"] = maxDepth;
        if (!additionalParameters.isNull()) obj["additionalParameters"] = additionalParameters;
        return obj;
    }
};


class Runtime : public Domain {
public:
    explicit Runtime(CDPConnection& connection) : Domain(connection, "Runtime") {}

    
    CDPResponse evaluate(const std::string& expression,
                          const std::string& objectGroup = "",
                          bool includeCommandLineAPI = false,
                          bool silent = false,
                          int contextId = 0,
                          bool returnByValue = false,
                          bool generatePreview = false,
                          bool userGesture = false,
                          bool awaitPromise = false,
                          bool throwOnSideEffect = false,
                          double timeout = 0,
                          bool disableBreaks = false,
                          bool replMode = false,
                          bool allowUnsafeEvalBlockedByCSP = false,
                          const std::string& uniqueContextId = "",
                          const SerializationOptions* serializationOptions = nullptr) {
        Params params;
        params.set("expression", expression);
        if (!objectGroup.empty()) params.set("objectGroup", objectGroup);
        if (includeCommandLineAPI) params.set("includeCommandLineAPI", true);
        if (silent) params.set("silent", true);
        if (contextId > 0) params.set("contextId", contextId);
        if (returnByValue) params.set("returnByValue", true);
        if (generatePreview) params.set("generatePreview", true);
        if (userGesture) params.set("userGesture", true);
        if (awaitPromise) params.set("awaitPromise", true);
        if (throwOnSideEffect) params.set("throwOnSideEffect", true);
        if (timeout > 0) params.set("timeout", timeout);
        if (disableBreaks) params.set("disableBreaks", true);
        if (replMode) params.set("replMode", true);
        if (allowUnsafeEvalBlockedByCSP) params.set("allowUnsafeEvalBlockedByCSP", true);
        if (!uniqueContextId.empty()) params.set("uniqueContextId", uniqueContextId);
        if (serializationOptions) params.set("serializationOptions", serializationOptions->toJson());
        return call("evaluate", params);
    }

    
    CDPResponse eval(const std::string& expression, bool returnByValue = true) {
        return evaluate(expression, "", false, false, 0, returnByValue);
    }

    
    std::string evalString(const std::string& expression, const std::string& defaultValue = "") {
        auto result = evaluate(expression, "", false, false, 0, true);
        if (result.hasError || result.hasException()) return defaultValue;
        return result.result.getStringAt("result/value", defaultValue);
    }

    
    int evalInt(const std::string& expression, int defaultValue = 0) {
        auto result = evaluate(expression, "", false, false, 0, true);
        if (result.hasError || result.hasException()) return defaultValue;
        return result.result.getIntAt("result/value", defaultValue);
    }

    
    bool evalBool(const std::string& expression, bool defaultValue = false) {
        auto result = evaluate(expression, "", false, false, 0, true);
        if (result.hasError || result.hasException()) return defaultValue;
        return result.result.getBoolAt("result/value", defaultValue);
    }

    
    double evalDouble(const std::string& expression, double defaultValue = 0.0) {
        auto result = evaluate(expression, "", false, false, 0, true);
        if (result.hasError || result.hasException()) return defaultValue;
        return result.result.getDoubleAt("result/value", defaultValue);
    }

    
    CDPResponse evalAsync(const std::string& expression, int timeoutMs = 30000) {
        Params params;
        params.set("expression", expression);
        params.set("awaitPromise", true);
        params.set("returnByValue", true);
        return call("evaluate", params, timeoutMs);
    }

    
    CDPResponse execute(const std::string& script) {
        return evaluate(script, "", false, true);  
    }

    
    CDPResponse callFunctionOn(const std::string& functionDeclaration,
                                const std::string& objectId = "",
                                const std::vector<CallArgument>& arguments = {},
                                bool silent = false,
                                bool returnByValue = false,
                                bool generatePreview = false,
                                bool userGesture = false,
                                bool awaitPromise = false,
                                int executionContextId = 0,
                                const std::string& objectGroup = "",
                                bool throwOnSideEffect = false,
                                const std::string& uniqueContextId = "",
                                const SerializationOptions* serializationOptions = nullptr) {
        Params params;
        params.set("functionDeclaration", functionDeclaration);
        if (!objectId.empty()) params.set("objectId", objectId);
        if (!arguments.empty()) {
            JsonArray args;
            for (const auto& arg : arguments) args.push_back(arg.toJson());
            params.set("arguments", args);
        }
        if (silent) params.set("silent", true);
        if (returnByValue) params.set("returnByValue", true);
        if (generatePreview) params.set("generatePreview", true);
        if (userGesture) params.set("userGesture", true);
        if (awaitPromise) params.set("awaitPromise", true);
        if (executionContextId > 0) params.set("executionContextId", executionContextId);
        if (!objectGroup.empty()) params.set("objectGroup", objectGroup);
        if (throwOnSideEffect) params.set("throwOnSideEffect", true);
        if (!uniqueContextId.empty()) params.set("uniqueContextId", uniqueContextId);
        if (serializationOptions) params.set("serializationOptions", serializationOptions->toJson());
        return call("callFunctionOn", params);
    }

    
    CDPResponse getProperties(const std::string& objectId,
                               bool ownProperties = false,
                               bool accessorPropertiesOnly = false,
                               bool generatePreview = false,
                               bool nonIndexedPropertiesOnly = false) {
        Params params;
        params.set("objectId", objectId);
        if (ownProperties) params.set("ownProperties", true);
        if (accessorPropertiesOnly) params.set("accessorPropertiesOnly", true);
        if (generatePreview) params.set("generatePreview", true);
        if (nonIndexedPropertiesOnly) params.set("nonIndexedPropertiesOnly", true);
        return call("getProperties", params);
    }

    
    CDPResponse releaseObject(const std::string& objectId) {
        return call("releaseObject", Params().set("objectId", objectId));
    }

    CDPResponse releaseObjectGroup(const std::string& objectGroup) {
        return call("releaseObjectGroup", Params().set("objectGroup", objectGroup));
    }

    
    CDPResponse compileScript(const std::string& expression,
                               const std::string& sourceURL,
                               bool persistScript,
                               int executionContextId = 0) {
        Params params;
        params.set("expression", expression);
        params.set("sourceURL", sourceURL);
        params.set("persistScript", persistScript);
        if (executionContextId > 0) params.set("executionContextId", executionContextId);
        return call("compileScript", params);
    }

    
    CDPResponse runScript(const std::string& scriptId,
                           int executionContextId = 0,
                           const std::string& objectGroup = "",
                           bool silent = false,
                           bool includeCommandLineAPI = false,
                           bool returnByValue = false,
                           bool generatePreview = false,
                           bool awaitPromise = false) {
        Params params;
        params.set("scriptId", scriptId);
        if (executionContextId > 0) params.set("executionContextId", executionContextId);
        if (!objectGroup.empty()) params.set("objectGroup", objectGroup);
        if (silent) params.set("silent", true);
        if (includeCommandLineAPI) params.set("includeCommandLineAPI", true);
        if (returnByValue) params.set("returnByValue", true);
        if (generatePreview) params.set("generatePreview", true);
        if (awaitPromise) params.set("awaitPromise", true);
        return call("runScript", params);
    }

    
    CDPResponse queryObjects(const std::string& prototypeObjectId, const std::string& objectGroup = "") {
        Params params;
        params.set("prototypeObjectId", prototypeObjectId);
        if (!objectGroup.empty()) params.set("objectGroup", objectGroup);
        return call("queryObjects", params);
    }

    
    CDPResponse globalLexicalScopeNames(int executionContextId = 0) {
        Params params;
        if (executionContextId > 0) params.set("executionContextId", executionContextId);
        return call("globalLexicalScopeNames", params);
    }

    
    CDPResponse awaitPromise(const std::string& promiseObjectId,
                              bool returnByValue = false,
                              bool generatePreview = false) {
        Params params;
        params.set("promiseObjectId", promiseObjectId);
        if (returnByValue) params.set("returnByValue", true);
        if (generatePreview) params.set("generatePreview", true);
        return call("awaitPromise", params);
    }

    
    CDPResponse discardConsoleEntries() { return call("discardConsoleEntries"); }

    
    CDPResponse addBinding(const std::string& name,
                            int executionContextId = 0,
                            const std::string& executionContextName = "") {
        Params params;
        params.set("name", name);
        if (executionContextId > 0) params.set("executionContextId", executionContextId);
        if (!executionContextName.empty()) params.set("executionContextName", executionContextName);
        return call("addBinding", params);
    }

    CDPResponse removeBinding(const std::string& name) {
        return call("removeBinding", Params().set("name", name));
    }

    
    CDPResponse getHeapUsage() { return call("getHeapUsage"); }

    
    CDPResponse getIsolateId() { return call("getIsolateId"); }

    
    CDPResponse setMaxCallStackSizeToCapture(int size) {
        return call("setMaxCallStackSizeToCapture", Params().set("size", size));
    }

    
    CDPResponse runIfWaitingForDebugger() { return call("runIfWaitingForDebugger"); }

    
    CDPResponse terminateExecution() { return call("terminateExecution"); }

    
    CDPResponse setAsyncCallStackDepth(int maxDepth) {
        return call("setAsyncCallStackDepth", Params().set("maxDepth", maxDepth));
    }

    
    CDPResponse getExceptionDetails(const std::string& errorObjectId) {
        return call("getExceptionDetails", Params().set("errorObjectId", errorObjectId));
    }

    
    CDPResponse setCustomObjectFormatterEnabled(bool enabled) {
        return call("setCustomObjectFormatterEnabled", Params().set("enabled", enabled));
    }

    
    void onConsoleAPICalled(std::function<void(const std::string& type,
                                                const JsonValue& args,
                                                int executionContextId,
                                                double timestamp,
                                                const JsonValue& stackTrace)> callback) {
        on("consoleAPICalled", [callback](const CDPEvent& event) {
            callback(
                event.params["type"].getString(),
                event.params["args"],
                event.params["executionContextId"].getInt(),
                event.params["timestamp"].getNumber(),
                event.params["stackTrace"]
            );
        });
    }

    void onExceptionThrown(std::function<void(double timestamp, const JsonValue& exceptionDetails)> callback) {
        on("exceptionThrown", [callback](const CDPEvent& event) {
            callback(
                event.params["timestamp"].getNumber(),
                event.params["exceptionDetails"]
            );
        });
    }

    void onExecutionContextCreated(std::function<void(const JsonValue& context)> callback) {
        on("executionContextCreated", [callback](const CDPEvent& event) {
            callback(event.params["context"]);
        });
    }

    void onExecutionContextDestroyed(std::function<void(int executionContextId)> callback) {
        on("executionContextDestroyed", [callback](const CDPEvent& event) {
            callback(event.params["executionContextId"].getInt());
        });
    }

    void onExecutionContextsCleared(std::function<void()> callback) {
        on("executionContextsCleared", [callback](const CDPEvent&) {
            callback();
        });
    }

    void onBindingCalled(std::function<void(const std::string& name,
                                             const std::string& payload,
                                             int executionContextId)> callback) {
        on("bindingCalled", [callback](const CDPEvent& event) {
            callback(
                event.params["name"].getString(),
                event.params["payload"].getString(),
                event.params["executionContextId"].getInt()
            );
        });
    }

    void onInspectRequested(std::function<void(const JsonValue& object, const JsonValue& hints)> callback) {
        on("inspectRequested", [callback](const CDPEvent& event) {
            callback(event.params["object"], event.params["hints"]);
        });
    }
};

} 
