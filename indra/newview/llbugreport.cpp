#include "llviewerprecompiledheaders.h"
#include "llbugreport.h"
#include "llbutton.h"
#include "llcorehttputil.h"
#include "llfloaterreg.h"
#include "llhttpconstants.h"
#include "llnotificationsutil.h"
#include "llsd.h"
#include "llsdserialize.h"
#include "lltextbox.h"
#include "lluicolortable.h"
#include "llviewercontrol.h"

const std::string LLBugReportFloater::API_BASE = "https://apps.easierit.org/igrid/feedback/index.php?page=api";
const std::string LLBugReportFloater::API_TOKEN = "247247b433de740a27e9cb9723947e6b";

LLBugReportFloater::LLBugReportFloater(const LLSD& seed)
    : LLFloater(seed)
    , mTitle(nullptr)
    , mCategory(nullptr)
    , mDetails(nullptr)
    , mStatus(nullptr)
    , mSubmitBtn(nullptr)
{
    mCommitCallbackRegistrar.add("BugReport.Submit", boost::bind(&LLBugReportFloater::onSubmit, this));
    mCommitCallbackRegistrar.add("BugReport.Cancel", boost::bind(&LLBugReportFloater::onCancel, this));
}

bool LLBugReportFloater::postBuild()
{
    mTitle = getChild<LLLineEditor>("bug_title");
    mCategory = getChild<LLComboBox>("bug_category");
    mDetails = getChild<LLTextEditor>("bug_details");
    mStatus = getChild<LLTextBox>("status_text");
    mSubmitBtn = getChild<LLButton>("submit_btn");
    return true;
}

void LLBugReportFloater::onOpen(const LLSD& key)
{
    mTitle->setText("");
    mCategory->setCurrentByIndex(0);
    mDetails->setText("");
    setStatus("");
    mSubmitBtn->setEnabled(true);
}

void LLBugReportFloater::setStatus(const std::string& text, bool is_error)
{
    if (mStatus)
    {
        mStatus->setText(text);
        mStatus->setColor(is_error ? LLUIColorTable::instance().getColor("Red") : LLUIColorTable::instance().getColor("LtGray_75"));
    }
}

void LLBugReportFloater::onCancel()
{
    closeFloater();
}

void LLBugReportFloater::onSubmit()
{
    std::string title = mTitle->getText();
    if (title.empty())
    {
        setStatus("Please enter a title", true);
        return;
    }

    std::string details = mDetails->getText();
    if (details.empty())
    {
        setStatus("Please enter details", true);
        return;
    }

    mSubmitBtn->setEnabled(false);
    setStatus("Submitting...");

    // Map combo item name to slug
    std::string cat_slug = mCategory->getValue().asString();
    LLSD cat_map;
    cat_map["bug-reports"] = 2;
    cat_map["feature-requests"] = 1;
    cat_map["support"] = 3;
    cat_map["other"] = 4;
    S32 board_id = cat_map[cat_slug].asInteger();

    // Build POST body
    LLSD body;
    body["title"] = title;
    body["details"] = details;
    body["board_id"] = board_id;

    LLSD headers;
    headers["Content-Type"] = "application/json";
    headers["X-API-Key"] = API_TOKEN;

    LLCore::HttpRequest::ptr_t http_request(new LLCore::HttpRequest);
    LLCoreHttpUtil::HttpCoroutineAdapter::ptr_t http_adapter(
        new LLCoreHttpUtil::HttpCoroutineAdapter("BugReportSubmit", LLCore::HttpRequest::DEFAULT_POLICY_ID));

    LLSD post_data = body;
    std::string url = API_BASE + "&action=create";

    // Async POST
    LLCore::HttpHeaders::ptr_t http_headers(new LLCore::HttpHeaders);
    http_headers->append(HTTP_OUT_HEADER_CONTENT_TYPE, "application/json");
    http_headers->append("X-API-Key", API_TOKEN);

    LLCoreHttpUtil::HttpCoroutineAdapter::callback_t callback =
        [this](const LLSD& result)
    {
        LLSD http_results = result[LLCoreHttpUtil::HttpCoroutineAdapter::HTTP_RESULTS];
        LLCore::HttpStatus status = LLCoreHttpUtil::HttpCoroutineAdapter::getStatusFromLLSD(http_results);

        if (status)
        {
            LLSD response = result["data"];
            if (response["ok"].asBoolean())
            {
                setStatus("Submitted successfully! ID: " + response["id"].asString());
                mSubmitBtn->setEnabled(true);
                LLNotificationsUtil::add("BugReportSubmitted");
            }
            else
            {
                setStatus("Error: " + response["error"].asString(), true);
                mSubmitBtn->setEnabled(true);
            }
        }
        else
        {
            setStatus("Network error: " + status.toString(), true);
            mSubmitBtn->setEnabled(true);
        }
    };

    http_adapter->post(headers, url, post_data, callback);
}

// Register the class
void registerBugReportFloater()
{
    LLFloaterReg::add("bug_report", "floater_bug_report.xml",
        (LLFloaterBuildFunc)&LLFloaterReg::build<LLBugReportFloater>);
}
