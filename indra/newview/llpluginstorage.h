/**
 * @file llpluginstorage.h
 * @brief Plugin-local key-value storage
 *
 * $LicenseInfo:firstyear=2024&license=viewerlgpl$
 * Tasia Viewer Plugin System
 * $/LicenseInfo$
 */

#ifndef LL_LLPLUGINSTORAGE_H
#define LL_LLPLUGINSTORAGE_H

#include <string>
#include <map>
#include "llsd.h"

/// Per-plugin isolated storage (key-value, persists to disk)
class LLPluginStorage
{
public:
    explicit LLPluginStorage(const std::string& plugin_id);

    /// Get a value
    LLSD get(const std::string& key, const LLSD& default_val = LLSD()) const;

    /// Set a value
    void set(const std::string& key, const LLSD& value);

    /// Delete a key
    bool del(const std::string& key);

    /// Check if key exists
    bool has(const std::string& key) const;

    /// Clear all storage for this plugin
    void clear();

    /// Save to disk
    void save();

    /// Load from disk
    void load();

    /// Get storage path for this plugin
    std::string getStoragePath() const;

    /// Max storage per plugin (1MB)
    static constexpr S32 MAX_STORAGE_BYTES = 1024 * 1024;

private:
    std::string mPluginId;
    std::map<std::string, LLSD> mData;
    bool mDirty = false;
};

#endif
