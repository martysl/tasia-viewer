/**
 * @file llplugineventbus.cpp
 * @brief Plugin event subscription and dispatch implementation
 *
 * $LicenseInfo:firstyear=2024&license=viewerlgpl$
 * Tasia Viewer Plugin System
 * $/LicenseInfo$
 */

#include "llviewerprecompiledheaders.h"
#include "llplugineventbus.h"

#include "lltimer.h"
#include "llsd.h"

const std::string LLPluginEventBus::EVENT_VIEWER_STARTED     = "viewer.started";
const std::string LLPluginEventBus::EVENT_VIEWER_SHUTDOWN    = "viewer.shutdown";
const std::string LLPluginEventBus::EVENT_LOGIN_STARTED      = "login.started";
const std::string LLPluginEventBus::EVENT_LOGIN_COMPLETED    = "login.completed";
const std::string LLPluginEventBus::EVENT_REGION_CHANGED     = "region.changed";
const std::string LLPluginEventBus::EVENT_AVATAR_NEARBY      = "avatar.nearby";
const std::string LLPluginEventBus::EVENT_AVATAR_LEFT        = "avatar.left";
const std::string LLPluginEventBus::EVENT_CHAT_RECEIVED      = "chat.received";
const std::string LLPluginEventBus::EVENT_CHAT_SENT          = "chat.sent";
const std::string LLPluginEventBus::EVENT_IM_RECEIVED        = "im.received";
const std::string LLPluginEventBus::EVENT_INVENTORY_CHANGED  = "inventory.changed";
const std::string LLPluginEventBus::EVENT_OBJECT_SELECTED    = "object.selected";
const std::string LLPluginEventBus::EVENT_OBJECT_TOUCHED     = "object.touched";
const std::string LLPluginEventBus::EVENT_TELEPORT_STARTED   = "teleport.started";
const std::string LLPluginEventBus::EVENT_TELEPORT_COMPLETED = "teleport.completed";

const std::string LLPluginEventBus::EVENT_MOM_ONLINE          = "mom.online";
const std::string LLPluginEventBus::EVENT_MOM_OFFLINE         = "mom.offline";
const std::string LLPluginEventBus::EVENT_MOM_ENTERED_SIM     = "mom.entered_sim";
const std::string LLPluginEventBus::EVENT_MOM_LEFT_SIM        = "mom.left_sim";
const std::string LLPluginEventBus::EVENT_MOM_DISTANCE_CHANGED = "mom.distance_changed";

LLPluginEventBus& LLPluginEventBus::instance()
{
    static LLPluginEventBus sInstance;
    return sInstance;
}

S32 LLPluginEventBus::subscribe(const std::string& plugin_id, const std::string& event_name,
                                std::function<void(const LLSD&)> callback)
{
    SubscriptionEntry entry;
    entry.mSubscription.mPluginId = plugin_id;
    entry.mSubscription.mEventName = event_name;
    entry.mSubscription.mSubscriptionId = mNextId++;
    entry.mSubscription.mCallback = callback;

    mSubscriptions[event_name].push_back(entry);
    return entry.mSubscription.mSubscriptionId;
}

bool LLPluginEventBus::unsubscribe(const std::string& plugin_id, S32 subscription_id)
{
    for (auto& event_pair : mSubscriptions)
    {
        auto& subs = event_pair.second;
        for (auto it = subs.begin(); it != subs.end(); ++it)
        {
            if (it->mSubscription.mSubscriptionId == subscription_id &&
                it->mSubscription.mPluginId == plugin_id)
            {
                subs.erase(it);
                return true;
            }
        }
    }
    return false;
}

void LLPluginEventBus::unsubscribeAll(const std::string& plugin_id)
{
    for (auto& event_pair : mSubscriptions)
    {
        auto& subs = event_pair.second;
        subs.erase(std::remove_if(subs.begin(), subs.end(),
            [&](const SubscriptionEntry& entry) {
                return entry.mSubscription.mPluginId == plugin_id;
            }), subs.end());
    }
}

void LLPluginEventBus::emit(const std::string& event_name, const LLSD& data)
{
    auto it = mSubscriptions.find(event_name);
    if (it == mSubscriptions.end())
        return;

    // Copy the list in case callbacks modify subscriptions
    auto subs_copy = it->second;

    for (const auto& entry : subs_copy)
    {
        if (!checkRateLimit(entry.mSubscription.mPluginId))
        {
            LL_WARNS("Plugins") << "Event rate limit hit for plugin: "
                                << entry.mSubscription.mPluginId << LL_ENDL;
            continue;
        }

        if (entry.mSubscription.mCallback)
        {
            entry.mSubscription.mCallback(data);
        }
    }
}

bool LLPluginEventBus::isSubscribed(const std::string& plugin_id, const std::string& event_name) const
{
    auto it = mSubscriptions.find(event_name);
    if (it == mSubscriptions.end())
        return false;

    for (const auto& entry : it->second)
    {
        if (entry.mSubscription.mPluginId == plugin_id)
            return true;
    }
    return false;
}

std::vector<std::string> LLPluginEventBus::getSubscriptions(const std::string& plugin_id) const
{
    std::vector<std::string> result;
    for (const auto& event_pair : mSubscriptions)
    {
        for (const auto& entry : event_pair.second)
        {
            if (entry.mSubscription.mPluginId == plugin_id)
            {
                result.push_back(event_pair.first);
                break;
            }
        }
    }
    return result;
}

bool LLPluginEventBus::checkRateLimit(const std::string& plugin_id)
{
    U64 now = LLTimer::getTotalSeconds();
    auto& timestamps = mRateLimitTracker[plugin_id];

    // Remove timestamps older than 1 second
    timestamps.erase(std::remove_if(timestamps.begin(), timestamps.end(),
        [now](U64 ts) { return now - ts > 1; }), timestamps.end());

    if (timestamps.size() >= (U32)LLPLUGIN_EVENT_RATE_LIMIT)
        return false;

    timestamps.push_back(now);
    return true;
}
