/**
 * @file llfloaterunlockextras.cpp
 * @brief Tasia extras unlock floater implementation
 *
 * $LicenseInfo:firstyear=2026&license=viewerlgpl$
 * Tasia Viewer
 * $/LicenseInfo$
 */

#include "llviewerprecompiledheaders.h"

#include "llfloaterunlockextras.h"

#include "llbutton.h"
#include "lllineeditor.h"
#include "lltextbase.h"
#include "llfloaterreg.h"
#include "lluictrlfactory.h"
#include "llnotificationsutil.h"
#include "llviewercontrol.h"
#include "loextras.h"
#include "llmd5.h"

LLFloaterUnlockExtras::LLFloaterUnlockExtras(const LLSD& key)
    : LLFloater(key)
{
    mAutoScale = true;
}

LLFloaterUnlockExtras::~LLFloaterUnlockExtras()
{
}

bool LLFloaterUnlockExtras::postBuild()
{
    getChild<LLButton>("unlock_btn")->setClickedCallback([this]() { onUnlockClicked(); });
    getChild<LLButton>("lock_btn")->setClickedCallback([this]() { onLockClicked(); });

    updateStatus();
    return TRUE;
}

void LLFloaterUnlockExtras::draw()
{
    if (mNeedsRefresh)
    {
        updateStatus();
        mNeedsRefresh = false;
    }
    LLFloater::draw();
}

void LLFloaterUnlockExtras::registerFloater()
{
    LLFloaterReg::add("unlock_extras", "floater_unlock_extras.xml",
                      (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterUnlockExtras>);
}

void LLFloaterUnlockExtras::onUnlockClicked()
{
    std::string entered = getChild<LLLineEditor>("password_edit")->getText();

    // Hash the entered password with MD5 and compare against the stored hash
    std::string digest(32, ' ');
    LLMD5 hash((const unsigned char*)entered.data(), entered.size());
    hash.hex_digest(&digest[0]);

    if (lolistorm_unlock_extras(digest))
    {
        getChild<LLTextBase>("status_text")->setText("Extra features unlocked for this session.");
        getChild<LLLineEditor>("password_edit")->setText("");
    }
    else
    {
        getChild<LLTextBase>("status_text")->setText("Wrong password.");
    }
}

void LLFloaterUnlockExtras::onLockClicked()
{
    lolistorm_lock_extras();
    getChild<LLTextBase>("status_text")->setText("Extra features locked.");
    getChild<LLLineEditor>("password_edit")->setText("");
}

void LLFloaterUnlockExtras::updateStatus()
{
    if (lolistorm_extras_unlocked())
    {
        getChild<LLTextBase>("status_text")->setText("Extra features are UNLOCKED for this session.");
    }
    else
    {
        getChild<LLTextBase>("status_text")->setText("Extra features are locked. Enter the password to unlock them for this session.");
    }
}
