/**
 * @file llpluginruntime.cpp
 * @brief Abstract plugin runtime base implementation
 *
 * $LicenseInfo:firstyear=2024&license=viewerlgpl$
 * Tasia Viewer Plugin System
 * $/LicenseInfo$
 */

#include "llviewerprecompiledheaders.h"
#include "llpluginruntime.h"
#include "llpluginpermissions.h"

#include "llsd.h"
#include "llfile.h"
#include "llpath.h"
#include "llstring.h"
#include "llviewercontrol.h"
#include <boost/filesystem.hpp>
#include <fstream>

#ifdef TASIA_ENABLE_LUA
extern "C" {
# include "lua.h"
# include "lualib.h"
# include "lauxlib.h"
# include "luacode.h"
}
#endif

using namespace fs;

// ==================== LLPluginRuntime (base) ====================

LLPluginRuntime::LLPluginRuntime(const LLPluginManifest& manifest)
    : mManifest(manifest)
{
}

LLPluginRuntime::~LLPluginRuntime()
{
    stop();
}

std::string LLPluginRuntime::getLogPath() const
{
    std::string log_dir = gDirUtilp->getExpandedFilename(LL_PATH_LOGS, "plugins");
    std::string safe_id = mManifest.getId();
    for (char& c : safe_id)
    {
        if (c == '.' || c == '-') c = '_';
    }
    return log_dir + "/" + safe_id + ".log";
}

void LLPluginRuntime::log(const std::string& message, bool is_error)
{
    std::string path = getLogPath();
    fs::path dir = fs::path(path).parent_path();
    if (!fs::exists(dir))
    {
        fs::create_directories(dir);
    }

    std::ofstream file(path, std::ios::app);
    if (file.is_open())
    {
        file << (is_error ? "[ERROR] " : "[INFO] ")
             << message << std::endl;
    }

    if (is_error)
    {
        LL_WARNS("Plugins") << "[" << mManifest.getId() << "] " << message << LL_ENDL;
    }
    else
    {
        LL_DEBUGS("Plugins") << "[" << mManifest.getId() << "] " << message << LL_ENDL;
    }
}

void LLPluginRuntime::queueMessage(const LLPluginMessage& msg)
{
    mIncomingQueue.push(msg);
}

void LLPluginRuntime::setMessageCallback(std::function<void(const LLPluginMessage&)> callback)
{
    mMessageCallback = callback;
}

void LLPluginRuntime::update()
{
    processOutgoingQueue();
}

void LLPluginRuntime::processOutgoingQueue()
{
    while (!mOutgoingQueue.empty())
    {
        auto msg = mOutgoingQueue.front();
        mOutgoingQueue.pop();
        if (mMessageCallback)
        {
            mMessageCallback(msg);
        }
    }
}

// ==================== LLLuaPluginHost ====================

LLLuaPluginHost::LLLuaPluginHost(const LLPluginManifest& manifest)
    : LLPluginRuntime(manifest)
{
}

LLLuaPluginHost::~LLLuaPluginHost()
{
    stop();
}

#ifdef TASIA_ENABLE_LUA

/// Lua allocator with memory limit
static void* luaPluginAllocator(void* ud, void* ptr, size_t osize, size_t nsize)
{
    constexpr size_t MAX_LUA_MEMORY = 64 * 1024 * 1024; // 64 MB per plugin
    static size_t total_alloc = 0;

    if (nsize == 0)
    {
        total_alloc -= osize;
        free(ptr);
        return NULL;
    }

    if (total_alloc + nsize - osize > MAX_LUA_MEMORY)
        return NULL; // out of memory

    void* new_ptr = realloc(ptr, nsize);
    if (new_ptr)
    {
        total_alloc += nsize - osize;
    }
    return new_ptr;
}

/// C API function: tasia.viewer.notify(message)
static int lua_viewer_notify(lua_State* L)
{
    const char* message = luaL_checkstring(L, 1);
    if (message)
    {
        LLSD args;
        args["MESSAGE"] = std::string(message);
        LLNotificationsUtil::add("SystemMessage", args);
    }
    return 0;
}

/// C API function: tasia.chat.send(message, channel)
static int lua_chat_send(lua_State* L)
{
    const char* message = luaL_checkstring(L, 1);
    S32 channel = luaL_optinteger(L, 2, 0);
    if (message)
    {
        LLChat chat;
        chat.mText = message;
        chat.mChatType = (channel == 0) ? CHAT_TYPE_NORMAL : CHAT_TYPE_DEBUG_MSG;
        chat.mFromName = gAgent.getAvatarName();
        chat.mFromID = gAgent.getID();
        LLViewerChat::sendChatFromViewer(chat, channel);
    }
    return 0;
}

/// Register Tasia API functions into Lua state
static void registerLuaAPI(lua_State* L, LLPluginRuntime* runtime)
{
    // Create tasia table
    lua_newtable(L);

    // tasia.viewer
    lua_newtable(L);
    lua_pushcfunction(L, lua_viewer_notify);
    lua_setfield(L, -2, "notify");
    lua_setfield(L, -2, "viewer");

    // tasia.chat
    lua_newtable(L);
    lua_pushcfunction(L, lua_chat_send);
    lua_setfield(L, -2, "send");
    lua_setfield(L, -2, "chat");

    // Set global "tasia"
    lua_setglobal(L, "tasia");
}

bool LLLuaPluginHost::init()
{
    // Create Lua state with custom allocator for memory limiting
    mL = lua_newstate(luaPluginAllocator, NULL);
    if (!mL)
    {
        mError = "Failed to create Lua state";
        mState = LLPluginState::ERROR;
        return false;
    }

    // Open standard libraries (safe subset)
    static const luaL_Reg safe_libs[] = {
        {"_G",           luaopen_base},
        {"table",        luaopen_table},
        {"string",       luaopen_string},
        {"math",         luaopen_math},
        {"utf8",         luaopen_utf8},
        {"coroutine",    luaopen_coroutine},
        {NULL, NULL}
    };

    const luaL_Reg* lib = safe_libs;
    for (; lib->func; lib++)
    {
        luaL_requiref(mL, lib->name, lib->func, 1);
        lua_pop(mL, 1);
    }

    // Apply Luau sandbox for isolation
    luaL_sandbox(mL);
    luaL_sandboxthread(mL);

    // Register Tasia API
    registerLuaAPI(mL, this);

    // Remove dangerous globals
    lua_pushnil(mL);
    lua_setglobal(mL, "dofile");
    lua_pushnil(mL);
    lua_setglobal(mL, "loadfile");
    lua_pushnil(mL);
    lua_setglobal(mL, "require");

    log("Lua runtime initialized");
    mState = LLPluginState::LOADING;
    return true;
}

bool LLLuaPluginHost::loadScript(const std::string& path)
{
    if (!mL) return false;

    // Read source file
    std::ifstream file(path);
    if (!file.is_open())
    {
        mError = "Cannot open script: " + path;
        log(mError, true);
        return false;
    }

    std::string source((std::istreambuf_iterator<char>(file)),
                        std::istreambuf_iterator<char>());
    file.close();

    // Compile source to bytecode using Luau compiler
    size_t bytecodeSize = 0;
    std::string chunkname = "@" + mManifest.getId();
    char* bytecode = luau_compile(source.c_str(), source.size(), NULL, &bytecodeSize);
    if (!bytecode)
    {
        mError = "Failed to compile Lua script";
        log(mError, true);
        return false;
    }

    // Load bytecode into VM
    int result = luau_load(mL, chunkname.c_str(), bytecode, bytecodeSize, 0);
    free(bytecode);

    if (result != 0)
    {
        // Get error from stack
        const char* err = lua_tostring(mL, -1);
        mError = "Lua load error: " + (err ? std::string(err) : "unknown");
        log(mError, true);
        lua_pop(mL, 1);
        return false;
    }

    log("Script loaded: " + path);
    return true;
}

bool LLLuaPluginHost::start()
{
    if (!mL) return false;

    // Load the entrypoint script
    std::string script_path = mManifest.getBasePath() + "/" + mManifest.getEntrypoint();
    if (!loadScript(script_path))
    {
        mState = LLPluginState::ERROR;
        return false;
    }

    // Execute the loaded chunk
    if (lua_pcall(mL, 0, 0, 0) != 0)
    {
        const char* err = lua_tostring(mL, -1);
        mError = "Lua error: " + (err ? std::string(err) : "unknown");
        log(mError, true);
        lua_pop(mL, 1);
        mState = LLPluginState::ERROR;
        return false;
    }

    log("Plugin started");
    mState = LLPluginState::RUNNING;
    return true;
}

void LLLuaPluginHost::stop()
{
    if (mL)
    {
        lua_close(mL);
        mL = nullptr;
    }
    mState = LLPluginState::UNLOADED;
}

void LLLuaPluginHost::sendMessage(const LLPluginMessage& msg)
{
    // Queue for processing in update()
    mOutgoingQueue.push(msg);
}

void LLLuaPluginHost::forwardEvent(const std::string& event_name, const LLSD& data)
{
    if (!mL || mState != LLPluginState::RUNNING) return;

    // Call the plugin's event handler via Lua
    // The plugin script registered handlers using tasia.events.on()
    // which stores them in a table. We call _tasia_events[event_name](data)

    lua_getglobal(mL, "_tasia_events");
    if (lua_isnil(mL, -1))
    {
        lua_pop(mL, 1);
        return;
    }

    lua_getfield(mL, -1, event_name.c_str());
    if (lua_isnil(mL, -1))
    {
        lua_pop(mL, 2);
        return;
    }

    // Push event data as a table
    lua_newtable(mL);
    for (const auto& pair : data)
    {
        lua_pushstring(mL, pair.first.c_str());
        // Convert LLSD to Lua value (simplified)
        switch (pair.second.type())
        {
        case LLSD::TypeString:
            lua_pushstring(mL, pair.second.asString().c_str());
            break;
        case LLSD::TypeInteger:
            lua_pushinteger(mL, pair.second.asInteger());
            break;
        case LLSD::TypeReal:
            lua_pushnumber(mL, pair.second.asReal());
            break;
        case LLSD::TypeBoolean:
            lua_pushboolean(mL, pair.second.asBoolean());
            break;
        default:
            lua_pushstring(mL, pair.second.asString().c_str());
            break;
        }
        lua_settable(mL, -3);
    }

    // Call the handler
    if (lua_pcall(mL, 1, 0, 0) != 0)
    {
        const char* err = lua_tostring(mL, -1);
        log(std::string("Event handler error: ") + (err ? err : "unknown"), true);
        lua_pop(mL, 1);
    }

    lua_pop(mL, 1); // pop _tasia_events table
}

#else // !TASIA_ENABLE_LUA

bool LLLuaPluginHost::init()
{
    log("Lua runtime not compiled in this build", true);
    mState = LLPluginState::ERROR;
    mError = "Lua support not available";
    return false;
}

bool LLLuaPluginHost::start() { return false; }
void LLLuaPluginHost::stop()
{
    mState = LLPluginState::UNLOADED;
}
void LLLuaPluginHost::sendMessage(const LLPluginMessage& msg) { }
void LLLuaPluginHost::forwardEvent(const std::string& event_name, const LLSD& data) { }

#endif // TASIA_ENABLE_LUA

// ==================== LLJavaScriptPluginHost ====================

LLJavaScriptPluginHost::LLJavaScriptPluginHost(const LLPluginManifest& manifest)
    : LLPluginRuntime(manifest)
{
}

LLJavaScriptPluginHost::~LLJavaScriptPluginHost()
{
    stop();
}

bool LLJavaScriptPluginHost::init()
{
    log("QuickJS runtime not compiled in this build", true);
    mState = LLPluginState::ERROR;
    mError = "JavaScript support not available";
    return false;
}

bool LLJavaScriptPluginHost::start()
{
    return false;
}

void LLJavaScriptPluginHost::stop()
{
    mState = LLPluginState::UNLOADED;
}

void LLJavaScriptPluginHost::sendMessage(const LLPluginMessage& msg)
{
    // Stub
}

void LLJavaScriptPluginHost::forwardEvent(const std::string& event_name, const LLSD& data)
{
    // Stub
}

// ==================== LLPythonPluginHost ====================

LLPythonPluginHost::LLPythonPluginHost(const LLPluginManifest& manifest)
    : LLPluginRuntime(manifest)
{
}

LLPythonPluginHost::~LLPythonPluginHost()
{
    cleanup();
}

bool LLPythonPluginHost::init()
{
    log("Python runtime not compiled in this build", true);
    mState = LLPluginState::ERROR;
    mError = "Python support not available";
    return false;
}

bool LLPythonPluginHost::start()
{
    return false;
}

void LLPythonPluginHost::stop()
{
    cleanup();
    mState = LLPluginState::UNLOADED;
}

void LLPythonPluginHost::sendMessage(const LLPluginMessage& msg)
{
    // Stub
}

void LLPythonPluginHost::forwardEvent(const std::string& event_name, const LLSD& data)
{
    // Stub
}

bool LLPythonPluginHost::launchHostProcess()
{
    return false; // Not implemented
}

void LLPythonPluginHost::sendThroughIPC(const std::string& data)
{
    // Not implemented
}

std::string LLPythonPluginHost::receiveFromIPC()
{
    return "";
}

void LLPythonPluginHost::cleanup()
{
    if (mPipeFd >= 0)
    {
#ifdef LL_WINDOWS
        closesocket(mPipeFd);
#else
        close(mPipeFd);
#endif
        mPipeFd = -1;
    }
}
