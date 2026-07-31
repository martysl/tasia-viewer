/**
 * @file llpluginmanifest.h
 * @brief Plugin manifest parsing and validation
 *
 * $LicenseInfo:firstyear=2024&license=viewerlgpl$
 * Tasia Viewer Plugin System
 * $/LicenseInfo$
 */

#ifndef LL_LLPLUGINMANIFEST_H
#define LL_LLPLUGINMANIFEST_H

#include <string>
#include <vector>
#include <map>
#include <set>

#include "llsd.h"

/// Supported plugin runtimes
enum class LLPluginRuntimeType
{
    NONE,
    LUA,
    JAVASCRIPT,
    PYTHON,
    NATIVE
};

inline std::string runtime_type_to_string(LLPluginRuntimeType t)
{
    switch (t)
    {
    case LLPluginRuntimeType::LUA:        return "lua";
    case LLPluginRuntimeType::JAVASCRIPT: return "javascript";
    case LLPluginRuntimeType::PYTHON:     return "python";
    case LLPluginRuntimeType::NATIVE:     return "native";
    default:                              return "";
    }
}

inline LLPluginRuntimeType string_to_runtime_type(const std::string& s)
{
    if (s == "lua")        return LLPluginRuntimeType::LUA;
    if (s == "javascript") return LLPluginRuntimeType::JAVASCRIPT;
    if (s == "python")     return LLPluginRuntimeType::PYTHON;
    if (s == "native")     return LLPluginRuntimeType::NATIVE;
    return LLPluginRuntimeType::NONE;
}

/// Current protocol API version
constexpr S32 LLPLUGIN_API_VERSION = 1;

/// Maximum manifest size
constexpr S32 LLPLUGIN_MANIFEST_MAX_BYTES = 1024 * 64; // 64KB

/// Plugin permissions as defined in manifest
struct LLPluginPermission
{
    std::string mName;

    bool operator<(const LLPluginPermission& other) const { return mName < other.mName; }
    bool operator==(const LLPluginPermission& other) const { return mName == other.mName; }
};

/// A parsed plugin manifest
class LLPluginManifest
{
public:
    LLPluginManifest();

    /// Parse a manifest.json file from disk
    /// @returns true if valid and parsed
    bool parse(const std::string& path);

    /// Parse from raw JSON string
    bool parseFromString(const std::string& json, const std::string& base_path);

    /// Validate the parsed manifest
    /// @returns human-readable error or empty string
    std::string validate() const;

    // Accessors
    const std::string&     getId()          const { return mId; }
    const std::string&     getName()        const { return mName; }
    const std::string&     getVersion()     const { return mVersion; }
    const std::string&     getAuthor()      const { return mAuthor; }
    const std::string&     getDescription() const { return mDescription; }
    const std::string&     getEntrypoint()  const { return mEntrypoint; }
    LLPluginRuntimeType    getRuntime()     const { return mRuntime; }
    S32                    getApiVersion()  const { return mApiVersion; }
    const std::vector<LLPluginPermission>& getPermissions() const { return mPermissions; }
    const std::string&     getBasePath()    const { return mBasePath; }
    bool                   isValid()        const { return mValid; }
    const std::string&     getError()       const { return mError; }

    /// Check if path is within the plugin directory (prevent traversal)
    static bool isPathSafe(const std::string& base_path, const std::string& requested_path);

    /// Validate plugin ID format
    static bool isValidPluginId(const std::string& id);

    /// Validate version string
    static bool isValidVersion(const std::string& version);

private:
    std::string     mId;
    std::string     mName;
    std::string     mVersion;
    std::string     mAuthor;
    std::string     mDescription;
    std::string     mEntrypoint;
    LLPluginRuntimeType mRuntime = LLPluginRuntimeType::NONE;
    S32             mApiVersion = 0;
    std::vector<LLPluginPermission> mPermissions;
    std::string     mBasePath;
    bool            mValid = false;
    std::string     mError;
};

/// Manager tracking all known manifests
class LLPluginManifestDB
{
public:
    static LLPluginManifestDB& instance();

    /// Register a manifest
    bool add(const LLPluginManifest& manifest);

    /// Remove by plugin ID
    bool remove(const std::string& plugin_id);

    /// Get manifest by ID
    LLPluginManifest* get(const std::string& plugin_id);

    /// Check for duplicate IDs
    bool hasId(const std::string& plugin_id) const;

    /// Get all manifests
    std::vector<LLPluginManifest> getAll() const;

    /// Clear all
    void clear();

private:
    LLPluginManifestDB() = default;
    std::map<std::string, LLPluginManifest> mManifests;
};

#endif // LL_LLPLUGINMANIFEST_H
