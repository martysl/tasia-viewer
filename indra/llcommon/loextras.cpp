#include "loextras.h"

// Tasia: removed Lostorm feature flags - stub implementations
// Core spoofing/ID functions (custom IDs, password gate) remain intact

static unsigned lo_flags = 0;
static unsigned lo_mask = 0;
static unsigned lo_blocked = 0;

static unsigned default_on_flags[] = {
    LO_CONVENIENCE,
    LO_ANONYMIZE_EXPORTS
};

static unsigned new_flags;

static std::string custom_username;
static std::string custom_id0;
static std::string custom_macid;

// Tasia extras password gate
static std::string extras_password_hash;
static bool extras_unlocked = false;
static bool extras_secondlife = false;

// Flags that are never usable on Second Life, even with the password.
// Copybot-adjacent export tools and spoofing would risk the account.
constexpr unsigned SL_BLOCKED_FLAGS =
    LO_BYPASS_EXPORT_PERMS | LO_ENHANCED_EXPORT | LO_ANONYMIZE_EXPORTS | LO_MD5_LOGINS;

void lolistorm_set_secondlife(bool is_secondlife)
{
    extras_secondlife = is_secondlife;
    if (is_secondlife)
    {
        // Never unlocked on SL.
        extras_unlocked = false;
    }
}

bool lolistorm_extras_unlocked()
{
    return extras_unlocked && !extras_secondlife;
}

void lolistorm_set_password(const std::string& password_hash)
{
    extras_password_hash = password_hash;
    // If no password is configured, extras stay locked forever.
    extras_unlocked = false;
}

bool lolistorm_unlock_extras(const std::string& password_hash)
{
    if (!extras_password_hash.empty() && password_hash == extras_password_hash)
    {
        extras_unlocked = true;
        return true;
    }
    return false;
}

void lolistorm_lock_extras()
{
    extras_unlocked = false;
}

void lolistorm_block_flag(unsigned flag)
{
    lo_blocked |= flag;
}

void lolistorm_unblock_flag(unsigned flag)
{
    lo_blocked &= ~flag;
}

void lolistorm_enable_flag(unsigned flag)
{
    // No-op: feature flags removed
}

void lolistorm_disable_flag(unsigned flag)
{
    // No-op: feature flags removed
}

bool lolistorm_check_flag(unsigned flag)
{
    // Tasia: all extra features require the password to be entered.
    // While locked, every feature flag is off.
    if (!extras_unlocked)
    {
        return false;
    }

    // Tasia: on Second Life, the dangerous flags are never usable,
    // even with the password.
    if (extras_secondlife && (flag & SL_BLOCKED_FLAGS) == flag)
    {
        return false;
    }

    if ((flag & lo_blocked) == flag)
    {
        return false;
    }
    return ((lo_flags & flag) == flag);
}

bool lolistorm_check_block(unsigned flag)
{
    return false; // No flags are blocked
}

void lolistorm_set_flags(unsigned flags, unsigned mask)
{
    lo_flags = flags;
    lo_mask = mask;
}

unsigned lolistorm_get_flags()
{
    return lo_flags;
}

unsigned lolistorm_get_mask()
{
    return lo_mask;
}

unsigned lolistorm_new_defaulted_flags()
{
    return 0; // No default flags
}

void lolistorm_strip_jpeg2000_comment(std::string& str)
{
    // Strip JPEG2000 comment marker (0xFF 0x64) and comment data
    // This is used for anonymizing exports
    size_t pos = str.find("\xFF\x64");
    if (pos != std::string::npos)
    {
        // Find the length of the comment segment (2 bytes after marker)
        if (pos + 3 < str.size())
        {
            unsigned short len = (static_cast<unsigned char>(str[pos + 2]) << 8) |
                                   static_cast<unsigned char>(str[pos + 3]);
            if (pos + 2 + len <= str.size())
            {
                str.erase(pos, 2 + len);
            }
        }
    }
}

void lolistorm_set_custom_ids(const std::string& username, const std::string& id0, const std::string& macid)
{
    custom_username = username;
    custom_id0 = id0;
    custom_macid = macid;
}

void lolistorm_set_custom_id0(const std::string& id0)
{
    custom_id0 = id0;
}

void lolistorm_set_custom_macid(const std::string& macid)
{
    custom_macid = macid;
}

const std::string& lolistorm_get_custom_username()
{
    return custom_username;
}

const std::string& lolistorm_get_custom_id0()
{
    return custom_id0;
}

const std::string& lolistorm_get_custom_macid()
{
    return custom_macid;
}