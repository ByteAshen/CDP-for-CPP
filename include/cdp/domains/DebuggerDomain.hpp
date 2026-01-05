#pragma once

#include "BaseDomain.hpp"
#include "RuntimeDomain.hpp"
#include <string>
#include <vector>
#include <optional>

namespace cdp {


struct Location {
    std::string scriptId;
    int lineNumber = 0;
    int columnNumber = 0;

    JsonValue toJson() const;
    static Location fromJson(const JsonValue& json);
};


struct CallFrame {
    std::string callFrameId;
    std::string functionName;
    std::optional<Location> functionLocation;
    Location location;
    std::string url;
    std::vector<std::pair<std::string, RemoteObject>> scopeChain;
    RemoteObject thisObject;
    std::optional<RemoteObject> returnValue;

    static CallFrame fromJson(const JsonValue& json);
};


struct Breakpoint {
    std::string breakpointId;
    std::vector<Location> locations;
};


struct ScriptInfo {
    std::string scriptId;
    std::string url;
    int startLine = 0;
    int startColumn = 0;
    int endLine = 0;
    int endColumn = 0;
    int executionContextId = 0;
    std::string hash;
    std::string sourceMapURL;
    bool hasSourceURL = false;
    bool isModule = false;
    int length = 0;

    static ScriptInfo fromJson(const JsonValue& json);
};


class DebuggerDomain : public BaseDomain {
public:
    using BaseDomain::BaseDomain;
    std::string domainName() const override { return "Debugger"; }

    
    CDPResponse enable(int maxScriptsCacheSize = 10000000);

    
    CDPResponse pause();
    CDPResponse resume();
    CDPResponse stepOver();
    CDPResponse stepInto(bool breakOnAsyncCall = false);
    CDPResponse stepOut();

    
    Breakpoint setBreakpoint(const Location& location, const std::string& condition = "");
    Breakpoint setBreakpointByUrl(int lineNumber, const std::string& url = "",
                                   const std::string& urlRegex = "",
                                   const std::string& scriptHash = "",
                                   int columnNumber = 0,
                                   const std::string& condition = "");
    CDPResponse removeBreakpoint(const std::string& breakpointId);
    CDPResponse setBreakpointsActive(bool active);
    CDPResponse setSkipAllPauses(bool skip);

    
    Breakpoint setInstrumentationBreakpoint(const std::string& instrumentation);
    

    EvaluationResult evaluateOnCallFrame(const std::string& callFrameId,
                                          const std::string& expression,
                                          bool returnByValue = false,
                                          bool generatePreview = false,
                                          bool throwOnSideEffect = false);

    
    std::string getScriptSource(const std::string& scriptId);
    CDPResponse setScriptSource(const std::string& scriptId, const std::string& scriptSource,
                                 bool dryRun = false);

    
    struct SearchMatch {
        int lineNumber;
        std::string lineContent;
    };
    std::vector<SearchMatch> searchInContent(const std::string& scriptId,
                                              const std::string& query,
                                              bool caseSensitive = false,
                                              bool isRegex = false);

    
    std::vector<Location> getPossibleBreakpoints(const Location& start,
                                                  const std::optional<Location>& end = std::nullopt,
                                                  bool restrictToFunction = false);

    
    CDPResponse continueToLocation(const Location& location,
                                    const std::string& targetCallFrames = "any");
    

    CDPResponse restartFrame(const std::string& callFrameId);

    
    CDPResponse setVariableValue(int scopeNumber, const std::string& variableName,
                                  const JsonValue& newValue, const std::string& callFrameId);

    
    CDPResponse setAsyncCallStackDepth(int maxDepth);

    
    CDPResponse setBlackboxPatterns(const std::vector<std::string>& patterns);
    CDPResponse setBlackboxedRanges(const std::string& scriptId,
                                     const std::vector<std::pair<Location, Location>>& positions);

    
    CDPResponse setReturnValue(const JsonValue& newValue);

    
    CDPResponse setPauseOnExceptions(const std::string& state);
    

    void onPaused(std::function<void(const std::vector<CallFrame>& callFrames,
                                      const std::string& reason,
                                      const JsonValue& data,
                                      const std::vector<std::string>& hitBreakpoints)> callback);
    void onResumed(std::function<void()> callback);
    void onScriptParsed(std::function<void(const ScriptInfo&)> callback);
    void onScriptFailedToParse(std::function<void(const ScriptInfo&)> callback);
    void onBreakpointResolved(std::function<void(const std::string& breakpointId,
                                                  const Location& location)> callback);
};

} 
