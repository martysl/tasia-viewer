/**
 * @file llpluginruntime.h
 * @brief Abstract plugin runtime interface and concrete runtime hosts
 *
 * $LicenseInfo:firstyear=2024&license=viewerlgpl$
 * Tasia Viewer Plugin System
 * $/LicenseInfo$
 */

#ifndef LL_LLPLUGINRUNTIME_H
#define LL_LLPLUGINRUNTIME_H

#include <string>
#include <memory>
#include <functional>
#include <queue>
#include <map>
#include "llsd.h"
#include "llpluginmanifest.h"
#include "llpluginprotocol.h"
#include "llpluginstorage.h"
#include "llplugineventbus.h"

/// Plugin state
enum class LLPluginState
{
    UNLOADED,
    LOADING,
    RUNNING,
    ERROR,
    DISABLED
};

/// Abstract base class for all plugin runtime hosts
class LLPluginRuntime
{
public:
    LLPluginRuntime(const LLPluginManifest& manifest);
    virtual ~LLPluginRuntime();

    /// Initialize the runtime and load the plugin
    virtual bool init() = 0;

    /// Start executing the plugin
    virtual bool start() = 0;

    /// Stop the plugin
    virtual void stop() = 0;

    /// Send a message (API request/response) to the plugin
    virtual void sendMessage(const LLPluginMessage& msg) = 0;

    /// Called periodically to process queued messages
    virtual void update();

    /// Get current state
    LLPluginState getState() const { return mState; }

    /// Get plugin ID
    std::string getPluginId() const { return mManifest.getId(); }

    /// Get manifest reference
    const LLPluginManifest& getManifest() const { return mManifest; }

    /// Get plugin log path
    std::string getLogPath() const;

    /// Log a message to plugin-specific log
    void log(const std::string& message, bool is_error = false);

    /// Queue a message to send to the plugin
    void queueMessage(const LLPluginMessage& msg);

    /// Called when a message is received from the plugin
    void setMessageCallback(std::function<void(const LLPluginMessage&)> callback);

    /// Forward an event to the plugin
    virtual void forwardEvent(const std::string& event_name, const LLSD& data) = 0;

    /// Get error message
    std::string getError() const { return mError; }

protected:
    LLPluginManifest mManifest;
    LLPluginState mState = LLPluginState::UNLOADED;
    std::string mError;

    /// Process callbacks and event forwarding
    void processOutgoingQueue();

    std::queue<LLPluginMessage> mIncomingQueue;
    std::queue<LLPluginMessage> mOutgoingQueue;
    std::function<void(const LLPluginMessage&)> mMessageCallback;

    /// Subscriptions to viewer events (for forwarding to plugin)
    std::vector<S32> mEventSubscriptions;
};

// ---- Lua runtime ----
class LLLuaPluginHost : public LLPluginRuntime
{
public:
    LLLuaPluginHost(const LLPluginManifest& manifest);
    ~LLLuaPluginHost() override;

    bool init() override;
    bool start() override;
    void stop() override;
    void sendMessage(const LLPluginMessage& msg) override;
    void forwardEvent(const std::string& event_name, const LLSD& data) override;

private:
    // Lua state (forward declaration avoids including lua headers)
    struct lua_State;
    lua_State* mL = nullptr;
    bool loadScript(const std::string& path);
};

// ---- JavaScript (QuickJS) runtime ----
class LLJavaScriptPluginHost : public LLPluginRuntime
{
public:
    LLJavaScriptPluginHost(const LLPluginManifest& manifest);
    ~LLJavaScriptPluginHost() override;

    bool init() override;
    bool start() override;
    void stop() override;
    void sendMessage(const LLPluginMessage& msg) override;
    void forwardEvent(const std::string& event_name, const LLSD& data) override;

private:
    void* mJSRuntime = nullptr; // JSRuntime*
    void* mJSContext = nullptr;  // JSContext*
    bool loadScript(const std::string& path);
};

// ---- Python IPC runtime ----
class LLPythonPluginHost : public LLPluginRuntime
{
public:
    LLPythonPluginHost(const LLPluginManifest& manifest);
    ~LLPythonPluginHost() override;

    bool init() override;
    bool start() override;
    void stop() override;
    void sendMessage(const LLPluginMessage& msg) override;
    void forwardEvent(const std::string& event_name, const LLSD& data) override;

    // Set IPC pipe/socket path
    void setIPCPath(const std::string& path) { mIPCPath = path; }

private:
    std::string mIPCPath;
    int mPipeFd = -1;      // Named pipe / Unix socket fd
    int mProcessHandle = 0; // Child process handle

    bool launchHostProcess();
    void sendThroughIPC(const std::string& data);
    std::string receiveFromIPC();
    void cleanup();
};

#endif
