/**
 * @file llpluginservice.h
 * @brief Core plugin service - dispatches API calls to viewer subsystems
 *
 * $LicenseInfo:firstyear=2024&license=viewerlgpl$
 * Tasia Viewer Plugin System
 * $/LicenseInfo$
 */

#ifndef LL_LLPLUGINSERVICE_H
#define LL_LLPLUGINSERVICE_H

#include <string>
#include <functional>
#include <memory>
#include "llsd.h"
#include "llpluginprotocol.h"
#include "llpluginpermissions.h"

/// Handles API method dispatch from plugins to viewer subsystems
class LLPluginService
{
public:
    static LLPluginService& instance();

    /// Initialize service - register all API methods
    void init();

    /// Dispatch an API call from a plugin
    /// @returns response message
    LLPluginMessage dispatch(const std::string& plugin_id, const LLPluginMessage& request);

    /// Register a custom method handler
    using MethodHandler = std::function<LLSD(const std::string& plugin_id, const LLSD& params)>;
    void registerHandler(const std::string& method, MethodHandler handler, const std::string& required_permission = "");

    // ---- Viewer API methods ----

    // viewer.*
    LLSD handleViewerGetVersion(const std::string& plugin_id, const LLSD& params);
    LLSD handleViewerGetGrid(const std::string& plugin_id, const LLSD& params);
    LLSD handleViewerNotify(const std::string& plugin_id, const LLSD& params);
    LLSD handleViewerOpenFloater(const std::string& plugin_id, const LLSD& params);

    // chat.*
    LLSD handleChatSend(const std::string& plugin_id, const LLSD& params);
    LLSD handleChatListen(const std::string& plugin_id, const LLSD& params);
    LLSD handleChatUnlisten(const std::string& plugin_id, const LLSD& params);

    // avatar.*
    LLSD handleAvatarGetSelf(const std::string& plugin_id, const LLSD& params);
    LLSD handleAvatarGetNearby(const std::string& plugin_id, const LLSD& params);
    LLSD handleAvatarGetPosition(const std::string& plugin_id, const LLSD& params);

    // world.*
    LLSD handleWorldGetRegion(const std::string& plugin_id, const LLSD& params);
    LLSD handleWorldTeleport(const std::string& plugin_id, const LLSD& params);

    // inventory.*
    LLSD handleInventorySearch(const std::string& plugin_id, const LLSD& params);
    LLSD handleInventoryWear(const std::string& plugin_id, const LLSD& params);
    LLSD handleInventoryAttach(const std::string& plugin_id, const LLSD& params);

    // camera.*
    LLSD handleCameraGet(const std::string& plugin_id, const LLSD& params);
    LLSD handleCameraSet(const std::string& plugin_id, const LLSD& params);
    LLSD handleCameraLookAt(const std::string& plugin_id, const LLSD& params);

    // ui.*
    LLSD handleUiCreatePanel(const std::string& plugin_id, const LLSD& params);
    LLSD handleUiDestroyPanel(const std::string& plugin_id, const LLSD& params);
    LLSD handleUiAddButton(const std::string& plugin_id, const LLSD& params);
    LLSD handleUiAddText(const std::string& plugin_id, const LLSD& params);
    LLSD handleUiShowDialog(const std::string& plugin_id, const LLSD& params);

    // network.*
    LLSD handleNetworkHttpRequest(const std::string& plugin_id, const LLSD& params);

    // storage.*
    LLSD handleStorageGet(const std::string& plugin_id, const LLSD& params);
    LLSD handleStorageSet(const std::string& plugin_id, const LLSD& params);
    LLSD handleStorageDelete(const std::string& plugin_id, const LLSD& params);

    // events.*
    LLSD handleEventsSubscribe(const std::string& plugin_id, const LLSD& params);
    LLSD handleEventsUnsubscribe(const std::string& plugin_id, const LLSD& params);

    // tasia.mom.*
    LLSD handleMomIsOnline(const std::string& plugin_id, const LLSD& params);
    LLSD handleMomIsOnSim(const std::string& plugin_id, const LLSD& params);
    LLSD handleMomDistance(const std::string& plugin_id, const LLSD& params);
    LLSD handleMomUuid(const std::string& plugin_id, const LLSD& params);
    LLSD handleMomName(const std::string& plugin_id, const LLSD& params);

private:
    LLPluginService() = default;

    struct HandlerEntry
    {
        MethodHandler mHandler;
        std::string   mRequiredPermission;
    };
    std::map<std::string, HandlerEntry> mHandlers;
};

#endif
