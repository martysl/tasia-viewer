#ifndef FS_FLOATER_AUTO_PARCEL_H
#define FS_FLOATER_AUTO_PARCEL_H

#include "llfloater.h"
#include "llscrolllistctrl.h"
#include "llgroupmgr.h"

class FSFloaterAutoParcel : public LLFloater, public LLGroupMgrObserver
{
public:
    FSFloaterAutoParcel(const LLSD& seed);
    ~FSFloaterAutoParcel();

    // LLGroupMgrObserver
    void changed(LLGroupChange gc) override;

    // LLFloater
    bool postBuild() override;
    void onOpen(const LLSD& key) override;

private:
    void refresh();
    void addCurrentParcel();
    void deleteSelected();
    void onSelectGroup(LLUUID group_id);
    void onSelectRole(LLUUID role_id);
    void showGroupPicker();
    void showRolePicker(const LLUUID& group_id);

    LLScrollListCtrl* mRulesList;
    LLTextBox* mCurrentParcelText;
    LLCheckBoxCtrl* mEnabledCheck;

    // For group/role selection flow
    std::string mPendingParcel;
    LLUUID mPendingGroup;
    LLUUID mPendingRole;

    // Group data cache
    std::vector<LLGroupData> mGroups;
    LLUUID mSelectedGroupID;
    std::vector<LLGroupTitle> mTitles;
};

#endif
