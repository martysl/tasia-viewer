/**
 * @file llpluginmanager.cpp
 * @brief Plugin manager implementation
 *
 * $LicenseInfo:firstyear=2024&license=viewerlgpl$
 * Tasia Viewer Plugin System
 * $/LicenseInfo$
 */

#include "llviewerprecompiledheaders.h"
#include "llpluginmanager.h"
#include "llpluginmanifest.h"
#include "llpluginruntime.h"
#include "llpluginpermissions.h"
#include "llpluginservice.h"
#include "llplugineventbus.h"
#include "llpluginprotocol.h"

#include "llappviewer.h"
#include "llviewercontrol.h"
#include "llfloaterreg.h"
#include "llsd.h"
#include "llpath.h"
#include "llstring.h"
#include "llnotificationsutil.h"
#include "lltrans.h"

#include <boost/filesystem.hpp>
#include <fstream>

namespace fs = boost::filesystem;

LLPluginManager& LLPluginManager::instance()
{
    static LLPluginManager sInstance;
    return sInstance;
}

void LLPluginManager::init()
{
    LL_INFOS("Plugins") << "Initializing plugin manager" << LL_ENDL;

    // Load permission store
    LLPluginPermissionStore::instance().load();

    // Check for command-line flags
    mPluginsDisabled = gSavedSettings.getBOOL("DisablePlugins");
    mSafeMode = gSavedSettings.getBOOL("SafeMode");

    if (mPluginsDisabled || mSafeMode)
    {
        LL_INFOS("Plugins") << "Plugins disabled via command-line or safe mode" << LL_ENDL;
        return;
    }

    // Check for crash markers
    std::string crash_path = gDirUtilp->getExpandedFilename(LL_PATH_LOGS, "plugins", CRASH_MARKER_FILE);
    if (fs::exists(crash_path))
    {
        std::ifstream marker(crash_path);
        std::string crashed_id;
        while (std::getline(marker, crashed_id))
        {
            if (!crashed_id.empty())
                mCrashedPlugins.push_back(crashed_id);
        }
        marker.close();
        fs::remove(crash_path);

        if (!mCrashedPlugins.empty())
        {
            LL_WARNS("Plugins") << "Detected plugin crashes from previous session" << LL_ENDL;
            mSafeMode = true;
        }
    }

    // Discover and auto-load enabled plugins
    discoverPlugins();

    // Emit viewer.started event
    emitEvent(LLPluginEventBus::EVENT_VIEWER_STARTED, LLSD());
}

void LLPluginManager::discoverPlugins()
{
    mDiscoveredManifests.clear();
    auto dirs = getPluginDirectories();

    for (const auto& dir : dirs)
    {
        if (!fs::exists(dir))
            continue;

        // Scan directories for manifest.json
        for (auto& entry : fs::directory_iterator(dir))
        {
            if (!fs::is_directory(entry))
            {
                // Check for .tasiaplugin file (ZIP format)
                if (entry.path().extension() == ".tasiaplugin")
                {
                    LL_INFOS("Plugins") << "Found packaged plugin: " << entry.path() << LL_ENDL;
                    // TODO: Extract and load
                    continue;
                }
                continue;
            }

            fs::path manifest_path = entry.path() / "manifest.json";
            if (!fs::exists(manifest_path))
                continue;

            LLPluginManifest manifest;
            if (manifest.parse(manifest_path.string()))
            {
                // Check for duplicate IDs
                if (LLPluginManifestDB::instance().hasId(manifest.getId()))
                {
                    LL_WARNS("Plugins") << "Duplicate plugin ID: " << manifest.getId() << LL_ENDL;
                    continue;
                }

                LLPluginManifestDB::instance().add(manifest);
                mDiscoveredManifests.push_back(manifest);
                LL_INFOS("Plugins") << "Discovered plugin: " << manifest.getName()
                                    << " (" << manifest.getId() << ")" << LL_ENDL;
            }
            else
            {
                LL_WARNS("Plugins") << "Invalid manifest: " << manifest_path
                                    << " - " << manifest.getError() << LL_ENDL;
            }
        }
    }
}

bool LLPluginManager::loadPlugin(const std::string& plugin_id)
{
    if (mPluginsDisabled || mSafeMode)
    {
        LL_INFOS("Plugins") << "Cannot load plugin in safe/disabled mode: " << plugin_id << LL_ENDL;
        return false;
    }

    // Already loaded
    if (mPlugins.find(plugin_id) != mPlugins.end())
    {
        LL_INFOS("Plugins") << "Plugin already loaded: " << plugin_id << LL_ENDL;
        return true;
    }

    // Find manifest
    LLPluginManifest* manifest = LLPluginManifestDB::instance().get(plugin_id);
    if (!manifest)
    {
        LL_WARNS("Plugins") << "Plugin manifest not found: " << plugin_id << LL_ENDL;
        return false;
    }

    // Check runtime availability
    if (!isRuntimeAvailable(manifest->getRuntime()))
    {
        LL_WARNS("Plugins") << "Runtime not available for: " << plugin_id << LL_ENDL;
        return false;
    }

    // Check permissions were granted
    auto granted = LLPluginPermissionStore::instance().getGrants(plugin_id);
    if (granted.empty() && !manifest->getPermissions().empty())
    {
        LL_INFOS("Plugins") << "Plugin needs permission grant: " << plugin_id << LL_ENDL;
        // Will be handled by UI
        return false;
    }

    // Create runtime
    LLPluginRuntime* runtime = createRuntime(*manifest);
    if (!runtime)
    {
        LL_WARNS("Plugins") << "Failed to create runtime for: " << plugin_id << LL_ENDL;
        return false;
    }

    // Init runtime
    if (!runtime->init())
    {
        LL_WARNS("Plugins") << "Failed to init runtime for: " << plugin_id << LL_ENDL;
        delete runtime;
        return false;
    }

    // Set message callback (plugin -> service dispatch)
    runtime->setMessageCallback([this, plugin_id](const LLPluginMessage& msg) {
        auto response = dispatch(plugin_id, msg);
        if (response.isResponse() || response.isError())
        {
            // Send response back to plugin
            auto* rt = getRuntime(plugin_id);
            if (rt) rt->queueMessage(response);
        }
    });

    mPlugins[plugin_id] = std::unique_ptr<LLPluginRuntime>(runtime);

    // Start the plugin
    if (!runtime->start())
    {
        LL_WARNS("Plugins") << "Failed to start plugin: " << plugin_id << LL_ENDL;
        mPlugins.erase(plugin_id);
        return false;
    }

    LL_INFOS("Plugins") << "Plugin loaded: " << manifest->getName() << LL_ENDL;
    return true;
}

void LLPluginManager::unloadPlugin(const std::string& plugin_id)
{
    auto it = mPlugins.find(plugin_id);
    if (it != mPlugins.end())
    {
        // Unsubscribe from all events
        LLPluginEventBus::instance().unsubscribeAll(plugin_id);
        it->second->stop();
        mPlugins.erase(it);
        LL_INFOS("Plugins") << "Plugin unloaded: " << plugin_id << LL_ENDL;
    }
}

bool LLPluginManager::enablePlugin(const std::string& plugin_id)
{
    return loadPlugin(plugin_id);
}

void LLPluginManager::disablePlugin(const std::string& plugin_id)
{
    unloadPlugin(plugin_id);
}

bool LLPluginManager::reloadPlugin(const std::string& plugin_id)
{
    unloadPlugin(plugin_id);
    return loadPlugin(plugin_id);
}

bool LLPluginManager::removePlugin(const std::string& plugin_id, bool delete_files)
{
    unloadPlugin(plugin_id);
    LLPluginManifestDB::instance().remove(plugin_id);
    LLPluginPermissionStore::instance().revokeAll(plugin_id);
    LLPluginPermissionStore::instance().save();

    if (delete_files)
    {
        // Find and delete plugin directory
        auto dirs = getPluginDirectories();
        for (const auto& dir : dirs)
        {
            if (!fs::exists(dir)) continue;
            for (auto& entry : fs::directory_iterator(dir))
            {
                if (!fs::is_directory(entry)) continue;
                fs::path mf = entry.path() / "manifest.json";
                if (!fs::exists(mf)) continue;

                LLPluginManifest m;
                if (m.parse(mf.string()) && m.getId() == plugin_id)
                {
                    fs::remove_all(entry.path());
                    LL_INFOS("Plugins") << "Deleted plugin directory: " << entry.path() << LL_ENDL;
                    return true;
                }
            }
        }
    }
    return true;
}

LLPluginInfo LLPluginManager::getPluginInfo(const std::string& plugin_id) const
{
    LLPluginInfo info;
    info.mId = plugin_id;

    auto* manifest = LLPluginManifestDB::instance().get(plugin_id);
    if (manifest)
    {
        info.mName = manifest->getName();
        info.mVersion = manifest->getVersion();
        info.mAuthor = manifest->getAuthor();
        info.mDescription = manifest->getDescription();
        info.mRuntime = manifest->getRuntime();
    }

    auto it = mPlugins.find(plugin_id);
    if (it != mPlugins.end())
    {
        info.mState = it->second->getState();
        info.mEnabled = true;
        info.mError = it->second->getError();
    }

    info.mHasPermissions = !LLPluginPermissionStore::instance().getGrants(plugin_id).empty();
    return info;
}

std::vector<LLPluginInfo> LLPluginManager::getAllPluginInfos() const
{
    std::vector<LLPluginInfo> result;
    auto all_manifests = LLPluginManifestDB::instance().getAll();

    for (const auto& manifest : all_manifests)
    {
        result.push_back(getPluginInfo(manifest.getId()));
    }

    return result;
}

LLPluginRuntime* LLPluginManager::getRuntime(const std::string& plugin_id)
{
    auto it = mPlugins.find(plugin_id);
    if (it == mPlugins.end())
        return nullptr;
    return it->second.get();
}

LLPluginMessage LLPluginManager::dispatch(const std::string& plugin_id, const LLPluginMessage& request)
{
    return LLPluginService::instance().dispatch(plugin_id, request);
}

LLPluginRuntime* LLPluginManager::createRuntime(const LLPluginManifest& manifest)
{
    switch (manifest.getRuntime())
    {
    case LLPluginRuntimeType::LUA:
        return new LLLuaPluginHost(manifest);
    case LLPluginRuntimeType::JAVASCRIPT:
        return new LLJavaScriptPluginHost(manifest);
    case LLPluginRuntimeType::PYTHON:
        return new LLPythonPluginHost(manifest);
    default:
        return nullptr;
    }
}

bool LLPluginManager::hasCrashedPlugins() const
{
    return !mCrashedPlugins.empty();
}

std::vector<std::string> LLPluginManager::getCrashedPluginIds() const
{
    return mCrashedPlugins;
}

void LLPluginManager::setSafeMode(bool enabled)
{
    mSafeMode = enabled;
    if (enabled)
    {
        shutdownAll();
    }
}

void LLPluginManager::recordCrash(const std::string& plugin_id)
{
    std::string crash_path = gDirUtilp->getExpandedFilename(LL_PATH_LOGS, "plugins", CRASH_MARKER_FILE);
    fs::path dir = fs::path(crash_path).parent_path();
    if (!fs::exists(dir))
        fs::create_directories(dir);

    std::ofstream marker(crash_path, std::ios::app);
    if (marker.is_open())
    {
        marker << plugin_id << std::endl;
    }
}

std::vector<std::string> LLPluginManager::getPluginDirectories()
{
    std::vector<std::string> dirs;
    std::string app_path = gDirUtilp->getAppRODataDir();

    // Linux: ./plugins
    dirs.push_back(app_path + "/plugins");

    // User plugin directory (%APPDATA%/plugins or ~/.config/plugins)
    std::string user_path = gDirUtilp->getExpandedFilename(LL_PATH_PER_SL_ACCOUNT, "plugins");
    dirs.push_back(user_path);

    return dirs;
}

void LLPluginManager::openPluginFolder()
{
    auto dirs = getPluginDirectories();
    if (!dirs.empty())
    {
        std::string user_path = dirs.back();
        if (!fs::exists(user_path))
        {
            fs::create_directories(user_path);
        }
        gDirUtilp->openDirectory(user_path);
    }
}

void LLPluginManager::updateAll()
{
    for (auto& pair : mPlugins)
    {
        if (pair.second->getState() == LLPluginState::RUNNING)
        {
            pair.second->update();
        }
    }
}

void LLPluginManager::shutdownAll()
{
    // Emit shutdown event
    emitEvent(LLPluginEventBus::EVENT_VIEWER_SHUTDOWN, LLSD());

    for (auto& pair : mPlugins)
    {
        pair.second->stop();
    }
    mPlugins.clear();

    // Save permission store
    LLPluginPermissionStore::instance().save();
}

bool LLPluginManager::isRuntimeAvailable(LLPluginRuntimeType type)
{
    switch (type)
    {
    case LLPluginRuntimeType::LUA:
#ifdef TASIA_ENABLE_LUA
        return true;
#else
        return false;
#endif
    case LLPluginRuntimeType::JAVASCRIPT:
#ifdef TASIA_ENABLE_JAVASCRIPT
        return true;
#else
        return false;
#endif
    case LLPluginRuntimeType::PYTHON:
#ifdef TASIA_ENABLE_PYTHON
        return true;
#else
        return false;
#endif
    default:
        return false;
    }
}

void LLPluginManager::emitEvent(const std::string& event_name, const LLSD& data)
{
    // Forward viewer events to plugin event bus
    LLPluginEventBus::instance().emit(event_name, data);

    // Also forward to each plugin runtime that has event subscriptions
    for (auto& pair : mPlugins)
    {
        if (pair.second->getState() == LLPluginState::RUNNING)
        {
            pair.second->forwardEvent(event_name, data);
        }
    }
}
