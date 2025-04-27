
#include "loextras.h"

static unsigned lo_flags = LO_FEATURE_MASK;
static unsigned lo_mask = 0;
static unsigned lo_blocked = LO_ENHANCED_EXPORT | LO_BYPASS_EXPORT_PERMS;

static unsigned default_on_flags[] = {
    LO_CONVENIENCE,
    LO_ANONYMIZE_EXPORTS
};

static unsigned new_flags;

static std::string custom_username;
static std::string custom_id0;
static std::string custom_macid;

void lolistorm_block_flag(unsigned flag)
{
    lo_blocked |= flag;
}

void lolistorm_unblock_flag(unsigned flag)
{
    lo_blocked &= ~flag;
}

void lolistorm_set_flags(unsigned flags, unsigned mask)
{
    for (unsigned x : default_on_flags)
    {
        if (!(mask & x))
        {
            flags |= x;
            new_flags |= x;
        }
    }

    lo_flags = flags;
    lo_mask = mask | LO_FEATURE_MASK;
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
    return new_flags;
}

void lolistorm_enable_flag(unsigned flag)
{
    lo_flags |= flag;
}

void lolistorm_disable_flag(unsigned flag)
{
    lo_flags &= ~flag;
}

bool lolistorm_check_flag(unsigned flag)
{
    if ((flag & lo_blocked) == flag)
    {
        return false;
    }
    return ((lo_flags & flag) == flag);
}

bool lolistorm_check_block(unsigned flag)
{
    return ((lo_blocked & flag) == flag);
}

void lolistorm_strip_jpeg2000_comment(std::string& str)
{
    const unsigned char* buf = (const unsigned char*)str.data();
    int len = (int)str.size();
    bool in_header = false;

    if (len < 6)
        return;

    for (int i = 0; i < len - 3; ++i)
    {
        if (buf[i] == 0xff)
        {
            if (buf[i+1] == 0x4f)
            {
                in_header = true;
            }
            else if (buf[i+1] == 0x90 || buf[i+1] == 0xd9)
            {
                return;
            }
            else if (in_header && buf[i+1] == 0x64)
            {
                int comment_len = (buf[i + 2] << 8) | buf[i + 3];

                if (comment_len > len - i)
                    comment_len = len - i;

                auto it = str.begin() + i;
                str.erase(it, it + 2 + comment_len);

                return;
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
