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
#include "lltextbox.h"
#include "llcheckboxctrl.h"

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
}

FSFloaterAutoParcel::~FSFloaterAutoParcel()
{
}

bool FSFloaterAutoParcel::postBuild()
{
    mRulesList = getChild<LLScrollListCtrl>("rules_list");
    mCurrentParcelText = getChild<LLTextBox>("current_parcel_name");
    mEnabledCheck = getChild<LLCheckBoxCtrl>("auto_parcel_enabled");

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
}

void FSFloaterAutoParcel::addCurrentParcel()
{
    LLParcel* parcel = LLViewerParcelMgr::getInstance()->getAgentParcel();
    if (!parcel) return;

    std::string parcel_name = parcel->getName();
    if (parcel_name.empty()) parcel_name = "(unnamed parcel)";

    // For now, just add a placeholder entry with no group set
    LLSD config = gSavedPerAccountSettings.getLLSD("AutoParcelChangeConfig");
    if (!config.isMap()) config = LLSD::emptyMap();

    LLSD entry;
    entry["group_id"] = LLUUID::null.asString();
    entry["role_id"] = LLUUID::null.asString();
    config[parcel_name] = entry;
    gSavedPerAccountSettings.setLLSD("AutoParcelChangeConfig", config);

    mEnabledCheck->setValue(true);
    gSavedSettings.setBOOL("AutoParcelChange", true);
    refresh();
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
    gSavedSettings.setBOOL("AutoParcelChange", mEnabledCheck->getValue().asBoolean());

    // Update current parcel name
    LLParcel* parcel = LLViewerParcelMgr::getInstance()->getAgentParcel();
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

            std::string group_name = "None";
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

            LLSD row;
            row["columns"][0]["column"] = "parcel_name";
            row["columns"][0]["value"] = parcel_name;
            row["columns"][1]["column"] = "group_name";
            row["columns"][1]["value"] = group_name;
            row["columns"][2]["column"] = "role_name";
            row["columns"][2]["value"] = "Default";
            mRulesList->addElement(row);
        }
    }
}
