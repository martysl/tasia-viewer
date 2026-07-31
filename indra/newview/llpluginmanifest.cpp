/**
 * @file llpluginmanifest.cpp
 * @brief Plugin manifest parsing and validation implementation
 *
 * $LicenseInfo:firstyear=2024&license=viewerlgpl$
 * Tasia Viewer Plugin System
 * $/LicenseInfo$
 */

#include "llviewerprecompiledheaders.h"
#include "llpluginmanifest.h"

#include "llsd.h"
#include "llsdserialize.h"
#include "llfile.h"
#include "llpath.h"
#include "llstring.h"
#include <boost/filesystem.hpp>
#include <regex>

namespace fs = boost::filesystem;

LLPluginManifest::LLPluginManifest()
{
}

bool LLPluginManifest::parse(const std::string& path)
{
    fs::path manifest_path(path);
    if (!fs::exists(manifest_path))
    {
        mError = "Manifest file not found: " + path;
        return false;
    }

    // Read the file
    LLSD data;
    llifstream infile(path);
    if (!infile.is_open())
    {
        mError = "Cannot open manifest: " + path;
        return false;
    }

    LLSDSerialize::fromNotation(data, infile, LLPLUGIN_MANIFEST_MAX_BYTES);
    if (data.isUndefined())
    {
        // Try JSON
        infile.clear();
        infile.seekg(0);
        std::string json((std::istreambuf_iterator<char>(infile)),
                          std::istreambuf_iterator<char>());
        return parseFromString(json, manifest_path.parent_path().string());
    }

    mBasePath = manifest_path.parent_path().string();
    return parseFromString(data, mBasePath);
}

bool LLPluginManifest::parseFromString(const std::string& json, const std::string& base_path)
{
    LLSD data;
    std::istringstream stream(json);
    if (LLSDSerialize::fromJSON(data, stream, LLPLUGIN_MANIFEST_MAX_BYTES) == LLSDParser::PARSE_FAILURE)
    {
        mError = "Invalid JSON in manifest";
        return false;
    }
    return parseFromString(data, base_path);
}

bool LLPluginManifest::parseFromString(const LLSD& data, const std::string& base_path)
{
    mBasePath = base_path;

    // Extract required fields
    mId = data["id"].asString();
    mName = data["name"].asString();
    mVersion = data["version"].asString();
    mAuthor = data["author"].asString();
    mDescription = data["description"].asString();
    mEntrypoint = data["entrypoint"].asString();
    mRuntime = string_to_runtime_type(data["runtime"].asString());
    mApiVersion = data["api_version"].asInteger();

    // Parse permissions
    mPermissions.clear();
    if (data["permissions"].isArray())
    {
        for (U32 i = 0; i < data["permissions"].size(); ++i)
        {
            LLPluginPermission perm;
            perm.mName = data["permissions"][i].asString();
            mPermissions.push_back(perm);
        }
    }

    mValid = true;
    mError = validate();
    if (!mError.empty())
    {
        mValid = false;
    }

    return mValid;
}

std::string LLPluginManifest::validate() const
{
    // Must have ID
    if (mId.empty())
        return "Missing plugin ID";
    if (!isValidPluginId(mId))
        return "Invalid plugin ID format: " + mId;

    // Must have name
    if (mName.empty())
        return "Missing plugin name for: " + mId;

    // Must have version
    if (mVersion.empty())
        return "Missing version for: " + mId;
    if (!isValidVersion(mVersion))
        return "Invalid version format for: " + mId;

    // Must have entrypoint
    if (mEntrypoint.empty())
        return "Missing entrypoint for: " + mId;

    // Validate entrypoint path is safe
    if (!isPathSafe(mBasePath, mEntrypoint))
        return "Entrypoint path traversal detected for: " + mId;

    // Must have valid runtime
    if (mRuntime == LLPluginRuntimeType::NONE)
        return "Invalid or missing runtime for: " + mId;

    // Must support current API version
    if (mApiVersion < 1 || mApiVersion > LLPLUGIN_API_VERSION)
        return "Unsupported API version " + std::to_string(mApiVersion) + " for: " + mId;

    return ""; // valid
}

bool LLPluginManifest::isPathSafe(const std::string& base_path, const std::string& requested_path)
{
    if (base_path.empty() || requested_path.empty())
        return false;

    // Reject absolute paths
    fs::path req(requested_path);
    if (req.is_absolute())
        return false;

    // Reject path traversal components
    std::string normalized = req.string();
    if (normalized.find("..") != std::string::npos)
        return false;

    // Resolve to check it's actually within base path
    fs::path base(base_path);
    fs::path full = base / req;
    try
    {
        full = fs::canonical(full);
        base = fs::canonical(base);
    }
    catch (const fs::filesystem_error&)
    {
        // If the file doesn't exist yet, check without canonical
        std::string full_str = (base / req).string();
        std::string base_str = base.string();
        return full_str.compare(0, base_str.length(), base_str) == 0;
    }

    // Check the resolved path starts with base path
    std::string full_str = full.string();
    std::string base_str = base.string();
    return full_str.compare(0, base_str.length(), base_str) == 0;
}

bool LLPluginManifest::isValidPluginId(const std::string& id)
{
    // Must be reverse domain notation: com.example.plugin
    static const std::regex id_regex(R"(^[a-zA-Z][a-zA-Z0-9]*(\.[a-zA-Z][a-zA-Z0-9]*)+$)");
    return std::regex_match(id, id_regex);
}

bool LLPluginManifest::isValidVersion(const std::string& version)
{
    // Must be semver: x.y.z
    static const std::regex version_regex(R"(^\d+\.\d+\.\d+$)");
    return std::regex_match(version, version_regex);
}

// ---- LLPluginManifestDB ----

LLPluginManifestDB& LLPluginManifestDB::instance()
{
    static LLPluginManifestDB sInstance;
    return sInstance;
}

bool LLPluginManifestDB::add(const LLPluginManifest& manifest)
{
    if (mManifests.find(manifest.getId()) != mManifests.end())
    {
        return false; // duplicate
    }
    mManifests[manifest.getId()] = manifest;
    return true;
}

bool LLPluginManifestDB::remove(const std::string& plugin_id)
{
    auto it = mManifests.find(plugin_id);
    if (it == mManifests.end())
        return false;
    mManifests.erase(it);
    return true;
}

LLPluginManifest* LLPluginManifestDB::get(const std::string& plugin_id)
{
    auto it = mManifests.find(plugin_id);
    if (it == mManifests.end())
        return nullptr;
    return &it->second;
}

bool LLPluginManifestDB::hasId(const std::string& plugin_id) const
{
    return mManifests.find(plugin_id) != mManifests.end();
}

std::vector<LLPluginManifest> LLPluginManifestDB::getAll() const
{
    std::vector<LLPluginManifest> result;
    for (const auto& pair : mManifests)
    {
        result.push_back(pair.second);
    }
    return result;
}

void LLPluginManifestDB::clear()
{
    mManifests.clear();
}
