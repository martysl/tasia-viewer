/**
 * @file llpluginservice.cpp
 * @brief Core plugin service implementation
 *
 * $LicenseInfo:firstyear=2024&license=viewerlgpl$
 * Tasia Viewer Plugin System
 * $/LicenseInfo$
 */

#include "llviewerprecompiledheaders.h"
#include "llpluginservice.h"
#include "llpluginpermissions.h"
#include "llplugineventbus.h"
#include "llpluginstorage.h"
#include "llpluginruntime.h"

#include "llagent.h"
#include "llagentdata.h"
#include "llviewercontrol.h"
#include "llviewerregion.h"
#include "llviewerchat.h"
#include "llviewerinventory.h"
#include "llinventorymodel.h"
#include "llchat.h"
#include "llfloaterreg.h"
#include "llnotificationsutil.h"
#include "llsd.h"
#include "lltrans.h"
#include "llurlentry.h"
#include "llvoiceclient.h"
#include "llworld.h"
#include "llhudeffectlookat.h"
#include "llavatarnamecache.h"
#include "llavatarappearance.h"
#include "llagentcamera.h"

LLPluginService& LLPluginService::instance()
{
    static LLPluginService sInstance;
    return sInstance;
}

void LLPluginService::init()
{
    // Register all API handlers with their required permissions

    // viewer.*
    registerHandler("viewer.getVersion",  [this](auto id, auto p) { return handleViewerGetVersion(id, p); });
    registerHandler("viewer.getGrid",     [this](auto id, auto p) { return handleViewerGetGrid(id, p); });
    registerHandler("viewer.notify",      [this](auto id, auto p) { return handleViewerNotify(id, p); }, "viewer.notify");
    registerHandler("viewer.openFloater", [this](auto id, auto p) { return handleViewerOpenFloater(id, p); }, "viewer.open_floater");

    // chat.*
    registerHandler("chat.send",    [this](auto id, auto p) { return handleChatSend(id, p); }, "chat.send");
    registerHandler("chat.listen",  [this](auto id, auto p) { return handleChatListen(id, p); }, "chat.read");
    registerHandler("chat.unlisten",[this](auto id, auto p) { return handleChatUnlisten(id, p); });

    // avatar.*
    registerHandler("avatar.getSelf",    [this](auto id, auto p) { return handleAvatarGetSelf(id, p); }, "avatar.read");
    registerHandler("avatar.getNearby",  [this](auto id, auto p) { return handleAvatarGetNearby(id, p); }, "avatar.nearby");
    registerHandler("avatar.getPosition",[this](auto id, auto p) { return handleAvatarGetPosition(id, p); }, "avatar.read");

    // world.*
    registerHandler("world.getRegion",  [this](auto id, auto p) { return handleWorldGetRegion(id, p); }, "world.read");
    registerHandler("world.teleport",   [this](auto id, auto p) { return handleWorldTeleport(id, p); }, "world.teleport");

    // inventory.*
    registerHandler("inventory.search", [this](auto id, auto p) { return handleInventorySearch(id, p); }, "inventory.read");
    registerHandler("inventory.wear",   [this](auto id, auto p) { return handleInventoryWear(id, p); }, "inventory.modify");
    registerHandler("inventory.attach", [this](auto id, auto p) { return handleInventoryAttach(id, p); }, "inventory.modify");

    // camera.*
    registerHandler("camera.get",    [this](auto id, auto p) { return handleCameraGet(id, p); }, "camera.read");
    registerHandler("camera.set",    [this](auto id, auto p) { return handleCameraSet(id, p); }, "camera.control");
    registerHandler("camera.lookAt", [this](auto id, auto p) { return handleCameraLookAt(id, p); }, "camera.control");

    // ui.*
    registerHandler("ui.createPanel",   [this](auto id, auto p) { return handleUiCreatePanel(id, p); }, "ui.create");
    registerHandler("ui.destroyPanel",  [this](auto id, auto p) { return handleUiDestroyPanel(id, p); }, "ui.create");
    registerHandler("ui.addButton",     [this](auto id, auto p) { return handleUiAddButton(id, p); }, "ui.create");
    registerHandler("ui.addText",       [this](auto id, auto p) { return handleUiAddText(id, p); }, "ui.create");
    registerHandler("ui.showDialog",    [this](auto id, auto p) { return handleUiShowDialog(id, p); }, "ui.create");

    // network.*
    registerHandler("network.httpRequest", [this](auto id, auto p) { return handleNetworkHttpRequest(id, p); }, "network.http");

    // storage.*
    registerHandler("storage.get",    [this](auto id, auto p) { return handleStorageGet(id, p); }, "storage.plugin");
    registerHandler("storage.set",    [this](auto id, auto p) { return handleStorageSet(id, p); }, "storage.plugin");
    registerHandler("storage.delete", [this](auto id, auto p) { return handleStorageDelete(id, p); }, "storage.plugin");

    // events.*
    registerHandler("events.subscribe",   [this](auto id, auto p) { return handleEventsSubscribe(id, p); });
    registerHandler("events.unsubscribe", [this](auto id, auto p) { return handleEventsUnsubscribe(id, p); });

    // tasia.mom.*
    registerHandler("tasia.mom.isOnline",  [this](auto id, auto p) { return handleMomIsOnline(id, p); }, "mom.read");
    registerHandler("tasia.mom.isOnSim",   [this](auto id, auto p) { return handleMomIsOnSim(id, p); }, "mom.read");
    registerHandler("tasia.mom.distance",  [this](auto id, auto p) { return handleMomDistance(id, p); }, "mom.read");
    registerHandler("tasia.mom.uuid",      [this](auto id, auto p) { return handleMomUuid(id, p); }, "mom.read");
    registerHandler("tasia.mom.name",      [this](auto id, auto p) { return handleMomName(id, p); }, "mom.read");
}

void LLPluginService::registerHandler(const std::string& method, MethodHandler handler, const std::string& required_permission)
{
    mHandlers[method] = { handler, required_permission };
}

LLPluginMessage LLPluginService::dispatch(const std::string& plugin_id, const LLPluginMessage& request)
{
    std::string method = request.getMethod();

    // Find handler
    auto it = mHandlers.find(method);
    if (it == mHandlers.end())
    {
        return LLPluginMessage::error(request.getId(), LLPluginError::METHOD_NOT_FOUND,
                                       "Method not found: " + method);
    }

    // Check permission
    if (!it->second.mRequiredPermission.empty())
    {
        if (!LLPluginPermissionStore::enforce(plugin_id, it->second.mRequiredPermission))
        {
            LL_WARNS("Plugins") << "Permission denied for " << plugin_id
                                << ": " << it->second.mRequiredPermission << LL_ENDL;
            return LLPluginMessage::error(request.getId(), LLPluginError::PERMISSION_DENIED,
                                           "Permission denied: " + it->second.mRequiredPermission);
        }
    }

    // Dispatch
    try
    {
        LLSD result = it->second.mHandler(plugin_id, request.getParams());
        return LLPluginMessage::response(request.getId(), result);
    }
    catch (const std::exception& e)
    {
        LL_WARNS("Plugins") << "Plugin API error in " << method << ": " << e.what() << LL_ENDL;
        return LLPluginMessage::error(request.getId(), LLPluginError::INTERNAL_ERROR,
                                       "Internal error: " + std::string(e.what()));
    }
}

// ==================== Viewer API Implementations ====================

LLSD LLPluginService::handleViewerGetVersion(const std::string& plugin_id, const LLSD& params)
{
    LLSD result;
    result["version"] = LLTrans::getString("TasiaVersion");
    result["build"] = gSavedSettings.getString("VersionChannel");
    return result;
}

LLSD LLPluginService::handleViewerGetGrid(const std::string& plugin_id, const LLSD& params)
{
    LLSD result;
    if (gAgent.getRegion())
    {
        result["grid"] = gAgent.getRegion()->getSimHostName();
        result["region"] = gAgent.getRegion()->getName();
    }
    return result;
}

LLSD LLPluginService::handleViewerNotify(const std::string& plugin_id, const LLSD& params)
{
    std::string text = params["message"].asString();
    if (!text.empty())
    {
        LLSD args;
        args["MESSAGE"] = text;
        LLNotificationsUtil::add("SystemMessage", args);
    }
    LLSD result;
    result["success"] = true;
    return result;
}

LLSD LLPluginService::handleViewerOpenFloater(const std::string& plugin_id, const LLSD& params)
{
    std::string floater_name = params["name"].asString();
    if (!floater_name.empty())
    {
        LLFloaterReg::showInstance(floater_name);
    }
    LLSD result;
    result["success"] = !floater_name.empty();
    return result;
}

// chat.*
LLSD LLPluginService::handleChatSend(const std::string& plugin_id, const LLSD& params)
{
    std::string message = params["message"].asString();
    S32 channel = params["channel"].asInteger();

    if (!message.empty())
    {
        LLChat chat;
        chat.mText = message;
        chat.mChatType = CHAT_TYPE_NORMAL;
        if (channel != 0)
        {
            chat.mChatType = CHAT_TYPE_DEBUG_MSG;
        }
        chat.mMuted = false;
        chat.mFromName = gAgent.getAvatarName();
        chat.mFromID = gAgent.getID();

        // Send chat into the viewer's chat pipeline
        LLViewerChat::sendChatFromViewer(chat, channel);
    }

    LLSD result;
    result["success"] = true;
    return result;
}

LLSD LLPluginService::handleChatListen(const std::string& plugin_id, const LLSD& params)
{
    // Chat listening is handled by the runtime host subscribing to events
    LLPluginEventBus::instance().subscribe(plugin_id, LLPluginEventBus::EVENT_CHAT_RECEIVED,
        [](const LLSD& data) {
            // Forward to plugin via runtime
            // Actual forwarding is done by the runtime host
        });
    LLSD result;
    result["success"] = true;
    return result;
}

LLSD LLPluginService::handleChatUnlisten(const std::string& plugin_id, const LLSD& params)
{
    LLPluginEventBus::instance().unsubscribeAll(plugin_id);
    LLSD result;
    result["success"] = true;
    return result;
}

// avatar.*
LLSD LLPluginService::handleAvatarGetSelf(const std::string& plugin_id, const LLSD& params)
{
    LLSD result;
    result["id"] = gAgent.getID().asString();
    result["name"] = gAgent.getAvatarName();
    result["position"]["x"] = gAgent.getPositionGlobal().mdV[VX];
    result["position"]["y"] = gAgent.getPositionGlobal().mdV[VY];
    result["position"]["z"] = gAgent.getPositionGlobal().mdV[VZ];
    return result;
}

LLSD LLPluginService::handleAvatarGetNearby(const std::string& plugin_id, const LLSD& params)
{
    LLSD result;
    // Avatar tracking is complex - return basic info
    result["nearby"] = LLSD::emptyArray();
    return result;
}

LLSD LLPluginService::handleAvatarGetPosition(const std::string& plugin_id, const LLSD& params)
{
    LLSD result;
    result["position"]["x"] = gAgent.getPositionGlobal().mdV[VX];
    result["position"]["y"] = gAgent.getPositionGlobal().mdV[VY];
    result["position"]["z"] = gAgent.getPositionGlobal().mdV[VZ];
    return result;
}

// world.*
LLSD LLPluginService::handleWorldGetRegion(const std::string& plugin_id, const LLSD& params)
{
    LLSD result;
    if (gAgent.getRegion())
    {
        result["name"] = gAgent.getRegion()->getName();
        result["host"] = gAgent.getRegion()->getSimHostName();
        result["id"] = gAgent.getRegion()->getRegionID().asString();
    }
    return result;
}

LLSD LLPluginService::handleWorldTeleport(const std::string& plugin_id, const LLSD& params)
{
    std::string destination = params["destination"].asString();
    if (!destination.empty())
    {
        LLSD path;
        path["type"] = "url";
        path["location"] = destination;
        gAgent.teleportViaLocation(destination);
    }
    LLSD result;
    result["success"] = !destination.empty();
    return result;
}

// inventory.*
LLSD LLPluginService::handleInventorySearch(const std::string& plugin_id, const LLSD& params)
{
    LLSD result;
    result["items"] = LLSD::emptyArray();
    // Full inventory search is complex - stub
    return result;
}

LLSD LLPluginService::handleInventoryWear(const std::string& plugin_id, const LLSD& params)
{
    LLSD result;
    result["success"] = false;
    result["error"] = "Not implemented";
    return result;
}

LLSD LLPluginService::handleInventoryAttach(const std::string& plugin_id, const LLSD& params)
{
    LLSD result;
    result["success"] = false;
    result["error"] = "Not implemented";
    return result;
}

// camera.*
LLSD LLPluginService::handleCameraGet(const std::string& plugin_id, const LLSD& params)
{
    LLSD result;
    result["position"]["x"] = gAgentCamera.getCameraPositionGlobal().mdV[VX];
    result["position"]["y"] = gAgentCamera.getCameraPositionGlobal().mdV[VY];
    result["position"]["z"] = gAgentCamera.getCameraPositionGlobal().mdV[VZ];
    return result;
}

LLSD LLPluginService::handleCameraSet(const std::string& plugin_id, const LLSD& params)
{
    LLSD result;
    result["success"] = false;
    result["error"] = "Not implemented";
    return result;
}

LLSD LLPluginService::handleCameraLookAt(const std::string& plugin_id, const LLSD& params)
{
    LLSD result;
    result["success"] = false;
    result["error"] = "Not implemented";
    return result;
}

// ui.*
LLSD LLPluginService::handleUiCreatePanel(const std::string& plugin_id, const LLSD& params)
{
    LLSD result;
    result["success"] = false;
    result["error"] = "Not implemented";
    return result;
}

LLSD LLPluginService::handleUiDestroyPanel(const std::string& plugin_id, const LLSD& params)
{
    LLSD result;
    result["success"] = false;
    result["error"] = "Not implemented";
    return result;
}

LLSD LLPluginService::handleUiAddButton(const std::string& plugin_id, const LLSD& params)
{
    LLSD result;
    result["success"] = false;
    result["error"] = "Not implemented";
    return result;
}

LLSD LLPluginService::handleUiAddText(const std::string& plugin_id, const LLSD& params)
{
    LLSD result;
    result["success"] = false;
    result["error"] = "Not implemented";
    return result;
}

LLSD LLPluginService::handleUiShowDialog(const std::string& plugin_id, const LLSD& params)
{
    std::string title = params["title"].asString();
    std::string message = params["message"].asString();
    if (!title.empty() && !message.empty())
    {
        LLSD args;
        args["TITLE"] = title;
        args["MESSAGE"] = message;
        LLNotificationsUtil::add("GenericAlert", args);
    }
    LLSD result;
    result["success"] = true;
    return result;
}

// network.*
LLSD LLPluginService::handleNetworkHttpRequest(const std::string& plugin_id, const LLSD& params)
{
    LLSD result;
    result["success"] = false;
    result["error"] = "Not implemented";
    return result;
}

// storage.*
LLSD LLPluginService::handleStorageGet(const std::string& plugin_id, const LLSD& params)
{
    LLPluginStorage storage(plugin_id);
    storage.load();
    LLSD result;
    result["value"] = storage.get(params["key"].asString());
    return result;
}

LLSD LLPluginService::handleStorageSet(const std::string& plugin_id, const LLSD& params)
{
    LLPluginStorage storage(plugin_id);
    storage.load();
    storage.set(params["key"].asString(), params["value"]);
    storage.save();
    LLSD result;
    result["success"] = true;
    return result;
}

LLSD LLPluginService::handleStorageDelete(const std::string& plugin_id, const LLSD& params)
{
    LLPluginStorage storage(plugin_id);
    storage.load();
    bool deleted = storage.del(params["key"].asString());
    storage.save();
    LLSD result;
    result["success"] = deleted;
    return result;
}

// events.*
LLSD LLPluginService::handleEventsSubscribe(const std::string& plugin_id, const LLSD& params)
{
    // Event subscription is handled by the runtime host
    LLSD result;
    result["success"] = true;
    return result;
}

LLSD LLPluginService::handleEventsUnsubscribe(const std::string& plugin_id, const LLSD& params)
{
    LLPluginEventBus::instance().unsubscribeAll(plugin_id);
    LLSD result;
    result["success"] = true;
    return result;
}

// tasia.mom.*
LLSD LLPluginService::handleMomIsOnline(const std::string& plugin_id, const LLSD& params)
{
    LLSD result;
    result["online"] = gAgent.getID().notNull(); // Simplified - real impl needs Mom UUID check
    return result;
}

LLSD LLPluginService::handleMomIsOnSim(const std::string& plugin_id, const LLSD& params)
{
    LLSD result;
    result["on_sim"] = false; // Simplified
    return result;
}

LLSD LLPluginService::handleMomDistance(const std::string& plugin_id, const LLSD& params)
{
    LLSD result;
    result["distance"] = -1.0; // Not implemented
    return result;
}

LLSD LLPluginService::handleMomUuid(const std::string& plugin_id, const LLSD& params)
{
    LLSD result;
    result["uuid"] = gSavedSettings.getString("TasiaMomAvatarUUID");
    return result;
}

LLSD LLPluginService::handleMomName(const std::string& plugin_id, const LLSD& params)
{
    LLSD result;
    result["name"] = gSavedSettings.getString("TasiaMomAvatarName");
    return result;
}
