

#include "cdp/domains/DebuggerDomain.hpp"

namespace cdp {

JsonValue Location::toJson() const {
    JsonObject obj;
    obj["scriptId"] = scriptId;
    obj["lineNumber"] = lineNumber;
    if (columnNumber > 0) obj["columnNumber"] = columnNumber;
    return obj;
}

Location Location::fromJson(const JsonValue& json) {
    Location loc;
    loc.scriptId = json["scriptId"].getString();
    loc.lineNumber = json["lineNumber"].getInt();
    loc.columnNumber = json["columnNumber"].getInt();
    return loc;
}

CallFrame CallFrame::fromJson(const JsonValue& json) {
    CallFrame frame;
    frame.callFrameId = json["callFrameId"].getString();
    frame.functionName = json["functionName"].getString();
    frame.url = json["url"].getString();
    frame.location = Location::fromJson(json["location"]);

    if (json.contains("functionLocation")) {
        frame.functionLocation = Location::fromJson(json["functionLocation"]);
    }

    frame.thisObject = RemoteObject::fromJson(json["this"]);

    if (json.contains("returnValue")) {
        frame.returnValue = RemoteObject::fromJson(json["returnValue"]);
    }

    if (json["scopeChain"].isArray()) {
        for (size_t i = 0; i < json["scopeChain"].size(); ++i) {
            const auto& scope = json["scopeChain"][i];
            frame.scopeChain.push_back({
                scope["type"].getString(),
                RemoteObject::fromJson(scope["object"])
            });
        }
    }

    return frame;
}

ScriptInfo ScriptInfo::fromJson(const JsonValue& json) {
    ScriptInfo info;
    info.scriptId = json["scriptId"].getString();
    info.url = json["url"].getString();
    info.startLine = json["startLine"].getInt();
    info.startColumn = json["startColumn"].getInt();
    info.endLine = json["endLine"].getInt();
    info.endColumn = json["endColumn"].getInt();
    info.executionContextId = json["executionContextId"].getInt();
    info.hash = json["hash"].getString();
    info.sourceMapURL = json["sourceMapURL"].getString();
    info.hasSourceURL = json["hasSourceURL"].getBool();
    info.isModule = json["isModule"].getBool();
    info.length = json["length"].getInt();
    return info;
}

CDPResponse DebuggerDomain::enable(int maxScriptsCacheSize) {
    JsonObject params;
    params["maxScriptsCacheSize"] = maxScriptsCacheSize;
    return sendCommand("Debugger.enable", params);
}

CDPResponse DebuggerDomain::pause() {
    return sendCommand("Debugger.pause");
}

CDPResponse DebuggerDomain::resume() {
    return sendCommand("Debugger.resume");
}

CDPResponse DebuggerDomain::stepOver() {
    return sendCommand("Debugger.stepOver");
}

CDPResponse DebuggerDomain::stepInto(bool breakOnAsyncCall) {
    JsonObject params;
    if (breakOnAsyncCall) params["breakOnAsyncCall"] = breakOnAsyncCall;
    return sendCommand("Debugger.stepInto", params);
}

CDPResponse DebuggerDomain::stepOut() {
    return sendCommand("Debugger.stepOut");
}

Breakpoint DebuggerDomain::setBreakpoint(const Location& location, const std::string& condition) {
    JsonObject params;
    params["location"] = location.toJson();
    if (!condition.empty()) params["condition"] = condition;

    auto response = sendCommand("Debugger.setBreakpoint", params);
    Breakpoint bp;

    if (response.isSuccess()) {
        bp.breakpointId = response.result["breakpointId"].getString();
        bp.locations.push_back(Location::fromJson(response.result["actualLocation"]));
    }

    return bp;
}

Breakpoint DebuggerDomain::setBreakpointByUrl(int lineNumber, const std::string& url,
                                               const std::string& urlRegex,
                                               const std::string& scriptHash,
                                               int columnNumber,
                                               const std::string& condition) {
    JsonObject params;
    params["lineNumber"] = lineNumber;
    if (!url.empty()) params["url"] = url;
    if (!urlRegex.empty()) params["urlRegex"] = urlRegex;
    if (!scriptHash.empty()) params["scriptHash"] = scriptHash;
    if (columnNumber > 0) params["columnNumber"] = columnNumber;
    if (!condition.empty()) params["condition"] = condition;

    auto response = sendCommand("Debugger.setBreakpointByUrl", params);
    Breakpoint bp;

    if (response.isSuccess()) {
        bp.breakpointId = response.result["breakpointId"].getString();
        if (response.result["locations"].isArray()) {
            for (size_t i = 0; i < response.result["locations"].size(); ++i) {
                bp.locations.push_back(Location::fromJson(response.result["locations"][i]));
            }
        }
    }

    return bp;
}

CDPResponse DebuggerDomain::removeBreakpoint(const std::string& breakpointId) {
    JsonObject params;
    params["breakpointId"] = breakpointId;
    return sendCommand("Debugger.removeBreakpoint", params);
}

CDPResponse DebuggerDomain::setBreakpointsActive(bool active) {
    JsonObject params;
    params["active"] = active;
    return sendCommand("Debugger.setBreakpointsActive", params);
}

CDPResponse DebuggerDomain::setSkipAllPauses(bool skip) {
    JsonObject params;
    params["skip"] = skip;
    return sendCommand("Debugger.setSkipAllPauses", params);
}

Breakpoint DebuggerDomain::setInstrumentationBreakpoint(const std::string& instrumentation) {
    JsonObject params;
    params["instrumentation"] = instrumentation;

    auto response = sendCommand("Debugger.setInstrumentationBreakpoint", params);
    Breakpoint bp;

    if (response.isSuccess()) {
        bp.breakpointId = response.result["breakpointId"].getString();
    }

    return bp;
}

EvaluationResult DebuggerDomain::evaluateOnCallFrame(const std::string& callFrameId,
                                                      const std::string& expression,
                                                      bool returnByValue,
                                                      bool generatePreview,
                                                      bool throwOnSideEffect) {
    JsonObject params;
    params["callFrameId"] = callFrameId;
    params["expression"] = expression;
    params["returnByValue"] = returnByValue;
    params["generatePreview"] = generatePreview;
    params["throwOnSideEffect"] = throwOnSideEffect;

    auto response = sendCommand("Debugger.evaluateOnCallFrame", params);
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

std::string DebuggerDomain::getScriptSource(const std::string& scriptId) {
    JsonObject params;
    params["scriptId"] = scriptId;

    auto response = sendCommand("Debugger.getScriptSource", params);
    if (response.isSuccess()) {
        return response.result["scriptSource"].getString();
    }
    return "";
}

CDPResponse DebuggerDomain::setScriptSource(const std::string& scriptId,
                                             const std::string& scriptSource,
                                             bool dryRun) {
    JsonObject params;
    params["scriptId"] = scriptId;
    params["scriptSource"] = scriptSource;
    params["dryRun"] = dryRun;
    return sendCommand("Debugger.setScriptSource", params);
}

std::vector<DebuggerDomain::SearchMatch> DebuggerDomain::searchInContent(
    const std::string& scriptId, const std::string& query,
    bool caseSensitive, bool isRegex) {
    JsonObject params;
    params["scriptId"] = scriptId;
    params["query"] = query;
    params["caseSensitive"] = caseSensitive;
    params["isRegex"] = isRegex;

    auto response = sendCommand("Debugger.searchInContent", params);
    std::vector<SearchMatch> matches;

    if (response.isSuccess() && response.result["result"].isArray()) {
        for (size_t i = 0; i < response.result["result"].size(); ++i) {
            const auto& match = response.result["result"][i];
            matches.push_back({
                match["lineNumber"].getInt(),
                match["lineContent"].getString()
            });
        }
    }

    return matches;
}

std::vector<Location> DebuggerDomain::getPossibleBreakpoints(
    const Location& start, const std::optional<Location>& end, bool restrictToFunction) {
    JsonObject params;
    params["start"] = start.toJson();
    if (end) params["end"] = end->toJson();
    params["restrictToFunction"] = restrictToFunction;

    auto response = sendCommand("Debugger.getPossibleBreakpoints", params);
    std::vector<Location> locations;

    if (response.isSuccess() && response.result["locations"].isArray()) {
        for (size_t i = 0; i < response.result["locations"].size(); ++i) {
            locations.push_back(Location::fromJson(response.result["locations"][i]));
        }
    }

    return locations;
}

CDPResponse DebuggerDomain::continueToLocation(const Location& location,
                                                const std::string& targetCallFrames) {
    JsonObject params;
    params["location"] = location.toJson();
    params["targetCallFrames"] = targetCallFrames;
    return sendCommand("Debugger.continueToLocation", params);
}

CDPResponse DebuggerDomain::restartFrame(const std::string& callFrameId) {
    JsonObject params;
    params["callFrameId"] = callFrameId;
    return sendCommand("Debugger.restartFrame", params);
}

CDPResponse DebuggerDomain::setVariableValue(int scopeNumber, const std::string& variableName,
                                              const JsonValue& newValue,
                                              const std::string& callFrameId) {
    JsonObject params;
    params["scopeNumber"] = scopeNumber;
    params["variableName"] = variableName;
    params["newValue"] = newValue;
    params["callFrameId"] = callFrameId;
    return sendCommand("Debugger.setVariableValue", params);
}

CDPResponse DebuggerDomain::setAsyncCallStackDepth(int maxDepth) {
    JsonObject params;
    params["maxDepth"] = maxDepth;
    return sendCommand("Debugger.setAsyncCallStackDepth", params);
}

CDPResponse DebuggerDomain::setBlackboxPatterns(const std::vector<std::string>& patterns) {
    JsonObject params;
    JsonArray patternsArray;
    for (const auto& pattern : patterns) {
        patternsArray.push_back(pattern);
    }
    params["patterns"] = patternsArray;
    return sendCommand("Debugger.setBlackboxPatterns", params);
}

CDPResponse DebuggerDomain::setBlackboxedRanges(const std::string& scriptId,
                                                 const std::vector<std::pair<Location, Location>>& positions) {
    JsonObject params;
    params["scriptId"] = scriptId;
    JsonArray positionsArray;
    for (const auto& [start, end] : positions) {
        JsonObject range;
        range["start"] = start.toJson();
        range["end"] = end.toJson();
        positionsArray.push_back(range);
    }
    params["positions"] = positionsArray;
    return sendCommand("Debugger.setBlackboxedRanges", params);
}

CDPResponse DebuggerDomain::setReturnValue(const JsonValue& newValue) {
    JsonObject params;
    params["newValue"] = newValue;
    return sendCommand("Debugger.setReturnValue", params);
}

CDPResponse DebuggerDomain::setPauseOnExceptions(const std::string& state) {
    JsonObject params;
    params["state"] = state;
    return sendCommand("Debugger.setPauseOnExceptions", params);
}

void DebuggerDomain::onPaused(
    std::function<void(const std::vector<CallFrame>&, const std::string&,
                       const JsonValue&, const std::vector<std::string>&)> callback) {
    subscribeEvent("paused", [callback](const CDPEvent& event) {
        std::vector<CallFrame> frames;
        if (event.params["callFrames"].isArray()) {
            for (size_t i = 0; i < event.params["callFrames"].size(); ++i) {
                frames.push_back(CallFrame::fromJson(event.params["callFrames"][i]));
            }
        }

        std::vector<std::string> hitBreakpoints;
        if (event.params["hitBreakpoints"].isArray()) {
            for (size_t i = 0; i < event.params["hitBreakpoints"].size(); ++i) {
                hitBreakpoints.push_back(event.params["hitBreakpoints"][i].getString());
            }
        }

        callback(
            frames,
            event.params["reason"].getString(),
            event.params["data"],
            hitBreakpoints
        );
    });
}

void DebuggerDomain::onResumed(std::function<void()> callback) {
    subscribeEvent("resumed", [callback](const CDPEvent&) {
        callback();
    });
}

void DebuggerDomain::onScriptParsed(std::function<void(const ScriptInfo&)> callback) {
    subscribeEvent("scriptParsed", [callback](const CDPEvent& event) {
        callback(ScriptInfo::fromJson(event.params));
    });
}

void DebuggerDomain::onScriptFailedToParse(std::function<void(const ScriptInfo&)> callback) {
    subscribeEvent("scriptFailedToParse", [callback](const CDPEvent& event) {
        callback(ScriptInfo::fromJson(event.params));
    });
}

void DebuggerDomain::onBreakpointResolved(
    std::function<void(const std::string&, const Location&)> callback) {
    subscribeEvent("breakpointResolved", [callback](const CDPEvent& event) {
        callback(
            event.params["breakpointId"].getString(),
            Location::fromJson(event.params["location"])
        );
    });
}

} 
