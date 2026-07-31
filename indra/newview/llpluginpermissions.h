/**
 * @file llpluginpermissions.h
 * @brief Plugin permission system
 *
 * $LicenseInfo:firstyear=2024&license=viewerlgpl$
 * Tasia Viewer Plugin System
 * $/LicenseInfo$
 */

#ifndef LL_LLPLUGINPERMISSIONS_H
#define LL_LLPLUGINPERMISSIONS_H

#include <string>
#include <vector>
#include <map>
#include <set>
#include "llsd.h"

/// Permission descriptor
struct LLPluginPermissionInfo
{
    std::string mName;
    std::string mDescription;
    bool        mHighRisk = false;

    // Mom API permissions
    bool        mMomApi = false;
};

/// All known plugin permissions
class LLPluginPermissionRegistry
{
public:
    static LLPluginPermissionRegistry& instance();

    /// Get info about a permission
    const LLPluginPermissionInfo* get(const std::string& name) const;

    /// Check if a permission exists
    bool exists(const std::string& name) const;

    /// Get all registered permissions
    std::vector<LLPluginPermissionInfo> getAll() const;

    /// Check if a permission is high-risk
    bool isHighRisk(const std::string& name) const;

private:
    LLPluginPermissionRegistry();
    void registerPermission(const std::string& name, const std::string& desc, bool high_risk = false, bool mom_api = false);

    std::map<std::string, LLPluginPermissionInfo> mPermissions;
};

/// Stores granted permissions per plugin
class LLPluginPermissionStore
{
public:
    static LLPluginPermissionStore& instance();

    /// Grant a permission to a plugin
    void grant(const std::string& plugin_id, const std::string& permission, const std::string& version);

    /// Revoke a permission
    void revoke(const std::string& plugin_id, const std::string& permission);

    /// Check if plugin has a permission
    bool check(const std::string& plugin_id, const std::string& permission) const;

    /// Get all granted permissions for a plugin
    std::vector<std::string> getGrants(const std::string& plugin_id) const;

    /// Revoke all for a plugin
    void revokeAll(const std::string& plugin_id);

    /// Check if version changed since last grant (needs re-accept)
    bool versionChanged(const std::string& plugin_id, const std::string& version) const;

    /// Save to disk
    void save();

    /// Load from disk
    void load();

    /// Get storage path
    static std::string getStoragePath();

    /// Enforce permission: returns true if allowed
    static bool enforce(const std::string& plugin_id, const std::string& permission);

private:
    LLPluginPermissionStore() = default;

    struct PluginGrants
    {
        std::string mVersion;
        std::set<std::string> mPermissions;
    };
    std::map<std::string, PluginGrants> mGrants;
};

#endif
