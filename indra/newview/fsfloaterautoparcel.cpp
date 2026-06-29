#include "llviewerprecompiledheaders.h"
#include "fsfloaterautoparcel.h"
#include "llagent.h"
#include "llgroupactions.h"
#include "llgroupmgr.h"
#include "llparcel.h"
#include "llscrolllistctrl.h"
#include "llsdserialize.h"
#include "llselectmgr.h"
#include "lltrans.h"
#include "llviewercontrol.h"
#include "llviewerparcelmgr.h"
#include "lluictrlfactory.h"
#include "llnotificationsutil.h"
#include "llfloaterreg.h"

FSFloaterAutoParcel::FSFloaterAutoParcel(const LLSD& seed)
    : LLFloater(seed)
    , LLGroupMgrObserver(LLUUID::null)
    , mRulesList(NULL)
    , mCurrentParcelText(NULL)
    , mEnabledCheck(NULL)
{
    mCommitCallbackRegistrar.add("AutoParcel.AddCurrent", boost::bind(&FSFloaterAutoParcel::addCurrentParcel, this));
    mCommitCallbackRegistrar.add("AutoParcel.Delete", boost::bind(&FSFloaterAutoParcel::deleteSelected, this));
    mCommitCallbackRegistrar.add("AutoParcel.Refresh", boost::bind(&FSFloaterAutoParcel::refresh, this));

    // Register checkbox callback
    mCommitCallbackRegistrar.add("AutoParcel.ToggleEnabled", [](LLUICtrl*, const LLSD& data) {
        gSavedSettings.setBOOL("AutoParcelChange", data.asBoolean());
    });
}

FSFloaterAutoParcel::~FSFloaterAutoParcel()
{
    if (mSelectedGroupID.notNull())
    {
        LLGroupMgr::getInstance()->removeObserver(this);
    }
}

bool FSFloaterAutoParcel::postBuild()
{
    mRulesList = getChild<LLScrollListCtrl>("rules_list");
    mCurrentParcelText = getChild<LLTextBox>("current_parcel_name");
    mEnabledCheck = getChild<LLCheckBoxCtrl>("auto_parcel_enabled");

    // Connect "Add" button - shows group picker then we prompt for role
    // Using the standard Firestorm group title flow
    mEnabledCheck->setCommitCallback(boost::bind(&FSFloaterAutoParcel::refresh, this));
    mEnabledCheck->setValue(gSavedSettings.getBOOL("AutoParcelChange"));

    refresh();
    return true;
}

void FSFloaterAutoParcel::onOpen(const LLSD& key)
{
    refresh();
}

void FSFloaterAutoParcel::changed(LLGroupChange gc)
{
    if (gc == GC_TITLES && mSelectedGroupID.notNull())
    {
        // Titles received - show role picker
        LLGroupMgr* mgr = LLGroupMgr::getInstance();
        LLGroupData* gd = mgr->getGroupData(mSelectedGroupID);
        if (gd)
        {
            mTitles = gd->mTitles;
        }
        mgr->removeObserver(this);

        // Build a notification for role selection
        LLSD args;
        args["GROUP_NAME"] = LLGroupMgr::getInstance()->getGroupName(mSelectedGroupID);
        LLSD payload;
        payload["parcel"] = mPendingParcel;
        payload["group_id"] = mSelectedGroupID;

        LLSD roles;
        roles["None"] = LLUUID::null.asString();
        for (const auto& title : mTitles)
        {
            roles[title.mTitle] = title.mRoleID.asString();
        }
        payload["roles"] = roles;

        LLNotificationsUtil::add("AutoParcelSelectRole", args, payload,
            boost::bind(&FSFloaterAutoParcel::onSelectRole, this, _1, _2));
    }
}

void FSFloaterAutoParcel::addCurrentParcel()
{
    LLViewerParcelMgr* mgr = LLViewerParcelMgr::getInstance();
    LLParcel* parcel = mgr->getParcel();
    if (!parcel)
    {
        LLNotificationsUtil::add("NoParcelData");
        return;
    }

    std::string parcel_name = parcel->getName();
    if (parcel_name.empty())
    {
        parcel_name = "(unnamed parcel)";
    }

    mPendingParcel = parcel_name;

    // Show group picker notification
    LLSD args;
    LLSD payload;
    payload["parcel"] = parcel_name;

    // Build list of groups
    LLSD groups;
    for (const auto& group : gAgent.mGroups)
    {
        groups[group.mName] = group.mID.asString();
    }
    payload["groups"] = groups;

    LLNotificationsUtil::add("AutoParcelSelectGroup", args, payload,
        boost::bind(&FSFloaterAutoParcel::onSelectGroup, this, _1, _2));
}

bool FSFloaterAutoParcel::onSelectGroup(const LLSD& notification, const LLSD& response)
{
    std::string group_name = response["group"].asString();
    LLSD payload = notification["payload"];
    mPendingParcel = payload["parcel"].asString();
    LLSD groups = payload["groups"];

    if (group_name.empty() || !groups.has(group_name))
    {
        return false;
    }

    mSelectedGroupID = LLUUID(groups[group_name].asString());
    mPendingGroup = mSelectedGroupID;

    // Request titles from the group
    LLGroupMgr::getInstance()->addObserver(this);
    LLGroupMgr::getInstance()->sendGroupTitlesRequest(mSelectedGroupID);

    return false;
}

bool FSFloaterAutoParcel::onSelectRole(const LLSD& notification, const LLSD& response)
{
    std::string role_name = response["role"].asString();
    LLSD payload = notification["payload"];

    std::string parcel_name = mPendingParcel;
    LLUUID group_id = mPendingGroup;
    LLUUID role_id = LLUUID::null;

    // Parse selected role
    LLSD roles = payload["roles"];
    if (roles.has(role_name))
    {
        role_id = LLUUID(roles[role_name].asString());
    }

    // Save to config
    LLSD config = gSavedPerAccountSettings.getLLSD("AutoParcelChangeConfig");
    LLSD entry;
    entry["group_id"] = group_id.asString();
    entry["role_id"] = role_id.asString();
    config[parcel_name] = entry;
    gSavedPerAccountSettings.setLLSD("AutoParcelChangeConfig", config);

    gSavedSettings.setBOOL("AutoParcelChange", true);
    mEnabledCheck->setValue(true);

    refresh();
    return true;
}

void FSFloaterAutoParcel::deleteSelected()
{
    LLScrollListItem* item = mRulesList->getFirstSelected();
    if (!item) return;

    std::string parcel_name = item->getColumn(0)->getValue().asString();

    LLSD config = gSavedPerAccountSettings.getLLSD("AutoParcelChangeConfig");
    if (config.has(parcel_name))
    {
        config.erase(parcel_name);
        gSavedPerAccountSettings.setLLSD("AutoParcelChangeConfig", config);
    }

    refresh();
}

void FSFloaterAutoParcel::refresh()
{
    // Update current parcel name
    LLViewerParcelMgr* mgr = LLViewerParcelMgr::getInstance();
    LLParcel* parcel = mgr->getParcel();
    if (parcel)
    {
        std::string name = parcel->getName();
        if (name.empty()) name = "(unnamed parcel)";
        mCurrentParcelText->setText(name);
    }

    // Update rules list
    mRulesList->clearRows();

    LLSD config = gSavedPerAccountSettings.getLLSD("AutoParcelChangeConfig");
    if (config.isMap())
    {
        for (LLSD::map_iterator it = config.beginMap(); it != config.endMap(); ++it)
        {
            std::string parcel_name = it->first;
            LLSD entry = it->second;
            LLUUID group_id(entry["group_id"].asString());

            std::string group_name = "Unknown";
            if (group_id.notNull())
            {
                for (const auto& g : gAgent.mGroups)
                {
                    if (g.mID == group_id)
                    {
                        group_name = g.mName;
                        break;
                    }
                }
            }

            std::string role_name = "Default";
            LLUUID role_id(entry["role_id"].asString());
            if (role_id.notNull())
            {
                role_name = "Custom";
            }

            LLSD row;
            row["columns"][0]["column"] = "parcel_name";
            row["columns"][0]["value"] = parcel_name;
            row["columns"][1]["column"] = "group_name";
            row["columns"][1]["value"] = group_name;
            row["columns"][2]["column"] = "role_name";
            row["columns"][2]["value"] = role_name;
            mRulesList->addElement(row);
        }
    }
}

void FSFloaterAutoParcel::showGroupPicker()
{
    // Implementation moved to notification-based flow
}

void FSFloaterAutoParcel::showRolePicker(const LLUUID& group_id)
{
    // Implementation moved to notification-based flow
}
