/**
 * @file llfloaterpluginmanager.h
 * @brief Plugin Manager UI floater
 *
 * $LicenseInfo:firstyear=2024&license=viewerlgpl$
 * Tasia Viewer Plugin System
 * $/LicenseInfo$
 */

#ifndef LL_LLFLOATERPLUGINMANAGER_H
#define LL_LLFLOATERPLUGINMANAGER_H

#include "llfloater.h"
#include "llsd.h"
#include "llpluginmanager.h"

class LLFloaterPluginManager : public LLFloater
{
public:
    LLFloaterPluginManager(const LLSD& key);
    ~LLFloaterPluginManager() override;

    /*virtual*/ BOOL postBuild() override;
    /*virtual*/ void draw() override;
    /*virtual*/ void onOpen(const LLSD& key) override;

    static void registerFloater();

    /// Refresh the plugin list display
    void refreshList();

private:
    /// Button handlers
    void onInstallPlugin();
    void onRemovePlugin();
    void onEnablePlugin();
    void onDisablePlugin();
    void onReloadPlugin();
    void onPermissions();
    void onOpenFolder();
    void onViewLogs();
    void onCheckUpdates();

    /// Get selected plugin ID
    std::string getSelectedPluginId() const;

    void updateButtons();

    LLSD mPluginList; // current list data
    bool mNeedsRefresh = true;
};

#endif
