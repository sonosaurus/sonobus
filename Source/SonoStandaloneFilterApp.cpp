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

#include "../JuceLibraryCode/JuceHeader.h"

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

#include "DebugLogC.h"

// You can set this flag in your build if you need to specify a different
// standalone JUCEApplication class for your app to use. If you don't
// set it then by default we'll just create a simple one as below.
//#if ! JUCE_USE_CUSTOM_PLUGIN_STANDALONE_APP

extern juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter();

#include "SonoStandaloneFilterWindow.h"
#include "SonoLookAndFeel.h"

namespace juce
{

//==============================================================================
class StandaloneFilterApp  : public JUCEApplication
{
public:
    StandaloneFilterApp()
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

    const String getApplicationName() override              { return JucePlugin_Name; }
    const String getApplicationVersion() override           { return JucePlugin_VersionString; }
    bool moreThanOneInstanceAllowed() override              { return true; }
    void anotherInstanceStarted (const String&) override    {}

    SonoLookAndFeel  sonoLNF;

    
    virtual StandaloneFilterWindow* createWindow()
    {
       #ifdef JucePlugin_PreferredChannelConfigurations
        StandalonePluginHolder::PluginInOuts channels[] = { JucePlugin_PreferredChannelConfigurations };
       #endif

        AudioDeviceManager::AudioDeviceSetup setupOptions;
        setupOptions.bufferSize = 256;
        
        return new StandaloneFilterWindow (getApplicationName(),
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
    }

    void shutdown() override
    {
        DebugLogC("shutdown");
        if (mainWindow.get() != nullptr)
            mainWindow->pluginHolder->savePluginState();
        
        mainWindow = nullptr;
        appProperties.saveIfNeeded();
    }

    void suspended() override
    {
        DebugLogC("suspended");
        if (mainWindow.get() != nullptr) {
            mainWindow->pluginHolder->savePluginState();

            if (auto * sonoproc = dynamic_cast<SonobusAudioProcessor*>(mainWindow->pluginHolder->processor.get())) {
                if (sonoproc->getNumberRemotePeers() == 0 && !mainWindow->pluginHolder->isInterAppAudioConnected()) {
                    // shutdown audio engine
                    DebugLogC("no connections shutting down audio");
                    mainWindow->getDeviceManager().closeAudioDevice();
                }
            }

        }
        
        appProperties.saveIfNeeded();


        
        Desktop::getInstance().setScreenSaverEnabled(true);        
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
                DebugLogC("dev not playing, restarting");
                mainWindow->getDeviceManager().restartLastAudioDevice();
            }
        }
        else {
            DebugLogC("was not actve: restarting");
            mainWindow->getDeviceManager().restartLastAudioDevice();
        }

    }
    
    //==============================================================================
    void systemRequestedQuit() override
    {
        DebugLogC("Requested quit");
        if (mainWindow.get() != nullptr)
            mainWindow->pluginHolder->savePluginState();

        appProperties.saveIfNeeded();

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
        DebugLogC("Memory warning");
    }
    
protected:
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

START_JUCE_APPLICATION (StandaloneFilterApp);

//#endif
