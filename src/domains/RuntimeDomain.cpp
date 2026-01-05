

#include "cdp/domains/RuntimeDomain.hpp"

namespace cdp {

namespace {
    RemoteObjectType parseType(const std::string& type) {
        if (type == "object") return RemoteObjectType::Object;
        if (type == "function") return RemoteObjectType::Function;
        if (type == "undefined") return RemoteObjectType::Undefined;
        if (type == "string") return RemoteObjectType::String;
        if (type == "number") return RemoteObjectType::Number;
        if (type == "boolean") return RemoteObjectType::Boolean;
        if (type == "symbol") return RemoteObjectType::Symbol;
        if (type == "bigint") return RemoteObjectType::Bigint;
        return RemoteObjectType::Undefined;
    }

    RemoteObjectSubtype parseSubtype(const std::string& subtype) {
        if (subtype == "array") return RemoteObjectSubtype::Array;
        if (subtype == "null") return RemoteObjectSubtype::Null;
        if (subtype == "node") return RemoteObjectSubtype::Node;
        if (subtype == "regexp") return RemoteObjectSubtype::Regexp;
        if (subtype == "date") return RemoteObjectSubtype::Date;
        if (subtype == "map") return RemoteObjectSubtype::Map;
        if (subtype == "set") return RemoteObjectSubtype::Set;
        if (subtype == "weakmap") return RemoteObjectSubtype::Weakmap;
        if (subtype == "weakset") return RemoteObjectSubtype::Weakset;
        if (subtype == "iterator") return RemoteObjectSubtype::Iterator;
        if (subtype == "generator") return RemoteObjectSubtype::Generator;
        if (subtype == "error") return RemoteObjectSubtype::Error;
        if (subtype == "proxy") return RemoteObjectSubtype::Proxy;
        if (subtype == "promise") return RemoteObjectSubtype::Promise;
        if (subtype == "typedarray") return RemoteObjectSubtype::Typedarray;
        if (subtype == "arraybuffer") return RemoteObjectSubtype::Arraybuffer;
        if (subtype == "dataview") return RemoteObjectSubtype::Dataview;
        return RemoteObjectSubtype::None;
    }
}

RemoteObject RemoteObject::fromJson(const JsonValue& json) {
    RemoteObject obj;
    obj.type = parseType(json["type"].getString());
    obj.subtype = parseSubtype(json["subtype"].getString());
    obj.className = json["className"].getString();
    obj.value = json["value"];
    obj.description = json["description"].getString();
    obj.objectId = json["objectId"].getString();
    obj.unserializableValue = json["unserializableValue"].getString();
    return obj;
}

ExceptionDetails ExceptionDetails::fromJson(const JsonValue& json) {
    ExceptionDetails details;
    details.exceptionId = json["exceptionId"].getInt();
    details.text = json["text"].getString();
    details.lineNumber = json["lineNumber"].getInt();
    details.columnNumber = json["columnNumber"].getInt();
    details.scriptId = json["scriptId"].getString();
    details.url = json["url"].getString();
    if (json.contains("exception")) {
        details.exception = RemoteObject::fromJson(json["exception"]);
    }
    return details;
}

CallArgument CallArgument::fromValue(const JsonValue& val) {
    CallArgument arg;
    arg.value = val;
    return arg;
}

CallArgument CallArgument::fromObjectId(const std::string& id) {
    CallArgument arg;
    arg.objectId = id;
    return arg;
}

JsonValue CallArgument::toJson() const {
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

PropertyDescriptor PropertyDescriptor::fromJson(const JsonValue& json) {
    PropertyDescriptor desc;
    desc.name = json["name"].getString();
    if (json.contains("value")) {
        desc.value = RemoteObject::fromJson(json["value"]);
    }
    desc.writable = json["writable"].getBool();
    desc.configurable = json["configurable"].getBool();
    desc.enumerable = json["enumerable"].getBool();
    desc.isOwn = json["isOwn"].getBool();
    if (json.contains("get")) {
        desc.get = RemoteObject::fromJson(json["get"]);
    }
    if (json.contains("set")) {
        desc.set = RemoteObject::fromJson(json["set"]);
    }
    return desc;
}

EvaluationResult RuntimeDomain::evaluate(const std::string& expression,
                                          bool returnByValue,
                                          bool awaitPromise,
                                          const std::string& contextId,
                                          bool silent,
                                          bool userGesture) {
    JsonObject params;
    params["expression"] = expression;
    params["returnByValue"] = returnByValue;
    params["awaitPromise"] = awaitPromise;
    params["silent"] = silent;
    params["userGesture"] = userGesture;

    if (!contextId.empty()) {
        params["contextId"] = std::stoi(contextId);
    }

    auto response = sendCommand("Runtime.evaluate", params);
    EvaluationResult result;

    if (response.isSuccess()) {
        result.result = RemoteObject::fromJson(response.result["result"]);
        if (response.result.contains("exceptionDetails")) {
            result.hasException = true;
            result.exceptionDetails = ExceptionDetails::fromJson(response.result["exceptionDetails"]);
        }
    } else {
        result.hasException = true;
        result.exceptionDetails.text = response.errorMessage;
    }

    return result;
}

EvaluationResult RuntimeDomain::callFunctionOn(const std::string& objectId,
                                                const std::string& functionDeclaration,
                                                const std::vector<CallArgument>& arguments,
                                                bool returnByValue,
                                                bool awaitPromise) {
    JsonObject params;
    params["objectId"] = objectId;
    params["functionDeclaration"] = functionDeclaration;
    params["returnByValue"] = returnByValue;
    params["awaitPromise"] = awaitPromise;

    if (!arguments.empty()) {
        JsonArray args;
        for (const auto& arg : arguments) {
            args.push_back(arg.toJson());
        }
        params["arguments"] = args;
    }

    auto response = sendCommand("Runtime.callFunctionOn", params);
    EvaluationResult result;

    if (response.isSuccess()) {
        result.result = RemoteObject::fromJson(response.result["result"]);
        if (response.result.contains("exceptionDetails")) {
            result.hasException = true;
            result.exceptionDetails = ExceptionDetails::fromJson(response.result["exceptionDetails"]);
        }
    } else {
        result.hasException = true;
        result.exceptionDetails.text = response.errorMessage;
    }

    return result;
}

std::vector<PropertyDescriptor> RuntimeDomain::getProperties(const std::string& objectId,
                                                              bool ownProperties,
                                                              bool generatePreview) {
    JsonObject params;
    params["objectId"] = objectId;
    params["ownProperties"] = ownProperties;
    params["generatePreview"] = generatePreview;

    auto response = sendCommand("Runtime.getProperties", params);
    std::vector<PropertyDescriptor> properties;

    if (response.isSuccess() && response.result["result"].isArray()) {
        const auto& arr = response.result["result"].asArray();
        for (const auto& item : arr) {
            properties.push_back(PropertyDescriptor::fromJson(item));
        }
    }

    return properties;
}

CDPResponse RuntimeDomain::releaseObject(const std::string& objectId) {
    JsonObject params;
    params["objectId"] = objectId;
    return sendCommand("Runtime.releaseObject", params);
}

CDPResponse RuntimeDomain::releaseObjectGroup(const std::string& objectGroup) {
    JsonObject params;
    params["objectGroup"] = objectGroup;
    return sendCommand("Runtime.releaseObjectGroup", params);
}

RuntimeDomain::CompileScriptResult RuntimeDomain::compileScript(const std::string& expression,
                                                                 const std::string& sourceURL,
                                                                 bool persistScript) {
    JsonObject params;
    params["expression"] = expression;
    params["sourceURL"] = sourceURL;
    params["persistScript"] = persistScript;

    auto response = sendCommand("Runtime.compileScript", params);
    CompileScriptResult result;

    if (response.isSuccess()) {
        result.scriptId = response.result["scriptId"].getString();
        if (response.result.contains("exceptionDetails")) {
            result.hasException = true;
            result.exceptionDetails = ExceptionDetails::fromJson(response.result["exceptionDetails"]);
        }
    } else {
        result.hasException = true;
        result.exceptionDetails.text = response.errorMessage;
    }

    return result;
}

EvaluationResult RuntimeDomain::runScript(const std::string& scriptId,
                                           bool returnByValue,
                                           bool awaitPromise) {
    JsonObject params;
    params["scriptId"] = scriptId;
    params["returnByValue"] = returnByValue;
    params["awaitPromise"] = awaitPromise;

    auto response = sendCommand("Runtime.runScript", params);
    EvaluationResult result;

    if (response.isSuccess()) {
        result.result = RemoteObject::fromJson(response.result["result"]);
        if (response.result.contains("exceptionDetails")) {
            result.hasException = true;
            result.exceptionDetails = ExceptionDetails::fromJson(response.result["exceptionDetails"]);
        }
    } else {
        result.hasException = true;
        result.exceptionDetails.text = response.errorMessage;
    }

    return result;
}

std::vector<RuntimeDomain::ExecutionContextDescription> RuntimeDomain::getExecutionContexts() {
    
    
    return {};
}

CDPResponse RuntimeDomain::discardConsoleEntries() {
    return sendCommand("Runtime.discardConsoleEntries");
}

EvaluationResult RuntimeDomain::queryObjects(const std::string& prototypeObjectId) {
    JsonObject params;
    params["prototypeObjectId"] = prototypeObjectId;

    auto response = sendCommand("Runtime.queryObjects", params);
    EvaluationResult result;

    if (response.isSuccess()) {
        result.result = RemoteObject::fromJson(response.result["objects"]);
    } else {
        result.hasException = true;
        result.exceptionDetails.text = response.errorMessage;
    }

    return result;
}

std::vector<std::string> RuntimeDomain::globalLexicalScopeNames(int executionContextId) {
    JsonObject params;
    if (executionContextId > 0) {
        params["executionContextId"] = executionContextId;
    }

    auto response = sendCommand("Runtime.globalLexicalScopeNames", params);
    std::vector<std::string> names;

    if (response.isSuccess() && response.result["names"].isArray()) {
        const auto& arr = response.result["names"].asArray();
        for (const auto& item : arr) {
            names.push_back(item.getString());
        }
    }

    return names;
}

EvaluationResult RuntimeDomain::awaitPromise(const std::string& promiseObjectId,
                                              bool returnByValue) {
    JsonObject params;
    params["promiseObjectId"] = promiseObjectId;
    params["returnByValue"] = returnByValue;

    auto response = sendCommand("Runtime.awaitPromise", params);
    EvaluationResult result;

    if (response.isSuccess()) {
        result.result = RemoteObject::fromJson(response.result["result"]);
        if (response.result.contains("exceptionDetails")) {
            result.hasException = true;
            result.exceptionDetails = ExceptionDetails::fromJson(response.result["exceptionDetails"]);
        }
    } else {
        result.hasException = true;
        result.exceptionDetails.text = response.errorMessage;
    }

    return result;
}

CDPResponse RuntimeDomain::addBinding(const std::string& name, int executionContextId) {
    JsonObject params;
    params["name"] = name;
    if (executionContextId > 0) {
        params["executionContextId"] = executionContextId;
    }
    return sendCommand("Runtime.addBinding", params);
}

CDPResponse RuntimeDomain::removeBinding(const std::string& name) {
    JsonObject params;
    params["name"] = name;
    return sendCommand("Runtime.removeBinding", params);
}

void RuntimeDomain::onConsoleAPICalled(
    std::function<void(const std::string&, const std::vector<RemoteObject>&, double)> callback) {
    subscribeEvent("consoleAPICalled", [callback](const CDPEvent& event) {
        std::vector<RemoteObject> args;
        if (event.params["args"].isArray()) {
            for (size_t i = 0; i < event.params["args"].size(); ++i) {
                args.push_back(RemoteObject::fromJson(event.params["args"][i]));
            }
        }
        callback(
            event.params["type"].getString(),
            args,
            event.params["timestamp"].getNumber()
        );
    });
}

void RuntimeDomain::onExceptionThrown(
    std::function<void(double, const ExceptionDetails&)> callback) {
    subscribeEvent("exceptionThrown", [callback](const CDPEvent& event) {
        callback(
            event.params["timestamp"].getNumber(),
            ExceptionDetails::fromJson(event.params["exceptionDetails"])
        );
    });
}

void RuntimeDomain::onExecutionContextCreated(
    std::function<void(const ExecutionContextDescription&)> callback) {
    subscribeEvent("executionContextCreated", [callback](const CDPEvent& event) {
        ExecutionContextDescription ctx;
        const auto& ctxJson = event.params["context"];
        ctx.id = ctxJson["id"].getInt();
        ctx.origin = ctxJson["origin"].getString();
        ctx.name = ctxJson["name"].getString();
        ctx.uniqueId = ctxJson["uniqueId"].getString();
        callback(ctx);
    });
}

void RuntimeDomain::onExecutionContextDestroyed(std::function<void(int)> callback) {
    subscribeEvent("executionContextDestroyed", [callback](const CDPEvent& event) {
        callback(event.params["executionContextId"].getInt());
    });
}

void RuntimeDomain::onBindingCalled(
    std::function<void(const std::string&, const std::string&, int)> callback) {
    subscribeEvent("bindingCalled", [callback](const CDPEvent& event) {
        callback(
            event.params["name"].getString(),
            event.params["payload"].getString(),
            event.params["executionContextId"].getInt()
        );
    });
}

} 
