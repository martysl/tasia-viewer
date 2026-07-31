/**
 * @file llpluginpermissions.cpp
 * @brief Plugin permission system implementation
 *
 * $LicenseInfo:firstyear=2024&license=viewerlgpl$
 * Tasia Viewer Plugin System
 * $/LicenseInfo$
 */

#include "llviewerprecompiledheaders.h"
#include "llpluginpermissions.h"

#include "llsd.h"
#include "llsdserialize.h"
#include "llfile.h"
#include "llpath.h"
#include "llstring.h"
#include "llviewercontrol.h"
#include <boost/filesystem.hpp>

namespace fs = boost::filesystem;

// ---- LLPluginPermissionRegistry ----

LLPluginPermissionRegistry& LLPluginPermissionRegistry::instance()
{
    static LLPluginPermissionRegistry sInstance;
    return sInstance;
}

LLPluginPermissionRegistry::LLPluginPermissionRegistry()
{
    // Viewer permissions
    registerPermission("viewer.notify",      "Show notifications in the viewer");
    registerPermission("viewer.open_floater", "Open viewer windows/floaters");

    // Chat permissions
    registerPermission("chat.read",  "Read public chat messages");
    registerPermission("chat.send",  "Send public chat messages");

    // IM permissions
    registerPermission("im.read",  "Read private IM messages", true);
    registerPermission("im.send",  "Send private IM messages", true);

    // Avatar permissions
    registerPermission("avatar.read",    "Read avatar information");
    registerPermission("avatar.nearby",  "Track nearby avatars");

    // Inventory permissions
    registerPermission("inventory.read",   "Read inventory");
    registerPermission("inventory.modify", "Modify inventory", true);

    // Camera permissions
    registerPermission("camera.read",    "Read camera state");
    registerPermission("camera.control", "Control camera", true);

    // World permissions
    registerPermission("world.read",      "Read world/region info");
    registerPermission("world.teleport",  "Teleport to other locations", true);

    // Network
    registerPermission("network.http",    "Make HTTP requests", true);

    // Storage
    registerPermission("storage.plugin",  "Access plugin-local storage");

    // UI
    registerPermission("ui.create",       "Create UI elements", true);

    // Clipboard
    registerPermission("clipboard.read",  "Read clipboard contents", true);
    registerPermission("clipboard.write", "Write to clipboard");

    // Filesystem
    registerPermission("filesystem.plugin",    "Access plugin directory files");
    registerPermission("filesystem.external",  "Access files outside plugin directory", true);

    // System
    registerPermission("native.execute",   "Execute native code/DLLs", true);
    registerPermission("process.execute",  "Execute external processes", true);
    registerPermission("microphone",       "Access microphone", true);

    // Mom API
    registerPermission("mom.read",   "Read Mom avatar status");
    registerPermission("mom.events", "Subscribe to Mom avatar events");
}

void LLPluginPermissionRegistry::registerPermission(const std::string& name, const std::string& desc, bool high_risk, bool mom_api)
{
    LLPluginPermissionInfo info;
    info.mName = name;
    info.mDescription = desc;
    info.mHighRisk = high_risk;
    info.mMomApi = mom_api;
    mPermissions[name] = info;
}

const LLPluginPermissionInfo* LLPluginPermissionRegistry::get(const std::string& name) const
{
    auto it = mPermissions.find(name);
    if (it == mPermissions.end())
        return nullptr;
    return &it->second;
}

bool LLPluginPermissionRegistry::exists(const std::string& name) const
{
    return mPermissions.find(name) != mPermissions.end();
}

std::vector<LLPluginPermissionInfo> LLPluginPermissionRegistry::getAll() const
{
    std::vector<LLPluginPermissionInfo> result;
    for (const auto& pair : mPermissions)
        result.push_back(pair.second);
    return result;
}

bool LLPluginPermissionRegistry::isHighRisk(const std::string& name) const
{
    auto it = mPermissions.find(name);
    if (it == mPermissions.end())
        return false;
    return it->second.mHighRisk;
}

// ---- LLPluginPermissionStore ----

LLPluginPermissionStore& LLPluginPermissionStore::instance()
{
    static LLPluginPermissionStore sInstance;
    return sInstance;
}

void LLPluginPermissionStore::grant(const std::string& plugin_id, const std::string& permission, const std::string& version)
{
    mGrants[plugin_id].mVersion = version;
    mGrants[plugin_id].mPermissions.insert(permission);
}

void LLPluginPermissionStore::revoke(const std::string& plugin_id, const std::string& permission)
{
    auto it = mGrants.find(plugin_id);
    if (it != mGrants.end())
    {
        it->second.mPermissions.erase(permission);
    }
}

bool LLPluginPermissionStore::check(const std::string& plugin_id, const std::string& permission) const
{
    auto it = mGrants.find(plugin_id);
    if (it == mGrants.end())
        return false;
    return it->second.mPermissions.find(permission) != it->second.mPermissions.end();
}

std::vector<std::string> LLPluginPermissionStore::getGrants(const std::string& plugin_id) const
{
    std::vector<std::string> result;
    auto it = mGrants.find(plugin_id);
    if (it != mGrants.end())
    {
        for (const auto& perm : it->second.mPermissions)
            result.push_back(perm);
    }
    return result;
}

void LLPluginPermissionStore::revokeAll(const std::string& plugin_id)
{
    mGrants.erase(plugin_id);
}

bool LLPluginPermissionStore::versionChanged(const std::string& plugin_id, const std::string& version) const
{
    auto it = mGrants.find(plugin_id);
    if (it == mGrants.end())
        return true; // not granted yet = changed
    return it->second.mVersion != version;
}

std::string LLPluginPermissionStore::getStoragePath()
{
    return gDirUtilp->getExpandedFilename(LL_PATH_PER_SL_ACCOUNT, "plugin_permissions.xml");
}

void LLPluginPermissionStore::save()
{
    LLSD data;
    for (const auto& plugin : mGrants)
    {
        LLSD plugin_data;
        plugin_data["version"] = plugin.second.mVersion;
        for (const auto& perm : plugin.second.mPermissions)
        {
            plugin_data["permissions"].append(perm);
        }
        data[plugin.first] = plugin_data;
    }

    std::string path = getStoragePath();
    llofstream file(path);
    if (file.is_open())
    {
        LLSDSerialize::toXML(data, file);
    }
}

void LLPluginPermissionStore::load()
{
    std::string path = getStoragePath();
    if (!fs::exists(path))
        return;

    LLSD data;
    llifstream file(path);
    if (!file.is_open())
        return;

    LLSDSerialize::fromXML(data, file);

    mGrants.clear();
    for (const auto& plugin_pair : data)
    {
        PluginGrants grants;
        grants.mVersion = plugin_pair.second["version"].asString();
        for (U32 i = 0; i < plugin_pair.second["permissions"].size(); ++i)
        {
            grants.mPermissions.insert(plugin_pair.second["permissions"][i].asString());
        }
        mGrants[plugin_pair.first] = grants;
    }
}

bool LLPluginPermissionStore::enforce(const std::string& plugin_id, const std::string& permission)
{
    return instance().check(plugin_id, permission);
}
