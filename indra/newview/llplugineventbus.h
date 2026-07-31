/**
 * @file llplugineventbus.h
 * @brief Plugin event subscription and dispatch system
 *
 * $LicenseInfo:firstyear=2024&license=viewerlgpl$
 * Tasia Viewer Plugin System
 * $/LicenseInfo$
 */

#ifndef LL_LLPLUGINEVENTBUS_H
#define LL_LLPLUGINEVENTBUS_H

#include <string>
#include <vector>
#include <map>
#include <set>
#include <functional>
#include <memory>
#include "llsd.h"

/// Maximum events per second per plugin
constexpr S32 LLPLUGIN_EVENT_RATE_LIMIT = 60;

/// Maximum subscriber call time in ms
constexpr S32 LLPLUGIN_EVENT_CALLBACK_TIME_LIMIT_MS = 500;

/// A single event subscription
struct LLPluginEventSubscription
{
    std::string mPluginId;
    std::string mEventName;
    S32         mSubscriptionId;

    // Callback function (called by runtime host to dispatch to plugin script)
    std::function<void(const LLSD& event_data)> mCallback;
};

/// Central event bus for plugin system events and internal viewer events
class LLPluginEventBus
{
public:
    static LLPluginEventBus& instance();

    /// Subscribe a plugin to an event
    S32 subscribe(const std::string& plugin_id, const std::string& event_name,
                  std::function<void(const LLSD&)> callback);

    /// Unsubscribe by ID
    bool unsubscribe(const std::string& plugin_id, S32 subscription_id);

    /// Unsubscribe all subscriptions for a plugin
    void unsubscribeAll(const std::string& plugin_id);

    /// Emit an event to all subscribers
    void emit(const std::string& event_name, const LLSD& data);

    /// Check if a plugin is subscribed to an event
    bool isSubscribed(const std::string& plugin_id, const std::string& event_name) const;

    /// Get all subscriptions for a plugin
    std::vector<std::string> getSubscriptions(const std::string& plugin_id) const;

    /// Rate limiting check per plugin
    bool checkRateLimit(const std::string& plugin_id);

    // Known event names
    static const std::string EVENT_VIEWER_STARTED;
    static const std::string EVENT_VIEWER_SHUTDOWN;
    static const std::string EVENT_LOGIN_STARTED;
    static const std::string EVENT_LOGIN_COMPLETED;
    static const std::string EVENT_REGION_CHANGED;
    static const std::string EVENT_AVATAR_NEARBY;
    static const std::string EVENT_AVATAR_LEFT;
    static const std::string EVENT_CHAT_RECEIVED;
    static const std::string EVENT_CHAT_SENT;
    static const std::string EVENT_IM_RECEIVED;
    static const std::string EVENT_INVENTORY_CHANGED;
    static const std::string EVENT_OBJECT_SELECTED;
    static const std::string EVENT_OBJECT_TOUCHED;
    static const std::string EVENT_TELEPORT_STARTED;
    static const std::string EVENT_TELEPORT_COMPLETED;

    // Mom API events
    static const std::string EVENT_MOM_ONLINE;
    static const std::string EVENT_MOM_OFFLINE;
    static const std::string EVENT_MOM_ENTERED_SIM;
    static const std::string EVENT_MOM_LEFT_SIM;
    static const std::string EVENT_MOM_DISTANCE_CHANGED;

private:
    LLPluginEventBus() = default;

    struct SubscriptionEntry
    {
        LLPluginEventSubscription mSubscription;
    };

    S32 mNextId = 1;
    std::map<std::string, std::vector<SubscriptionEntry>> mSubscriptions; // event_name -> subscribers

    // Rate limiting per plugin (timestamp counts)
    std::map<std::string, std::vector<U64>> mRateLimitTracker;
};

#endif
