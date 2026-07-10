#include "llviewerprecompiledheaders.h"
#include "lltasiaguard.h"

#include "llagent.h"
#include "llbutton.h"
#include "llcorehttputil.h"
#include "llfloaterreg.h"
#include "llhttpconstants.h"
#include "llnotificationsutil.h"
#include "llsd.h"
#include "llsdserialize.h"
#include "lltextbox.h"
#include "lluicolortable.h"
#include "lluri.h"
#include "llviewercontrol.h"

const std::string LLTasiaGuardFloater::SELF_UNBAN_URL = "https://apps.easierit.org/igrid/self-unban.php";
const std::string LLTasiaGuardFloater::MOM_UUID = "43827618-1993-43d8-bf6b-fc966a943381";

LLTasiaGuardFloater::LLTasiaGuardFloater(const LLSD& seed)
    : LLFloater(seed)
{
    mCommitCallbackRegistrar.add("TasiaGuard.Unban", boost::bind(&LLTasiaGuardFloater::onUnban, this));
}

bool LLTasiaGuardFloater::postBuild()
{
    mIP = getChild<LLLineEditor>("unban_ip");
    mFirstName = getChild<LLLineEditor>("unban_first_name");
    mLastName = getChild<LLLineEditor>("unban_last_name");
    mStatus = getChild<LLTextBox>("unban_status");
    mUnbanBtn = getChild<LLButton>("unban_btn");
    return true;
}

void LLTasiaGuardFloater::onOpen(const LLSD& key)
{
    // Auto-fill IP from viewer's current region connection
    std::string ip = gSavedSettings.getString("ExternalIP");
    if (ip.empty())
    {
        // Fallback: try to detect from agent region
        ip = "auto-detected";
    }

    mIP->setText(ip);
    mIP->setEnabled(false);  // IP is read-only since it's detected

    // Auto-fill avatar name from current agent
    std::string first_name = gAgent.getFirstName();
    std::string last_name = gAgent.getLastName();
    mFirstName->setText(first_name);
    mLastName->setText(last_name);

    setStatus("");
    mUnbanBtn->setEnabled(true);
}

void LLTasiaGuardFloater::setStatus(const std::string& text, bool is_error)
{
    if (mStatus)
    {
        mStatus->setText(text);
        mStatus->setColor(is_error ? LLUIColorTable::instance().getColor("Red") : LLUIColorTable::instance().getColor("LtGray_75"));
    }
}

void LLTasiaGuardFloater::onUnban()
{
    std::string first_name = mFirstName->getText();
    std::string last_name = mLastName->getText();

    if (first_name.empty() || last_name.empty())
    {
        setStatus("Please fill in your avatar name", true);
        return;
    }

    mUnbanBtn->setEnabled(false);
    setStatus("Requesting unban...");

    // Build POST data - matches the PHP form fields
    LLSD post_data;
    post_data["ip"] = gSavedSettings.getString("ExternalIP");
    post_data["uuid"] = MOM_UUID;
    post_data["first_name"] = first_name;
    post_data["last_name"] = last_name;
    post_data["auth_name"] = "Tasia";

    LLCore::HttpRequest::ptr_t http_request(new LLCore::HttpRequest);
    LLCoreHttpUtil::HttpCoroutineAdapter::ptr_t http_adapter(
        new LLCoreHttpUtil::HttpCoroutineAdapter("TasiaGuardUnban", LLCore::HttpRequest::DEFAULT_POLICY_ID));

    LLCore::HttpHeaders::ptr_t http_headers(new LLCore::HttpHeaders);
    http_headers->append(HTTP_OUT_HEADER_CONTENT_TYPE, "application/x-www-form-urlencoded");

    // Encode form data manually
    std::string form_body = "ip=" + LLURI::urlEscape(post_data["ip"].asString())
                          + "&uuid=" + LLURI::urlEscape(post_data["uuid"].asString())
                          + "&first_name=" + LLURI::urlEscape(post_data["first_name"].asString())
                          + "&last_name=" + LLURI::urlEscape(post_data["last_name"].asString())
                          + "&auth_name=" + LLURI::urlEscape(post_data["auth_name"].asString());

    LLCoreHttpUtil::HttpCoroutineAdapter::callback_t callback =
        [this](const LLSD& result)
    {
        LLSD http_results = result[LLCoreHttpUtil::HttpCoroutineAdapter::HTTP_RESULTS];
        LLCore::HttpStatus status = LLCoreHttpUtil::HttpCoroutineAdapter::getStatusFromLLSD(http_results);

        mUnbanBtn->setEnabled(true);

        if (status)
        {
            std::string response_text = result["data"].asString();
            // Try to parse as JSON
            LLSD response;
            if (LLSDSerialize::fromJSON(response_text, response))
            {
                std::string msg = response["message"].asString();
                if (!msg.empty())
                    setStatus(msg);
                else if (response["ok"].asBoolean())
                    setStatus("Unban request submitted successfully!");
                else
                    setStatus("Error: " + response["error"].asString(), true);
            }
            else
            {
                setStatus("Unban request sent. Check connection status.");
            }

            LLNotificationsUtil::add("TasiaGuardUnbanComplete");
        }
        else
        {
            setStatus("Network error: " + status.toString(), true);
        }
    };

    http_adapter->post(SELF_UNBAN_URL, http_headers, form_body, callback);
}

void registerTasiaGuardFloater()
{
    LLFloaterReg::add("tasiaguard", "floater_tasiaguard.xml",
        (LLFloaterBuildFunc)&LLFloaterReg::build<LLTasiaGuardFloater>);
}
