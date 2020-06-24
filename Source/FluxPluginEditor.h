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
    void configLabel(Label * lab, bool val);

    void showPatchbay(bool flag);
    
    void showFormatChooser(int peerindex);
    
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
    std::unique_ptr<TextButton> mAddRemoteSourceButton;
    std::unique_ptr<TextButton> mAddRemoteSinkButton;
    std::unique_ptr<TextButton> mRemoveRemoteSourceButton;
    std::unique_ptr<TextButton> mRemoveRemoteSinkButton;
    std::unique_ptr<ToggleButton> mStreamingEnabledButton;
    std::unique_ptr<TextButton> mPatchbayButton;

    std::unique_ptr<Slider> mInGainSlider;
    std::unique_ptr<Slider> mDrySlider;
    std::unique_ptr<Slider> mWetSlider;
    std::unique_ptr<Slider> mBufferTimeSlider;
    
    std::unique_ptr<Label> mInGainLabel;
    std::unique_ptr<Label> mDryLabel;
    std::unique_ptr<Label> mWetLabel;
    std::unique_ptr<Label> mBufferTimeLabel;


    
    
    std::unique_ptr<PatchMatrixView> mPatchMatrixView;
    //std::unique_ptr<DialogWindow> mPatchbayWindow;
    WeakReference<Component> patchbayCalloutBox;

    struct PeerViewInfo {
        std::unique_ptr<Label> nameLabel;
        std::unique_ptr<ToggleButton> sendActiveButton;
        std::unique_ptr<ToggleButton> recvActiveButton;
        std::unique_ptr<Label>  statusLabel;
        std::unique_ptr<Label>  levelLabel;
        std::unique_ptr<Slider> levelSlider;
        std::unique_ptr<Label>  bufferTimeLabel;
        std::unique_ptr<Slider> bufferTimeSlider;        
        std::unique_ptr<TextButton> removeButton;        
        std::unique_ptr<SonoChoiceButton> formatChoiceButton;
        std::unique_ptr<Slider> packetsizeSlider;        

        FlexBox mainbox;
        FlexBox namebox;
        FlexBox activebox;
        FlexBox controlsbox;
    };
    
    
    OwnedArray<PeerViewInfo> mPeerViews;

    PeerViewInfo * createPeerViewInfo();
    void rebuildPeerViews();
    void updatePeerViews();

    
    std::unique_ptr<TableListBox> mRemoteSinkListBox;
    std::unique_ptr<TableListBox> mRemoteSourceListBox;
    
    FlexBox mainBox;
    FlexBox titleBox;
    FlexBox middleBox;
    FlexBox localBox;
    FlexBox remoteBox;
    FlexBox remoteSourceBox;
    FlexBox remoteSinkBox;
    FlexBox paramsBox;
    FlexBox inGainBox;
    FlexBox dryBox;
    FlexBox wetBox;
    FlexBox bufferTimeBox;
    FlexBox peersBox;
        
    std::unique_ptr<AudioProcessorValueTreeState::SliderAttachment> mInGainAttachment;
    std::unique_ptr<AudioProcessorValueTreeState::SliderAttachment> mDryAttachment;
    std::unique_ptr<AudioProcessorValueTreeState::SliderAttachment> mWetAttachment;
    std::unique_ptr<AudioProcessorValueTreeState::SliderAttachment> mBufferTimeAttachment;
    std::unique_ptr<AudioProcessorValueTreeState::ButtonAttachment> mStreamingEnabledAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FluxAoOAudioProcessorEditor)
};
