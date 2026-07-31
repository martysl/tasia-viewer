/**
 * @file llpluginstorage.cpp
 * @brief Plugin-local key-value storage implementation
 *
 * $LicenseInfo:firstyear=2024&license=viewerlgpl$
 * Tasia Viewer Plugin System
 * $/LicenseInfo$
 */

#include "llviewerprecompiledheaders.h"
#include "llpluginstorage.h"

#include "llsd.h"
#include "llsdserialize.h"
#include "llfile.h"
#include "llpath.h"
#include "llstring.h"
#include <boost/filesystem.hpp>

namespace fs = boost::filesystem;

LLPluginStorage::LLPluginStorage(const std::string& plugin_id)
    : mPluginId(plugin_id)
{
}

std::string LLPluginStorage::getStoragePath() const
{
    std::string plugin_dir = gDirUtilp->getExpandedFilename(LL_PATH_PER_SL_ACCOUNT, "plugins", "data");
    // Sanitize plugin ID for filename
    std::string safe_id = mPluginId;
    for (char& c : safe_id)
    {
        if (c == '.' || c == '-') c = '_';
    }
    return plugin_dir + "/" + safe_id + ".xml";
}

LLSD LLPluginStorage::get(const std::string& key, const LLSD& default_val) const
{
    auto it = mData.find(key);
    if (it == mData.end())
        return default_val;
    return it->second;
}

void LLPluginStorage::set(const std::string& key, const LLSD& value)
{
    mData[key] = value;
    mDirty = true;
}

bool LLPluginStorage::del(const std::string& key)
{
    auto it = mData.find(key);
    if (it == mData.end())
        return false;
    mData.erase(it);
    mDirty = true;
    return true;
}

bool LLPluginStorage::has(const std::string& key) const
{
    return mData.find(key) != mData.end();
}

void LLPluginStorage::clear()
{
    mData.clear();
    mDirty = true;
}

void LLPluginStorage::save()
{
    if (!mDirty) return;

    LLSD data;
    for (const auto& pair : mData)
    {
        data[pair.first] = pair.second;
    }

    std::string path = getStoragePath();
    // Ensure directory exists
    fs::path dir = fs::path(path).parent_path();
    if (!fs::exists(dir))
    {
        fs::create_directories(dir);
    }

    llofstream file(path);
    if (file.is_open())
    {
        LLSDSerialize::toXML(data, file);
        mDirty = false;
    }
}

void LLPluginStorage::load()
{
    std::string path = getStoragePath();
    if (!fs::exists(path))
        return;

    LLSD data;
    llifstream file(path);
    if (!file.is_open())
        return;

    LLSDSerialize::fromXML(data, file);

    mData.clear();
    for (const auto& pair : data)
    {
        mData[pair.first] = pair.second;
    }
}
