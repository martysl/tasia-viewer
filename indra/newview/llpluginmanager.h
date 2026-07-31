/**
 * @file llpluginmanager.h
 * @brief Plugin manager - discovery, lifecycle, permissions, UI integration
 *
 * $LicenseInfo:firstyear=2024&license=viewerlgpl$
 * Tasia Viewer Plugin System
 * $/LicenseInfo$
 */

#ifndef LL_LLPLUGINMANAGER_H
#define LL_LLPLUGINMANAGER_H

#include <string>
#include <vector>
#include <map>
#include <memory>
#include "llsd.h"
#include "llpluginmanifest.h"
#include "llpluginruntime.h"
#include "llpluginpermissions.h"
#include "llpluginprotocol.h"

/// Plugin information for UI display
struct LLPluginInfo
{
    std::string mId;
    std::string mName;
    std::string mVersion;
    std::string mAuthor;
    std::string mDescription;
    LLPluginRuntimeType mRuntime = LLPluginRuntimeType::NONE;
    LLPluginState mState = LLPluginState::UNLOADED;
    bool mEnabled = false;
    std::string mError;
    bool mHasPermissions = false;
};

/// Central plugin manager
class LLPluginManager
{
public:
    static LLPluginManager& instance();

    /// Initialize: discover, load permission store, register menu
    void init();

    /// Discover plugins in plugin directories
    void discoverPlugins();

    /// Load a plugin by ID
    bool loadPlugin(const std::string& plugin_id);

    /// Unload a plugin by ID
    void unloadPlugin(const std::string& plugin_id);

    /// Enable a plugin
    bool enablePlugin(const std::string& plugin_id);

    /// Disable a plugin
    void disablePlugin(const std::string& plugin_id);

    /// Reload a plugin
    bool reloadPlugin(const std::string& plugin_id);

    /// Remove a plugin
    bool removePlugin(const std::string& plugin_id, bool delete_files = false);

    /// Get plugin info for UI
    LLPluginInfo getPluginInfo(const std::string& plugin_id) const;

    /// Get all plugin infos
    std::vector<LLPluginInfo> getAllPluginInfos() const;

    /// Get runtime for a plugin
    LLPluginRuntime* getRuntime(const std::string& plugin_id);

    /// Dispatch an API call
    LLPluginMessage dispatch(const std::string& plugin_id, const LLPluginMessage& request);

    /// Check if any plugin crashed during last startup
    bool hasCrashedPlugins() const;

    /// Get crash list
    std::vector<std::string> getCrashedPluginIds() const;

    /// Safe mode: disable all user plugins
    void setSafeMode(bool enabled);
    bool isSafeMode() const { return mSafeMode; }
    bool arePluginsDisabled() const { return mPluginsDisabled; }

    /// Handle failed startup
    void recordCrash(const std::string& plugin_id);

    /// Get plugin directories
    static std::vector<std::string> getPluginDirectories();

    /// Open plugin folder in file browser
    static void openPluginFolder();

    /// Update all running plugins (called per frame)
    void updateAll();

    /// Shut down all plugins
    void shutdownAll();

    /// Check if a runtime type is available
    static bool isRuntimeAvailable(LLPluginRuntimeType type);

private:
    LLPluginManager() = default;

    /// Create appropriate runtime for a manifest
    LLPluginRuntime* createRuntime(const LLPluginManifest& manifest);

    /// Emit a viewer event to subscribed plugins
    void emitEvent(const std::string& event_name, const LLSD& data);

    /// Plugin storage (key: plugin_id)
    std::map<std::string, std::unique_ptr<LLPluginRuntime>> mPlugins;
    std::vector<LLPluginManifest> mDiscoveredManifests;

    bool mSafeMode = false;
    bool mPluginsDisabled = false;

    // Crashed plugin tracking
    std::vector<std::string> mCrashedPlugins;
    static constexpr const char* CRASH_MARKER_FILE = "plugin_crash_marker";
};

#endif
