// SPDX-License-Identifier: GPLv3-or-later
// Copyright (C) 2020 Jesse Chappell

/*
  ==============================================================================

   This file is part of the JUCE library.
   Copyright (c) 2017 - ROLI Ltd.

   JUCE is an open source library subject to commercial or open-source
   licensing.

   By using JUCE, you agree to the terms of both the JUCE 5 End-User License
   Agreement and JUCE 5 Privacy Policy (both updated and effective as of the
   27th April 2017).

   End User License Agreement: www.juce.com/juce-5-licence
   Privacy Policy: www.juce.com/juce-5-privacy-policy

   Or: You may also use this code under the terms of the GPL v3 (see
   www.gnu.org/licenses).

   JUCE IS PROVIDED "AS IS" WITHOUT ANY WARRANTY, AND ALL WARRANTIES, WHETHER
   EXPRESSED OR IMPLIED, INCLUDING MERCHANTABILITY AND FITNESS FOR PURPOSE, ARE
   DISCLAIMED.

  ==============================================================================
*/

// needed for crappy windows
#define NOMINMAX


#include "JuceHeader.h"

#include "juce_core/system/juce_TargetPlatform.h"
#include "juce_audio_plugin_client/utility/juce_CheckSettingMacros.h"

#if !JUCE_LINUX
#include "juce_audio_plugin_client/utility/juce_IncludeSystemHeaders.h"
#include "juce_audio_plugin_client/utility/juce_IncludeModuleHeaders.h"
#include "juce_audio_plugin_client/utility/juce_FakeMouseMoveGenerator.h"
#include "juce_audio_plugin_client/utility/juce_WindowsHooks.h"
#endif

#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_gui_extra/juce_gui_extra.h>
#include <juce_audio_utils/juce_audio_utils.h>


// You can set this flag in your build if you need to specify a different
// standalone JUCEApplication class for your app to use. If you don't
// set it then by default we'll just create a simple one as below.
//#if ! JUCE_USE_CUSTOM_PLUGIN_STANDALONE_APP

extern juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter();

#include "SonoStandaloneFilterWindow.h"
#include "SonoLookAndFeel.h"

#include "SonobusPluginEditor.h"

#if JUCE_ANDROID
#include "android/SonoBusActivity.h"

#if JUCE_USE_ANDROID_OPENSLES || JUCE_USE_ANDROID_OBOE
 #include "juce_audio_devices/native/juce_android_HighPerformanceAudioHelpers.h"
#endif

#endif

namespace juce
{

//==============================================================================
class SonobusStandaloneFilterApp  : public JUCEApplication, public Timer
{
public:
    SonobusStandaloneFilterApp()
    {
        PluginHostType::jucePlugInClientCurrentWrapperType = AudioProcessor::wrapperType_Standalone;

        PropertiesFile::Options options;

        options.applicationName     = getApplicationName();
        options.filenameSuffix      = ".settings";
        options.osxLibrarySubFolder = "Application Support/" + getApplicationName();
       #if JUCE_LINUX
        options.folderName          = "~/.config";
       #else
        options.folderName          = "";
       #endif

        appProperties.setStorageParameters (options);

        LookAndFeel::setDefaultLookAndFeel(&sonoLNF);
    }

    ~SonobusStandaloneFilterApp()
    {
#if JUCE_ANDROID && JUCE_OPENGL
        detachGL();
#endif
    }

    const String getApplicationName() override              { return JucePlugin_Name; }
    const String getApplicationVersion() override           { return JucePlugin_VersionString; }
    bool moreThanOneInstanceAllowed() override              { return true; }

    SonoLookAndFeel  sonoLNF;

    
    virtual StandaloneFilterWindow* createWindow()
    {
       #ifdef JucePlugin_PreferredChannelConfigurations
        StandalonePluginHolder::PluginInOuts channels[] = { JucePlugin_PreferredChannelConfigurations };
       #endif

        AudioDeviceManager::AudioDeviceSetup setupOptions;
        setupOptions.sampleRate = 48000;
#if JUCE_MAC
        setupOptions.bufferSize = 128;
#elif JUCE_ANDROID
        setupOptions.bufferSize = 192;
        setupOptions.sampleRate = AndroidHighPerformanceAudioHelpers::getNativeSampleRate();
#else
        setupOptions.bufferSize = 256;
#endif

        File settingsFile = appProperties.getStorageParameters().getDefaultFile();
        File crashSentinelFile = settingsFile.getSiblingFile("SENTINEL");
        if (crashSentinelFile.existsAsFile()) {
            DBG("CRASH SENTINEL STILL EXISTS, moving old settings away!");
            File oldfile = settingsFile.getSiblingFile(String("POSSIBLY_BAD_") + Time::getCurrentTime().formatted("%Y-%m-%d_%H.%M.%S"));
            settingsFile.moveFileTo(oldfile);
            AlertWindow::showMessageBoxAsync (AlertWindow::WarningIcon,
                                              TRANS("Crashed Last Time"),
                                              TRANS("Looks like you crashed on launch last time, restoring default settings!"));
        }
        else {
            crashSentinelFile.create();
        }

        auto wind = new StandaloneFilterWindow (getApplicationName(),
                                           LookAndFeel::getDefaultLookAndFeel().findColour (ResizableWindow::backgroundColourId),
                                           appProperties.getUserSettings(),
                                           false, {}, &setupOptions
                                          #ifdef JucePlugin_PreferredChannelConfigurations
                                           , juce::Array<StandalonePluginHolder::PluginInOuts> (channels, juce::numElementsInArray (channels))
                                          #else
                                           , {}
                                          #endif
                                          //#if JUCE_DONT_AUTO_OPEN_MIDI_DEVICES_ON_MOBILE
                                           , false
                                          //#endif
                                           );

        // if we got here, we didn't crash on initialization!
        crashSentinelFile.deleteFile();

        return wind;
    }

    //==============================================================================
    void initialise (const String&) override
    {
        mainWindow.reset (createWindow());

       #if JUCE_STANDALONE_FILTER_WINDOW_USE_KIOSK_MODE
        Desktop::getInstance().setKioskModeComponent (mainWindow.get(), false);
       #endif

        mainWindow->setVisible (true);
        
        Desktop::getInstance().setScreenSaverEnabled(false);


        if (auto * sonoproc = dynamic_cast<SonobusAudioProcessor*>(mainWindow->pluginHolder->processor.get())) {
            if (sonoproc->hasEditor()) {
                if (auto * sonoeditor = dynamic_cast<SonobusAudioProcessorEditor*>(sonoproc->createEditorIfNeeded())) {
                    sonoeditor->saveSettingsIfNeeded = [this]() {
                        mainWindow->pluginHolder->savePluginState();
                    };
                }
            }
        }

#if JUCE_ANDROID && JUCE_OPENGL
        attachGL();
#endif

#if JUCE_MAC
        disableAppNap();
#endif

#if JUCE_ANDROID
        startTimer(500);
#endif

    }

    void timerCallback() override
    {
#if JUCE_ANDROID
        DBG("setting foreground service active");
        setAndroidForegroundServiceActive(true);
        stopTimer();
#endif
    }

    void shutdown() override
    {
        DBG("shutdown");
        if (mainWindow.get() != nullptr)
            mainWindow->pluginHolder->savePluginState();

#if JUCE_ANDROID
        setAndroidForegroundServiceActive(false);
  #if JUCE_OPENGL
        detachGL();
  #endif
#endif

        mainWindow = nullptr;
        appProperties.saveIfNeeded();

    }
    
    void urlOpened(const URL & url) override {
        DBG("Url opened: " << url.toString(true));
        
        if (mainWindow.get() != nullptr) {
            
            if (auto * sonoproc = dynamic_cast<SonobusAudioProcessor*>(mainWindow->pluginHolder->processor.get())) {
                if (sonoproc->hasEditor()) {
                    if (auto * sonoeditor = dynamic_cast<SonobusAudioProcessorEditor*>(sonoproc->createEditorIfNeeded())) {
                        sonoeditor->handleURL(url.toString(true));
                        mainWindow->toFront(true);
                    }
                }
            }
        }        
    }
    
    void anotherInstanceStarted (const String& url) override    {
        
        DBG("Url handled from another instance: " << url);
        
        if (mainWindow.get() != nullptr) {
            
            if (auto * sonoproc = dynamic_cast<SonobusAudioProcessor*>(mainWindow->pluginHolder->processor.get())) {
                if (sonoproc->hasEditor()) {
                    if (auto * sonoeditor = dynamic_cast<SonobusAudioProcessorEditor*>(sonoproc->createEditorIfNeeded())) {
                        sonoeditor->handleURL(url);
                        mainWindow->toFront(true);
                    }
                }
            }
        }
    }
        
    void suspended() override
    {
        DBG("suspended");
        if (mainWindow.get() != nullptr) {
            mainWindow->pluginHolder->savePluginState();

            if (auto * sonoproc = dynamic_cast<SonobusAudioProcessor*>(mainWindow->pluginHolder->processor.get())) {
                if (sonoproc->getNumberRemotePeers() == 0 && !mainWindow->pluginHolder->isInterAppAudioConnected()) {
                    // shutdown audio engine
                    DBG("no connections shutting down audio");
                    mainWindow->getDeviceManager().closeAudioDevice();

#if JUCE_ANDROID
                    setAndroidForegroundServiceActive(false);
#endif
                }
                else {
#if JUCE_ANDROID
                    setAndroidForegroundServiceActive(true);
#endif
                }
            }

        }
        
        appProperties.saveIfNeeded();


        
        Desktop::getInstance().setScreenSaverEnabled(true);        
    }

    void setAndroidForegroundServiceActive(bool flag)
    {
    #if JUCE_ANDROID
        LocalRef<jobject> activity (getMainActivity());

        if (activity != nullptr) {
            getEnv()->CallVoidMethod(activity.get(), SonoBusActivity.setForegroundServiceActive, flag);
        }
    #endif
    }


    void resumed() override
    {
        Desktop::getInstance().setScreenSaverEnabled(false);
        
#if JUCE_STANDALONE_FILTER_WINDOW_USE_KIOSK_MODE
        Desktop::getInstance().setKioskModeComponent (nullptr, false);
        Desktop::getInstance().setKioskModeComponent (mainWindow.get(), false);
#endif

        if (auto * dev = mainWindow->getDeviceManager().getCurrentAudioDevice()) {
            if (!dev->isPlaying()) {
                DBG("dev not playing, restarting");
                mainWindow->getDeviceManager().restartLastAudioDevice();
            }
        }
        else {
            DBG("was not actve: restarting");
            mainWindow->getDeviceManager().restartLastAudioDevice();
        }

#if JUCE_ANDROID
        setAndroidForegroundServiceActive(true);
#endif

    }
    
    //==============================================================================
    void systemRequestedQuit() override
    {
        DBG("Requested quit");
        if (mainWindow.get() != nullptr) {
            mainWindow->pluginHolder->savePluginState();

        }

        appProperties.saveIfNeeded();

        
        if (mainWindow.get() != nullptr) {
            if (auto * editor = mainWindow->getEditor()) {
                if (auto * sonoeditor = dynamic_cast<SonobusAudioProcessorEditor*>(editor)) {
                    if (!sonoeditor->requestedQuit()) {
                        // they'll handle it
                        return;
                    }
                }
            }
        }
        
        if (ModalComponentManager::getInstance()->cancelAllModalComponents())
        {
            Timer::callAfterDelay (100, []()
            {
                if (auto app = JUCEApplicationBase::getInstance())
                    app->systemRequestedQuit();
            });
        }
        else
        {
            quit();
        }
    }

    void memoryWarningReceived()  override
    {
        DBG("Memory warning");
    }

    void attachGL()
    {
        if (mainWindow) {
#if JUCE_OPENGL  && JUCE_ANDROID
            DBG("activating GL context");
            openGLContext.makeActive();
            DBG("OPEN GL attaching to " << (long) mainWindow.get());
            openGLContext.attachTo (*mainWindow);
#endif
        }
    }

    void detachGL()
    {

    #if JUCE_OPENGL && JUCE_ANDROID
        DBG("About to detach opengl context");
        if (openGLContext.isAttached()) {
            openGLContext.detach();
            DBG("About to deactivate opengl context");
            openGLContext.deactivateCurrentContext();
            DBG("done detach opengl context");
        }
    #endif
    }

protected:

#if JUCE_OPENGL
    OpenGLContext openGLContext;
#endif

    ApplicationProperties appProperties;
    std::unique_ptr<StandaloneFilterWindow> mainWindow;

};

} // namespace juce

#if JucePlugin_Build_Standalone && JUCE_IOS

using namespace juce;

bool JUCE_CALLTYPE juce_isInterAppAudioConnected()
{
    if (auto holder = StandalonePluginHolder::getInstance())
        return holder->isInterAppAudioConnected();

    return false;
}

void JUCE_CALLTYPE juce_switchToHostApplication()
{
    if (auto holder = StandalonePluginHolder::getInstance())
        holder->switchToHostApplication();
}

#if JUCE_MODULE_AVAILABLE_juce_gui_basics
Image JUCE_CALLTYPE juce_getIAAHostIcon (int size)
{
    if (auto holder = StandalonePluginHolder::getInstance())
        return holder->getIAAHostIcon (size);

    return Image();
}
#endif
#endif

START_JUCE_APPLICATION (SonobusStandaloneFilterApp);

//#endif
