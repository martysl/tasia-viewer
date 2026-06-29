#ifndef FS_FLOATER_AUTO_PARCEL_H
#define FS_FLOATER_AUTO_PARCEL_H

#include "llfloater.h"
#include "llscrolllistctrl.h"
#include "llgroupmgr.h"
#include "lltextbox.h"
#include "llcheckboxctrl.h"

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

    LLScrollListCtrl* mRulesList;
    LLTextBox* mCurrentParcelText;
    LLCheckBoxCtrl* mEnabledCheck;
};

#endif
