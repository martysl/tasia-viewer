/**
 * LOstorm Extras floater
 */

#include "llviewerprecompiledheaders.h"

#include "fspanellogin.h"
#include "llcontrol.h"
#include "lllineeditor.h"
#include "lltextbase.h"

#include "lofloaterextras.h"

#include "loextras.h"
#include "lospoof.h"

#include "llfloaterreg.h"
#include "lofloaterspoof.h"

extern LLControlGroup gSavedSettings;


void LOFloaterExtras::update_labels()
{
    LLUICtrl* custom_login_ids_chk = getChild<LLUICtrl>("custom_login_ids_chk");
    LLLineEditor* custom_id0 = getChild<LLLineEditor>("custom_id0");
    LLLineEditor* custom_macid = getChild<LLLineEditor>("custom_macid");

    LLTextBase* username = getChild<LLTextBase>("username");

    const std::string& id0 = lolistorm_get_custom_id0();
    const std::string& macid = lolistorm_get_custom_macid();

    bool have_custom = (!id0.empty() || !macid.empty());
    custom_login_ids_chk->setValue(have_custom);

    custom_id0->setText(id0);
    custom_macid->setText(macid);
    custom_id0->setEnabled(have_custom);
    custom_macid->setEnabled(have_custom);

    username->setValue(lolistorm_get_custom_username());
}

void LOFloaterExtras::logged_in()
{
    LLUICtrl* custom_login_ids_chk = getChild<LLUICtrl>("custom_login_ids_chk");
    LLLineEditor* custom_id0 = getChild<LLLineEditor>("custom_id0");
    LLLineEditor* custom_macid = getChild<LLLineEditor>("custom_macid");

    // Changing these after login is pointless as they will never be saved
    custom_login_ids_chk->setEnabled(false);
    custom_id0->setEnabled(false);
    custom_macid->setEnabled(false);

    // Hide the values to prevent leaking them accidentally while playing
    if (!custom_id0->getValue().asStringRef().empty())
        custom_id0->setValue("[hidden]");
    if (!custom_macid->getValue().asStringRef().empty())
        custom_macid->setValue("[hidden]");
}

LOFloaterExtras::LOFloaterExtras(const LLSD& key)
    : LLFloater(key)
{
}

LOFloaterExtras::~LOFloaterExtras()
{
}

bool LOFloaterExtras::postBuild()
{
    update_labels();

    std::pair<const char*, unsigned> checkboxes[]{
        {"convenience_chk", LO_CONVENIENCE},
        {"bypass_export_perms_chk", LO_BYPASS_EXPORT_PERMS},
        {"enhanced_export_chk", LO_ENHANCED_EXPORT},
        {"anonymize_exports_chk", LO_ANONYMIZE_EXPORTS},
        {"md5_login_chk", LO_MD5_LOGINS}
    };

    for (auto& p : checkboxes)
    {
        const char* ctrl_name = p.first;
        unsigned flag = p.second;

        LLUICtrl* ctrl = getChild<LLUICtrl>(ctrl_name);

        ctrl->setValue(LLSD(lolistorm_check_flag(flag)));

        if ((LO_FEATURE_MASK & flag) == flag)
            ctrl->setEnabled(true);

        ctrl->setCommitCallback([flag](LLUICtrl *ctrl, const LLSD&)
        {
            bool value = ctrl->getValue().asBoolean();

            if (value)
                lolistorm_enable_flag(flag);
            else
                lolistorm_disable_flag(flag);

            gSavedSettings.setU32("LOExtraFeatures", lolistorm_get_flags());
        });
    }

    LLUICtrl* custom_login_ids_chk = getChild<LLUICtrl>("custom_login_ids_chk");
    LLLineEditor* custom_id0 = getChild<LLLineEditor>("custom_id0");
    LLLineEditor* custom_macid = getChild<LLLineEditor>("custom_macid");

    auto update_spoof_window = []()
    {
        auto&& floater_spoof = LLFloaterReg::findTypedInstance<LOFloaterSpoof>("lo_spoof");

        if (floater_spoof)
            floater_spoof->update_labels();
    };

    custom_login_ids_chk->setCommitCallback([update_spoof_window, custom_id0, custom_macid](LLUICtrl *ctrl, const LLSD&)
    {
        bool enabled = ctrl->getValue().asBoolean();

        std::string id0, macid;

        if (enabled)
        {
            id0 = lolistorm_get_custom_id0();
            macid = lolistorm_get_custom_macid();
        }

        lolistorm_set_custom_id0(id0);
        lolistorm_set_custom_macid(macid);

        custom_id0->setValue(id0);
        custom_macid->setValue(macid);

        custom_id0->setEnabled(enabled);
        custom_macid->setEnabled(enabled);

        update_spoof_window();
    });

    custom_id0->setCommitCallback([update_spoof_window](LLUICtrl *ctrl, const LLSD&)
    {
        lolistorm_set_custom_id0(ctrl->getValue());
        update_spoof_window();
    });

    custom_macid->setCommitCallback([update_spoof_window](LLUICtrl *ctrl, const LLSD&)
    {
        lolistorm_set_custom_macid(ctrl->getValue());
        update_spoof_window();
    });

    LLLineEditor* custom_login_background = getChild<LLLineEditor>("custom_login_background");

    custom_login_background->setValue(gSavedSettings.getString("LOCustomLoginBackground"));

    custom_login_background->setCommitCallback([](LLUICtrl *ctrl, const LLSD&)
    {
        gSavedSettings.setString("LOCustomLoginBackground", ctrl->getValue());
        FSPanelLogin::loadLoginPage();
    });

    return true;
}
