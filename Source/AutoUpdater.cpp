

#include "JuceHeader.h"
//#include "jucer_Application.h"
#include "AutoUpdater.h"

//==============================================================================
LatestVersionCheckerAndUpdater::LatestVersionCheckerAndUpdater()
    : Thread ("VersionChecker")
{
}

LatestVersionCheckerAndUpdater::~LatestVersionCheckerAndUpdater()
{
    stopThread (1000);
    clearSingletonInstance();
}

void LatestVersionCheckerAndUpdater::checkForNewVersion (bool showAlerts)
{
    if (! isThreadRunning())
    {
        showAlertWindows = showAlerts;
        startThread (3);
    }
}

//==============================================================================
void LatestVersionCheckerAndUpdater::run()
{
    auto info = VersionInfo::fetchLatestFromUpdateServer();

    if (info == nullptr)
    {
        if (showAlertWindows)
            AlertWindow::showMessageBoxAsync (AlertWindow::WarningIcon,
                                              "Update Server Communication Error",
                                              "Failed to communicate with the SonoBus update server.\n"
                                              "Please try again in a few minutes.\n\n"
                                              "If this problem persists you can download the latest version of SonoBus from beta.sonobus.com");

        return;
    }

    if (! info->isNewerVersionThanCurrent())
    {
        if (showAlertWindows)
            AlertWindow::showMessageBoxAsync (AlertWindow::InfoIcon,
                                              "No New Version Available",
                                              "Your SonoBus version is up to date.");
        return;
    }

    auto osString = []
    {
       #if JUCE_MAC
        return "mac";
       #elif JUCE_WINDOWS
        return "win";
       #elif JUCE_LINUX
        return "linux";
       #else
        jassertfalse;
        return "Unknown";
       #endif
    }();

    String requiredFilename ("sonobus-" + info->versionString + "-" + osString + ".zip");

    for (auto& asset : info->assets)
    {
        if (asset.name == requiredFilename)
        {
            auto versionString = info->versionString;
            auto releaseNotes  = info->releaseNotes;

            MessageManager::callAsync ([this, versionString, releaseNotes, asset]
            {
                askUserAboutNewVersion (versionString, releaseNotes, asset);
            });

            return;
        }
    }

    if (showAlertWindows)
        AlertWindow::showMessageBoxAsync (AlertWindow::WarningIcon,
                                          "Failed to find any new downloads",
                                          "Please try again in a few minutes.");
}

//==============================================================================
class UpdateDialog  : public Component
{
public:
    UpdateDialog (const String& newVersion, const String& releaseNotes)
    {
        titleLabel.setText ("SonoBus version " + newVersion, dontSendNotification);
        titleLabel.setFont ({ 15.0f, Font::bold });
        titleLabel.setJustificationType (Justification::centred);
        addAndMakeVisible (titleLabel);

        contentLabel.setText ("A new version of SonoBus is available - would you like to download it?", dontSendNotification);
        contentLabel.setFont (15.0f);
        contentLabel.setJustificationType (Justification::topLeft);
        addAndMakeVisible (contentLabel);

        releaseNotesLabel.setText ("Release notes:", dontSendNotification);
        releaseNotesLabel.setFont (15.0f);
        releaseNotesLabel.setJustificationType (Justification::topLeft);
        addAndMakeVisible (releaseNotesLabel);

        releaseNotesEditor.setMultiLine (true);
        releaseNotesEditor.setReadOnly (true);
        releaseNotesEditor.setText (releaseNotes);
        addAndMakeVisible (releaseNotesEditor);

        addAndMakeVisible (chooseButton);
        chooseButton.onClick = [this] { exitModalStateWithResult (1); };

        addAndMakeVisible (cancelButton);
        cancelButton.onClick = [this]
        {
            //if (dontAskAgainButton.getToggleState())
            //    getGlobalProperties().setValue (Ids::dontQueryForUpdate.toString(), 1);
            //else
            //    getGlobalProperties().removeValue (Ids::dontQueryForUpdate);

            exitModalStateWithResult (-1);
        };

        //dontAskAgainButton.setToggleState (getGlobalProperties().getValue (Ids::dontQueryForUpdate, {}).isNotEmpty(), dontSendNotification);
        //addAndMakeVisible (dontAskAgainButton);

        appIcon = Drawable::createFromImageData (BinaryData::sonobus_logo_96_png,
                                                  BinaryData::sonobus_logo_96_pngSize);
        lookAndFeelChanged();

        setSize (500, 280);
    }

    void resized() override
    {
        auto b = getLocalBounds().reduced (10);

        auto topSlice = b.removeFromTop (appIconBounds.getHeight())
                         .withTrimmedLeft (appIconBounds.getWidth());

        titleLabel.setBounds (topSlice.removeFromTop (25));
        topSlice.removeFromTop (5);
        contentLabel.setBounds (topSlice.removeFromTop (25));

        auto buttonBounds = b.removeFromBottom (60);
        buttonBounds.removeFromBottom (25);
        chooseButton.setBounds (buttonBounds.removeFromLeft (buttonBounds.getWidth() / 2).reduced (20, 0));
        cancelButton.setBounds (buttonBounds.reduced (20, 0));
        dontAskAgainButton.setBounds (cancelButton.getBounds().withY (cancelButton.getBottom() + 5).withHeight (20));

        releaseNotesEditor.setBounds (b.reduced (0, 10));
    }

    void paint (Graphics& g) override
    {
        
        g.fillAll (Colour(0xff222222));

        if (appIcon != nullptr)
            appIcon->drawWithin (g, appIconBounds.toFloat(),
                                  RectanglePlacement::stretchToFit, 1.0f);
    }

    static std::unique_ptr<DialogWindow> launchDialog (const String& newVersionString,
                                                       const String& releaseNotes)
    {
        DialogWindow::LaunchOptions options;

        options.dialogTitle = "Download SonoBus version " + newVersionString + "?";
        options.resizable = false;

        auto* content = new UpdateDialog (newVersionString, releaseNotes);
        options.content.set (content, true);

        std::unique_ptr<DialogWindow> dialog (options.create());

        content->setParentWindow (dialog.get());
        dialog->enterModalState (true, nullptr, true);

        return dialog;
    }

private:
    void lookAndFeelChanged() override
    {
        //cancelButton.setColour (TextButton::buttonColourId, findColour (secondaryButtonBackgroundColourId));
        releaseNotesEditor.applyFontToAllText (releaseNotesEditor.getFont());
    }

    void setParentWindow (DialogWindow* parent)
    {
        parentWindow = parent;
    }

    void exitModalStateWithResult (int result)
    {
        if (parentWindow != nullptr)
            parentWindow->exitModalState (result);
    }

    Label titleLabel, contentLabel, releaseNotesLabel;
    TextEditor releaseNotesEditor;
    TextButton chooseButton { "Choose Location..." }, cancelButton { "Cancel" };
    ToggleButton dontAskAgainButton { "Don't ask again" };
    std::unique_ptr<Drawable> appIcon;
    Rectangle<int> appIconBounds { 10, 10, 64, 64 };

    DialogWindow* parentWindow = nullptr;
};

void LatestVersionCheckerAndUpdater::askUserForLocationToDownload (const VersionInfo::Asset& asset)
{
    FileChooser chooser ("Please select the location into which you would like to install the new version",
                         //{ getAppSettings().getStoredPath (Ids::jucePath, TargetOS::getThisOS()).get() });
                         { File::getSpecialLocation(File::globalApplicationsDirectory) });                         

    if (chooser.browseForDirectory())
    {
        auto targetFolder = chooser.getResult();

        // By default we will install into 'targetFolder/JUCE', but we should install into
        // 'targetFolder' if that is an existing JUCE directory.
#if 0
        bool willOverwriteJuceFolder = [&targetFolder]
        {
            if (isJUCEFolder (targetFolder))
                return true;

            targetFolder = targetFolder.getChildFile ("SonoBus");

            return isJUCEFolder (targetFolder);
        }();
#endif

        targetFolder = targetFolder.getChildFile ("SonoBus");

        
        auto targetFolderPath = targetFolder.getFullPathName();

#if 0
        if (willOverwriteJuceFolder)
        {
            if (targetFolder.getChildFile (".git").isDirectory())
            {
                AlertWindow::showMessageBoxAsync (AlertWindow::WarningIcon, "Downloading New JUCE Version",
                                                  targetFolderPath + "\n\nis a GIT repository!\n\nYou should use a \"git pull\" to update it to the latest version.");

                return;
            }

            if (! AlertWindow::showOkCancelBox (AlertWindow::WarningIcon, "Overwrite Existing JUCE Folder?",
                                                "Do you want to replace the folder\n\n" + targetFolderPath + "\n\nwith the latest version from juce.com?\n\n"
                                                "This will move the existing folder to " + targetFolderPath + "_old."))
            {
                return;
            }
        }
        else 
#endif
        if (targetFolder.exists())
        {
            if (! AlertWindow::showOkCancelBox (AlertWindow::WarningIcon, "Existing File Or Directory",
                                                "Do you want to move\n\n" + targetFolderPath + "\n\nto\n\n" + targetFolderPath + "_old?"))
            {
                return;
            }
        }

        downloadAndInstall (asset, targetFolder);
    }
}

void LatestVersionCheckerAndUpdater::askUserAboutNewVersion (const String& newVersionString,
                                                             const String& releaseNotes,
                                                             const VersionInfo::Asset& asset)
{
    dialogWindow = UpdateDialog::launchDialog (newVersionString, releaseNotes);

    if (auto* mm = ModalComponentManager::getInstance())
        mm->attachCallback (dialogWindow.get(),
                            ModalCallbackFunction::create ([this, asset] (int result)
                                                           {
                                                               if (result == 1)
                                                                    askUserForLocationToDownload (asset);

                                                                dialogWindow.reset();
                                                            }));
}

//==============================================================================
class DownloadAndInstallThread   : private ThreadWithProgressWindow
{
public:
    DownloadAndInstallThread  (const VersionInfo::Asset& a, const File& t, std::function<void()>&& cb)
        : ThreadWithProgressWindow ("Downloading New Version", true, true),
          asset (a), targetFolder (t), completionCallback (std::move (cb))
    {
        launchThread (3);
    }

private:
    void run() override
    {
        setProgress (-1.0);

        MemoryBlock zipData;
        auto result = download (zipData);

        if (result.wasOk() && ! threadShouldExit())
            result = install (zipData);

        if (result.failed())
            MessageManager::callAsync ([result] { AlertWindow::showMessageBoxAsync (AlertWindow::WarningIcon,
                                                                                    "Installation Failed",
                                                                                    result.getErrorMessage()); });
        else
            MessageManager::callAsync (completionCallback);
    }

    Result download (MemoryBlock& dest)
    {
        setStatusMessage ("Downloading...");

        int statusCode = 0;
        auto inStream = VersionInfo::createInputStreamForAsset (asset, statusCode);

        if (inStream != nullptr && statusCode == 200)
        {
            int64 total = 0;
            MemoryOutputStream mo (dest, true);

            for (;;)
            {
                if (threadShouldExit())
                    return Result::fail ("Cancelled");

                auto written = mo.writeFromInputStream (*inStream, 8192);

                if (written == 0)
                    break;

                total += written;

                setStatusMessage ("Downloading... " + File::descriptionOfSizeInBytes (total));
            }

            return Result::ok();
        }

        return Result::fail ("Failed to download from: " + asset.url);
    }

    Result install (const MemoryBlock& data)
    {
        setStatusMessage ("Installing...");

        MemoryInputStream input (data, false);
        ZipFile zip (input);

        if (zip.getNumEntries() == 0)
            return Result::fail ("The downloaded file was not a valid SonoBus release file!");

        struct ScopedDownloadFolder
        {
            ScopedDownloadFolder (const File& installTargetFolder)
            {
                folder = installTargetFolder.getSiblingFile (installTargetFolder.getFileNameWithoutExtension() + "_download").getNonexistentSibling();
                jassert (folder.createDirectory());
            }

            ~ScopedDownloadFolder()   { folder.deleteRecursively(); }

            File folder;
        };

        ScopedDownloadFolder unzipTarget (targetFolder);

        if (! unzipTarget.folder.isDirectory())
            return Result::fail ("Couldn't create a temporary folder to unzip the new version!");

        auto r = zip.uncompressTo (unzipTarget.folder);

        if (r.failed())
            return r;

        if (threadShouldExit())
            return Result::fail ("Cancelled");

       #if JUCE_LINUX || JUCE_MAC
        r = setFilePermissions (unzipTarget.folder, zip);

        if (r.failed())
            return r;

        if (threadShouldExit())
            return Result::fail ("Cancelled");
       #endif

        if (targetFolder.exists())
        {
            auto oldFolder = targetFolder.getSiblingFile (targetFolder.getFileNameWithoutExtension() + "_old").getNonexistentSibling();

            if (! targetFolder.moveFileTo (oldFolder))
                return Result::fail ("Could not remove the existing folder!\n\n"
                                     "This may happen if you are trying to download into a directory that requires administrator privileges to modify.\n"
                                     "Please select a folder that is writable by the current user.");
        }

        if (! unzipTarget.folder.getChildFile ("SonoBus").moveFileTo (targetFolder))
            return Result::fail ("Could not overwrite the existing folder!\n\n"
                                 "This may happen if you are trying to download into a directory that requires administrator privileges to modify.\n"
                                 "Please select a folder that is writable by the current user.");

        return Result::ok();
    }

    Result setFilePermissions (const File& root, const ZipFile& zip)
    {
        constexpr uint32 executableFlag = (1 << 22);

        for (int i = 0; i < zip.getNumEntries(); ++i)
        {
            auto* entry = zip.getEntry (i);

            if ((entry->externalFileAttributes & executableFlag) != 0 && entry->filename.getLastCharacter() != '/')
            {
                auto exeFile = root.getChildFile (entry->filename);

                if (! exeFile.exists())
                    return Result::fail ("Failed to find executable file when setting permissions " + exeFile.getFileName());

                if (! exeFile.setExecutePermission (true))
                    return Result::fail ("Failed to set executable file permission for " + exeFile.getFileName());
            }
        }

        return Result::ok();
    }

    VersionInfo::Asset asset;
    File targetFolder;
    std::function<void()> completionCallback;
};

void restartProcess (const File& targetFolder)
{
   #if JUCE_MAC || JUCE_LINUX
    #if JUCE_MAC
     auto newProcess = targetFolder.getChildFile ("SonoBus.app").getChildFile ("Contents").getChildFile ("MacOS").getChildFile ("SonoBus");
    #elif JUCE_LINUX
     auto newProcess = targetFolder.getChildFile ("SonoBus");
    #endif

    StringArray command ("/bin/sh", "-c", "while killall -0 SonoBus; do sleep 5; done; " + newProcess.getFullPathName().quoted());
   #elif JUCE_WINDOWS
    auto newProcess = targetFolder.getChildFile ("SonoBus.exe");

    auto command = "cmd.exe /c\"@echo off & for /l %a in (0) do ( tasklist | find \"SonoBus\" >nul & ( if errorlevel 1 ( "
                    + targetFolder.getChildFile ("SonoBus.exe").getFullPathName().quoted() + " & exit /b ) else ( timeout /t 10 >nul ) ) )\"";
   #endif

#if (!JUCE_IOS)
    if (newProcess.existsAsFile())
    {
        ChildProcess restartProcess;
        restartProcess.start (command, 0);

        JUCEApplicationBase::getInstance()->systemRequestedQuit();
        //StandaloneFilterApp::getApp().systemRequestedQuit();
    }
#endif
}

void LatestVersionCheckerAndUpdater::downloadAndInstall (const VersionInfo::Asset& asset, const File& targetFolder)
{
    installer.reset (new DownloadAndInstallThread (asset, targetFolder,
                                                   [this, targetFolder]
                                                   {
                                                       installer.reset();
                                                       restartProcess (targetFolder);
                                                   }));
}

//==============================================================================
JUCE_IMPLEMENT_SINGLETON (LatestVersionCheckerAndUpdater)
