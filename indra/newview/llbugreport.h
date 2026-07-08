#ifndef LL_BUGREPORT_H
#define LL_BUGREPORT_H

#include "llfloater.h"
#include "lllineeditor.h"
#include "llcombobox.h"
#include "lltexteditor.h"

class LLBugReportFloater : public LLFloater
{
public:
    LLBugReportFloater(const LLSD& seed);
    ~LLBugReportFloater() = default;

    bool postBuild() override;
    void onOpen(const LLSD& key) override;

private:
    void onSubmit();
    void onCancel();
    void setStatus(const std::string& text, bool is_error = false);

    LLLineEditor* mTitle;
    LLComboBox* mCategory;
    LLTextEditor* mDetails;
    LLTextBox* mStatus;
    LLButton* mSubmitBtn;

    static const std::string API_BASE;
    static const std::string API_TOKEN;
};

#endif
