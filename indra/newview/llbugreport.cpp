#include "llviewerprecompiledheaders.h"
#include "llbugreport.h"

#include "llbutton.h"
#include "llcorehttputil.h"
#include "llcoros.h"
#include "llfloaterreg.h"
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

static void submitBugCoro(std::string url, LLSD body)
{
    LLCore::HttpRequest::ptr_t httpRequest(new LLCore::HttpRequest);
    LLCoreHttpUtil::HttpCoroutineAdapter::ptr_t httpAdapter(
        new LLCoreHttpUtil::HttpCoroutineAdapter("BugReportSubmit", LLCore::HttpRequest::DEFAULT_POLICY_ID));
    LLCore::HttpHeaders::ptr_t httpHeaders(new LLCore::HttpHeaders);
    LLCore::HttpOptions::ptr_t httpOptions(new LLCore::HttpOptions);

    httpHeaders->append("Content-Type", "application/json");
    httpOptions->setTimeout(15);

    LLSD result = httpAdapter->postJsonAndSuspend(httpRequest, url, body, httpOptions, httpHeaders);

    LLSD http_results = result[LLCoreHttpUtil::HttpCoroutineAdapter::HTTP_RESULTS];
    LLCore::HttpStatus status = LLCoreHttpUtil::HttpCoroutineAdapter::getStatusFromLLSD(http_results);

    if (status)
    {
        LLNotificationsUtil::add("BugReportSubmitted");
    }
    else
    {
        LL_WARNS("BugReport") << "Submission failed" << LL_ENDL;
        LLNotificationsUtil::add("BugReportSubmitted");
    }
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

    std::string cat_slug = mCategory->getValue().asString();
    LLSD cat_map;
    cat_map["bug-reports"] = 2;
    cat_map["feature-requests"] = 1;
    cat_map["support"] = 3;
    cat_map["other"] = 4;
    S32 board_id = cat_map[cat_slug].asInteger();

    LLSD body;
    body["title"] = title;
    body["details"] = details;
    body["board_id"] = board_id;

    std::string url = API_BASE + "&action=create";

    LLCoros::instance().launch("BugReportSubmitCoro",
        boost::bind(&submitBugCoro, url, body));

    setStatus("Submitted.");
    mSubmitBtn->setEnabled(true);
}
