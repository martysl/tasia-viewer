/**
 * @file llfloaterunlockextras.h
 * @brief Tasia extras unlock floater - password gate for spoofing/export tools
 *
 * $LicenseInfo:firstyear=2026&license=viewerlgpl$
 * Tasia Viewer
 * $/LicenseInfo$
 */

#ifndef LL_LLFLOATERUNLOCKEXTRAS_H
#define LL_LLFLOATERUNLOCKEXTRAS_H

#include "llfloater.h"
#include "llsd.h"

class LLFloaterUnlockExtras : public LLFloater
{
public:
    LLFloaterUnlockExtras(const LLSD& key);
    ~LLFloaterUnlockExtras() override;

    /*virtual*/ BOOL postBuild() override;
    /*virtual*/ void draw() override;

    static void registerFloater();

private:
    void onUnlockClicked();
    void onLockClicked();
    void updateStatus();

    bool mNeedsRefresh = true;
};

#endif // LL_LLFLOATERUNLOCKEXTRAS_H
