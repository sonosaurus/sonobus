// SPDX-License-Identifier: GPLv3-or-later WITH Appstore-exception
// Copyright (C) 2021 Jesse Chappell

#include "OptionsView.h"
#include "SonobusPluginEditor.h"

#if JUCE_ANDROID
#include "juce_core/native/juce_BasicNativeHeaders.h"
#include "juce_core/juce_core.h"
#include "juce_core/native/juce_JNIHelpers_android.h"
#endif

using namespace SonoAudio;

class SonobusOptionsTabbedComponent : public TabbedComponent
{
public:
    SonobusOptionsTabbedComponent(TabbedButtonBar::Orientation orientation, OptionsView & editor_) : TabbedComponent(orientation), editor(editor_) {

    }

    void currentTabChanged (int newCurrentTabIndex, const String& newCurrentTabName) override {

        editor.optionsTabChanged(newCurrentTabIndex);
    }

protected:
    OptionsView & editor;

};


enum {
    nameTextColourId = 0x1002830,
    selectedColourId = 0x1002840,
    separatorColourId = 0x1002850,
};


void OptionsView::initializeLanguages()
{
    // TODO smarter way of figuring out what languages are available
    languages.add(TRANS("System Default Language")); languagesNative.add(""); codes.add("");

    languages.add(TRANS("English")); languagesNative.add("English"); codes.add("en");
    languages.add(TRANS("Spanish")); languagesNative.add(CharPointer_UTF8 ("espa\xc3\xb1ol")); codes.add("es");
    languages.add(TRANS("French"));  languagesNative.add(CharPointer_UTF8 ("fran\xc3\xa7""ais")); codes.add("fr");
    languages.add(TRANS("Italian"));  languagesNative.add("italiano"); codes.add("it");
    languages.add(TRANS("German"));  languagesNative.add("Deutsch"); codes.add("de");
    languages.add(TRANS("Portuguese (Portugal)"));  languagesNative.add(CharPointer_UTF8 ("Portugu\xc3\xaas (Portugal)")); codes.add("pt-pt");
    languages.add(TRANS("Portuguese (Brazil)"));  languagesNative.add(CharPointer_UTF8 ("Portugu\xc3\xaas (Brasil)")); codes.add("pt-br");
    languages.add(TRANS("Dutch"));  languagesNative.add("Nederlands"); codes.add("nl");

    if (processor.getUseUniversalFont()) {
        languages.add(TRANS("Japanese")); languagesNative.add(CharPointer_UTF8 ("\xe6\x97\xa5\xe6\x9c\xac\xe8\xaa\x9e")); codes.add("ja");
    } else {
        languages.add(TRANS("Japanese")); languagesNative.add("Japanese"); codes.add("ja"); // TODO fix and use above when we have a font that can display this all the time
    }

    if (processor.getUseUniversalFont()) {
        languages.add(TRANS("Korean")); languagesNative.add(juce::CharPointer_UTF8 ("\xed\x95\x9c\xea\xb5\xad\xec\x9d\xb8")); codes.add("ko");
    } else {
        languages.add(TRANS("Korean")); languagesNative.add("Korean"); codes.add("ko");
    }

    if (processor.getUseUniversalFont()) {
        languages.add(TRANS("Chinese (Simplified)")); languagesNative.add(CharPointer_UTF8 ("\xe4\xb8\xad\xe6\x96\x87\xef\xbc\x88\xe7\xae\x80\xe4\xbd\x93\xef\xbc\x89")); codes.add("zh-hans");
    }
    else {
        languages.add(TRANS("Chinese (Simplified)")); languagesNative.add("Chinese (Simplified)"); codes.add("zh-hans");
    }
    //languages.add(TRANS("Chinese (Traditional)")); languagesNative.add(CharPointer_UTF8 ("\xe4\xb8\xad\xe6\x96\x87\xef\xbc\x88\xe7\xb9\x81\xe9\xab\x94\xef\xbc\x89")); codes.add("zh-hant");

    languages.add(TRANS("Russian"));  languagesNative.add(juce::CharPointer_UTF8 ("p\xd1\x83\xd1\x81\xd1\x81\xd0\xba\xd0\xb8\xd0\xb9")); codes.add("ru");


    // TODO - parse any user files with localized_%s.txt in our settings folder and add as options if not existing already

}


OptionsView::OptionsView(SonobusAudioProcessor& proc, std::function<AudioDeviceManager*()> getaudiodevicemanager)
: Component(), getAudioDeviceManager(getaudiodevicemanager), processor(proc), smallLNF(14), sonoSliderLNF(13)
{
    setColour (nameTextColourId, Colour::fromFloatRGBA(1.0f, 1.0f, 1.0f, 0.9f));
    setColour (selectedColourId, Colour::fromFloatRGBA(0.0f, 0.4f, 0.8f, 0.5f));
    setColour (separatorColourId, Colour::fromFloatRGBA(0.3f, 0.3f, 0.3f, 0.3f));

    sonoSliderLNF.textJustification = Justification::centredRight;
    sonoSliderLNF.sliderTextJustification = Justification::centredRight;

    initializeLanguages();

    mRecOptionsComponent = std::make_unique<Component>();
    mOptionsComponent = std::make_unique<Component>();

    mSettingsTab = std::make_unique<TabbedComponent>(TabbedButtonBar::Orientation::TabsAtTop);
    mSettingsTab->setTabBarDepth(36);
    mSettingsTab->setOutline(0);
    mSettingsTab->getTabbedButtonBar().setMinimumTabScaleFactor(0.1f);
    //mSettingsTab->addComponentListener(this);

    mBufferTimeSlider    = std::make_unique<Slider>(Slider::LinearBar,  Slider::TextBoxRight);
    mBufferTimeSlider->setName("time");
    mBufferTimeSlider->setTitle(TRANS("Default Jitter Buffer"));
    mBufferTimeSlider->setSliderSnapsToMousePosition(false);
    mBufferTimeSlider->setChangeNotificationOnlyOnRelease(true);
    mBufferTimeSlider->setDoubleClickReturnValue(true, 15.0);
    mBufferTimeSlider->setTextBoxIsEditable(true);
    mBufferTimeSlider->setScrollWheelEnabled(false);
    mBufferTimeSlider->setColour(Slider::trackColourId, Colour::fromFloatRGBA(0.1, 0.4, 0.6, 0.3));
    mBufferTimeSlider->setWantsKeyboardFocus(true);

    mBufferTimeSlider->setTooltip(TRANS("This controls controls the default jitter buffer size to start with. When using the Auto modes, it is recommended to keep the value at the minimum so it starts from the lowest possible value. Generally you will only want to set this higher if you are using a Manual mode default which is not recommended."));

    mBufferTimeAttachment     = std::make_unique<AudioProcessorValueTreeState::SliderAttachment> (processor.getValueTreeState(), SonobusAudioProcessor::paramDefaultNetbufMs, *mBufferTimeSlider);

    mOptionsAutosizeDefaultChoice = std::make_unique<SonoChoiceButton>();
    mOptionsAutosizeDefaultChoice->addChoiceListener(this);
    mOptionsAutosizeDefaultChoice->addItem(TRANS("Manual"), SonobusAudioProcessor::AutoNetBufferModeOff);
    mOptionsAutosizeDefaultChoice->addItem(TRANS("Auto Up"), SonobusAudioProcessor::AutoNetBufferModeAutoIncreaseOnly);
    mOptionsAutosizeDefaultChoice->addItem(TRANS("Auto"), SonobusAudioProcessor::AutoNetBufferModeAutoFull);
    mOptionsAutosizeDefaultChoice->addItem(TRANS("Initial Auto"), SonobusAudioProcessor::AutoNetBufferModeInitAuto);

    mOptionsAutosizeDefaultChoice->setTooltip(TRANS("This controls how the jitter buffers are automatically adjusted based on network conditions. The Auto mode is the recommended choice as it will adjust the jitter buffers up or down based on current conditions. The Auto-Up will only make the buffers larger. The Initial Auto will do an initial adjustment from the smallest value and once it stabilizes will no longer change, even if network conditions worsen. Manual will let you set the jitter buffer manually, leaving it up to you to deal with if network conditions change, but can be useful with known users."));

    mOptionsFormatChoiceDefaultChoice = std::make_unique<SonoChoiceButton>();
    mOptionsFormatChoiceDefaultChoice->setTitle(TRANS("Default Send Quality:"));

    mOptionsFormatChoiceDefaultChoice->addChoiceListener(this);
    int numformats = processor.getNumberAudioCodecFormats();
    for (int i=0; i < numformats; ++i) {
        SonobusAudioProcessor::AudioCodecFormatInfo finfo;
        processor.getAudioCodeFormatInfo(i, finfo);
        auto name = finfo.name;
        if (finfo.codec == SonobusAudioProcessor::AudioCodecFormatCodec::CodecOpus && finfo.bitrate < 96000) {
            name += String(" (*)");
        }
        mOptionsFormatChoiceDefaultChoice->addItem(name, i+1);
    }
    mOptionsFormatChoiceDefaultChoice->addItem("(*) " + TRANS("not recommended"), -2, true, true);

    mOptionsFormatChoiceDefaultChoice->setTooltip(TRANS("The default send quality will be used when you first connect with someone. The values specified with a kbps/ch are Opus compressed audio and use less network bandwidth at the expense of a little latency. It is not recommended to use less than 96 kpbs/ch, as that will increase latency more. The PCM 16bit (and above) use uncompressed audio data and will use the most network bandwidth, but have the least latency and CPU load. If you are connecting with a known small group who all have network service that can support it, using PCM 16 bit is recommended for the lowest latency. Otherwise 96 kbps/ch is a good default."));


    mOptionsAutosizeStaticLabel = std::make_unique<Label>("", TRANS("Default Jitter Buffer"));
    configLabel(mOptionsAutosizeStaticLabel.get(), false);
    mOptionsAutosizeStaticLabel->setJustificationType(Justification::centredLeft);
    mOptionsAutosizeStaticLabel->setAccessible(false);

    mOptionsFormatChoiceStaticLabel = std::make_unique<Label>("", TRANS("Default Send Quality:"));
    configLabel(mOptionsFormatChoiceStaticLabel.get(), false);
    mOptionsFormatChoiceStaticLabel->setJustificationType(Justification::centredRight);


    mOptionsLanguageChoice = std::make_unique<SonoChoiceButton>();
    mOptionsLanguageChoice->setTitle(TRANS("Language"));
    mOptionsLanguageChoice->addChoiceListener(this);
    auto overridelang = processor.getLanguageOverrideCode();
    int langsel = -1;
    for (int i=0; i < languages.size(); ++i) {
        String itemname;
        if (languagesNative[i] != languages[i] && !languagesNative[i].isEmpty()) {
            itemname = languagesNative[i] + " - " + languages[i];
        }
        else {
            itemname = languages[i];
        }
        mOptionsLanguageChoice->addItem(itemname, i);
        if (overridelang == codes[i]) {
            langsel = i;
        }
    }

    if (langsel >= 0) {
        mOptionsLanguageChoice->setSelectedId(langsel);
    }

    mOptionsLanguageLabel = std::make_unique<Label>("", TRANS("Language:"));
    configLabel(mOptionsLanguageLabel.get(), false);
    mOptionsLanguageLabel->setJustificationType(Justification::centredRight);


    mOptionsUnivFontButton = std::make_unique<ToggleButton>(TRANS("Use Universal Font"));
    mOptionsUnivFontButton->setTooltip(TRANS("Use font that always supports Chinese, Japanese, and Korean characters. Can cause slowdowns on some systems, so only use it if you need it."));
    mOptionsUnivFontButton->setToggleState(processor.getUseUniversalFont(), dontSendNotification);
    mOptionsUnivFontButton->addListener(this);


    //mOptionsHearLatencyButton = std::make_unique<ToggleButton>(TRANS("Make Latency Test Audible"));
    //mOptionsHearLatencyButton->addListener(this);
    //mHearLatencyTestAttachment = std::make_unique<AudioProcessorValueTreeState::ButtonAttachment> (p.getValueTreeState(), SonobusAudioProcessor::paramHearLatencyTest, *mOptionsHearLatencyButton);

    mOptionsMetRecordedButton = std::make_unique<ToggleButton>(TRANS("Metronome output recorded in full mix"));
    mOptionsMetRecordedButton->addListener(this);
    mMetRecordedAttachment = std::make_unique<AudioProcessorValueTreeState::ButtonAttachment> (processor.getValueTreeState(), SonobusAudioProcessor::paramMetIsRecorded, *mOptionsMetRecordedButton);

    mOptionsRecFinishOpenButton = std::make_unique<ToggleButton>(TRANS("Open finished recording for playback"));
    mOptionsRecFinishOpenButton->addListener(this);

    mOptionsRecStealth = std::make_unique<ToggleButton>(TRANS("Enable stealth recording"));
    mOptionsRecStealth->addListener(this);

    mOptionsRecFilesStaticLabel = std::make_unique<Label>("", TRANS("Record feature creates the following files:"));
    configLabel(mOptionsRecFilesStaticLabel.get(), false);
    mOptionsRecFilesStaticLabel->setJustificationType(Justification::centredLeft);

    mOptionsRecMixButton = std::make_unique<ToggleButton>(TRANS("Full Mix"));
    mOptionsRecMixButton->addListener(this);

    mOptionsRecMixMinusButton = std::make_unique<ToggleButton>(TRANS("Full Mix without yourself"));
    mOptionsRecMixMinusButton->addListener(this);

    mOptionsRecSelfButton = std::make_unique<ToggleButton>(TRANS("Yourself"));
    mOptionsRecSelfButton->addListener(this);

    mOptionsRecOthersButton = std::make_unique<ToggleButton>(TRANS("Each Connected User"));
    mOptionsRecOthersButton->addListener(this);

    mOptionsRecSelfPostFxButton = std::make_unique<ToggleButton>(TRANS("Record yourself including input FX"));
    mOptionsRecSelfPostFxButton->addListener(this);

    mOptionsRecSelfSilenceMutedButton = std::make_unique<ToggleButton>(TRANS("Silence self recording when input is muted"));
    mOptionsRecSelfSilenceMutedButton->addListener(this);


    mRecFormatChoice = std::make_unique<SonoChoiceButton>();
    mRecFormatChoice->addChoiceListener(this);
    mRecFormatChoice->addItem(TRANS("FLAC"), SonobusAudioProcessor::FileFormatFLAC);
    mRecFormatChoice->addItem(TRANS("WAV"), SonobusAudioProcessor::FileFormatWAV);
    mRecFormatChoice->addItem(TRANS("OGG"), SonobusAudioProcessor::FileFormatOGG);

    mRecBitsChoice = std::make_unique<SonoChoiceButton>();
    mRecBitsChoice->addChoiceListener(this);
    mRecBitsChoice->addItem(TRANS("16 bit"), 16);
    mRecBitsChoice->addItem(TRANS("24 bit"), 24);


    mRecFormatStaticLabel = std::make_unique<Label>("", TRANS("Audio File Format:"));
    configLabel(mRecFormatStaticLabel.get(), false);
    mRecFormatStaticLabel->setJustificationType(Justification::centredRight);


    mRecLocationStaticLabel = std::make_unique<Label>("", TRANS("Record Location:"));
    configLabel(mRecLocationStaticLabel.get(), false);
    mRecLocationStaticLabel->setJustificationType(Justification::centredRight);

    mRecLocationButton = std::make_unique<TextButton>("fileloc");
    mRecLocationButton->setButtonText("");
    mRecLocationButton->setLookAndFeel(&smallLNF);
    mRecLocationButton->addListener(this);




    mOptionsUseSpecificUdpPortButton = std::make_unique<ToggleButton>(TRANS("Use Specific UDP Port"));
    mOptionsUseSpecificUdpPortButton->addListener(this);

    mOptionsDynamicResamplingButton = std::make_unique<ToggleButton>(TRANS("Use Drift Correction (NOT RECOMMENDED)"));
    mDynamicResamplingAttachment = std::make_unique<AudioProcessorValueTreeState::ButtonAttachment> (processor.getValueTreeState(), SonobusAudioProcessor::paramDynamicResampling, *mOptionsDynamicResamplingButton);

    mOptionsAutoReconnectButton = std::make_unique<ToggleButton>(TRANS("Auto-Reconnect to Last Group"));
    mAutoReconnectAttachment = std::make_unique<AudioProcessorValueTreeState::ButtonAttachment> (processor.getValueTreeState(), SonobusAudioProcessor::paramAutoReconnectLast, *mOptionsAutoReconnectButton);

    mOptionsOverrideSamplerateButton = std::make_unique<ToggleButton>(TRANS("Override Device Sample Rate"));
    mOptionsOverrideSamplerateButton->addListener(this);

    mOptionsShouldCheckForUpdateButton = std::make_unique<ToggleButton>(TRANS("Automatically check for updates"));
    mOptionsShouldCheckForUpdateButton->addListener(this);

    mOptionsSliderSnapToMouseButton = std::make_unique<ToggleButton>(TRANS("Sliders Snap to Clicked Position"));
    mOptionsSliderSnapToMouseButton->addListener(this);

    mOptionsDisableShortcutButton = std::make_unique<ToggleButton>(TRANS("Disable keyboard shortcuts"));
    mOptionsDisableShortcutButton->addListener(this);

#if JUCE_IOS
    if (JUCEApplicationBase::isStandaloneApp()) {
        mOptionsAllowBluetoothInput = std::make_unique<ToggleButton>(TRANS("Allow Bluetooth Input"));
        mOptionsAllowBluetoothInput->addListener(this);
    }
#endif

    mOptionsInputLimiterButton = std::make_unique<ToggleButton>(TRANS("Use Input FX Limiter"));
    mOptionsInputLimiterButton->addListener(this);

    mOptionsChangeAllFormatButton = std::make_unique<ToggleButton>(TRANS("Change all connected"));
    mOptionsChangeAllFormatButton->addListener(this);
    mOptionsChangeAllFormatButton->setLookAndFeel(&smallLNF);

    mOptionsUdpPortEditor = std::make_unique<TextEditor>("udp");
    mOptionsUdpPortEditor->addListener(this);
    mOptionsUdpPortEditor->setFont(Font(16 * SonoLookAndFeel::getFontScale()));
    mOptionsUdpPortEditor->setText(""); // 100.36.128.246:11000
    mOptionsUdpPortEditor->setKeyboardType(TextEditor::numericKeyboard);
    mOptionsUdpPortEditor->setInputRestrictions(5, "0123456789");

    configEditor(mOptionsUdpPortEditor.get());


    mOptionsDefaultLevelSlider     = std::make_unique<Slider>(Slider::LinearHorizontal,  Slider::TextBoxAbove);
    mOptionsDefaultLevelSlider->setName("uservol");
    mOptionsDefaultLevelSlider->setTitle(TRANS("Default User Level"));
    mOptionsDefaultLevelSlider->setSliderSnapsToMousePosition(processor.getSlidersSnapToMousePosition());
    mOptionsDefaultLevelSlider->setScrollWheelEnabled(false);
    configLevelSlider(mOptionsDefaultLevelSlider.get());
    mOptionsDefaultLevelSlider->setTextBoxStyle(Slider::TextBoxAbove, true, 80, 18);
    mOptionsDefaultLevelSlider->setTextBoxIsEditable(true);
    //mOptionsDefaultLevelSlider->setSliderStyle(Slider::SliderStyle::LinearBar);
    mOptionsDefaultLevelSlider->setWantsKeyboardFocus(true);

    mDefaultLevelAttachment =  std::make_unique<AudioProcessorValueTreeState::SliderAttachment> (processor.getValueTreeState(), SonobusAudioProcessor::paramDefaultPeerLevel, *mOptionsDefaultLevelSlider);


    mOptionsDefaultLevelSliderLabel = std::make_unique<Label>("", TRANS("Default User Level"));
    mOptionsDefaultLevelSliderLabel->setAccessible(false);
    configLabel(mOptionsDefaultLevelSliderLabel.get(), false);
    mOptionsDefaultLevelSliderLabel->setJustificationType(Justification::centredLeft);

    auto autodropname = TRANS("Auto Adjust Drop Threshold");
    mOptionsAutoDropThreshSlider = std::make_unique<Slider>(Slider::LinearBar,  Slider::TextBoxRight);
    mOptionsAutoDropThreshSlider->setTitle(autodropname);
    mOptionsAutoDropThreshSlider->setRange(1.0, 20.0, 0.1);
    mOptionsAutoDropThreshSlider->setSkewFactor(0.5);
    mOptionsAutoDropThreshSlider->setName("dropthresh");
    mOptionsAutoDropThreshSlider->setTextValueSuffix(" " + TRANS("sec"));
    mOptionsAutoDropThreshSlider->setSliderSnapsToMousePosition(false);
    mOptionsAutoDropThreshSlider->setChangeNotificationOnlyOnRelease(true);
    mOptionsAutoDropThreshSlider->setDoubleClickReturnValue(true, 2.0);
    mOptionsAutoDropThreshSlider->setTextBoxIsEditable(false);
    mOptionsAutoDropThreshSlider->setScrollWheelEnabled(false);
    mOptionsAutoDropThreshSlider->setColour(Slider::trackColourId, Colour::fromFloatRGBA(0.1, 0.4, 0.6, 0.3));
    mOptionsAutoDropThreshSlider->setWantsKeyboardFocus(true);

    mOptionsAutoDropThreshSlider->onValueChange = [this]() {
        auto thresh = 1.0 / jmax(1.0, mOptionsAutoDropThreshSlider->getValue());
        processor.setAutoresizeBufferDropRateThreshold(thresh);
        
        // Send OSC message for OptionsAutoDropThreshSlider value change
        if (processor.getOSCEnabled()) {
            processor.getOSCManager().sendMessage("/OptionsAutoDropThreshSlider", static_cast<float>(mOptionsAutoDropThreshSlider->getValue()));
        }
    };

    mOptionsAutoDropThreshSlider->setTooltip(TRANS("This controls how sensitive the auto-jitter buffer adjustment is when there are audio dropouts. The jitter buffer size will be increased if there are any dropouts within the number of seconds specified here. When this value is smaller it will be less likely to increase the jitter buffer size automatically."));


    mOptionsAutoDropThreshLabel = std::make_unique<Label>("", autodropname);
    mOptionsAutoDropThreshLabel->setAccessible(false);
    configLabel(mOptionsAutoDropThreshLabel.get(), false);
    mOptionsAutoDropThreshLabel->setJustificationType(Justification::centredLeft);

    auto maxrecvpadname = TRANS("Sync Receive Padding");
    mOptionsMaxRecvPaddingSlider = std::make_unique<Slider>(Slider::LinearBar,  Slider::TextBoxRight);
    mOptionsMaxRecvPaddingSlider->setTitle(maxrecvpadname);
    mOptionsMaxRecvPaddingSlider->setRange(0.0, 500.0, 1.0);
    mOptionsMaxRecvPaddingSlider->setName("maxrecvpad");
    mOptionsMaxRecvPaddingSlider->setTextValueSuffix(" " + TRANS(""));
    mOptionsMaxRecvPaddingSlider->setSliderSnapsToMousePosition(false);
    mOptionsMaxRecvPaddingSlider->setChangeNotificationOnlyOnRelease(true);
    mOptionsMaxRecvPaddingSlider->setDoubleClickReturnValue(true, 2.0);
    mOptionsMaxRecvPaddingSlider->setTextBoxIsEditable(true);
    mOptionsMaxRecvPaddingSlider->setScrollWheelEnabled(false);
    mOptionsMaxRecvPaddingSlider->setColour(Slider::trackColourId, Colour::fromFloatRGBA(0.1, 0.4, 0.6, 0.3));
    mOptionsMaxRecvPaddingSlider->setWantsKeyboardFocus(true);

    mMaxRecvPaddingAttachment = std::make_unique<AudioProcessorValueTreeState::SliderAttachment> (processor.getValueTreeState(), SonobusAudioProcessor::paramMaxRecvPaddingMs, *mOptionsMaxRecvPaddingSlider);

    mOptionsMaxRecvPaddingSlider->setTooltip(TRANS("This controls the padding value added to the maximum receive latency when using the Receive Sync feature. The value is in milliseconds and can range from 0 to 500. The default is 2ms."));

    mOptionsMaxRecvPaddingLabel = std::make_unique<Label>("", maxrecvpadname);
    mOptionsMaxRecvPaddingLabel->setAccessible(false);
    configLabel(mOptionsMaxRecvPaddingLabel.get(), false);
    mOptionsMaxRecvPaddingLabel->setJustificationType(Justification::centredLeft);

    // OSC Enable toggle
    mOSCEnabledButton = std::make_unique<ToggleButton>(TRANS("Enable OSC"));
    mOSCEnabledButton->addListener(this);
    mOSCEnabledButton->setTooltip(TRANS("Enable or disable OSC (Open Sound Control) functionality"));

    mOSCSendStateOnStartButton = std::make_unique<ToggleButton>(TRANS("Send state to target on start"));
    mOSCSendStateOnStartButton->addListener(this);
    mOSCSendStateOnStartButton->setTooltip(TRANS("When enabled, sends current values of all OSC-enabled controls to the target address when OSC is enabled"));

    // OSC Configuration UI elements
    mOSCTargetIPAddressLabel = std::make_unique<Label>("", TRANS("OSC Target IP Address:"));
    configLabel(mOSCTargetIPAddressLabel.get(), false);
    mOSCTargetIPAddressLabel->setJustificationType(Justification::centredRight);
    
    mOSCTargetIPAddressEditor = std::make_unique<TextEditor>("osctargetip");
    mOSCTargetIPAddressEditor->addListener(this);
    mOSCTargetIPAddressEditor->setFont(Font(16 * SonoLookAndFeel::getFontScale()));
    configEditor(mOSCTargetIPAddressEditor.get());
    mOSCTargetIPAddressEditor->setTooltip(TRANS("IP address for outbound OSC messages"));
    
    mOSCTargetPortLabel = std::make_unique<Label>("", TRANS("OSC Target Port:"));
    configLabel(mOSCTargetPortLabel.get(), false);
    mOSCTargetPortLabel->setJustificationType(Justification::centredRight);
    
    mOSCTargetPortEditor = std::make_unique<TextEditor>("osctargetport");
    mOSCTargetPortEditor->addListener(this);
    mOSCTargetPortEditor->setFont(Font(16 * SonoLookAndFeel::getFontScale()));
    mOSCTargetPortEditor->setKeyboardType(TextEditor::numericKeyboard);
    mOSCTargetPortEditor->setInputRestrictions(5, "0123456789");
    configEditor(mOSCTargetPortEditor.get());
    mOSCTargetPortEditor->setTooltip(TRANS("Port for outbound OSC messages (1-65535)"));
    
    mOSCReceivePortLabel = std::make_unique<Label>("", TRANS("OSC Receive Port:"));
    configLabel(mOSCReceivePortLabel.get(), false);
    mOSCReceivePortLabel->setJustificationType(Justification::centredRight);
    
    mOSCReceivePortEditor = std::make_unique<TextEditor>("oscrecvport");
    mOSCReceivePortEditor->addListener(this);
    mOSCReceivePortEditor->setFont(Font(16 * SonoLookAndFeel::getFontScale()));
    mOSCReceivePortEditor->setKeyboardType(TextEditor::numericKeyboard);
    mOSCReceivePortEditor->setInputRestrictions(5, "0123456789");
    configEditor(mOSCReceivePortEditor.get());
    mOSCReceivePortEditor->setTooltip(TRANS("Port for receiving OSC messages (1-65535)"));

    mOptionsSavePluginDefaultButton = std::make_unique<TextButton>("saveopt");
    mOptionsSavePluginDefaultButton->setButtonText(TRANS("Save as default plugin options"));
    mOptionsSavePluginDefaultButton->setLookAndFeel(&smallLNF);
    mOptionsSavePluginDefaultButton->addListener(this);

    mOptionsResetPluginDefaultButton = std::make_unique<TextButton>("resetopt");
    mOptionsResetPluginDefaultButton->setButtonText(TRANS("Reset default plugin options"));
    mOptionsResetPluginDefaultButton->setLookAndFeel(&smallLNF);
    mOptionsResetPluginDefaultButton->addListener(this);

    

    mVersionLabel = std::make_unique<Label>("", TRANS("Version: ") + ProjectInfo::versionString);
    configLabel(mVersionLabel.get(), true);
    mVersionLabel->setJustificationType(Justification::centredRight);

    addAndMakeVisible(mSettingsTab.get());

    mOptionsComponent->addAndMakeVisible(mOptionsAutosizeStaticLabel.get());
    mOptionsComponent->addAndMakeVisible(mBufferTimeSlider.get());
    mOptionsComponent->addAndMakeVisible(mOptionsAutosizeDefaultChoice.get());
    mOptionsComponent->addAndMakeVisible(mOptionsFormatChoiceDefaultChoice.get());
    mOptionsComponent->addAndMakeVisible(mOptionsFormatChoiceStaticLabel.get());
    //mOptionsComponent->addAndMakeVisible(mOptionsHearLatencyButton.get());
    mOptionsComponent->addAndMakeVisible(mOptionsUdpPortEditor.get());
    mOptionsComponent->addAndMakeVisible(mOptionsUseSpecificUdpPortButton.get());
    mOptionsComponent->addAndMakeVisible(mOptionsDynamicResamplingButton.get());
    mOptionsComponent->addAndMakeVisible(mOptionsAutoReconnectButton.get());
    mOptionsComponent->addAndMakeVisible(mOptionsInputLimiterButton.get());
    mOptionsComponent->addAndMakeVisible(mOptionsDefaultLevelSlider.get());
    mOptionsComponent->addAndMakeVisible(mOptionsDefaultLevelSliderLabel.get());
    mOptionsComponent->addAndMakeVisible(mOptionsChangeAllFormatButton.get());
    mOptionsComponent->addAndMakeVisible(mVersionLabel.get());
    mOptionsComponent->addAndMakeVisible(mOptionsLanguageChoice.get());
    mOptionsComponent->addAndMakeVisible(mOptionsUnivFontButton.get());
    mOptionsComponent->addAndMakeVisible(mOptionsLanguageLabel.get());
    mOptionsComponent->addAndMakeVisible(mOptionsAutoDropThreshSlider.get());
    mOptionsComponent->addAndMakeVisible(mOptionsAutoDropThreshLabel.get());
    mOptionsComponent->addAndMakeVisible(mOptionsMaxRecvPaddingSlider.get());
    mOptionsComponent->addAndMakeVisible(mOptionsMaxRecvPaddingLabel.get());
    
    // Add OSC Configuration UI elements
    mOptionsComponent->addAndMakeVisible(mOSCEnabledButton.get());
    mOptionsComponent->addAndMakeVisible(mOSCSendStateOnStartButton.get());
    mOptionsComponent->addAndMakeVisible(mOSCTargetIPAddressLabel.get());
    mOptionsComponent->addAndMakeVisible(mOSCTargetIPAddressEditor.get());
    mOptionsComponent->addAndMakeVisible(mOSCTargetPortLabel.get());
    mOptionsComponent->addAndMakeVisible(mOSCTargetPortEditor.get());
    mOptionsComponent->addAndMakeVisible(mOSCReceivePortLabel.get());
    mOptionsComponent->addAndMakeVisible(mOSCReceivePortEditor.get());

    if (!JUCEApplication::isStandaloneApp()) {
        mOptionsComponent->addAndMakeVisible(mOptionsSavePluginDefaultButton.get());
        mOptionsComponent->addAndMakeVisible(mOptionsResetPluginDefaultButton.get());
    }

    //mOptionsComponent->addAndMakeVisible(mTitleImage.get());

    if (JUCEApplicationBase::isStandaloneApp()) {
        mOptionsComponent->addAndMakeVisible(mOptionsOverrideSamplerateButton.get());
        mOptionsComponent->addAndMakeVisible(mOptionsShouldCheckForUpdateButton.get());
        if (mOptionsAllowBluetoothInput) {
            mOptionsComponent->addAndMakeVisible(mOptionsAllowBluetoothInput.get());
        }
    }
    mOptionsComponent->addAndMakeVisible(mOptionsSliderSnapToMouseButton.get());
    mOptionsComponent->addAndMakeVisible(mOptionsDisableShortcutButton.get());



    mRecOptionsComponent->addAndMakeVisible(mOptionsMetRecordedButton.get());
    mRecOptionsComponent->addAndMakeVisible(mOptionsRecFinishOpenButton.get());
    mRecOptionsComponent->addAndMakeVisible(mOptionsRecStealth.get());
    mRecOptionsComponent->addAndMakeVisible(mOptionsRecFilesStaticLabel.get());
    mRecOptionsComponent->addAndMakeVisible(mOptionsRecMixButton.get());
    mRecOptionsComponent->addAndMakeVisible(mOptionsRecSelfButton.get());
    mRecOptionsComponent->addAndMakeVisible(mOptionsRecMixMinusButton.get());
    mRecOptionsComponent->addAndMakeVisible(mOptionsRecOthersButton.get());
    mRecOptionsComponent->addAndMakeVisible(mOptionsRecSelfPostFxButton.get());
    mRecOptionsComponent->addAndMakeVisible(mOptionsRecSelfSilenceMutedButton.get());
    mRecOptionsComponent->addAndMakeVisible(mRecFormatChoice.get());
    mRecOptionsComponent->addAndMakeVisible(mRecBitsChoice.get());
    mRecOptionsComponent->addAndMakeVisible(mRecFormatStaticLabel.get());
    mRecOptionsComponent->addAndMakeVisible(mRecLocationButton.get());
    mRecOptionsComponent->addAndMakeVisible(mRecLocationStaticLabel.get());


    if (JUCEApplicationBase::isStandaloneApp() && getAudioDeviceManager && getAudioDeviceManager())
    {
        if (!mAudioDeviceSelector) {
            int minNumInputs  = std::numeric_limits<int>::max(), maxNumInputs  = 0,
            minNumOutputs = std::numeric_limits<int>::max(), maxNumOutputs = 0;

            auto updateMinAndMax = [] (int newValue, int& minValue, int& maxValue)
            {
                minValue = jmin (minValue, newValue);
                maxValue = jmax (maxValue, newValue);
            };

            /*
            if (channelConfiguration.size() > 0)
            {
                auto defaultConfig =  channelConfiguration.getReference (0);
                updateMinAndMax ((int) defaultConfig.numIns,  minNumInputs,  maxNumInputs);
                updateMinAndMax ((int) defaultConfig.numOuts, minNumOutputs, maxNumOutputs);
            }
             */

            if (auto* bus = processor.getBus (true, 0)) {
                auto maxsup = bus->getMaxSupportedChannels(128);
                updateMinAndMax (maxsup, minNumInputs, maxNumInputs);
                updateMinAndMax (bus->getDefaultLayout().size(), minNumInputs, maxNumInputs);
                if (bus->isNumberOfChannelsSupported(1)) {
                    updateMinAndMax (1, minNumInputs, maxNumInputs);
                }
                if (bus->isNumberOfChannelsSupported(0)) {
                    updateMinAndMax (0, minNumInputs, maxNumInputs);
                }
            }

            if (auto* bus = processor.getBus (false, 0)) {
                auto maxsup = bus->getMaxSupportedChannels(128);
                updateMinAndMax (maxsup, minNumOutputs, maxNumOutputs);
                updateMinAndMax (bus->getDefaultLayout().size(), minNumOutputs, maxNumOutputs);
                if (bus->isNumberOfChannelsSupported(1)) {
                    updateMinAndMax (1, minNumOutputs, maxNumOutputs);
                }
                if (bus->isNumberOfChannelsSupported(0)) {
                    updateMinAndMax (0, minNumOutputs, maxNumOutputs);
                }
            }


            minNumInputs  = jmin (minNumInputs,  maxNumInputs);
            minNumOutputs = jmin (minNumOutputs, maxNumOutputs);



            mAudioDeviceSelector = std::make_unique<AudioDeviceSelectorComponent>(*getAudioDeviceManager(),
                                                                                  minNumInputs, maxNumInputs,
                                                                                  minNumOutputs, maxNumOutputs,
                                                                                  false, // show MIDI input
                                                                                  false,
                                                                                  false, false);

#if JUCE_IOS || JUCE_ANDROID
            mAudioDeviceSelector->setItemHeight(44);
#endif

            mAudioOptionsViewport = std::make_unique<Viewport>();
            mAudioOptionsViewport->setViewedComponent(mAudioDeviceSelector.get(), false);


        }

        if (firsttime) {
            mSettingsTab->addTab(TRANS("AUDIO"), Colour::fromFloatRGBA(0.1, 0.1, 0.1, 1.0), mAudioOptionsViewport.get(), false);
        }

    }

    mOtherOptionsViewport = std::make_unique<Viewport>();
    mOtherOptionsViewport->setViewedComponent(mOptionsComponent.get(), false);

    mSettingsTab->addTab(TRANS("OPTIONS"),Colour::fromFloatRGBA(0.1, 0.1, 0.1, 1.0), mOtherOptionsViewport.get(), false);
   // mSettingsTab->addTab(TRANS("HELP"), Colour::fromFloatRGBA(0.1, 0.1, 0.1, 1.0), mHelpComponent.get(), false);

    mRecordOptionsViewport = std::make_unique<Viewport>();
    mRecordOptionsViewport->setViewedComponent(mRecOptionsComponent.get(), false);

    mSettingsTab->addTab(TRANS("RECORDING"),Colour::fromFloatRGBA(0.1, 0.1, 0.1, 1.0), mRecordOptionsViewport.get(), false);

    setFocusContainerType(FocusContainerType::keyboardFocusContainer);

    mSettingsTab->setFocusContainerType(FocusContainerType::none);
    mSettingsTab->getTabbedButtonBar().setFocusContainerType(FocusContainerType::none);
    mSettingsTab->getTabbedButtonBar().setWantsKeyboardFocus(true);
    mSettingsTab->setWantsKeyboardFocus(true);
    for (int i=0; i < mSettingsTab->getTabbedButtonBar().getNumTabs(); ++i) {
        if (auto tabbut = mSettingsTab->getTabbedButtonBar().getTabButton(i)) {
            tabbut->setRadioGroupId(3);
            tabbut->setWantsKeyboardFocus(true);
        }
        if (auto tabcomp = mSettingsTab->getTabContentComponent(i)) {
            tabcomp->setFocusContainerType(FocusContainerType::focusContainer);
        }
    }

    updateLayout();
}

OptionsView::~OptionsView() {}

juce::Rectangle<int> OptionsView::getMinimumContentBounds() const {
    int defWidth = 200;
    int defHeight = 100;
    return Rectangle<int>(0,0,defWidth,defHeight);
}

juce::Rectangle<int> OptionsView::getPreferredContentBounds() const
{
    return Rectangle<int> (0, 0, 300, prefHeight);
}


void OptionsView::timerCallback(int timerid)
{

}

void OptionsView::grabInitialFocus()
{
    if (auto * butt = mSettingsTab->getTabbedButtonBar().getTabButton(mSettingsTab->getCurrentTabIndex())) {
        butt->setWantsKeyboardFocus(true);
        butt->grabKeyboardFocus();
    }
}


void OptionsView::configLabel(Label *label, bool val)
{
    if (val) {
        label->setFont(12);
        label->setColour(Label::textColourId, Colour(0x90eeeeee));
        label->setJustificationType(Justification::centred);
    }
    else {
        label->setFont(14);
        //label->setColour(Label::textColourId, Colour(0xaaeeeeee));
        label->setJustificationType(Justification::centredLeft);
    }
}

void OptionsView::configLevelSlider(Slider * slider)
{
    //slider->setVelocityBasedMode(true);
    //slider->setVelocityModeParameters(2.5, 1, 0.05);
    //slider->setTextBoxStyle(Slider::NoTextBox, true, 40, 18);
    slider->setSliderStyle(Slider::SliderStyle::LinearHorizontal);
    slider->setTextBoxStyle(Slider::TextBoxAbove, true, 50, 14);
    slider->setMouseDragSensitivity(128);
    slider->setScrollWheelEnabled(false);
    //slider->setPopupDisplayEnabled(true, false, this);
    slider->setColour(Slider::textBoxBackgroundColourId, Colours::transparentBlack);
    slider->setColour(Slider::textBoxOutlineColourId, Colours::transparentBlack);
    slider->setColour(Slider::textBoxTextColourId, Colour(0x90eeeeee));
    slider->setColour(TooltipWindow::textColourId, Colour(0xf0eeeeee));

    slider->setLookAndFeel(&sonoSliderLNF);
}

void OptionsView::configEditor(TextEditor *editor, bool passwd)
{
    editor->addListener(this);
    if (passwd)  {
        editor->setIndents(8, 6);
    } else {
        editor->setIndents(8, 8);
    }
}

void OptionsView::updateState(bool ignorecheck)
{
    mOptionsFormatChoiceDefaultChoice->setSelectedItemIndex(processor.getDefaultAudioCodecFormat(), dontSendNotification);
    mOptionsAutosizeDefaultChoice->setSelectedId((int)processor.getDefaultAutoresizeBufferMode(), dontSendNotification);

    mOptionsChangeAllFormatButton->setToggleState(processor.getChangingDefaultAudioCodecSetsExisting(), dontSendNotification);

    mOptionsAutoDropThreshSlider->setValue(1 / jmax(0.001f, processor.getAutoresizeBufferDropRateThreshold()), dontSendNotification);

    int port = processor.getUseSpecificUdpPort();
    if (port > 0) {
        mOptionsUdpPortEditor->setText(String::formatted("%d", port), dontSendNotification);
        mOptionsUdpPortEditor->setEnabled(true);
        mOptionsUdpPortEditor->setAlpha(1.0);
        if (!ignorecheck) {
            mOptionsUseSpecificUdpPortButton->setToggleState(true, dontSendNotification);
        }
    }
    else {
        port = processor.getUdpLocalPort();
        mOptionsUdpPortEditor->setEnabled(mOptionsUseSpecificUdpPortButton->getToggleState());
        mOptionsUdpPortEditor->setAlpha(mOptionsUseSpecificUdpPortButton->getToggleState() ? 1.0 : 0.6);
        mOptionsUdpPortEditor->setText(String::formatted("%d", port), dontSendNotification);
        if (!ignorecheck) {
            mOptionsUseSpecificUdpPortButton->setToggleState(false, dontSendNotification);
        }
    }


    if (JUCEApplication::isStandaloneApp()) {
        if (getShouldOverrideSampleRateValue) {
            Value * val = getShouldOverrideSampleRateValue();
            mOptionsOverrideSamplerateButton->setToggleState((bool)val->getValue(), dontSendNotification);
        }
        if (getShouldCheckForNewVersionValue) {
            Value * val = getShouldCheckForNewVersionValue();
            mOptionsShouldCheckForUpdateButton->setToggleState((bool)val->getValue(), dontSendNotification);
        }

        if (getAllowBluetoothInputValue && mOptionsAllowBluetoothInput) {
            Value * val = getAllowBluetoothInputValue();
            mOptionsAllowBluetoothInput->setToggleState((bool)val->getValue(), dontSendNotification);
        }

    }

    mOptionsUnivFontButton->setToggleState(processor.getUseUniversalFont(), dontSendNotification);

    mOptionsSliderSnapToMouseButton->setToggleState(processor.getSlidersSnapToMousePosition(), dontSendNotification);
    mOptionsDisableShortcutButton->setToggleState(processor.getDisableKeyboardShortcuts(), dontSendNotification);

    uint32 recmask = processor.getDefaultRecordingOptions();

    mOptionsRecOthersButton->setToggleState((recmask & SonobusAudioProcessor::RecordIndividualUsers) != 0, dontSendNotification);
    mOptionsRecMixButton->setToggleState((recmask & SonobusAudioProcessor::RecordMix) != 0, dontSendNotification);
    mOptionsRecMixMinusButton->setToggleState((recmask & SonobusAudioProcessor::RecordMixMinusSelf) != 0, dontSendNotification);
    mOptionsRecSelfButton->setToggleState((recmask & SonobusAudioProcessor::RecordSelf) != 0, dontSendNotification);

    mOptionsRecSelfPostFxButton->setToggleState(!processor.getSelfRecordingPreFX(), dontSendNotification);
    mOptionsRecSelfSilenceMutedButton->setToggleState(processor.getSelfRecordingSilenceWhenMuted(), dontSendNotification);

    mOptionsRecFinishOpenButton->setToggleState(processor.getRecordFinishOpens(), dontSendNotification);
    mOptionsRecStealth->setToggleState(processor.getRecordStealth(), dontSendNotification);

    // Update OSC Configuration UI
    bool oscEnabled = processor.getOSCEnabled();
    mOSCEnabledButton->setToggleState(oscEnabled, dontSendNotification);
    mOSCSendStateOnStartButton->setToggleState(processor.getOSCSendStateOnStart(), dontSendNotification);
    mOSCTargetIPAddressEditor->setText(processor.getOSCTargetIPAddress(), dontSendNotification);
    mOSCTargetPortEditor->setText(String(processor.getOSCTargetPort()), dontSendNotification);
    mOSCReceivePortEditor->setText(String(processor.getOSCReceivePort()), dontSendNotification);
    
    // Enable/disable OSC fields based on OSC enabled state
    mOSCSendStateOnStartButton->setEnabled(oscEnabled);
    mOSCSendStateOnStartButton->setAlpha(oscEnabled ? 1.0 : 0.6);
    mOSCTargetIPAddressEditor->setEnabled(oscEnabled);
    mOSCTargetIPAddressEditor->setAlpha(oscEnabled ? 1.0 : 0.6);
    mOSCTargetPortEditor->setEnabled(oscEnabled);
    mOSCTargetPortEditor->setAlpha(oscEnabled ? 1.0 : 0.6);
    mOSCReceivePortEditor->setEnabled(oscEnabled);
    mOSCReceivePortEditor->setAlpha(oscEnabled ? 1.0 : 0.6);

    mRecFormatChoice->setSelectedId((int)processor.getDefaultRecordingFormat(), dontSendNotification);
    mRecBitsChoice->setSelectedId((int)processor.getDefaultRecordingBitsPerSample(), dontSendNotification);

    auto recdirurl = processor.getDefaultRecordingDirectory();
    String dispath = recdirurl.getFileName();
    if (recdirurl.isLocalFile()) {
        File recdir = recdirurl.getLocalFile();
        dispath = recdir.getRelativePathFrom(File::getSpecialLocation (File::userHomeDirectory));
        if (dispath.startsWith(".")) dispath = recdir.getFullPathName();
    }
    mRecLocationButton->setButtonText(dispath);

    CompressorParams limparams;
    processor.getInputLimiterParams(0, limparams);
    mOptionsInputLimiterButton->setToggleState(limparams.enabled, dontSendNotification);

    if (getShouldOverrideSampleRateValue) {
        Value * val = getShouldOverrideSampleRateValue();
        mOptionsOverrideSamplerateButton->setToggleState((bool)val->getValue(), dontSendNotification);
    }
    if (getShouldCheckForNewVersionValue) {
        Value * val = getShouldCheckForNewVersionValue();
        mOptionsShouldCheckForUpdateButton->setToggleState((bool)val->getValue(), dontSendNotification);
    }
    if (mOptionsAllowBluetoothInput && getAllowBluetoothInputValue) {
        Value * val = getAllowBluetoothInputValue();
        mOptionsAllowBluetoothInput->setToggleState((bool)val->getValue(), dontSendNotification);
    }
}


void OptionsView::updateLayout()
{
    int minKnobWidth = 50;
    int minSliderWidth = 50;
    int minPannerWidth = 40;
    int maxPannerWidth = 100;
    int minitemheight = 36;
    int knobitemheight = 80;
    int minpassheight = 30;
    int setitemheight = 36;
    int minButtonWidth = 90;
    int sliderheight = 44;
    int inmeterwidth = 22 ;
    int outmeterwidth = 22 ;
    int servLabelWidth = 72;
    int iconheight = 24;
    int iconwidth = iconheight;
    int knoblabelheight = 18;
    int panbuttwidth = 26;

#if JUCE_IOS || JUCE_ANDROID
    // make the button heights a bit more for touchscreen purposes
    minitemheight = 44;
    knobitemheight = 90;
    minpassheight = 38;
    panbuttwidth = 32;
#endif

    // options
    optionsNetbufBox.items.clear();
    optionsNetbufBox.flexDirection = FlexBox::Direction::row;
    optionsNetbufBox.items.add(FlexItem(12, 12));
    optionsNetbufBox.items.add(FlexItem(minButtonWidth, minitemheight, *mBufferTimeSlider).withMargin(0).withFlex(3));
    optionsNetbufBox.items.add(FlexItem(4, 12));
    optionsNetbufBox.items.add(FlexItem(80, minitemheight, *mOptionsAutosizeDefaultChoice).withMargin(0).withFlex(0));

    optionsSendQualBox.items.clear();
    optionsSendQualBox.flexDirection = FlexBox::Direction::row;
    optionsSendQualBox.items.add(FlexItem(minButtonWidth, minitemheight, *mOptionsFormatChoiceStaticLabel).withMargin(0).withFlex(1));
    optionsSendQualBox.items.add(FlexItem(minButtonWidth, minitemheight, *mOptionsFormatChoiceDefaultChoice).withMargin(0).withFlex(1));

    optionsLanguageBox.items.clear();
    optionsLanguageBox.flexDirection = FlexBox::Direction::row;
    optionsLanguageBox.items.add(FlexItem(minButtonWidth-10, minitemheight, *mOptionsLanguageLabel).withMargin(0).withFlex(0.5f));
    optionsLanguageBox.items.add(FlexItem(minButtonWidth, minitemheight, *mOptionsLanguageChoice).withMargin(0).withFlex(3));
    optionsLanguageBox.items.add(FlexItem(minButtonWidth-10, minitemheight, *mOptionsUnivFontButton).withMargin(0).withFlex(1.0));

    //optionsHearlatBox.items.clear();
    //optionsHearlatBox.flexDirection = FlexBox::Direction::row;
    //optionsHearlatBox.items.add(FlexItem(10, 12));
    //optionsHearlatBox.items.add(FlexItem(minButtonWidth, minitemheight, *mOptionsHearLatencyButton).withMargin(0).withFlex(1));

    optionsDefaultLevelBox.items.clear();
    optionsDefaultLevelBox.flexDirection = FlexBox::Direction::row;
    optionsDefaultLevelBox.items.add(FlexItem(12, 12));
    //optionsDefaultLevelBox.items.add(FlexItem(minButtonWidth, minitemheight, *mOptionsDefaultLevelSliderLabel).withMargin(0).withFlex(1));
    //optionsDefaultLevelBox.items.add(FlexItem(4, 12));
    optionsDefaultLevelBox.items.add(FlexItem(100, minitemheight, *mOptionsDefaultLevelSlider).withMargin(0).withFlex(1));

    optionsAutoDropThreshBox.items.clear();
    optionsAutoDropThreshBox.flexDirection = FlexBox::Direction::row;
    optionsAutoDropThreshBox.items.add(FlexItem(42, 12));
    optionsAutoDropThreshBox.items.add(FlexItem(100, minitemheight, *mOptionsAutoDropThreshSlider).withMargin(0).withFlex(1));

    optionsMaxRecvPaddingBox.items.clear();
    optionsMaxRecvPaddingBox.flexDirection = FlexBox::Direction::row;
    optionsMaxRecvPaddingBox.items.add(FlexItem(42, 12));
    optionsMaxRecvPaddingBox.items.add(FlexItem(100, minitemheight, *mOptionsMaxRecvPaddingSlider).withMargin(0).withFlex(1));

    // OSC Configuration layout
    optionsOSCEnabledBox.items.clear();
    optionsOSCEnabledBox.flexDirection = FlexBox::Direction::row;
    optionsOSCEnabledBox.items.add(FlexItem(10, 12).withFlex(0));
    optionsOSCEnabledBox.items.add(FlexItem(180, minpassheight, *mOSCEnabledButton).withMargin(0).withFlex(1));
    
    optionsOSCSendStateOnStartBox.items.clear();
    optionsOSCSendStateOnStartBox.flexDirection = FlexBox::Direction::row;
    optionsOSCSendStateOnStartBox.items.add(FlexItem(10, 12).withFlex(0));
    optionsOSCSendStateOnStartBox.items.add(FlexItem(220, minpassheight, *mOSCSendStateOnStartButton).withMargin(0).withFlex(1));
    
    optionsOSCTargetIPBox.items.clear();
    optionsOSCTargetIPBox.flexDirection = FlexBox::Direction::row;
    optionsOSCTargetIPBox.items.add(FlexItem(10, 12));
    optionsOSCTargetIPBox.items.add(FlexItem(minButtonWidth, minitemheight, *mOSCTargetIPAddressLabel).withMargin(0).withFlex(0));
    optionsOSCTargetIPBox.items.add(FlexItem(3, 5));
    optionsOSCTargetIPBox.items.add(FlexItem(100, minitemheight, *mOSCTargetIPAddressEditor).withMargin(0).withFlex(1));
    optionsOSCTargetIPBox.items.add(FlexItem(10, 5));
    
    optionsOSCTargetPortBox.items.clear();
    optionsOSCTargetPortBox.flexDirection = FlexBox::Direction::row;
    optionsOSCTargetPortBox.items.add(FlexItem(10, 12));
    optionsOSCTargetPortBox.items.add(FlexItem(minButtonWidth, minitemheight, *mOSCTargetPortLabel).withMargin(0).withFlex(0));
    optionsOSCTargetPortBox.items.add(FlexItem(3, 5));
    optionsOSCTargetPortBox.items.add(FlexItem(80, minitemheight, *mOSCTargetPortEditor).withMargin(0).withFlex(0.5));
    optionsOSCTargetPortBox.items.add(FlexItem(10, 5));
    
    optionsOSCReceivePortBox.items.clear();
    optionsOSCReceivePortBox.flexDirection = FlexBox::Direction::row;
    optionsOSCReceivePortBox.items.add(FlexItem(10, 12));
    optionsOSCReceivePortBox.items.add(FlexItem(minButtonWidth, minitemheight, *mOSCReceivePortLabel).withMargin(0).withFlex(0));
    optionsOSCReceivePortBox.items.add(FlexItem(3, 5));
    optionsOSCReceivePortBox.items.add(FlexItem(80, minitemheight, *mOSCReceivePortEditor).withMargin(0).withFlex(0.5));
    optionsOSCReceivePortBox.items.add(FlexItem(10, 5));


    optionsUdpBox.items.clear();
    optionsUdpBox.flexDirection = FlexBox::Direction::row;
    optionsUdpBox.items.add(FlexItem(10, 12));
    optionsUdpBox.items.add(FlexItem(minButtonWidth, minitemheight, *mOptionsUseSpecificUdpPortButton).withMargin(0).withFlex(1));
    optionsUdpBox.items.add(FlexItem(90, minitemheight, *mOptionsUdpPortEditor).withMargin(0).withFlex(0));

    optionsDynResampleBox.items.clear();
    optionsDynResampleBox.flexDirection = FlexBox::Direction::row;
    optionsDynResampleBox.items.add(FlexItem(10, 12).withFlex(0));
    optionsDynResampleBox.items.add(FlexItem(180, minpassheight, *mOptionsDynamicResamplingButton).withMargin(0).withFlex(1));

    optionsAutoReconnectBox.items.clear();
    optionsAutoReconnectBox.flexDirection = FlexBox::Direction::row;
    optionsAutoReconnectBox.items.add(FlexItem(10, 12).withFlex(0));
    optionsAutoReconnectBox.items.add(FlexItem(180, minpassheight, *mOptionsAutoReconnectButton).withMargin(0).withFlex(1));

    optionsOverrideSamplerateBox.items.clear();
    optionsOverrideSamplerateBox.flexDirection = FlexBox::Direction::row;
    optionsOverrideSamplerateBox.items.add(FlexItem(10, 12).withFlex(0));
    optionsOverrideSamplerateBox.items.add(FlexItem(180, minpassheight, *mOptionsOverrideSamplerateButton).withMargin(0).withFlex(1));

    optionsInputLimitBox.items.clear();
    optionsInputLimitBox.flexDirection = FlexBox::Direction::row;
    optionsInputLimitBox.items.add(FlexItem(10, 12).withFlex(0));
    optionsInputLimitBox.items.add(FlexItem(180, minpassheight, *mOptionsInputLimiterButton).withMargin(0).withFlex(1));

    optionsChangeAllQualBox.items.clear();
    optionsChangeAllQualBox.flexDirection = FlexBox::Direction::row;
    optionsChangeAllQualBox.items.add(FlexItem(10, 12).withFlex(1));
    optionsChangeAllQualBox.items.add(FlexItem(180, minpassheight, *mOptionsChangeAllFormatButton).withMargin(0).withFlex(0));

    optionsCheckForUpdateBox.items.clear();
    optionsCheckForUpdateBox.flexDirection = FlexBox::Direction::row;
    optionsCheckForUpdateBox.items.add(FlexItem(10, 12).withFlex(0));
    optionsCheckForUpdateBox.items.add(FlexItem(180, minpassheight, *mOptionsShouldCheckForUpdateButton).withMargin(0).withFlex(1));

    optionsSnapToMouseBox.items.clear();
    optionsSnapToMouseBox.flexDirection = FlexBox::Direction::row;
    optionsSnapToMouseBox.items.add(FlexItem(10, 12).withFlex(0));
    optionsSnapToMouseBox.items.add(FlexItem(180, minpassheight, *mOptionsSliderSnapToMouseButton).withMargin(0).withFlex(1));

    optionsDisableShortcutsBox.items.clear();
    optionsDisableShortcutsBox.flexDirection = FlexBox::Direction::row;
    optionsDisableShortcutsBox.items.add(FlexItem(10, 12).withFlex(0));
    optionsDisableShortcutsBox.items.add(FlexItem(180, minpassheight, *mOptionsDisableShortcutButton).withMargin(0).withFlex(1));


    optionsAllowBluetoothBox.items.clear();
    optionsAllowBluetoothBox.flexDirection = FlexBox::Direction::row;
    if (mOptionsAllowBluetoothInput) {
        optionsAllowBluetoothBox.items.add(FlexItem(10, 12).withFlex(0));
        optionsAllowBluetoothBox.items.add(FlexItem(180, minpassheight, *mOptionsAllowBluetoothInput).withMargin(0).withFlex(1));
    }

    optionsPluginDefaultBox.items.clear();
    optionsPluginDefaultBox.flexDirection = FlexBox::Direction::row;
    optionsPluginDefaultBox.items.add(FlexItem(10, 12).withFlex(0));
    optionsPluginDefaultBox.items.add(FlexItem(80, minpassheight, *mOptionsSavePluginDefaultButton).withMargin(0).withFlex(1));
    optionsPluginDefaultBox.items.add(FlexItem(6, 12).withFlex(0));
    optionsPluginDefaultBox.items.add(FlexItem(80, minpassheight, *mOptionsResetPluginDefaultButton).withMargin(0).withFlex(1));

    
    optionsBox.items.clear();
    optionsBox.flexDirection = FlexBox::Direction::column;
    optionsBox.items.add(FlexItem(4, 6));
    optionsBox.items.add(FlexItem(100, 15, *mVersionLabel).withMargin(2).withFlex(0));
    optionsBox.items.add(FlexItem(4, 4));
    optionsBox.items.add(FlexItem(100, minitemheight, optionsLanguageBox).withMargin(2).withFlex(0));
    optionsBox.items.add(FlexItem(4, 4));
    optionsBox.items.add(FlexItem(100, minitemheight, optionsSendQualBox).withMargin(2).withFlex(0));
    optionsBox.items.add(FlexItem(100, minitemheight - 10, optionsChangeAllQualBox).withMargin(1).withFlex(0));
    optionsBox.items.add(FlexItem(4, 4));
    optionsBox.items.add(FlexItem(100, minitemheight, optionsNetbufBox).withMargin(2).withFlex(0));
    optionsBox.items.add(FlexItem(4, 3));
    optionsBox.items.add(FlexItem(100, minitemheight, optionsAutoDropThreshBox).withMargin(2).withFlex(0));
    optionsBox.items.add(FlexItem(4, 3));
    optionsBox.items.add(FlexItem(100, minitemheight, optionsMaxRecvPaddingBox).withMargin(2).withFlex(0));
    optionsBox.items.add(FlexItem(4, 10));
    
    // Add OSC Configuration boxes
    optionsBox.items.add(FlexItem(100, minpassheight, optionsOSCEnabledBox).withMargin(2).withFlex(0));
    optionsBox.items.add(FlexItem(4, 3));
    optionsBox.items.add(FlexItem(100, minpassheight, optionsOSCSendStateOnStartBox).withMargin(2).withFlex(0));
    optionsBox.items.add(FlexItem(4, 3));
    optionsBox.items.add(FlexItem(100, minitemheight, optionsOSCTargetIPBox).withMargin(2).withFlex(0));
    optionsBox.items.add(FlexItem(4, 3));
    optionsBox.items.add(FlexItem(100, minitemheight, optionsOSCTargetPortBox).withMargin(2).withFlex(0));
    optionsBox.items.add(FlexItem(4, 3));
    optionsBox.items.add(FlexItem(100, minitemheight, optionsOSCReceivePortBox).withMargin(2).withFlex(0));
    optionsBox.items.add(FlexItem(4, 10));
    
    optionsBox.items.add(FlexItem(100, minitemheight, optionsDefaultLevelBox).withMargin(2).withFlex(0));
    optionsBox.items.add(FlexItem(4, 6));
    optionsBox.items.add(FlexItem(100, minpassheight, optionsInputLimitBox).withMargin(2).withFlex(0));
    //optionsBox.items.add(FlexItem(100, minpassheight, optionsHearlatBox).withMargin(2).withFlex(0));
    optionsBox.items.add(FlexItem(100, minpassheight, optionsSnapToMouseBox).withMargin(2).withFlex(0));
    optionsBox.items.add(FlexItem(100, minpassheight, optionsAutoReconnectBox).withMargin(2).withFlex(0));
    optionsBox.items.add(FlexItem(100, minitemheight, optionsUdpBox).withMargin(2).withFlex(0));
    if (JUCEApplicationBase::isStandaloneApp()) {
        optionsBox.items.add(FlexItem(100, minpassheight, optionsOverrideSamplerateBox).withMargin(2).withFlex(0));
        if (mOptionsAllowBluetoothInput) {
            optionsBox.items.add(FlexItem(100, minpassheight, optionsAllowBluetoothBox).withMargin(2).withFlex(0));
        }
        optionsBox.items.add(FlexItem(100, minpassheight, optionsCheckForUpdateBox).withMargin(2).withFlex(0));
    }
    optionsBox.items.add(FlexItem(100, minpassheight, optionsDisableShortcutsBox).withMargin(2).withFlex(0));
    optionsBox.items.add(FlexItem(100, minpassheight, optionsDynResampleBox).withMargin(2).withFlex(0));

    if ( ! JUCEApplicationBase::isStandaloneApp()) {
        optionsBox.items.add(FlexItem(100, minitemheight, optionsPluginDefaultBox).withMargin(2).withFlex(0));
    }
    
    minOptionsHeight = 0;
    for (auto & item : optionsBox.items) {
        minOptionsHeight += item.minHeight + item.margin.top + item.margin.bottom;
    }

    // record options

    optionsMetRecordBox.items.clear();
    optionsMetRecordBox.flexDirection = FlexBox::Direction::row;
    optionsMetRecordBox.items.add(FlexItem(10, 12));

    optionsMetRecordBox.items.add(FlexItem(minButtonWidth, minitemheight, *mOptionsMetRecordedButton).withMargin(0).withFlex(1));

    int indentw = 40;

    optionsRecordDirBox.items.clear();
    optionsRecordDirBox.flexDirection = FlexBox::Direction::row;
    optionsRecordDirBox.items.add(FlexItem(115, minitemheight, *mRecLocationStaticLabel).withMargin(0).withFlex(0));
    optionsRecordDirBox.items.add(FlexItem(minButtonWidth, minitemheight, *mRecLocationButton).withMargin(0).withFlex(3));

    optionsRecordFormatBox.items.clear();
    optionsRecordFormatBox.flexDirection = FlexBox::Direction::row;
    optionsRecordFormatBox.items.add(FlexItem(115, minitemheight, *mRecFormatStaticLabel).withMargin(0).withFlex(0));
    optionsRecordFormatBox.items.add(FlexItem(minButtonWidth, minitemheight, *mRecFormatChoice).withMargin(0).withFlex(1));
    optionsRecordFormatBox.items.add(FlexItem(2, 4));
    optionsRecordFormatBox.items.add(FlexItem(80, minitemheight, *mRecBitsChoice).withMargin(0).withFlex(0.25));

    optionsRecMixBox.items.clear();
    optionsRecMixBox.flexDirection = FlexBox::Direction::row;
    optionsRecMixBox.items.add(FlexItem(indentw, 12));
    optionsRecMixBox.items.add(FlexItem(minButtonWidth, minpassheight, *mOptionsRecMixButton).withMargin(0).withFlex(1));

    optionsRecMixMinusBox.items.clear();
    optionsRecMixMinusBox.flexDirection = FlexBox::Direction::row;
    optionsRecMixMinusBox.items.add(FlexItem(indentw, 12));
    optionsRecMixMinusBox.items.add(FlexItem(minButtonWidth, minpassheight, *mOptionsRecMixMinusButton).withMargin(0).withFlex(1));

    optionsRecSelfBox.items.clear();
    optionsRecSelfBox.flexDirection = FlexBox::Direction::row;
    optionsRecSelfBox.items.add(FlexItem(indentw, 12));
    optionsRecSelfBox.items.add(FlexItem(minButtonWidth, minpassheight, *mOptionsRecSelfButton).withMargin(0).withFlex(1));

    optionsRecOthersBox.items.clear();
    optionsRecOthersBox.flexDirection = FlexBox::Direction::row;
    optionsRecOthersBox.items.add(FlexItem(indentw, 12));
    optionsRecOthersBox.items.add(FlexItem(minButtonWidth, minpassheight, *mOptionsRecOthersButton).withMargin(0).withFlex(1));

    optionsRecordSelfPostFxBox.items.clear();
    optionsRecordSelfPostFxBox.flexDirection = FlexBox::Direction::row;
    optionsRecordSelfPostFxBox.items.add(FlexItem(10, 12));
    optionsRecordSelfPostFxBox.items.add(FlexItem(minButtonWidth, minpassheight, *mOptionsRecSelfPostFxButton).withMargin(0).withFlex(1));

    optionsRecordSilentSelfMuteBox.items.clear();
    optionsRecordSilentSelfMuteBox.flexDirection = FlexBox::Direction::row;
    optionsRecordSilentSelfMuteBox.items.add(FlexItem(10, 12));
    optionsRecordSilentSelfMuteBox.items.add(FlexItem(minButtonWidth, minpassheight, *mOptionsRecSelfSilenceMutedButton).withMargin(0).withFlex(1));

    optionsRecordFinishBox.items.clear();
    optionsRecordFinishBox.flexDirection = FlexBox::Direction::row;
    optionsRecordFinishBox.items.add(FlexItem(10, 12));
    optionsRecordFinishBox.items.add(FlexItem(minButtonWidth, minpassheight, *mOptionsRecFinishOpenButton).withMargin(0).withFlex(1));

    optionsRecordFinishBox.items.clear();
    optionsRecordFinishBox.flexDirection = FlexBox::Direction::row;
    optionsRecordFinishBox.items.add(FlexItem(10, 12));
    optionsRecordFinishBox.items.add(FlexItem(minButtonWidth, minpassheight, *mOptionsRecStealth).withMargin(0).withFlex(1));

    recOptionsBox.items.clear();
    recOptionsBox.flexDirection = FlexBox::Direction::column;
    recOptionsBox.items.add(FlexItem(4, 6));
//#if !(JUCE_IOS || JUCE_ANDROID)
#if !(JUCE_IOS)
    recOptionsBox.items.add(FlexItem(100, minitemheight, optionsRecordDirBox).withMargin(2).withFlex(0));
#endif
    recOptionsBox.items.add(FlexItem(100, minitemheight, optionsRecordFormatBox).withMargin(2).withFlex(0));
    recOptionsBox.items.add(FlexItem(4, 4));
    recOptionsBox.items.add(FlexItem(100, minpassheight, *mOptionsRecFilesStaticLabel).withMargin(2).withFlex(0));
    recOptionsBox.items.add(FlexItem(100, minpassheight, optionsRecMixBox).withMargin(2).withFlex(0));
    recOptionsBox.items.add(FlexItem(100, minpassheight, optionsRecMixMinusBox).withMargin(2).withFlex(0));
    recOptionsBox.items.add(FlexItem(100, minpassheight, optionsRecSelfBox).withMargin(2).withFlex(0));
    recOptionsBox.items.add(FlexItem(100, minpassheight, optionsRecOthersBox).withMargin(2).withFlex(0));
    recOptionsBox.items.add(FlexItem(4, 4));
    recOptionsBox.items.add(FlexItem(100, minpassheight, optionsMetRecordBox).withMargin(2).withFlex(0));
    recOptionsBox.items.add(FlexItem(100, minpassheight, optionsRecordSelfPostFxBox).withMargin(2).withFlex(0));
    recOptionsBox.items.add(FlexItem(100, minpassheight, optionsRecordSilentSelfMuteBox).withMargin(2).withFlex(0));
    recOptionsBox.items.add(FlexItem(100, minpassheight, optionsRecordFinishBox).withMargin(2).withFlex(0));
    minRecOptionsHeight = 0;
    for (auto & item : recOptionsBox.items) {
        minRecOptionsHeight += item.minHeight + item.margin.top + item.margin.bottom;
    }


    mainBox.items.clear();
    mainBox.flexDirection = FlexBox::Direction::column;
    mainBox.items.add(FlexItem(100, minitemheight, *mSettingsTab).withMargin(0).withFlex(1));

    prefHeight = jmax(minOptionsHeight, minRecOptionsHeight) + mSettingsTab->getTabBarDepth();
}

void OptionsView::resized()  {

    mainBox.performLayout(getLocalBounds().reduced(2, 2));

    auto innerbounds = mSettingsTab->getLocalBounds();

    if (mAudioDeviceSelector) {
        mAudioDeviceSelector->setBounds(Rectangle<int>(0,0,innerbounds.getWidth() - 10,mAudioDeviceSelector->getHeight()));
    }
    mOptionsComponent->setBounds(Rectangle<int>(0,0,innerbounds.getWidth() - 10, minOptionsHeight));
    mRecOptionsComponent->setBounds(Rectangle<int>(0,0,innerbounds.getWidth() - 10, minRecOptionsHeight));



    optionsBox.performLayout(mOptionsComponent->getLocalBounds());
    recOptionsBox.performLayout(mRecOptionsComponent->getLocalBounds());

    mOptionsAutosizeStaticLabel->setBounds(mBufferTimeSlider->getBounds().removeFromLeft(mBufferTimeSlider->getWidth()*0.75));

    auto deflabbounds = mOptionsDefaultLevelSlider->getBounds().removeFromLeft(mOptionsDefaultLevelSlider->getWidth()*0.75);
    deflabbounds.removeFromBottom(mOptionsDefaultLevelSlider->getHeight()*0.4);
    mOptionsDefaultLevelSliderLabel->setBounds(deflabbounds);
    mOptionsDefaultLevelSlider->setMouseDragSensitivity(jmax(128, mOptionsDefaultLevelSlider->getWidth()));

    mOptionsAutoDropThreshLabel->setBounds(mOptionsAutoDropThreshSlider->getBounds().removeFromLeft(mOptionsAutoDropThreshSlider->getWidth()*0.75));

    mOptionsMaxRecvPaddingLabel->setBounds(mOptionsMaxRecvPaddingSlider->getBounds().removeFromLeft(mOptionsMaxRecvPaddingSlider->getWidth()*0.75));

}

void OptionsView::showAudioTab()
{
    if (mSettingsTab->getNumTabs() == 3) {
        mSettingsTab->setCurrentTabIndex(0);
    }
}

void OptionsView::showOptionsTab()
{
    mSettingsTab->setCurrentTabIndex(mSettingsTab->getNumTabs() == 3 ? 1 : 0);
}

void OptionsView::showRecordingTab()
{
    mSettingsTab->setCurrentTabIndex(mSettingsTab->getNumTabs() == 3 ? 2 : 1);
}


void OptionsView::optionsTabChanged (int newCurrentTabIndex)
{

}

void OptionsView::showWarnings()
{
#if JUCE_WINDOWS
    if (JUCEApplicationBase::isStandaloneApp() && getAudioDeviceManager())
    {
        // on windows, if current audio device type isn't ASIO, prompt them that it should be
        auto devtype = getAudioDeviceManager()->getCurrentAudioDeviceType();
        if (!devtype.equalsIgnoreCase("ASIO")) {
            auto mesg = String(TRANS("Using an ASIO audio device type is strongly recommended. If your audio interface did not come with one, please install ASIO4ALL (asio4all.org) and configure it first."));
            showPopTip(mesg, 0, mSettingsTab->getTabbedButtonBar().getTabButton(0), 320);
        }

    }
#endif
}

void OptionsView::textEditorReturnKeyPressed (TextEditor& ed)
{
    DBG("Return pressed");
    if (&ed == mOptionsUdpPortEditor.get()) {
        int port = mOptionsUdpPortEditor->getText().getIntValue();
        changeUdpPort(port);
    }
}

void OptionsView::textEditorEscapeKeyPressed (TextEditor& ed)
{
    DBG("escape pressed");
}

void OptionsView::textEditorTextChanged (TextEditor& ed)
{

}


void OptionsView::textEditorFocusLost (TextEditor& ed)
{
    // only one we care about live is udp port
    if (&ed == mOptionsUdpPortEditor.get()) {
        int port = mOptionsUdpPortEditor->getText().getIntValue();
        changeUdpPort(port);
    }
    else if (&ed == mOSCTargetIPAddressEditor.get()) {
        processor.setOSCTargetIPAddress(mOSCTargetIPAddressEditor->getText());
        
        // Send OSC message for OSCTargetIPAddress change
        if (processor.getOSCEnabled()) {
            processor.getOSCManager().sendMessage("/OSCTargetIPAddress", mOSCTargetIPAddressEditor->getText());
        }
    }
    else if (&ed == mOSCTargetPortEditor.get()) {
        int port = mOSCTargetPortEditor->getText().getIntValue();
        // Validate port range (1-65535)
        if (port < 1 || port > 65535) {
            port = 6001; // Reset to default
            mOSCTargetPortEditor->setText(String(port), dontSendNotification);
        }
        processor.setOSCTargetPort(port);
        
        // Send OSC message for OSCTargetPort change
        if (processor.getOSCEnabled()) {
            processor.getOSCManager().sendMessage("/OSCTargetPort", port);
        }
    }
    else if (&ed == mOSCReceivePortEditor.get()) {
        int port = mOSCReceivePortEditor->getText().getIntValue();
        // Validate port range (1-65535)
        if (port < 1 || port > 65535) {
            port = 6000; // Reset to default
            mOSCReceivePortEditor->setText(String(port), dontSendNotification);
        }
        processor.setOSCReceivePort(port);
        
        // Send OSC message for OSCReceivePort change
        if (processor.getOSCEnabled()) {
            processor.getOSCManager().sendMessage("/OSCReceivePort", port);
        }
    }
}

void OptionsView::changeUdpPort(int port)
{
    if (port >= 0) {
        DBG("changing udp port to: " << port);
        processor.setUseSpecificUdpPort(port);

        //updateState();
        
        // Send OSC message for OptionsUdpPortEditor change
        if (processor.getOSCEnabled()) {
            processor.getOSCManager().sendMessage("/OptionsUdpPortEditor", port);
        }
    }
    updateState(true);

}

void OptionsView::buttonClicked (Button* buttonThatWasClicked)
{
    if (buttonThatWasClicked == mRecLocationButton.get()) {
        // browse folder chooser
        SafePointer<OptionsView> safeThis (this);

#if JUCE_ANDROID
        if (getAndroidSDKVersion() < 29) {
            if (! RuntimePermissions::isGranted (RuntimePermissions::readExternalStorage))
            {
                RuntimePermissions::request (RuntimePermissions::readExternalStorage,
                                             [safeThis] (bool granted) mutable
                                             {
                    if (granted)
                        safeThis->buttonClicked (safeThis->mRecLocationButton.get());
                });
                return;
            }
        }
#endif
        
         
        chooseRecDirBrowser();
    }
    else if (buttonThatWasClicked == mOptionsInputLimiterButton.get()) {
        CompressorParams params;
        for (int j=0; j < processor.getInputGroupCount(); ++j) {
            processor.getInputLimiterParams(j, params);
            params.enabled = mOptionsInputLimiterButton->getToggleState();
            processor.setInputLimiterParams(j, params);
        }
        
        // Send OSC message for OptionsInputLimiterButton state change
        if (processor.getOSCEnabled()) {
            processor.getOSCManager().sendMessage("/OptionsInputLimiterButton", mOptionsInputLimiterButton->getToggleState() ? 1 : 0);
        }
    }
    else if (buttonThatWasClicked == mOptionsRecMixButton.get()
             || buttonThatWasClicked == mOptionsRecSelfButton.get()
             || buttonThatWasClicked == mOptionsRecOthersButton.get()
             || buttonThatWasClicked == mOptionsRecMixMinusButton.get()
             ) {
        uint32 recmask = 0;
        recmask |= (mOptionsRecMixButton->getToggleState() ? SonobusAudioProcessor::RecordMix : 0);
        recmask |= (mOptionsRecOthersButton->getToggleState() ? SonobusAudioProcessor::RecordIndividualUsers : 0);
        recmask |= (mOptionsRecSelfButton->getToggleState() ? SonobusAudioProcessor::RecordSelf : 0);
        recmask |= (mOptionsRecMixMinusButton->getToggleState() ? SonobusAudioProcessor::RecordMixMinusSelf : 0);

        // ensure at least one is selected
        if (recmask == 0) {
            recmask = SonobusAudioProcessor::RecordMix;
            mOptionsRecMixButton->setToggleState(true, dontSendNotification);
        }

        processor.setDefaultRecordingOptions(recmask);
        
        // Send OSC messages for recording button state changes
        if (processor.getOSCEnabled()) {
            processor.getOSCManager().sendMessage("/OptionsRecMixButton", mOptionsRecMixButton->getToggleState() ? 1 : 0);
            processor.getOSCManager().sendMessage("/OptionsRecSelfButton", mOptionsRecSelfButton->getToggleState() ? 1 : 0);
            processor.getOSCManager().sendMessage("/OptionsRecOthersButton", mOptionsRecOthersButton->getToggleState() ? 1 : 0);
            processor.getOSCManager().sendMessage("/OptionsRecMixMinusButton", mOptionsRecMixMinusButton->getToggleState() ? 1 : 0);
        }
    }
    else if (buttonThatWasClicked == mOptionsChangeAllFormatButton.get()) {
        processor.setChangingDefaultAudioCodecSetsExisting(mOptionsChangeAllFormatButton->getToggleState());
        
        // Send OSC message for OptionsChangeAllFormatButton state change
        if (processor.getOSCEnabled()) {
            processor.getOSCManager().sendMessage("/OptionsChangeAllFormatButton", mOptionsChangeAllFormatButton->getToggleState() ? 1 : 0);
        }
    }
    else if (buttonThatWasClicked == mOptionsRecSelfPostFxButton.get()) {
        processor.setSelfRecordingPreFX(!mOptionsRecSelfPostFxButton->getToggleState());
        
        // Send OSC message for OptionsRecSelfPostFxButton state change
        if (processor.getOSCEnabled()) {
            processor.getOSCManager().sendMessage("/OptionsRecSelfPostFxButton", mOptionsRecSelfPostFxButton->getToggleState() ? 1 : 0);
        }
    }
    else if (buttonThatWasClicked == mOptionsRecSelfSilenceMutedButton.get()) {
        processor.setSelfRecordingSilenceWhenMuted(mOptionsRecSelfSilenceMutedButton->getToggleState());
        
        // Send OSC message for OptionsRecSelfSilenceMutedButton state change
        if (processor.getOSCEnabled()) {
            processor.getOSCManager().sendMessage("/OptionsRecSelfSilenceMutedButton", mOptionsRecSelfSilenceMutedButton->getToggleState() ? 1 : 0);
        }
    }
    else if (buttonThatWasClicked == mOptionsRecFinishOpenButton.get()) {
        processor.setRecordFinishOpens(mOptionsRecFinishOpenButton->getToggleState());
    }
    else if (buttonThatWasClicked == mOptionsRecStealth.get()) {
        processor.setRecordStealth(mOptionsRecStealth->getToggleState());
        
        // Send OSC message for OptionsRecStealth state change
        if (processor.getOSCEnabled()) {
            processor.getOSCManager().sendMessage("/OptionsRecStealth", mOptionsRecStealth->getToggleState() ? 1 : 0);
        }
    }
    else if (buttonThatWasClicked == mOSCEnabledButton.get()) {
        bool enabled = mOSCEnabledButton->getToggleState();
        processor.setOSCEnabled(enabled);
        
        // Update the enabled/disabled state of OSC fields
        mOSCSendStateOnStartButton->setEnabled(enabled);
        mOSCSendStateOnStartButton->setAlpha(enabled ? 1.0 : 0.6);
        mOSCTargetIPAddressEditor->setEnabled(enabled);
        mOSCTargetIPAddressEditor->setAlpha(enabled ? 1.0 : 0.6);
        mOSCTargetPortEditor->setEnabled(enabled);
        mOSCTargetPortEditor->setAlpha(enabled ? 1.0 : 0.6);
        mOSCReceivePortEditor->setEnabled(enabled);
        mOSCReceivePortEditor->setAlpha(enabled ? 1.0 : 0.6);
        
        // If enabling OSC and send state on start is enabled, send all current values
        if (enabled && processor.getOSCSendStateOnStart()) {
            if (auto* editor = processor.getActiveEditor()) {
                if (auto* sonobusEditor = dynamic_cast<SonobusAudioProcessorEditor*>(editor)) {
                    sonobusEditor->sendAllOSCState();
                }
            }
        }
    }
    else if (buttonThatWasClicked == mOSCSendStateOnStartButton.get()) {
        bool enabled = mOSCSendStateOnStartButton->getToggleState();
        processor.setOSCSendStateOnStart(enabled);
        
        // If enabled and OSC is active, send all current values immediately
        if (enabled && processor.getOSCEnabled()) {
            if (auto* editor = processor.getActiveEditor()) {
                if (auto* sonobusEditor = dynamic_cast<SonobusAudioProcessorEditor*>(editor)) {
                    sonobusEditor->sendAllOSCState();
                }
            }
        }
    }
    else if (buttonThatWasClicked == mOptionsUseSpecificUdpPortButton.get()) {
        if (!mOptionsUseSpecificUdpPortButton->getToggleState()) {
            // toggled off, change back to use system chosen port
            changeUdpPort(0);
        } else {
            updateState(true);
        }
        
        // Send OSC message for OptionsUseSpecificUdpPortButton state change
        if (processor.getOSCEnabled()) {
            processor.getOSCManager().sendMessage("/OptionsUseSpecificUdpPortButton", mOptionsUseSpecificUdpPortButton->getToggleState() ? 1 : 0);
        }
    }
    else if (buttonThatWasClicked == mOptionsOverrideSamplerateButton.get()) {

        if (JUCEApplicationBase::isStandaloneApp() && getShouldOverrideSampleRateValue) {
            Value * val = getShouldOverrideSampleRateValue();
            val->setValue((bool)mOptionsOverrideSamplerateButton->getToggleState());
        }
        
        // Send OSC message for OptionsOverrideSamplerateButton state change
        if (processor.getOSCEnabled()) {
            processor.getOSCManager().sendMessage("/OptionsOverrideSamplerateButton", mOptionsOverrideSamplerateButton->getToggleState() ? 1 : 0);
        }
    }
    else if (buttonThatWasClicked == mOptionsAllowBluetoothInput.get()) {

        if (JUCEApplicationBase::isStandaloneApp() && getAllowBluetoothInputValue && getAudioDeviceManager) {
            Value * val = getAllowBluetoothInputValue();
            val->setValue((bool)mOptionsAllowBluetoothInput->getToggleState());
#if JUCE_IOS
            // get current audio device and change a setting if necessary
            if (auto device = dynamic_cast<iOSAudioIODevice*> (getAudioDeviceManager()->getCurrentAudioDevice())) {
                device->setAllowBluetoothInput((bool)val->getValue());
            }
#endif
        }
    }
    else if (buttonThatWasClicked == mOptionsShouldCheckForUpdateButton.get()) {

        if (JUCEApplicationBase::isStandaloneApp() && getShouldCheckForNewVersionValue) {
            Value * val = getShouldCheckForNewVersionValue();
            val->setValue((bool)mOptionsShouldCheckForUpdateButton->getToggleState());

            //if (mOptionsShouldCheckForUpdateButton->getToggleState()) {
            //    startTimer(CheckForNewVersionTimerId, 3000);
            //}
        }
        
        // Send OSC message for OptionsShouldCheckForUpdateButton state change
        if (processor.getOSCEnabled()) {
            processor.getOSCManager().sendMessage("/OptionsShouldCheckForUpdateButton", mOptionsShouldCheckForUpdateButton->getToggleState() ? 1 : 0);
        }
    }
    else if (buttonThatWasClicked == mOptionsSliderSnapToMouseButton.get()) {
        bool newval = mOptionsSliderSnapToMouseButton->getToggleState();
        processor.setSlidersSnapToMousePosition(newval);

        mOptionsDefaultLevelSlider->setSliderSnapsToMousePosition(newval);
        
        if (updateSliderSnap) {
            updateSliderSnap();
        }
        
        // Send OSC message for OptionsSliderSnapToMouseButton state change
        if (processor.getOSCEnabled()) {
            processor.getOSCManager().sendMessage("/OptionsSliderSnapToMouseButton", newval ? 1 : 0);
        }
    }
    else if (buttonThatWasClicked == mOptionsDisableShortcutButton.get()) {
        bool newval = mOptionsDisableShortcutButton->getToggleState();
        processor.setDisableKeyboardShortcuts(newval);
        if (updateKeybindings) {
            updateKeybindings();
        }
        
        // Send OSC message for OptionsDisableShortcutButton state change
        if (processor.getOSCEnabled()) {
            processor.getOSCManager().sendMessage("/OptionsDisableShortcutButton", newval ? 1 : 0);
        }
    }
    else if (buttonThatWasClicked == mOptionsSavePluginDefaultButton.get()) {
        processor.saveCurrentAsDefaultPluginSettings();
    }
    else if (buttonThatWasClicked == mOptionsResetPluginDefaultButton.get()) {
        processor.resetDefaultPluginSettings();
    }

    else if (buttonThatWasClicked == mOptionsUnivFontButton.get()) {
        bool newval = mOptionsUnivFontButton->getToggleState();
        String message;
        String title;
        if (JUCEApplication::isStandaloneApp()) {
            message = TRANS("In order to change the universal font option, the application must be closed and restarted by you.");
            title = TRANS("App restart required");
        } else {
            message = TRANS("In order to change the universal font option, the plugin host must close the plugin view and reopen it.");
            title = TRANS("Host session reload required");
        }

        AlertWindow::showOkCancelBox(AlertWindow::WarningIcon,
                                     title,
                                     message,
                                     TRANS("Change and Close"),
                                     TRANS("Cancel"),
                                     this,
                                     ModalCallbackFunction::create( [this,newval](int result) {
            if (result) {

                processor.setUseUniversalFont(newval);

                if (JUCEApplication::isStandaloneApp()) {
                    if (saveSettingsIfNeeded) {
                        saveSettingsIfNeeded();
                    }
                    Timer::callAfterDelay(500, [] {
                        JUCEApplication::getInstance()->quit();
                    });
                }
            }
            else {
                mOptionsUnivFontButton->setToggleState(!newval, dontSendNotification);
            }
        }));


    }

}


void OptionsView::choiceButtonSelected(SonoChoiceButton *comp, int index, int ident)
{
    if (comp == mOptionsFormatChoiceDefaultChoice.get()) {
        processor.setDefaultAudioCodecFormat(index);
    }
    else if (comp == mOptionsAutosizeDefaultChoice.get()) {
        processor.setDefaultAutoresizeBufferMode((SonobusAudioProcessor::AutoNetBufferMode) ident);
    }
    else if (comp == mRecFormatChoice.get()) {
        processor.setDefaultRecordingFormat((SonobusAudioProcessor::RecordFileFormat) ident);
    }
    else if (comp == mRecBitsChoice.get()) {
        processor.setDefaultRecordingBitsPerSample(ident);
    }
    else if (comp == mOptionsLanguageChoice.get()) {
        String code = codes[ident];
        //app->mainConfig.languageOverrideCode =  codes[comp->getRowId()].toStdString();

        String message;
        String title;
        if (JUCEApplication::isStandaloneApp()) {
            message = TRANS("In order to change the language, the application must be closed and restarted by you.");
            title = TRANS("App restart required");
        } else {
            message = TRANS("In order to change the language, the plugin host must close the plugin view and reopen it.");
            title = TRANS("Host session reload required");
        }

        AlertWindow::showOkCancelBox(AlertWindow::WarningIcon,
                                     title,
                                     message,
                                     TRANS("Change and Close"),
                                     TRANS("Cancel"),
                                     this,
                                     ModalCallbackFunction::create( [this,code](int result) {
            if (result) {
                String langOverride = code;
                if (setupLocalisation == nullptr || !setupLocalisation(langOverride)) {
                    DBG("Error overriding language with " << langOverride);

                    langOverride = "";
                }

                processor.setLanguageOverrideCode(langOverride);

                if (JUCEApplication::isStandaloneApp()) {
                    if (saveSettingsIfNeeded) {
                        saveSettingsIfNeeded();
                    }
                    Timer::callAfterDelay(500, [] {
                        JUCEApplication::getInstance()->quit();
                    });
                }
            }
        }));
    }

}

void OptionsView::chooseRecDirBrowser()
{
    SafePointer<OptionsView> safeThis (this);

    File recdir;
    if (processor.getDefaultRecordingDirectory().isLocalFile()) {
        recdir = processor.getDefaultRecordingDirectory().getLocalFile();
    }

    mFileChooser.reset(new FileChooser(TRANS("Choose the folder for new recordings"),
                                       recdir,
                                       "",
                                       true, false, getTopLevelComponent()));


    int modes = FileBrowserComponent::canSelectDirectories | FileBrowserComponent::openMode;
    mFileChooser->launchAsync (modes,
                               [safeThis] (const FileChooser& chooser) mutable
                               {
        auto results = chooser.getURLResults();
        if (safeThis != nullptr && results.size() > 0)
        {
            auto url = results.getReference (0);

            DBG("Chose directory: " <<  url.toString(false));

#if JUCE_ANDROID
            auto docdir = AndroidDocument::fromTree(url);
            if (!docdir.hasValue()) {
                docdir = AndroidDocument::fromFile(url.getLocalFile());
            }
            
            if (docdir.hasValue()) {
                AndroidDocumentPermission::takePersistentReadWriteAccess(url);
                if (docdir.getInfo().isDirectory()) {
                    safeThis->processor.setDefaultRecordingDirectory(url);
                }
            }
#else
            if (url.isLocalFile()) {
                File lfile = url.getLocalFile();
                if (lfile.isDirectory()) {
                    safeThis->processor.setDefaultRecordingDirectory(url);
                } else {
                    auto parurl = URL(lfile.getParentDirectory());
                    safeThis->processor.setDefaultRecordingDirectory(parurl);
                }

            }
#endif

            safeThis->updateState();

        }

        if (safeThis) {
            safeThis->mFileChooser.reset();
        }

    }, nullptr);

}



void OptionsView::showPopTip(const String & message, int timeoutMs, Component * target, int maxwidth)
{
    popTip.reset(new BubbleMessageComponent());
    popTip->setAllowedPlacement(BubbleComponent::above);
    
    if (target) {
        if (auto * parent = target->findParentComponentOfClass<AudioProcessorEditor>()) {
            parent->addChildComponent (popTip.get());
        } else {
            addChildComponent(popTip.get());            
        }
    }
    else {
        addChildComponent(popTip.get());
    }
    
    AttributedString text(message);
    text.setJustification (Justification::centred);
    text.setColour (findColour (TextButton::textColourOffId));
    text.setFont(Font(12 * SonoLookAndFeel::getFontScale()));
    if (target) {
        popTip->showAt(target, text, timeoutMs);
    }
    else {
        Rectangle<int> topbox(getWidth()/2 - maxwidth/2, 0, maxwidth, 2);
        popTip->showAt(topbox, text, timeoutMs);
    }
    popTip->toFront(false);
    //AccessibilityHandler::postAnnouncement(message, AccessibilityHandler::AnnouncementPriority::high);
}

void OptionsView::paint(Graphics & g)
{
    /*
    //g.fillAll (Colours::black);
    Rectangle<int> bounds = getLocalBounds();

    bounds.reduce(1, 1);
    bounds.removeFromLeft(3);
    
    g.setColour(bgColor);
    g.fillRoundedRectangle(bounds.toFloat(), 6.0f);
    g.setColour(outlineColor);
    g.drawRoundedRectangle(bounds.toFloat(), 6.0f, 0.5f);
*/
}

