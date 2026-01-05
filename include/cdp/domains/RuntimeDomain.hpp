#pragma once

#include "BaseDomain.hpp"
#include <optional>

namespace cdp {


enum class RemoteObjectType {
    Object,
    Function,
    Undefined,
    String,
    Number,
    Boolean,
    Symbol,
    Bigint
};


enum class RemoteObjectSubtype {
    None,
    Array,
    Null,
    Node,
    Regexp,
    Date,
    Map,
    Set,
    Weakmap,
    Weakset,
    Iterator,
    Generator,
    Error,
    Proxy,
    Promise,
    Typedarray,
    Arraybuffer,
    Dataview
};


struct RemoteObject {
    RemoteObjectType type = RemoteObjectType::Undefined;
    RemoteObjectSubtype subtype = RemoteObjectSubtype::None;
    std::string className;
    JsonValue value;
    std::string description;
    std::string objectId;
    std::string unserializableValue;

    bool isUndefined() const { return type == RemoteObjectType::Undefined; }
    bool isNull() const { return subtype == RemoteObjectSubtype::Null; }
    bool isArray() const { return subtype == RemoteObjectSubtype::Array; }

    static RemoteObject fromJson(const JsonValue& json);
};


struct ExceptionDetails {
    int exceptionId = 0;
    std::string text;
    int lineNumber = 0;
    int columnNumber = 0;
    std::string scriptId;
    std::string url;
    RemoteObject exception;

    static ExceptionDetails fromJson(const JsonValue& json);
};


struct EvaluationResult {
    RemoteObject result;
    bool hasException = false;
    ExceptionDetails exceptionDetails;

    bool success() const { return !hasException; }
};


struct CallArgument {
    JsonValue value;
    std::string objectId;
    std::string unserializableValue;

    static CallArgument fromValue(const JsonValue& val);
    static CallArgument fromObjectId(const std::string& id);
    JsonValue toJson() const;
};


struct PropertyDescriptor {
    std::string name;
    RemoteObject value;
    bool writable = false;
    bool configurable = false;
    bool enumerable = false;
    bool isOwn = false;
    RemoteObject get;
    RemoteObject set;

    static PropertyDescriptor fromJson(const JsonValue& json);
};


class RuntimeDomain : public BaseDomain {
public:
    using BaseDomain::BaseDomain;
    std::string domainName() const override { return "Runtime"; }

    
    EvaluationResult evaluate(const std::string& expression,
                               bool returnByValue = false,
                               bool awaitPromise = false,
                               const std::string& contextId = "",
                               bool silent = false,
                               bool userGesture = false);

    
    EvaluationResult callFunctionOn(const std::string& objectId,
                                     const std::string& functionDeclaration,
                                     const std::vector<CallArgument>& arguments = {},
                                     bool returnByValue = false,
                                     bool awaitPromise = false);

    
    std::vector<PropertyDescriptor> getProperties(const std::string& objectId,
                                                   bool ownProperties = true,
                                                   bool generatePreview = false);

    
    CDPResponse releaseObject(const std::string& objectId);
    CDPResponse releaseObjectGroup(const std::string& objectGroup);

    
    struct CompileScriptResult {
        std::string scriptId;
        bool hasException = false;
        ExceptionDetails exceptionDetails;
    };
    CompileScriptResult compileScript(const std::string& expression,
                                       const std::string& sourceURL,
                                       bool persistScript = true);

    
    EvaluationResult runScript(const std::string& scriptId,
                                bool returnByValue = false,
                                bool awaitPromise = false);

    
    struct ExecutionContextDescription {
        int id = 0;
        std::string origin;
        std::string name;
        std::string uniqueId;
    };
    std::vector<ExecutionContextDescription> getExecutionContexts();

    
    CDPResponse discardConsoleEntries();

    
    EvaluationResult queryObjects(const std::string& prototypeObjectId);

    
    std::vector<std::string> globalLexicalScopeNames(int executionContextId = 0);

    
    EvaluationResult awaitPromise(const std::string& promiseObjectId,
                                   bool returnByValue = false);

    
    CDPResponse addBinding(const std::string& name, int executionContextId = 0);
    CDPResponse removeBinding(const std::string& name);

    
    void onConsoleAPICalled(std::function<void(const std::string& type,
                                                const std::vector<RemoteObject>& args,
                                                double timestamp)> callback);
    void onExceptionThrown(std::function<void(double timestamp,
                                               const ExceptionDetails& details)> callback);
    void onExecutionContextCreated(std::function<void(const ExecutionContextDescription&)> callback);
    void onExecutionContextDestroyed(std::function<void(int contextId)> callback);
    void onBindingCalled(std::function<void(const std::string& name,
                                             const std::string& payload,
                                             int executionContextId)> callback);
};

} 
