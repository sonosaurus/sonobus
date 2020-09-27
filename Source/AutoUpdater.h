// SPDX-License-Identifier: GPLv3-or-later
// Copyright (C) 2020 Jesse Chappell



#pragma once

#include "VersionInfo.h"

class DownloadAndInstallThread;

class LatestVersionCheckerAndUpdater   : public DeletedAtShutdown,
                                         private Thread
{
public:
    LatestVersionCheckerAndUpdater();
    ~LatestVersionCheckerAndUpdater() override;

    void checkForNewVersion (bool showAlerts);

    //==============================================================================
    JUCE_DECLARE_SINGLETON_SINGLETHREADED_MINIMAL (LatestVersionCheckerAndUpdater)

private:
    //==============================================================================
    void run() override;
    void askUserAboutNewVersion (const String&, const String&, const VersionInfo::Asset&);
    void askUserForLocationToDownload (const VersionInfo::Asset&);
    void downloadAndInstall (const VersionInfo::Asset&, const File&);

    //==============================================================================
    bool showAlertWindows = false;

    std::unique_ptr<DownloadAndInstallThread> installer;
    std::unique_ptr<Component> dialogWindow;
};
