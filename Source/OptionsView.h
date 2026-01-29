// SPDX-License-Identifier: GPLv3-or-later WITH Appstore-exception
// Copyright (C) 2021 Jesse Chappell


#pragma once

#include "JuceHeader.h"

#include "SonobusPluginProcessor.h"
#include "SonoLookAndFeel.h"
#include "SonoChoiceButton.h"
#include "SonoDrawableButton.h"
#include "GenericItemChooser.h"

// Forward declaration
class SonobusAudioProcessorEditor;

class OptionsView :
public Component,
public Button::Listener,
public SonoChoiceButton::Listener,
public GenericItemChooser::Listener,
public TextEditor::Listener,
public MultiTimer
{
public:
    OptionsView(SonobusAudioProcessor& proc, std::function<AudioDeviceManager*()> getaudiodevicemanager);
    virtual ~OptionsView();


    class Listener {
    public:
        virtual ~Listener() {}
        virtual void optionsChanged(OptionsView *comp) {}
    };

    void addListener(Listener * listener) { listeners.add(listener); }
    void removeListener(Listener * listener) { listeners.remove(listener); }

    void timerCallback(int timerid) override;

    void buttonClicked (Button* buttonThatWasClicked) override;

    void choiceButtonSelected(SonoChoiceButton *comp, int index, int ident) override;


    void textEditorReturnKeyPressed (TextEditor&) override;
    void textEditorEscapeKeyPressed (TextEditor&) override;
    void textEditorTextChanged (TextEditor&) override;
    void textEditorFocusLost (TextEditor&) override;

    juce::Rectangle<int> getMinimumContentBounds() const;
    juce::Rectangle<int> getPreferredContentBounds() const;

    void grabInitialFocus();

    void updateState(bool ignorecheck=false);
    void updateLayout();

    void paint(Graphics & g) override;
    void resized() override;

    void showPopTip(const String & message, int timeoutMs, Component * target, int maxwidth=100);

    void optionsTabChanged (int newCurrentTabIndex);

    void showAudioTab();
    void showOptionsTab();
    void showRecordingTab();

    void showWarnings();

    Slider* getOptionsMaxRecvPaddingSlider() { return mOptionsMaxRecvPaddingSlider.get(); }
    ToggleButton* getOptionsRecStealth() { return mOptionsRecStealth.get(); }
    
    // Additional getters for OSC support
    ToggleButton* getOptionsDynamicResamplingButton() { return mOptionsDynamicResamplingButton.get(); }
    ToggleButton* getOptionsAutoReconnectButton() { return mOptionsAutoReconnectButton.get(); }
    ToggleButton* getOptionsInputLimiterButton() { return mOptionsInputLimiterButton.get(); }
    Slider* getOptionsDefaultLevelSlider() { return mOptionsDefaultLevelSlider.get(); }
    Slider* getOptionsAutoDropThreshSlider() { return mOptionsAutoDropThreshSlider.get(); }
    SonoChoiceButton* getOptionsAutosizeDefaultChoice() { return mOptionsAutosizeDefaultChoice.get(); }
    SonoChoiceButton* getOptionsFormatChoiceDefaultChoice() { return mOptionsFormatChoiceDefaultChoice.get(); }
    ToggleButton* getOptionsMetRecordedButton() { return mOptionsMetRecordedButton.get(); }
    ToggleButton* getOptionsRecFinishOpenButton() { return mOptionsRecFinishOpenButton.get(); }
    ToggleButton* getOptionsRecMixButton() { return mOptionsRecMixButton.get(); }
    ToggleButton* getOptionsRecMixMinusButton() { return mOptionsRecMixMinusButton.get(); }
    ToggleButton* getOptionsRecSelfButton() { return mOptionsRecSelfButton.get(); }
    ToggleButton* getOptionsRecOthersButton() { return mOptionsRecOthersButton.get(); }
    ToggleButton* getOptionsRecSelfPostFxButton() { return mOptionsRecSelfPostFxButton.get(); }
    ToggleButton* getOptionsRecSelfSilenceMutedButton() { return mOptionsRecSelfSilenceMutedButton.get(); }
    ToggleButton* getOptionsChangeAllFormatButton() { return mOptionsChangeAllFormatButton.get(); }
    ToggleButton* getOptionsSliderSnapToMouseButton() { return mOptionsSliderSnapToMouseButton.get(); }
    ToggleButton* getOptionsDisableShortcutButton() { return mOptionsDisableShortcutButton.get(); }
    SonoChoiceButton* getRecFormatChoice() { return mRecFormatChoice.get(); }
    SonoChoiceButton* getRecBitsChoice() { return mRecBitsChoice.get(); }
    Slider* getBufferTimeSlider() { return mBufferTimeSlider.get(); }
    ToggleButton* getOptionsUseSpecificUdpPortButton() { return mOptionsUseSpecificUdpPortButton.get(); }
    ToggleButton* getOptionsOverrideSamplerateButton() { return mOptionsOverrideSamplerateButton.get(); }
    ToggleButton* getOptionsShouldCheckForUpdateButton() { return mOptionsShouldCheckForUpdateButton.get(); }
    TextEditor* getOptionsUdpPortEditor() { return mOptionsUdpPortEditor.get(); }
    TextEditor* getOSCTargetIPAddressEditor() { return mOSCTargetIPAddressEditor.get(); }
    TextEditor* getOSCTargetPortEditor() { return mOSCTargetPortEditor.get(); }
    TextEditor* getOSCReceivePortEditor() { return mOSCReceivePortEditor.get(); }

    std::function<AudioDeviceManager*()> getAudioDeviceManager; // = []() { return 0; };
    std::function<Value*()> getShouldOverrideSampleRateValue; // = []() { return 0; };
    std::function<Value*()> getShouldCheckForNewVersionValue; // = []() { return 0; };
    std::function<Value*()> getAllowBluetoothInputValue; // = []() { return 0; };
    std::function<void()> updateSliderSnap; // = []() { return 0; };
    std::function<void()> updateKeybindings; // = []() { return 0; };
    std::function<bool(const String &)> setupLocalisation; // = []() { return 0; };
    std::function<void()> saveSettingsIfNeeded; // = []() { return 0; };


protected:

    void initializeLanguages();

    void configEditor(TextEditor *editor, bool passwd = false);
    void configLabel(Label *label, bool val=false);
    void configLevelSlider(Slider *);

    void changeUdpPort(int port);
    void chooseRecDirBrowser();


    SonobusAudioProcessor& processor;

    SonoBigTextLookAndFeel smallLNF;
    SonoBigTextLookAndFeel sonoSliderLNF;


    ListenerList<Listener> listeners;

    std::unique_ptr<AudioDeviceSelectorComponent> mAudioDeviceSelector;
    std::unique_ptr<Viewport> mAudioOptionsViewport;
    std::unique_ptr<Viewport> mOtherOptionsViewport;
    std::unique_ptr<Viewport> mRecordOptionsViewport;


    std::unique_ptr<Component> mOptionsComponent;
    std::unique_ptr<Component> mRecOptionsComponent;

    int minOptionsHeight = 0;
    int minRecOptionsHeight = 0;

    uint32 settingsClosedTimestamp = 0;

    std::unique_ptr<FileChooser> mFileChooser;

    std::unique_ptr<Slider> mBufferTimeSlider;

    std::unique_ptr<SonoChoiceButton> mOptionsAutosizeDefaultChoice;
    std::unique_ptr<SonoChoiceButton> mOptionsFormatChoiceDefaultChoice;
    std::unique_ptr<Label>  mOptionsAutosizeStaticLabel;
    std::unique_ptr<Label>  mOptionsFormatChoiceStaticLabel;

    std::unique_ptr<ToggleButton> mOptionsUseSpecificUdpPortButton;
    std::unique_ptr<TextEditor>  mOptionsUdpPortEditor;
    std::unique_ptr<Label> mVersionLabel;
    std::unique_ptr<ToggleButton> mOptionsChangeAllFormatButton;

    std::unique_ptr<ToggleButton> mOptionsHearLatencyButton;
    std::unique_ptr<ToggleButton> mOptionsMetRecordedButton;
    std::unique_ptr<ToggleButton> mOptionsDynamicResamplingButton;
    std::unique_ptr<ToggleButton> mOptionsOverrideSamplerateButton;
    std::unique_ptr<ToggleButton> mOptionsShouldCheckForUpdateButton;
    std::unique_ptr<ToggleButton> mOptionsAutoReconnectButton;
    std::unique_ptr<ToggleButton> mOptionsSliderSnapToMouseButton;
    std::unique_ptr<ToggleButton> mOptionsAllowBluetoothInput;
    std::unique_ptr<ToggleButton> mOptionsDisableShortcutButton;
    std::unique_ptr<TextButton> mOptionsSavePluginDefaultButton;
    std::unique_ptr<TextButton> mOptionsResetPluginDefaultButton;

    std::unique_ptr<ToggleButton> mOptionsInputLimiterButton;
    std::unique_ptr<Label> mOptionsDefaultLevelSliderLabel;
    std::unique_ptr<Slider> mOptionsDefaultLevelSlider;

    std::unique_ptr<Label> mOptionsAutoDropThreshLabel;
    std::unique_ptr<Slider> mOptionsAutoDropThreshSlider;

    std::unique_ptr<Label> mOptionsMaxRecvPaddingLabel;
    std::unique_ptr<Slider> mOptionsMaxRecvPaddingSlider;

    std::unique_ptr<SonoChoiceButton> mOptionsLanguageChoice;
    std::unique_ptr<Label> mOptionsLanguageLabel;
    std::unique_ptr<ToggleButton> mOptionsUnivFontButton;

    // OSC Configuration UI elements
    std::unique_ptr<ToggleButton> mOSCEnabledButton;
    std::unique_ptr<ToggleButton> mOSCSendStateOnStartButton;
    std::unique_ptr<Label> mOSCTargetIPAddressLabel;
    std::unique_ptr<TextEditor> mOSCTargetIPAddressEditor;
    std::unique_ptr<Label> mOSCTargetPortLabel;
    std::unique_ptr<TextEditor> mOSCTargetPortEditor;
    std::unique_ptr<Label> mOSCReceivePortLabel;
    std::unique_ptr<TextEditor> mOSCReceivePortEditor;


    std::unique_ptr<Label> mOptionsRecFilesStaticLabel;
    std::unique_ptr<ToggleButton> mOptionsRecMixButton;
    std::unique_ptr<ToggleButton> mOptionsRecMixMinusButton;
    std::unique_ptr<ToggleButton> mOptionsRecSelfButton;
    std::unique_ptr<ToggleButton> mOptionsRecOthersButton;
    std::unique_ptr<ToggleButton> mOptionsRecSelfPostFxButton;
    std::unique_ptr<ToggleButton> mOptionsRecSelfSilenceMutedButton;
    std::unique_ptr<SonoChoiceButton> mRecFormatChoice;
    std::unique_ptr<SonoChoiceButton> mRecBitsChoice;
    std::unique_ptr<Label> mRecFormatStaticLabel;
    std::unique_ptr<Label> mRecLocationStaticLabel;
    std::unique_ptr<TextButton> mRecLocationButton;
    std::unique_ptr<ToggleButton> mOptionsRecFinishOpenButton;
    std::unique_ptr<ToggleButton> mOptionsRecStealth;


    FlexBox mainBox;

    FlexBox optionsBox;
    FlexBox optionsNetbufBox;
    FlexBox optionsSendQualBox;
    FlexBox optionsHearlatBox;
    FlexBox optionsUdpBox;
    FlexBox optionsDynResampleBox;
    FlexBox optionsOverrideSamplerateBox;
    FlexBox optionsCheckForUpdateBox;
    FlexBox optionsChangeAllQualBox;
    FlexBox optionsInputLimitBox;
    FlexBox optionsAutoReconnectBox;
    FlexBox optionsSnapToMouseBox;
    FlexBox optionsDisableShortcutsBox;
    FlexBox optionsDefaultLevelBox;
    FlexBox optionsLanguageBox;
    FlexBox optionsAllowBluetoothBox;
    FlexBox optionsAutoDropThreshBox;
    FlexBox optionsMaxRecvPaddingBox;
    FlexBox optionsPluginDefaultBox;
    FlexBox optionsOSCEnabledBox;
    FlexBox optionsOSCSendStateOnStartBox;
    FlexBox optionsOSCTargetIPBox;
    FlexBox optionsOSCTargetPortBox;
    FlexBox optionsOSCReceivePortBox;

    FlexBox recOptionsBox;
    FlexBox optionsRecordFormatBox;
    FlexBox optionsRecMixBox;
    FlexBox optionsRecSelfBox;
    FlexBox optionsRecMixMinusBox;
    FlexBox optionsRecOthersBox;
    FlexBox optionsMetRecordBox;
    FlexBox optionsRecordDirBox;
    FlexBox optionsRecordSelfPostFxBox;
    FlexBox optionsRecordSilentSelfMuteBox;
    FlexBox optionsRecordFinishBox;


    std::unique_ptr<TabbedComponent> mSettingsTab;

    int minHeight = 0;
    int prefHeight = 0;
    bool firsttime = true;

    // language stuff
    StringArray languages;
    StringArray languagesNative;
    StringArray codes;

    String mActiveLanguageCode;


    std::unique_ptr<AudioProcessorValueTreeState::ButtonAttachment> mMetRecordedAttachment;
    std::unique_ptr<AudioProcessorValueTreeState::ButtonAttachment> mDynamicResamplingAttachment;
    std::unique_ptr<AudioProcessorValueTreeState::ButtonAttachment> mAutoReconnectAttachment;
    std::unique_ptr<AudioProcessorValueTreeState::SliderAttachment> mBufferTimeAttachment;
    std::unique_ptr<AudioProcessorValueTreeState::SliderAttachment> mDefaultLevelAttachment;
    std::unique_ptr<AudioProcessorValueTreeState::SliderAttachment> mMaxRecvPaddingAttachment;


    // keep this down here, so it gets destroyed early
    std::unique_ptr<BubbleMessageComponent> popTip;


    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OptionsView)

};
