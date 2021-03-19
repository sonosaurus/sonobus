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

//#if JUCE_MODULE_AVAILABLE_juce_audio_plugin_client
//extern juce::AudioProcessor* JUCE_API JUCE_CALLTYPE createPluginFilterOfType (juce::AudioProcessor::WrapperType type);
//#endif


#include "CrossPlatformUtils.h"

// HACK
#include "SonobusPluginEditor.h"

#include <limits>
#include <algorithm>

namespace juce
{

//==============================================================================
/**
    An object that creates and plays a standalone instance of an AudioProcessor.

    The object will create your processor using the same createPluginFilter()
    function that the other plugin wrappers use, and will run it through the
    computer's audio/MIDI devices using AudioDeviceManager and AudioProcessorPlayer.

    @tags{Audio}
*/
class StandalonePluginHolder    : private AudioIODeviceCallback,
                                  private Timer
{
public:
    //==============================================================================
    /** Structure used for the number of inputs and outputs. */
    struct PluginInOuts   { short numIns, numOuts; };

    //==============================================================================
    /** Creates an instance of the default plugin.

        The settings object can be a PropertySet that the class should use to store its
        settings - the takeOwnershipOfSettings indicates whether this object will delete
        the settings automatically when no longer needed. The settings can also be nullptr.

        A default device name can be passed in.

        Preferably a complete setup options object can be used, which takes precedence over
        the preferredDefaultDeviceName and allows you to select the input & output device names,
        sample rate, buffer size etc.

        In all instances, the settingsToUse will take precedence over the "preferred" options if not null.
    */
    StandalonePluginHolder (PropertySet* settingsToUse,
                            bool takeOwnershipOfSettings = true,
                            const String& preferredDefaultDeviceName = String(),
                            const AudioDeviceManager::AudioDeviceSetup* preferredSetupOptions = nullptr,
                            const Array<PluginInOuts>& channels = Array<PluginInOuts>(),
                           #if JUCE_ANDROID || JUCE_IOS
                            bool shouldAutoOpenMidiDevices = true
                           #else
                            bool shouldAutoOpenMidiDevices = false
                           #endif
                            )

        : settings (settingsToUse, takeOwnershipOfSettings),
          channelConfiguration (channels),
#if JUCE_IOS
          shouldMuteInput (! isInterAppAudioConnected()),
#else
          shouldMuteInput (var(false)),    
#endif
          autoOpenMidiDevices (shouldAutoOpenMidiDevices),
          shouldOverrideSampleRate (var(true)),
          shouldCheckForNewVersion (var(true))
    {
        createPlugin();

        auto inChannels = (channelConfiguration.size() > 0 ? channelConfiguration[0].numIns
                                                           : processor->getMainBusNumInputChannels());

        if (preferredSetupOptions != nullptr)
            options.reset (new AudioDeviceManager::AudioDeviceSetup (*preferredSetupOptions));

        auto audioInputRequired = (inChannels > 0);

        if (audioInputRequired && RuntimePermissions::isRequired (RuntimePermissions::recordAudio)
            && ! RuntimePermissions::isGranted (RuntimePermissions::recordAudio))
            RuntimePermissions::request (RuntimePermissions::recordAudio,
                                         [this, preferredDefaultDeviceName] (bool granted) { init (granted, preferredDefaultDeviceName); });
        else
            init (audioInputRequired, preferredDefaultDeviceName);
    }

    void init (bool enableAudioInput, const String& preferredDefaultDeviceName)
    {
#if JUCE_WINDOWS
        // use ASIO as default.. the options will override
        deviceManager.setCurrentAudioDeviceType("ASIO", false);
#endif
        setupAudioDevices (enableAudioInput, preferredDefaultDeviceName, options.get());
        reloadPluginState();
        startPlaying();

       if (autoOpenMidiDevices)
           startTimer (500);
    }

    virtual ~StandalonePluginHolder()
    {
        stopTimer();

        deletePlugin();
        shutDownAudioDevices();
    }

    //==============================================================================
    virtual void createPlugin()
    {
      #if JUCE_MODULE_AVAILABLE_juce_audio_plugin_client
        processor.reset (::createPluginFilterOfType (AudioProcessor::wrapperType_Standalone));
      #else
        AudioProcessor::setTypeOfNextNewPlugin (AudioProcessor::wrapperType_Standalone);
        processor.reset (createPluginFilter());
        AudioProcessor::setTypeOfNextNewPlugin (AudioProcessor::wrapperType_Undefined);
      #endif
        jassert (processor != nullptr); // Your createPluginFilter() function must return a valid object!

        processor->disableNonMainBuses();

        processor->setRateAndBufferSizeDetails (48000, 256);

        int inChannels = (channelConfiguration.size() > 0 ? channelConfiguration[0].numIns
                                                          : processor->getMainBusNumInputChannels());

        int outChannels = (channelConfiguration.size() > 0 ? channelConfiguration[0].numOuts
                                                           : processor->getMainBusNumOutputChannels());

#if (JUCE_IOS || JUCE_ANDROID)
        processorHasPotentialFeedbackLoop = (inChannels > 0 && outChannels > 0);
#endif
    }

    virtual void deletePlugin()
    {
        stopPlaying();
        processor = nullptr;
    }

    static String getFilePatterns (const String& fileSuffix)
    {
        if (fileSuffix.isEmpty())
            return {};

        return (fileSuffix.startsWithChar ('.') ? "*" : "*.") + fileSuffix;
    }

    //==============================================================================
    Value& getMuteInputValue()                           { return shouldMuteInput; }
    bool getProcessorHasPotentialFeedbackLoop() const    { return processorHasPotentialFeedbackLoop; }

    Value& getShouldOverrideSampleRateValue()                           { return shouldOverrideSampleRate; }
    Value& getShouldCheckForNewVersionValue()                           { return shouldCheckForNewVersion; }

    StringArray& getRecentSetupFiles()                           { return recentSetupFiles; }
    String& getLastRecentsFolder()                           { return lastRecentsSetupFolder; }

    
    //==============================================================================
    File getLastFile() const
    {
        File f;

        if (settings != nullptr)
            f = File (settings->getValue ("lastStateFile"));

        if (f == File())
            f = File::getSpecialLocation (File::userDocumentsDirectory);

        return f;
    }

    void setLastFile (const FileChooser& fc)
    {
        if (settings != nullptr)
            settings->setValue ("lastStateFile", fc.getResult().getFullPathName());
    }

    /** Pops up a dialog letting the user save the processor's state to a file. */
    void askUserToSaveState (const String& fileSuffix = String())
    {
       #if JUCE_MODAL_LOOPS_PERMITTED
        FileChooser fc (TRANS("Save current state"), getLastFile(), getFilePatterns (fileSuffix));

        if (fc.browseForFileToSave (true))
        {
            setLastFile (fc);

            MemoryBlock data;
            processor->getStateInformation (data);

            if (! fc.getResult().replaceWithData (data.getData(), data.getSize()))
                AlertWindow::showMessageBoxAsync (AlertWindow::WarningIcon,
                                                  TRANS("Error while saving"),
                                                  TRANS("Couldn't write to the specified file!"));
        }
       #else
        ignoreUnused (fileSuffix);
       #endif
    }

    /** Pops up a dialog letting the user re-load the processor's state from a file. */
    void askUserToLoadState (const String& fileSuffix = String())
    {
       #if JUCE_MODAL_LOOPS_PERMITTED
        FileChooser fc (TRANS("Load a saved state"), getLastFile(), getFilePatterns (fileSuffix));

        if (fc.browseForFileToOpen())
        {
            setLastFile (fc);

            MemoryBlock data;

            if (fc.getResult().loadFileAsData (data))
                processor->setStateInformation (data.getData(), (int) data.getSize());
            else
                AlertWindow::showMessageBoxAsync (AlertWindow::WarningIcon,
                                                  TRANS("Error while loading"),
                                                  TRANS("Couldn't read from the specified file!"));
        }
       #else
        ignoreUnused (fileSuffix);
       #endif
    }

    //==============================================================================
    void startPlaying()
    {

        
        player.setProcessor (processor.get());

      #if JucePlugin_Enable_IAA && JUCE_IOS
              if (auto device = dynamic_cast<iOSAudioIODevice*> (deviceManager.getCurrentAudioDevice()))
              {
                  processor->setPlayHead (device->getAudioPlayHead());
                  device->setMidiMessageCollector (&player.getMidiMessageCollector());
              }
      #endif
    }

    void stopPlaying()
    {
        player.setProcessor (nullptr);
    }

    //==============================================================================
    /** Shows an audio properties dialog box modally. */
    void showAudioSettingsDialog()
    {
        DialogWindow::LaunchOptions o;

        int minNumInputs  = std::numeric_limits<int>::max(), maxNumInputs  = 0,
            minNumOutputs = std::numeric_limits<int>::max(), maxNumOutputs = 0;

        auto updateMinAndMax = [] (int newValue, int& minValue, int& maxValue)
        {
            minValue = jmin (minValue, newValue);
            maxValue = jmax (maxValue, newValue);
        };

        if (channelConfiguration.size() > 0)
        {
            auto defaultConfig = channelConfiguration.getReference (0);
            updateMinAndMax ((int) defaultConfig.numIns,  minNumInputs,  maxNumInputs);
            updateMinAndMax ((int) defaultConfig.numOuts, minNumOutputs, maxNumOutputs);
        }

        if (auto* bus = processor->getBus (true, 0)) {
            updateMinAndMax (bus->getDefaultLayout().size(), minNumInputs, maxNumInputs);
            if (bus->isNumberOfChannelsSupported(1)) {
                updateMinAndMax (1, minNumInputs, maxNumInputs);                
            }
        }

        if (auto* bus = processor->getBus (false, 0)) {
            updateMinAndMax (bus->getDefaultLayout().size(), minNumOutputs, maxNumOutputs);
            if (bus->isNumberOfChannelsSupported(1)) {
                updateMinAndMax (1, minNumOutputs, maxNumOutputs);                
            }
        }

        
        minNumInputs  = jmin (minNumInputs,  maxNumInputs);
        minNumOutputs = jmin (minNumOutputs, maxNumOutputs);

        o.content.setOwned (new SettingsComponent (*this, deviceManager,
                                                          minNumInputs,
                                                          maxNumInputs,
                                                          minNumOutputs,
                                                          maxNumOutputs));
        o.content->setSize (500, 550);

        //o.dialogTitle                   = TRANS("Audio/MIDI Settings");
        o.dialogTitle                   = TRANS("Audio Settings");
        o.dialogBackgroundColour        = o.content->getLookAndFeel().findColour (ResizableWindow::backgroundColourId);
        o.escapeKeyTriggersCloseButton  = true;
        o.useNativeTitleBar             = true;
        o.resizable                     = false;

        o.launchAsync();
    }

    void saveAudioDeviceState()
    {
        if (settings != nullptr)
        {
            std::unique_ptr<XmlElement> xml (deviceManager.createStateXml());

            settings->setValue ("audioSetup", xml.get());

            settings->setValue ("shouldOverrideSampleRate", (bool) shouldOverrideSampleRate.getValue());
            settings->setValue ("shouldCheckForNewVersion", (bool) shouldCheckForNewVersion.getValue());

            auto recentSetupXml = std::make_unique<XmlElement>("PATHLIST");
            for (auto fname : recentSetupFiles) {
                auto pathchild = recentSetupXml->createNewChildElement("PATH");
                pathchild->addTextElement(fname);
            }

            settings->setValue ("recentSetupFiles", recentSetupXml.get());
            settings->setValue ("lastRecentsSetupFolder", lastRecentsSetupFolder);


#if ! (JUCE_IOS || JUCE_ANDROID)
            //  settings->setValue ("shouldMuteInput", (bool) shouldMuteInput.getValue());
#endif
        }
    }

    void reloadAudioDeviceState (bool enableAudioInput,
                                 const String& preferredDefaultDeviceName,
                                 const AudioDeviceManager::AudioDeviceSetup* preferredSetupOptions)
    {
        std::unique_ptr<XmlElement> savedState;

        std::unique_ptr<AudioDeviceManager::AudioDeviceSetup> prefSetupOptions;
        
        if (preferredSetupOptions) {
            prefSetupOptions = std::make_unique<AudioDeviceManager::AudioDeviceSetup>(*preferredSetupOptions);
        }

        
        if (settings != nullptr)
        {
            savedState = settings->getXmlValue ("audioSetup");

            shouldOverrideSampleRate.setValue (settings->getBoolValue ("shouldOverrideSampleRate", (bool) shouldOverrideSampleRate.getValue()));
            shouldCheckForNewVersion.setValue (settings->getBoolValue ("shouldCheckForNewVersion", (bool) shouldCheckForNewVersion.getValue()));

            auto setupfiles = settings->getXmlValue("recentSetupFiles");
            if (setupfiles) {
                recentSetupFiles.clearQuick();
                for (int i=0; i < setupfiles->getNumChildElements(); ++i) {
                    auto child = setupfiles->getChildElement(i);
                    if (child->getNumChildElements() > 0 && child->getFirstChildElement()->isTextElement()) {
                        auto fname = child->getFirstChildElement()->getText();
                        recentSetupFiles.add(fname);
                    }
                }
                DBG("Loaded recent setups: " << recentSetupFiles.joinIntoString("\n"));
            }

            lastRecentsSetupFolder = settings->getValue("lastRecentsSetupFolder", lastRecentsSetupFolder);

           #if ! (JUCE_IOS || JUCE_ANDROID)
            shouldMuteInput.setValue (settings->getBoolValue ("shouldMuteInput", false));
           #endif

            // now remove samplerate from saved state if necessary
            // as well as preferredSetupOptions
            if (!((bool)shouldOverrideSampleRate.getValue())) {
                DBG("NOT OVERRIDING SAMPLERATE");
                if (savedState && savedState->hasAttribute("audioDeviceRate")) {
                    savedState->removeAttribute("audioDeviceRate");
                }
                if (prefSetupOptions) {
                    prefSetupOptions->sampleRate = 0;
                }
            }
        }

        
        auto totalInChannels  = processor->getMainBusNumInputChannels();
        auto totalOutChannels = processor->getMainBusNumOutputChannels();

        if (channelConfiguration.size() > 0)
        {
            auto defaultConfig = channelConfiguration.getReference (0);
            totalInChannels  = defaultConfig.numIns;
            totalOutChannels = defaultConfig.numOuts;
        }

        deviceManager.initialise (enableAudioInput ? totalInChannels : 0,
                                  totalOutChannels,
                                  savedState.get(),
                                  true,
                                  preferredDefaultDeviceName,
                                  prefSetupOptions.get());
    }

    //==============================================================================
    void savePluginState()
    {
        if (settings != nullptr && processor != nullptr)
        {
            auto * sonobusprocessor = dynamic_cast<SonobusAudioProcessor*>(processor.get());

            bool usexmlstate = sonobusprocessor != nullptr;

            MemoryBlock data;

            if (usexmlstate && sonobusprocessor) {
                sonobusprocessor->getStateInformationWithOptions (data, true, usexmlstate);
                std::unique_ptr<XmlElement> filtxml = juce::parseXML(String::createStringFromData(data.getData(), (int)data.getSize()));
                if (filtxml) {
                    settings->setValue ("filterStateXML", filtxml.get());
                    // remove old binary one
                    settings->removeValue("filterState");
                } else {
                    // failed for some reason try it binary-style
                    data.reset();
                    processor->getStateInformation (data);
                    settings->setValue ("filterState", data.toBase64Encoding());
                }
            } else {
                processor->getStateInformation (data);
                settings->setValue ("filterState", data.toBase64Encoding());
            }
        }
    }

    void reloadPluginState()
    {
        if (settings != nullptr)
        {
            MemoryBlock data;
            auto * sonobusprocessor = dynamic_cast<SonobusAudioProcessor*>(processor.get());

            if (sonobusprocessor != nullptr && settings->containsKey("filterStateXML")) {
                String filtxml = settings->getValue ("filterStateXML");
                data.replaceWith(filtxml.toUTF8(), filtxml.getNumBytesAsUTF8());
                if (data.getSize() > 0) {
                    sonobusprocessor->setStateInformationWithOptions (data.getData(), (int) data.getSize(), true, true);
                    return;
                }
            }

            if (data.fromBase64Encoding (settings->getValue ("filterState")) && data.getSize() > 0)
                processor->setStateInformation (data.getData(), (int) data.getSize());
        }
    }

    //==============================================================================
    void switchToHostApplication()
    {
       #if JUCE_IOS
        if (auto device = dynamic_cast<iOSAudioIODevice*> (deviceManager.getCurrentAudioDevice()))
            device->switchApplication();
       #endif
    }

    bool isInterAppAudioConnected()
    {
       #if JUCE_IOS
        if (auto device = dynamic_cast<iOSAudioIODevice*> (deviceManager.getCurrentAudioDevice()))
            return device->isInterAppAudioConnected();
       #endif

        return false;
    }

   #if JUCE_MODULE_AVAILABLE_juce_gui_basics
    Image getIAAHostIcon (int size)
    {
       #if JUCE_IOS && JucePlugin_Enable_IAA
        if (auto device = dynamic_cast<iOSAudioIODevice*> (deviceManager.getCurrentAudioDevice()))
            return device->getIcon (size);
       #else
        ignoreUnused (size);
       #endif

        return {};
    }
   #endif

    static StandalonePluginHolder* getInstance();

    //==============================================================================
    OptionalScopedPointer<PropertySet> settings;
    std::unique_ptr<AudioProcessor> processor;
    AudioDeviceManager deviceManager;
    AudioProcessorPlayer player;
    Array<PluginInOuts> channelConfiguration;

    // don't avoid feedback loop by default
    bool processorHasPotentialFeedbackLoop = false;
    Value shouldMuteInput;
    AudioBuffer<float> emptyBuffer;
    bool autoOpenMidiDevices;

    Value shouldOverrideSampleRate;

    Value shouldCheckForNewVersion;
    StringArray recentSetupFiles;
    String  lastRecentsSetupFolder;

    std::unique_ptr<AudioDeviceManager::AudioDeviceSetup> options;
    StringArray lastMidiDevices;

private:
    //==============================================================================
    class SettingsComponent : public Component
    {
    public:
        SettingsComponent (StandalonePluginHolder& pluginHolder,
                           AudioDeviceManager& deviceManagerToUse,
                           int minAudioInputChannels,
                           int maxAudioInputChannels,
                           int minAudioOutputChannels,
                           int maxAudioOutputChannels)
            : owner (pluginHolder),
              deviceSelector (deviceManagerToUse,
                              minAudioInputChannels, maxAudioInputChannels,
                              minAudioOutputChannels, maxAudioOutputChannels,
                              false, // show MIDI input
                              (pluginHolder.processor.get() != nullptr && pluginHolder.processor->producesMidi()),
                              false, false),
              shouldMuteLabel  ("Feedback Loop:", "Feedback Loop:"),
              shouldMuteButton ("Mute audio input")
        {
            setOpaque (true);

            shouldMuteButton.setClickingTogglesState (true);
            shouldMuteButton.getToggleStateValue().referTo (owner.shouldMuteInput);

            addAndMakeVisible (deviceSelector);

            if (owner.getProcessorHasPotentialFeedbackLoop())
            {
                addAndMakeVisible (shouldMuteButton);
                addAndMakeVisible (shouldMuteLabel);

                shouldMuteLabel.attachToComponent (&shouldMuteButton, true);
            }
        }

        void paint (Graphics& g) override
        {
            g.fillAll (getLookAndFeel().findColour (ResizableWindow::backgroundColourId));
        }

        void resized() override
        {
            auto r = getLocalBounds();

            if (owner.getProcessorHasPotentialFeedbackLoop())
            {
                auto itemHeight = deviceSelector.getItemHeight();
                auto extra = r.removeFromTop (itemHeight);

                auto seperatorHeight = (itemHeight >> 1);
                shouldMuteButton.setBounds (Rectangle<int> (extra.proportionOfWidth (0.35f), seperatorHeight,
                                                            extra.proportionOfWidth (0.60f), deviceSelector.getItemHeight()));

                r.removeFromTop (seperatorHeight);
            }

            deviceSelector.setBounds (r);
        }

    private:
        //==============================================================================
        StandalonePluginHolder& owner;
        AudioDeviceSelectorComponent deviceSelector;
        Label shouldMuteLabel;
        ToggleButton shouldMuteButton;

        //==============================================================================
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SettingsComponent)
    };

    //==============================================================================
    void audioDeviceIOCallback (const float** inputChannelData,
                                int numInputChannels,
                                float** outputChannelData,
                                int numOutputChannels,
                                int numSamples) override
    {
        const bool inputMuted = shouldMuteInput.getValue();

        if (inputMuted)
        {
            emptyBuffer.clear();
            inputChannelData = emptyBuffer.getArrayOfReadPointers();
        }

        player.audioDeviceIOCallback (inputChannelData, numInputChannels,
                                      outputChannelData, numOutputChannels, numSamples);
    }

    void audioDeviceAboutToStart (AudioIODevice* device) override
    {
        emptyBuffer.setSize (device->getActiveInputChannels().countNumberOfSetBits(), device->getCurrentBufferSizeSamples());
        emptyBuffer.clear();

        player.audioDeviceAboutToStart (device);
        player.setMidiOutput (deviceManager.getDefaultMidiOutput());
        
#if JUCE_IOS
        if (auto iosdevice = dynamic_cast<iOSAudioIODevice*> (deviceManager.getCurrentAudioDevice())) {
#if JucePlugin_Enable_IAA
            processor->setPlayHead (iosdevice->getAudioPlayHead());
            iosdevice->setMidiMessageCollector (&player.getMidiMessageCollector());            
#endif
            processorHasPotentialFeedbackLoop = !iosdevice->isHeadphonesConnected() && !isInterAppAudioConnected();
            shouldMuteInput.setValue(processorHasPotentialFeedbackLoop);
        }        
#endif
    }

    void audioDeviceStopped() override
    {
        player.setMidiOutput (nullptr);
        player.audioDeviceStopped();
        emptyBuffer.setSize (0, 0);
    }

    //==============================================================================
    void setupAudioDevices (bool enableAudioInput,
                            const String& preferredDefaultDeviceName,
                            const AudioDeviceManager::AudioDeviceSetup* preferredSetupOptions)
    {
        deviceManager.addAudioCallback (this);
        deviceManager.addMidiInputCallback ({}, &player);

        reloadAudioDeviceState (enableAudioInput, preferredDefaultDeviceName, preferredSetupOptions);
    }

    void shutDownAudioDevices()
    {
        saveAudioDeviceState();

        deviceManager.removeMidiInputCallback ({}, &player);
        deviceManager.removeAudioCallback (this);
    }

    void timerCallback() override
    {
        auto newMidiDevices = MidiInput::getDevices();

        if (newMidiDevices != lastMidiDevices)
        {
            for (auto& oldDevice : lastMidiDevices)
                if (! newMidiDevices.contains (oldDevice))
                    deviceManager.setMidiInputEnabled (oldDevice, false);

            for (auto& newDevice : newMidiDevices)
                if (! lastMidiDevices.contains (newDevice))
                    deviceManager.setMidiInputEnabled (newDevice, true);
        }
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (StandalonePluginHolder)
};

//==============================================================================
/**
    A class that can be used to run a simple standalone application containing your filter.

    Just create one of these objects in your JUCEApplicationBase::initialise() method, and
    let it do its work. It will create your filter object using the same createPluginFilter() function
    that the other plugin wrappers use.

    @tags{Audio}
*/
class StandaloneFilterWindow    : public DocumentWindow,
                                  public Button::Listener,
                                  public Timer
{
public:
    //==============================================================================
    typedef StandalonePluginHolder::PluginInOuts PluginInOuts;

    //==============================================================================
    /** Creates a window with a given title and colour.
        The settings object can be a PropertySet that the class should use to
        store its settings (it can also be null). If takeOwnershipOfSettings is
        true, then the settings object will be owned and deleted by this object.
    */
    StandaloneFilterWindow (const String& title,
                            Colour backgroundColour,
                            PropertySet* settingsToUse,
                            bool takeOwnershipOfSettings,
                            const String& preferredDefaultDeviceName = String(),
                            const AudioDeviceManager::AudioDeviceSetup* preferredSetupOptions = nullptr,
                            const Array<PluginInOuts>& constrainToConfiguration = {},
                           #if JUCE_ANDROID || JUCE_IOS
                            bool autoOpenMidiDevices = true
                           #else
                            bool autoOpenMidiDevices = false
                           #endif
                            )
        : DocumentWindow (title, backgroundColour, DocumentWindow::allButtons),
          optionsButton ("Options")
    {
        
        
       #if JUCE_IOS || JUCE_ANDROID
        setTitleBarHeight (0);
       #else
        setTitleBarButtonsRequired (DocumentWindow::minimiseButton | DocumentWindow::closeButton | DocumentWindow::maximiseButton, false);

        Component::addAndMakeVisible (optionsButton);
        optionsButton.addListener (this);
        optionsButton.setTriggeredOnMouseDown (true);
        setUsingNativeTitleBar(true);
        #endif

        setResizable (true, false);
        
        pluginHolder.reset (new StandalonePluginHolder (settingsToUse, takeOwnershipOfSettings,
                                                        preferredDefaultDeviceName, preferredSetupOptions,
                                                        constrainToConfiguration, autoOpenMidiDevices));

       #if JUCE_IOS || JUCE_ANDROID
        setFullScreen (true);
        setContentOwned (new MainContentComponent (*this), false);
       #else
        setContentOwned (new MainContentComponent (*this), true);

        if (auto* props = pluginHolder->settings.get())
        {
            const int x = props->getIntValue ("windowX", -100);
            const int y = props->getIntValue ("windowY", -100);
            const int w = props->getIntValue ("windowW", -100);
            const int h = props->getIntValue ("windowH", -100);

            if (x != -100 && y != -100) {
                if (w != -100 && h != -100) {
                    setBoundsConstrained ({ x, y, w, h });
                }
                else {
                    setBoundsConstrained ({ x, y, getWidth(), getHeight() });
                }
            }
            else {
                if (w != -100 && h != -100) {
                    centreWithSize (w, h);
                } else {
                    centreWithSize (getWidth(), getHeight());                    
                }
            }
        }
        else
        {
            centreWithSize (getWidth(), getHeight());
        }
       #endif
        
#if JUCE_IOS
      //  startTimer(500);
#endif
    }

    ~StandaloneFilterWindow()
    {
       #if (! JUCE_IOS) && (! JUCE_ANDROID)
        if (auto* props = pluginHolder->settings.get())
        {
            props->setValue ("windowX", getX());
            props->setValue ("windowY", getY());
            props->setValue ("windowW", getWidth());
            props->setValue ("windowH", getHeight());
        }
       #endif

        pluginHolder->stopPlaying();
        clearContentComponent();
        pluginHolder = nullptr;
    }

    AudioProcessorEditor * getEditor() const {
        auto maincontent = dynamic_cast<MainContentComponent *>(getContentComponent());
        if (maincontent) {
            return maincontent->getEditor();
        }
        return nullptr;
    }
    
    //==============================================================================
    AudioProcessor* getAudioProcessor() const noexcept      { return pluginHolder->processor.get(); }
    AudioDeviceManager& getDeviceManager() const noexcept   { return pluginHolder->deviceManager; }

    bool iaaConnected = false;
    
    void timerCallback() override {
#if JUCE_IOS
        if (auto iosdevice = dynamic_cast<iOSAudioIODevice*> (getDeviceManager().getCurrentAudioDevice())) {
            if (pluginHolder->isInterAppAudioConnected() != iaaConnected) {
                // reload state
                iaaConnected = pluginHolder->isInterAppAudioConnected();
                DBG("IAA state changed: " << (int) iaaConnected << "  " <<  iosdevice->getActiveInputChannels().toString(10));
                
#if 0
                pluginHolder->stopPlaying();
                clearContentComponent();
                pluginHolder->deletePlugin();

                //if (auto* props = pluginHolder->settings.get())
                //    props->removeValue ("filterState");
                getDeviceManager().restartLastAudioDevice();
                
                pluginHolder->createPlugin();
                pluginHolder->init(true, "");

                setContentOwned (new MainContentComponent (*this), true);
                //pluginHolder->startPlaying();
#endif
            }
        }
#endif
                
    }
    
    /** Deletes and re-creates the plugin, resetting it to its default state. */
    void resetToDefaultState()
    {
        pluginHolder->stopPlaying();
        clearContentComponent();
        pluginHolder->deletePlugin();

        if (auto* props = pluginHolder->settings.get())
            props->removeValue ("filterState");

        pluginHolder->createPlugin();
        setContentOwned (new MainContentComponent (*this), true);
        pluginHolder->startPlaying();
    }

    //==============================================================================
    void closeButtonPressed() override
    {
        pluginHolder->savePluginState();

        JUCEApplicationBase::getInstance()->systemRequestedQuit();
    }

    void buttonClicked (Button*) override
    {
        PopupMenu m;
        m.addItem (1, TRANS("Audio/MIDI Settings..."));
        m.addSeparator();
        m.addItem (2, TRANS("Save current state..."));
        m.addItem (3, TRANS("Load a saved state..."));
        m.addSeparator();
        m.addItem (4, TRANS("Reset to default state"));

        m.showMenuAsync (PopupMenu::Options(),
                         ModalCallbackFunction::forComponent (menuCallback, this));
    }

    void handleMenuResult (int result)
    {
        switch (result)
        {
            case 1:  pluginHolder->showAudioSettingsDialog(); break;
            case 2:  pluginHolder->askUserToSaveState(); break;
            case 3:  pluginHolder->askUserToLoadState(); break;
            case 4:  resetToDefaultState(); break;
            default: break;
        }
    }

    static void menuCallback (int result, StandaloneFilterWindow* button)
    {
        if (button != nullptr && result != 0)
            button->handleMenuResult (result);
    }

    void resized() override
    {
        DBG("resized: " << getWidth() << "  " << getHeight());

        DocumentWindow::resized();
        //if (getTitleBarHeight() <= 0) {
        //    optionsButton.setBounds (8, 6, 60, 34);            
        //} else {
            optionsButton.setBounds (8, 6, 60, getTitleBarHeight() - 8);
       // }
    }

    virtual StandalonePluginHolder* getPluginHolder()    { return pluginHolder.get(); }

    std::unique_ptr<StandalonePluginHolder> pluginHolder;

    
private:
    //==============================================================================
    class MainContentComponent  : public Component,
                                  private Value::Listener,
                                  private Button::Listener,
                                  private ComponentListener
    {
    public:
        MainContentComponent (StandaloneFilterWindow& filterWindow)
            : owner (filterWindow), notification (this),
              editor (owner.getAudioProcessor()->createEditorIfNeeded())
        {
            Value& inputMutedValue = owner.pluginHolder->getMuteInputValue();

            if (editor != nullptr)
            {
                // hack to allow editor to get devicemanager
                if (auto * sonoeditor = dynamic_cast<SonobusAudioProcessorEditor*>(editor.get())) {
                    sonoeditor->getAudioDeviceManager = [this]() { return &owner.getDeviceManager();  };
                    sonoeditor->getInputChannelGroupsView()->getAudioDeviceManager = [this]() { return &owner.getDeviceManager();  };
                    sonoeditor->getPeersContainerView()->getAudioDeviceManager = [this]() { return &owner.getDeviceManager();  };
                    sonoeditor->isInterAppAudioConnected = [this]() { return owner.pluginHolder->isInterAppAudioConnected();  };
                    sonoeditor->getIAAHostIcon = [this](int size) { return owner.pluginHolder->getIAAHostIcon(size);  };
                    sonoeditor->switchToHostApplication = [this]() { return owner.pluginHolder->switchToHostApplication(); };
                    sonoeditor->getShouldOverrideSampleRateValue = [this]() { return &(owner.pluginHolder->getShouldOverrideSampleRateValue()); };
                    sonoeditor->getShouldCheckForNewVersionValue = [this]() { return &(owner.pluginHolder->getShouldCheckForNewVersionValue()); };
                    sonoeditor->getRecentSetupFiles = [this]() { return &(owner.pluginHolder->getRecentSetupFiles()); };
                    sonoeditor->getLastRecentsFolder = [this]() { return &owner.pluginHolder->getLastRecentsFolder(); };
                }
                
                editor->addComponentListener (this);
                componentMovedOrResized (*editor, false, true);

                addAndMakeVisible (editor.get());
            }

            addChildComponent (notification);

            if (owner.pluginHolder->getProcessorHasPotentialFeedbackLoop())
            {
                inputMutedValue.addListener (this);
                shouldShowNotification = inputMutedValue.getValue();
            }

            inputMutedChanged (shouldShowNotification);
        }

        ~MainContentComponent()
        {
            if (editor != nullptr)
            {
                editor->removeComponentListener (this);
                owner.pluginHolder->processor->editorBeingDeleted (editor.get());
                editor = nullptr;
            }

            Value& inputMutedValue = owner.pluginHolder->getMuteInputValue();
            inputMutedValue.removeListener(this);
        }

        void resized() override
        {         
            auto r = getLocalBounds();

            bool portrait = getWidth() < getHeight();
            bool tall = getHeight() > 500;
            if (portrait != isPortrait || isTall != tall) {
                isPortrait = portrait;
                isTall = tall;

                // call resized again if on iOS, due to dumb stuff related to safe area insets not being updated
#if JUCE_IOS
                Timer::callAfterDelay(150, [this]() { 
                    this->resized();             
                });
#endif
            }
            
            float safetop=0.0f, safebottom=0.0f, safeleft=0.0f, saferight=0.0f;
            getSafeAreaInsets(getWindowHandle(), safetop, safebottom, safeleft, saferight);
            
            topInset = safetop;
            bottomInset = safebottom;
            leftInset = safeleft;
            rightInset = saferight;
            
            r.removeFromTop((int)safetop);
            r.removeFromBottom((int)safebottom);
            r.removeFromLeft((int)safeleft);
            r.removeFromRight((int)saferight);
            
            
            if (shouldShowNotification) {
                notification.setBounds (r.removeFromTop (NotificationArea::height));                
                topInset += NotificationArea::height; 
            }

            editor->setBounds (r);
        }

        AudioProcessorEditor * getEditor() const { return editor.get(); }
        
    private:
        
        bool isPortrait = false;
        bool isTall = false;
        
        //==============================================================================
        class NotificationArea : public Component
        {
        public:
            enum { height = 60 };

            NotificationArea (Button::Listener* settingsButtonListener)
                : notification ("notification", "Audio input is muted to avoid\nfeedback loop.\nHeadphones recommended!"),
                 //#if JUCE_IOS || JUCE_ANDROID
                  settingsButton ("Unmute Input")
                 //#else
                 // settingsButton ("Settings...")
                 //#endif
            {
                setOpaque (true);

                notification.setColour (Label::textColourId, Colours::black);

                settingsButton.setColour (TextButton::textColourOffId, Colours::white);
                settingsButton.setColour (TextButton::buttonColourId, Colours::red);

                settingsButton.addListener (settingsButtonListener);

                addAndMakeVisible (notification);
                addAndMakeVisible (settingsButton);
            }

            void paint (Graphics& g) override
            {
                auto r = getLocalBounds();

                g.setColour (Colours::darkgoldenrod);
                g.fillRect (r.removeFromBottom (1));

                g.setColour (Colours::lightgoldenrodyellow);
                g.fillRect (r);
            }

            void resized() override
            {
                auto r = getLocalBounds().reduced (5);

                settingsButton.setBounds (r.removeFromRight (70));
                notification.setBounds (r);
            }
        private:
            Label notification;
            TextButton settingsButton;
        };

        //==============================================================================
        void inputMutedChanged (bool newInputMutedValue)
        {
            shouldShowNotification = newInputMutedValue;
            notification.setVisible (shouldShowNotification);

           #if JUCE_IOS || JUCE_ANDROID
            resized();
           #else
            setSize (editor->getWidth(),
                     editor->getHeight()
                     + (shouldShowNotification ? NotificationArea::height : 0));
           #endif
        }

        void valueChanged (Value& value) override     { inputMutedChanged (value.getValue()); }
        void buttonClicked (Button*) override
        {
           //#if JUCE_IOS || JUCE_ANDROID
            owner.pluginHolder->getMuteInputValue().setValue (false);
           //#else
           // owner.pluginHolder->showAudioSettingsDialog();
           //#endif
        }

        //==============================================================================
        void componentMovedOrResized (Component&, bool, bool wasResized) override
        {
            if (wasResized && editor != nullptr)
                setSize (editor->getWidth() + leftInset + rightInset,
                         editor->getHeight() + topInset + bottomInset);
        }

        //==============================================================================
        StandaloneFilterWindow& owner;
        NotificationArea notification;
        std::unique_ptr<AudioProcessorEditor> editor;
        bool shouldShowNotification = false;

        int topInset = 0;
        int bottomInset = 0;
        int leftInset = 0;
        int rightInset = 0;
        
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainContentComponent)
    };

    //==============================================================================
    TextButton optionsButton;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (StandaloneFilterWindow)
};

inline StandalonePluginHolder* StandalonePluginHolder::getInstance()
{
   #if JucePlugin_Enable_IAA || JucePlugin_Build_Standalone
    if (PluginHostType::getPluginLoadedAs() == AudioProcessor::wrapperType_Standalone)
    {
        auto& desktop = Desktop::getInstance();
        const int numTopLevelWindows = desktop.getNumComponents();

        for (int i = 0; i < numTopLevelWindows; ++i)
            if (auto window = dynamic_cast<StandaloneFilterWindow*> (desktop.getComponent (i)))
                return window->getPluginHolder();
    }
   #endif

    return nullptr;
}

} // namespace juce
