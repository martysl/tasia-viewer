/**
 * @file llfloaterpluginmanager.cpp
 * @brief Plugin Manager UI floater implementation
 *
 * $LicenseInfo:firstyear=2024&license=viewerlgpl$
 * Tasia Viewer Plugin System
 * $/LicenseInfo$
 */

#include "llviewerprecompiledheaders.h"
#include "llfloaterpluginmanager.h"
#include "llpluginmanager.h"
#include "llpluginmanifest.h"
#include "llpluginpermissions.h"

#include "llbutton.h"
#include "llscrolllistctrl.h"
#include "lltextbox.h"
#include "llfloaterreg.h"
#include "lluictrlfactory.h"
#include "llnotificationsutil.h"
#include "llfilepicker.h"
#include "lltrans.h"
#include "llsd.h"
#include "llstring.h"
#include "llfile.h"

LLFloaterPluginManager::LLFloaterPluginManager(const LLSD& key)
    : LLFloater(key)
{
    mAutoScale = true;
}

LLFloaterPluginManager::~LLFloaterPluginManager()
{
}

BOOL LLFloaterPluginManager::postBuild()
{
    // Connect buttons
    getChild<LLButton>("install_btn")->setClickedCallback([this]() { onInstallPlugin(); });
    getChild<LLButton>("remove_btn")->setClickedCallback([this]() { onRemovePlugin(); });
    getChild<LLButton>("enable_btn")->setClickedCallback([this]() { onEnablePlugin(); });
    getChild<LLButton>("disable_btn")->setClickedCallback([this]() { onDisablePlugin(); });
    getChild<LLButton>("reload_btn")->setClickedCallback([this]() { onReloadPlugin(); });
    getChild<LLButton>("permissions_btn")->setClickedCallback([this]() { onPermissions(); });
    getChild<LLButton>("open_folder_btn")->setClickedCallback([this]() { onOpenFolder(); });
    getChild<LLButton>("view_logs_btn")->setClickedCallback([this]() { onViewLogs(); });
    getChild<LLButton>("check_updates_btn")->setClickedCallback([this]() { onCheckUpdates(); });

    // Plugin list selection callback
    getChild<LLScrollListCtrl>("plugin_list")->setCommitCallback([this]() { updateButtons(); });

    refreshList();
    return TRUE;
}

void LLFloaterPluginManager::draw()
{
    if (mNeedsRefresh)
    {
        refreshList();
        mNeedsRefresh = false;
    }
    LLFloater::draw();
}

void LLFloaterPluginManager::onOpen(const LLSD& key)
{
    refreshList();
}

void LLFloaterPluginManager::registerFloater()
{
    LLFloaterReg::add("plugin_manager", "floater_plugin_manager.xml",
                      (LLFloaterBuildFunc)&LLFloaterReg::build<LLFloaterPluginManager>);
}

void LLFloaterPluginManager::refreshList()
{
    auto* list = getChild<LLScrollListCtrl>("plugin_list");
    if (!list) return;

    list->deleteAllItems();

    auto plugins = LLPluginManager::instance().getAllPluginInfos();
    for (const auto& plugin : plugins)
    {
        LLSD row;
        row["id"] = plugin.mId;

        LLSD columns;
        columns["name"] = plugin.mName.empty() ? plugin.mId : plugin.mName;

        // Runtime badge
        std::string runtime_str = runtime_type_to_string(plugin.mRuntime);

        // State
        std::string state_str;
        switch (plugin.mState)
        {
        case LLPluginState::RUNNING:  state_str = "Running"; break;
        case LLPluginState::LOADING:  state_str = "Loading"; break;
        case LLPluginState::ERROR:    state_str = "Error"; break;
        case LLPluginState::DISABLED: state_str = "Disabled"; break;
        default:                      state_str = "Unloaded"; break;
        }

        std::string enabled_str = plugin.mEnabled ? "Yes" : "No";
        if (LLPluginManager::instance().isSafeMode() || LLPluginManager::instance().arePluginsDisabled())
        {
            enabled_str = "Safe Mode";
        }

        LLSD column_values;
        column_values[0] = plugin.mName;
        column_values[1] = plugin.mId;
        column_values[2] = plugin.mVersion;
        column_values[3] = plugin.mAuthor;
        column_values[4] = runtime_str;
        column_values[5] = enabled_str;
        column_values[6] = state_str;
        column_values[7] = plugin.mError;

        row["columns"] = column_values;
        list->addElement(row);
    }

    updateButtons();
}

std::string LLFloaterPluginManager::getSelectedPluginId() const
{
    auto* list = getChild<LLScrollListCtrl>("plugin_list");
    if (!list) return "";

    auto num = list->getAllSelected();
    if (num.empty()) return "";

    return num[0]->getValue().asString();
}

void LLFloaterPluginManager::updateButtons()
{
    std::string id = getSelectedPluginId();
    bool has_selection = !id.empty();

    getChildView("remove_btn")->setEnabled(has_selection);
    getChildView("enable_btn")->setEnabled(has_selection);
    getChildView("disable_btn")->setEnabled(has_selection);
    getChildView("reload_btn")->setEnabled(has_selection);
    getChildView("permissions_btn")->setEnabled(has_selection);
    getChildView("view_logs_btn")->setEnabled(has_selection);
}

void LLFloaterPluginManager::onInstallPlugin()
{
    LLFilePicker& picker = LLFilePicker::instance();
    if (picker.getOpenFile(LLFilePicker::FFLOAD_ALL))
    {
        std::string filename = picker.getFirstFile();
        // TODO: Handle .tasiaplugin packages
        // For now, just note the installation
        LLSD args;
        args["MESSAGE"] = "Plugin installation from file not yet supported.\nPlace plugins in the plugins/ folder.";
        LLNotificationsUtil::add("SystemMessage", args);
    }
}

void LLFloaterPluginManager::onRemovePlugin()
{
    std::string id = getSelectedPluginId();
    if (id.empty()) return;

    LLSD args;
    args["PLUGIN_NAME"] = id;
    LLNotificationsUtil::add("PluginRemoveConfirm", args, [this](const LLSD& notification, const LLSD& response)
    {
        if (LLNotificationsUtil::getSelectedOption(notification, response) == 0)
        {
            LLPluginManager::instance().removePlugin(getSelectedPluginId(), true);
            refreshList();
        }
    });
}

void LLFloaterPluginManager::onEnablePlugin()
{
    std::string id = getSelectedPluginId();
    if (id.empty()) return;

    // Check permissions
    auto* manifest = LLPluginManifestDB::instance().get(id);
    if (manifest && !manifest->getPermissions().empty())
    {
        // Check if version changed (needs re-consent)
        if (LLPluginPermissionStore::instance().versionChanged(id, manifest->getVersion()))
        {
            // Show permission dialog
            LLSD args;
            args["PLUGIN_NAME"] = manifest->getName();
            args["PERMISSIONS"] = manifest->getPermissions().size();
            LLNotificationsUtil::add("PluginPermissionsRequest", args,
                [this, id](const LLSD& notification, const LLSD& response)
            {
                if (LLNotificationsUtil::getSelectedOption(notification, response) == 0)
                {
                    auto* m = LLPluginManifestDB::instance().get(id);
                    if (m)
                    {
                        for (const auto& perm : m->getPermissions())
                        {
                            LLPluginPermissionStore::instance().grant(id, perm.mName, m->getVersion());
                        }
                        LLPluginPermissionStore::instance().save();
                    }
                    LLPluginManager::instance().enablePlugin(id);
                    refreshList();
                }
            });
            return;
        }
    }

    LLPluginManager::instance().enablePlugin(id);
    refreshList();
}

void LLFloaterPluginManager::onDisablePlugin()
{
    std::string id = getSelectedPluginId();
    if (id.empty()) return;

    LLPluginManager::instance().disablePlugin(id);
    refreshList();
}

void LLFloaterPluginManager::onReloadPlugin()
{
    std::string id = getSelectedPluginId();
    if (id.empty()) return;

    LLPluginManager::instance().reloadPlugin(id);
    refreshList();
}

void LLFloaterPluginManager::onPermissions()
{
    std::string id = getSelectedPluginId();
    if (id.empty()) return;

    std::string perm_text;
    auto granted = LLPluginPermissionStore::instance().getGrants(id);
    for (const auto& perm : granted)
    {
        perm_text += "  - " + perm + "\n";
    }

    if (perm_text.empty())
        perm_text = "(none granted)";

    LLSD args;
    args["MESSAGE"] = "Plugin: " + id + "\n\nGranted permissions:\n" + perm_text;
    LLNotificationsUtil::add("SystemMessage", args);
}

void LLFloaterPluginManager::onOpenFolder()
{
    LLPluginManager::openPluginFolder();
}

void LLFloaterPluginManager::onViewLogs()
{
    std::string id = getSelectedPluginId();
    if (id.empty()) return;

    // Open the log directory in file browser
    std::string log_dir = gDirUtilp->getExpandedFilename(LL_PATH_LOGS, "plugins");
    gDirUtilp->openDirectory(log_dir);
}

void LLFloaterPluginManager::onCheckUpdates()
{
    LLSD args;
    args["MESSAGE"] = "Plugin update checking not yet implemented.";
    LLNotificationsUtil::add("SystemMessage", args);
}
