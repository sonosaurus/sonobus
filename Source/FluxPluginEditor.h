/*
  ==============================================================================

    This file was auto-generated!

    It contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"

#include "FluxPluginProcessor.h"
#include "SonoLookAndFeel.h"
#include "SonoChoiceButton.h"
#include "SonoDrawableButton.h"

class PeersContainerView;

//==============================================================================
/**
*/
class FluxAoOAudioProcessorEditor  : public AudioProcessorEditor, public MultiTimer,
public Button::Listener,
public AudioProcessorValueTreeState::Listener,
public Slider::Listener,
public SonoChoiceButton::Listener, 
public AsyncUpdater
{
public:
    FluxAoOAudioProcessorEditor (FluxAoOAudioProcessor&);
    ~FluxAoOAudioProcessorEditor();

    //==============================================================================
    void paint (Graphics&) override;
    void resized() override;

    void mouseDown (const MouseEvent& event)  override;

    void timerCallback(int timerid) override;

    void buttonClicked (Button* buttonThatWasClicked) override;

    void sliderValueChanged (Slider* slider) override;

    void parameterChanged (const String&, float newValue) override;
    void handleAsyncUpdate() override;

    void choiceButtonSelected(SonoChoiceButton *comp, int index, int ident) override;

private:

    void updateLayout();

    void updateState();
    
    void configKnobSlider(Slider *);
    //void configLabel(Label * lab, bool val);
    //void configLevelSlider(Slider *);
    
    void showPatchbay(bool flag);
    
    void showFormatChooser(int peerindex);
    
    void showOrHideSettings();
    
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    FluxAoOAudioProcessor& processor;

    SonoLookAndFeel sonoLookAndFeel;
    SonoBigTextLookAndFeel sonoSliderLNF;

    class PatchMatrixView;
    
    std::unique_ptr<Label> mRemoteAddressStaticLabel;
    std::unique_ptr<TextEditor> mAddRemoteHostEditor;
    std::unique_ptr<TextEditor> mAddRemotePortEditor;
    std::unique_ptr<Label> mLocalAddressStaticLabel;
    std::unique_ptr<Label> mLocalAddressLabel;
    std::unique_ptr<TextButton> mConnectButton;
    std::unique_ptr<TextButton> mPatchbayButton;
    std::unique_ptr<SonoDrawableButton> mSettingsButton;

    std::unique_ptr<Slider> mInGainSlider;
    std::unique_ptr<Slider> mInMonPanSlider1;
    std::unique_ptr<Slider> mInMonPanSlider2;
    std::unique_ptr<Slider> mDrySlider;
    std::unique_ptr<Slider> mOutGainSlider;
    std::unique_ptr<Slider> mBufferTimeSlider;
    
    std::unique_ptr<Label> mInGainLabel;
    std::unique_ptr<Label> mDryLabel;
    std::unique_ptr<Label> mWetLabel;
    std::unique_ptr<Label> mOutGainLabel;

    SonoLookAndFeel inMeterLnf;
    SonoLookAndFeel outMeterLnf;

    std::unique_ptr<foleys::LevelMeter> inputMeter;
    std::unique_ptr<foleys::LevelMeter> outputMeter;

    
    
    std::unique_ptr<PatchMatrixView> mPatchMatrixView;
    //std::unique_ptr<DialogWindow> mPatchbayWindow;
    WeakReference<Component> patchbayCalloutBox;

    

    
    
    
    std::unique_ptr<Viewport> mPeerViewport;
    std::unique_ptr<PeersContainerView> mPeerContainer;

    int peersHeight = 0;
    bool settingsWasShownOnDown = false;
    WeakReference<Component> settingsCalloutBox;

    int inChannels = 0;
    int outChannels = 0;
    
    
    std::unique_ptr<TableListBox> mRemoteSinkListBox;
    std::unique_ptr<TableListBox> mRemoteSourceListBox;
    
    FlexBox mainBox;
    FlexBox titleBox;

    FlexBox inputBox;
    FlexBox addressBox;
    FlexBox middleBox;

    FlexBox remoteBox;
    FlexBox remoteSourceBox;
    FlexBox remoteSinkBox;
    FlexBox paramsBox;
    FlexBox inGainBox;
    FlexBox dryBox;
    FlexBox wetBox;
    FlexBox toolbarBox;
        
    std::unique_ptr<AudioProcessorValueTreeState::SliderAttachment> mInGainAttachment;
    std::unique_ptr<AudioProcessorValueTreeState::SliderAttachment> mInMonPan1Attachment;
    std::unique_ptr<AudioProcessorValueTreeState::SliderAttachment> mInMonPan2Attachment;
    std::unique_ptr<AudioProcessorValueTreeState::SliderAttachment> mDryAttachment;
    std::unique_ptr<AudioProcessorValueTreeState::SliderAttachment> mWetAttachment;
    std::unique_ptr<AudioProcessorValueTreeState::SliderAttachment> mBufferTimeAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FluxAoOAudioProcessorEditor)
};
