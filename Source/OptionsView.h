// SPDX-License-Identifier: GPLv3-or-later WITH Appstore-exception
// Copyright (C) 2021 Jesse Chappell


#pragma once

#include "JuceHeader.h"

#include "SonobusPluginProcessor.h"
#include "SonoLookAndFeel.h"
#include "SonoChoiceButton.h"
#include "SonoDrawableButton.h"
#include "GenericItemChooser.h"


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

    std::unique_ptr<SonoChoiceButton> mOptionsLanguageChoice;
    std::unique_ptr<Label> mOptionsLanguageLabel;
    std::unique_ptr<ToggleButton> mOptionsUnivFontButton;


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
    FlexBox optionsPluginDefaultBox;

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


    // keep this down here, so it gets destroyed early
    std::unique_ptr<BubbleMessageComponent> popTip;


    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OptionsView)

};
