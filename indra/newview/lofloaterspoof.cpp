/**
 * ID Spoof floater
 */

#include "llviewerprecompiledheaders.h"

#include "llcontrol.h"
#include "lllineeditor.h"

#include "lofloaterspoof.h"

#include "lospoof.h"

extern LLControlGroup gSavedSettings;

void LOFloaterSpoof::update_labels()
{
    auto&& real_nodeid = getChild<LLLineEditor>("real_nodeid");
    auto&& real_machineid = getChild<LLLineEditor>("real_machineid");
    auto&& real_id0 = getChild<LLLineEditor>("real_id0");
    auto&& real_macid = getChild<LLLineEditor>("real_macid");

    auto&& spoof_nodeid = getChild<LLLineEditor>("faux_nodeid");
    auto&& spoof_machineid = getChild<LLLineEditor>("faux_machineid");
    auto&& spoof_id0 = getChild<LLLineEditor>("spoof_id0");
    auto&& spoof_macid = getChild<LLLineEditor>("spoof_macid");

    auto&& username = getChild<LLLineEditor>("username");
    auto&& seed = getChild<LLLineEditor>("seed");

    real_nodeid->setText(lolistorm_get_real_nodeid_str());
    real_machineid->setText(lolistorm_get_real_machineid_str());
    real_id0->setText(lolistorm_get_real_serial());
    real_macid->setText(lolistorm_get_real_macid_str());

    spoof_nodeid->setText(lolistorm_get_faux_nodeid_str());
    spoof_machineid->setText(lolistorm_get_faux_machineid_str());
    spoof_id0->setText(lolistorm_get_id0());
    spoof_macid->setText(lolistorm_get_macid());

    username->setText(lolistorm_get_username());
    seed->setText(lolistorm_get_seed());
}

LOFloaterSpoof::LOFloaterSpoof(const LLSD& key)
    : LLFloater(key)
{
}

LOFloaterSpoof::~LOFloaterSpoof()
{
}

bool LOFloaterSpoof::postBuild()
{
    update_labels();

    getChild<LLUICtrl>("reroll_btn")->setCommitCallback([this](LLUICtrl* ctrl, const LLSD&)
    {
        lolistorm_reroll_seed();
        gSavedSettings.setString("LOSpoofRandomSeed", lolistorm_get_seed());
        update_labels();
    });

    center();

    return true;
}
