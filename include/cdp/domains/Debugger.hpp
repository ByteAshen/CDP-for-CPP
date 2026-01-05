#pragma once

#include "Domain.hpp"

namespace cdp {


struct Location {
    std::string scriptId;
    int lineNumber;
    int columnNumber = -1;

    JsonValue toJson() const {
        JsonObject obj;
        obj["scriptId"] = scriptId;
        obj["lineNumber"] = lineNumber;
        if (columnNumber >= 0) obj["columnNumber"] = columnNumber;
        return obj;
    }

    static Location fromJson(const JsonValue& json) {
        Location loc;
        loc.scriptId = json["scriptId"].getString();
        loc.lineNumber = json["lineNumber"].getInt();
        loc.columnNumber = json["columnNumber"].getInt();
        return loc;
    }
};


struct ScriptPosition {
    int lineNumber;
    int columnNumber;

    JsonValue toJson() const {
        JsonObject obj;
        obj["lineNumber"] = lineNumber;
        obj["columnNumber"] = columnNumber;
        return obj;
    }
};


struct CallFrame {
    std::string callFrameId;
    std::string functionName;
    Location functionLocation;
    Location location;
    std::string url;
    JsonValue scopeChain;
    JsonValue thisObject;
    JsonValue returnValue;
    bool canBeRestarted = false;

    static CallFrame fromJson(const JsonValue& json) {
        CallFrame cf;
        cf.callFrameId = json["callFrameId"].getString();
        cf.functionName = json["functionName"].getString();
        if (!json["functionLocation"].isNull()) {
            cf.functionLocation = Location::fromJson(json["functionLocation"]);
        }
        cf.location = Location::fromJson(json["location"]);
        cf.url = json["url"].getString();
        cf.scopeChain = json["scopeChain"];
        cf.thisObject = json["this"];
        cf.returnValue = json["returnValue"];
        cf.canBeRestarted = json["canBeRestarted"].getBool();
        return cf;
    }
};


struct SearchMatch {
    int lineNumber;
    std::string lineContent;

    static SearchMatch fromJson(const JsonValue& json) {
        SearchMatch sm;
        sm.lineNumber = json["lineNumber"].getInt();
        sm.lineContent = json["lineContent"].getString();
        return sm;
    }
};


struct BreakLocation {
    std::string scriptId;
    int lineNumber;
    int columnNumber = -1;
    std::string type;  

    static BreakLocation fromJson(const JsonValue& json) {
        BreakLocation bl;
        bl.scriptId = json["scriptId"].getString();
        bl.lineNumber = json["lineNumber"].getInt();
        bl.columnNumber = json["columnNumber"].getInt();
        bl.type = json["type"].getString();
        return bl;
    }
};


class Debugger : public Domain {
public:
    explicit Debugger(CDPConnection& connection) : Domain(connection, "Debugger") {}

    
    CDPResponse enable(double maxScriptsCacheSize = 0) {
        Params params;
        if (maxScriptsCacheSize > 0) params.set("maxScriptsCacheSize", maxScriptsCacheSize);
        return call("enable", params);
    }

    CDPResponse disable() { return call("disable"); }

    
    CDPResponse setBreakpointsActive(bool active) {
        return call("setBreakpointsActive", Params().set("active", active));
    }

    CDPResponse setSkipAllPauses(bool skip) {
        return call("setSkipAllPauses", Params().set("skip", skip));
    }

    CDPResponse setBreakpointByUrl(int lineNumber,
                                    const std::string& url = "",
                                    const std::string& urlRegex = "",
                                    const std::string& scriptHash = "",
                                    int columnNumber = -1,
                                    const std::string& condition = "") {
        Params params;
        params.set("lineNumber", lineNumber);
        if (!url.empty()) params.set("url", url);
        if (!urlRegex.empty()) params.set("urlRegex", urlRegex);
        if (!scriptHash.empty()) params.set("scriptHash", scriptHash);
        if (columnNumber >= 0) params.set("columnNumber", columnNumber);
        if (!condition.empty()) params.set("condition", condition);
        return call("setBreakpointByUrl", params);
    }

    CDPResponse setBreakpoint(const Location& location,
                               const std::string& condition = "") {
        Params params;
        params.set("location", location.toJson());
        if (!condition.empty()) params.set("condition", condition);
        return call("setBreakpoint", params);
    }

    CDPResponse setBreakpointOnFunctionCall(const std::string& objectId,
                                             const std::string& condition = "") {
        Params params;
        params.set("objectId", objectId);
        if (!condition.empty()) params.set("condition", condition);
        return call("setBreakpointOnFunctionCall", params);
    }

    CDPResponse removeBreakpoint(const std::string& breakpointId) {
        return call("removeBreakpoint", Params().set("breakpointId", breakpointId));
    }

    CDPResponse getPossibleBreakpoints(const Location& start,
                                        const Location* end = nullptr,
                                        bool restrictToFunction = false) {
        Params params;
        params.set("start", start.toJson());
        if (end) params.set("end", end->toJson());
        if (restrictToFunction) params.set("restrictToFunction", true);
        return call("getPossibleBreakpoints", params);
    }

    
    CDPResponse setPauseOnExceptions(const std::string& state) {
        
        return call("setPauseOnExceptions", Params().set("state", state));
    }

    
    CDPResponse continueToLocation(const Location& location,
                                    const std::string& targetCallFrames = "") {
        Params params;
        params.set("location", location.toJson());
        if (!targetCallFrames.empty()) params.set("targetCallFrames", targetCallFrames);
        return call("continueToLocation", params);
    }

    CDPResponse pause() { return call("pause"); }

    CDPResponse resume(bool terminateOnResume = false) {
        Params params;
        if (terminateOnResume) params.set("terminateOnResume", true);
        return call("resume", params);
    }

    CDPResponse stepOver(const std::vector<std::string>& skipList = {}) {
        Params params;
        if (!skipList.empty()) {
            JsonArray arr;
            for (const auto& s : skipList) arr.push_back(s);
            params.set("skipList", arr);
        }
        return call("stepOver", params);
    }

    CDPResponse stepInto(bool breakOnAsyncCall = false,
                          const std::vector<std::string>& skipList = {}) {
        Params params;
        if (breakOnAsyncCall) params.set("breakOnAsyncCall", true);
        if (!skipList.empty()) {
            JsonArray arr;
            for (const auto& s : skipList) arr.push_back(s);
            params.set("skipList", arr);
        }
        return call("stepInto", params);
    }

    CDPResponse stepOut() { return call("stepOut"); }

    
    CDPResponse getStackTrace(const JsonValue& stackTraceId) {
        return call("getStackTrace", Params().set("stackTraceId", stackTraceId));
    }

    CDPResponse restartFrame(const std::string& callFrameId,
                              const std::string& mode = "") {
        Params params;
        params.set("callFrameId", callFrameId);
        if (!mode.empty()) params.set("mode", mode);
        return call("restartFrame", params);
    }

    
    CDPResponse evaluateOnCallFrame(const std::string& callFrameId,
                                     const std::string& expression,
                                     const std::string& objectGroup = "",
                                     bool includeCommandLineAPI = false,
                                     bool silent = false,
                                     bool returnByValue = false,
                                     bool generatePreview = false,
                                     bool throwOnSideEffect = false,
                                     double timeout = 0) {
        Params params;
        params.set("callFrameId", callFrameId);
        params.set("expression", expression);
        if (!objectGroup.empty()) params.set("objectGroup", objectGroup);
        if (includeCommandLineAPI) params.set("includeCommandLineAPI", true);
        if (silent) params.set("silent", true);
        if (returnByValue) params.set("returnByValue", true);
        if (generatePreview) params.set("generatePreview", true);
        if (throwOnSideEffect) params.set("throwOnSideEffect", true);
        if (timeout > 0) params.set("timeout", timeout);
        return call("evaluateOnCallFrame", params);
    }

    CDPResponse setVariableValue(int scopeNumber,
                                  const std::string& variableName,
                                  const JsonValue& newValue,
                                  const std::string& callFrameId) {
        return call("setVariableValue", Params()
            .set("scopeNumber", scopeNumber)
            .set("variableName", variableName)
            .set("newValue", newValue)
            .set("callFrameId", callFrameId));
    }

    
    CDPResponse getScriptSource(const std::string& scriptId) {
        return call("getScriptSource", Params().set("scriptId", scriptId));
    }

    CDPResponse setScriptSource(const std::string& scriptId,
                                 const std::string& scriptSource,
                                 bool dryRun = false,
                                 bool allowTopFrameEditing = false) {
        Params params;
        params.set("scriptId", scriptId);
        params.set("scriptSource", scriptSource);
        if (dryRun) params.set("dryRun", true);
        if (allowTopFrameEditing) params.set("allowTopFrameEditing", true);
        return call("setScriptSource", params);
    }

    CDPResponse searchInContent(const std::string& scriptId,
                                 const std::string& query,
                                 bool caseSensitive = false,
                                 bool isRegex = false) {
        Params params;
        params.set("scriptId", scriptId);
        params.set("query", query);
        if (caseSensitive) params.set("caseSensitive", true);
        if (isRegex) params.set("isRegex", true);
        return call("searchInContent", params);
    }

    
    CDPResponse setBlackboxPatterns(const std::vector<std::string>& patterns) {
        JsonArray arr;
        for (const auto& p : patterns) arr.push_back(p);
        return call("setBlackboxPatterns", Params().set("patterns", arr));
    }

    CDPResponse setBlackboxedRanges(const std::string& scriptId,
                                     const std::vector<ScriptPosition>& positions) {
        JsonArray arr;
        for (const auto& p : positions) arr.push_back(p.toJson());
        return call("setBlackboxedRanges", Params()
            .set("scriptId", scriptId)
            .set("positions", arr));
    }

    
    CDPResponse setAsyncCallStackDepth(int maxDepth) {
        return call("setAsyncCallStackDepth", Params().set("maxDepth", maxDepth));
    }

    
    CDPResponse setInstrumentationBreakpoint(const std::string& instrumentation) {
        return call("setInstrumentationBreakpoint",
            Params().set("instrumentation", instrumentation));
    }

    CDPResponse removeInstrumentationBreakpoint(const std::string& instrumentation) {
        return call("removeInstrumentationBreakpoint",
            Params().set("instrumentation", instrumentation));
    }

    
    CDPResponse setReturnValue(const JsonValue& newValue) {
        return call("setReturnValue", Params().set("newValue", newValue));
    }

    
    void onScriptParsed(std::function<void(const std::string& scriptId,
                                            const std::string& url,
                                            int startLine,
                                            int startColumn,
                                            int endLine,
                                            int endColumn,
                                            int executionContextId,
                                            const std::string& hash,
                                            const JsonValue& executionContextAuxData,
                                            bool isLiveEdit,
                                            const std::string& sourceMapURL,
                                            bool hasSourceURL,
                                            bool isModule,
                                            int length,
                                            const JsonValue& stackTrace,
                                            int codeOffset,
                                            const std::string& scriptLanguage,
                                            const std::string& embedderName)> callback) {
        on("scriptParsed", [callback](const CDPEvent& event) {
            callback(
                event.params["scriptId"].getString(),
                event.params["url"].getString(),
                event.params["startLine"].getInt(),
                event.params["startColumn"].getInt(),
                event.params["endLine"].getInt(),
                event.params["endColumn"].getInt(),
                event.params["executionContextId"].getInt(),
                event.params["hash"].getString(),
                event.params["executionContextAuxData"],
                event.params["isLiveEdit"].getBool(),
                event.params["sourceMapURL"].getString(),
                event.params["hasSourceURL"].getBool(),
                event.params["isModule"].getBool(),
                event.params["length"].getInt(),
                event.params["stackTrace"],
                event.params["codeOffset"].getInt(),
                event.params["scriptLanguage"].getString(),
                event.params["embedderName"].getString()
            );
        });
    }

    void onScriptFailedToParse(std::function<void(const std::string& scriptId,
                                                    const std::string& url,
                                                    int startLine,
                                                    int startColumn,
                                                    int endLine,
                                                    int endColumn,
                                                    int executionContextId,
                                                    const std::string& hash,
                                                    const JsonValue& executionContextAuxData,
                                                    const std::string& sourceMapURL,
                                                    bool hasSourceURL,
                                                    bool isModule,
                                                    int length,
                                                    const JsonValue& stackTrace,
                                                    int codeOffset,
                                                    const std::string& scriptLanguage,
                                                    const std::string& embedderName)> callback) {
        on("scriptFailedToParse", [callback](const CDPEvent& event) {
            callback(
                event.params["scriptId"].getString(),
                event.params["url"].getString(),
                event.params["startLine"].getInt(),
                event.params["startColumn"].getInt(),
                event.params["endLine"].getInt(),
                event.params["endColumn"].getInt(),
                event.params["executionContextId"].getInt(),
                event.params["hash"].getString(),
                event.params["executionContextAuxData"],
                event.params["sourceMapURL"].getString(),
                event.params["hasSourceURL"].getBool(),
                event.params["isModule"].getBool(),
                event.params["length"].getInt(),
                event.params["stackTrace"],
                event.params["codeOffset"].getInt(),
                event.params["scriptLanguage"].getString(),
                event.params["embedderName"].getString()
            );
        });
    }

    void onPaused(std::function<void(const std::vector<CallFrame>& callFrames,
                                       const std::string& reason,
                                       const JsonValue& data,
                                       const std::vector<std::string>& hitBreakpoints,
                                       const JsonValue& asyncStackTrace,
                                       const JsonValue& asyncStackTraceId,
                                       const JsonValue& asyncCallStackTraceId)> callback) {
        on("paused", [callback](const CDPEvent& event) {
            std::vector<CallFrame> frames;
            if (event.params["callFrames"].isArray()) {
                for (const auto& cf : event.params["callFrames"].asArray()) {
                    frames.push_back(CallFrame::fromJson(cf));
                }
            }
            std::vector<std::string> hitBps;
            if (event.params["hitBreakpoints"].isArray()) {
                for (const auto& bp : event.params["hitBreakpoints"].asArray()) {
                    hitBps.push_back(bp.getString());
                }
            }
            callback(
                frames,
                event.params["reason"].getString(),
                event.params["data"],
                hitBps,
                event.params["asyncStackTrace"],
                event.params["asyncStackTraceId"],
                event.params["asyncCallStackTraceId"]
            );
        });
    }

    void onResumed(std::function<void()> callback) {
        on("resumed", [callback](const CDPEvent&) {
            callback();
        });
    }

    void onBreakpointResolved(std::function<void(const std::string& breakpointId,
                                                   const Location& location)> callback) {
        on("breakpointResolved", [callback](const CDPEvent& event) {
            callback(
                event.params["breakpointId"].getString(),
                Location::fromJson(event.params["location"])
            );
        });
    }
};

} 
