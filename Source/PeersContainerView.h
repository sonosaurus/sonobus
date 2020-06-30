/*
  ==============================================================================

    PeersContainerView.h
    Created: 27 Jun 2020 12:42:15pm
    Author:  Jesse Chappell

  ==============================================================================
*/

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"

#include "FluxPluginProcessor.h"
#include "SonoLookAndFeel.h"
#include "SonoChoiceButton.h"
#include "SonoDrawableButton.h"


class PeerViewInfo : public Component {
public:
    PeerViewInfo();
    virtual ~PeerViewInfo() {}
    
    void paint(Graphics& g) override;
    void resized() override;
    
    //void updateLayout();
    
    
    SonoLookAndFeel rmeterLnf;
    SonoLookAndFeel smeterLnf;
    
    std::unique_ptr<Label> nameLabel;
    std::unique_ptr<ToggleButton> sendActiveButton;
    std::unique_ptr<ToggleButton> recvActiveButton;
    std::unique_ptr<SonoDrawableButton> menuButton;
    std::unique_ptr<Label>  statusLabel;
    std::unique_ptr<Label>  levelLabel;
    std::unique_ptr<Slider> levelSlider;
    std::unique_ptr<Slider> panSlider1;
    std::unique_ptr<Slider> panSlider2;
    std::unique_ptr<Label>  bufferTimeLabel;
    std::unique_ptr<Slider> bufferTimeSlider;        
    std::unique_ptr<ToggleButton> autosizeButton;
    std::unique_ptr<SonoChoiceButton> formatChoiceButton;

    std::unique_ptr<Label>  staticLatencyLabel;
    std::unique_ptr<Label>  latencyLabel;
    std::unique_ptr<Label>  staticPingLabel;
    std::unique_ptr<Label>  pingLabel;

    std::unique_ptr<Label>  sendActualBitrateLabel;
    std::unique_ptr<Label>  recvActualBitrateLabel;

    
    std::unique_ptr<foleys::LevelMeter> recvMeter;
    std::unique_ptr<foleys::LevelMeter> sendMeter;
    
    
    FlexBox mainbox;
    FlexBox mainsendbox;
    FlexBox mainrecvbox;
    FlexBox namebox;
    FlexBox sendbox;
    FlexBox sendmeterbox;
    FlexBox recvbox;
    FlexBox recvnetbox;
    FlexBox netstatbox;
    FlexBox pingbox;
    FlexBox latencybox;
    FlexBox recvlevelbox;

    int64_t lastBytesRecv = 0;
    int64_t lastBytesSent = 0;
};

class PeersContainerView : public Component,
public Button::Listener,
public Slider::Listener,
public SonoChoiceButton::Listener
{
public:
    PeersContainerView(FluxAoOAudioProcessor&);
    
    void paint(Graphics & g) override;
    
    void resized() override;
    
    void buttonClicked (Button* buttonThatWasClicked) override;        
    void sliderValueChanged (Slider* slider) override;
    void choiceButtonSelected(SonoChoiceButton *comp, int index, int ident) override;
    
    int getPeerViewCount() const { return mPeerViews.size(); }
    
    void rebuildPeerViews();
    void updatePeerViews();
    
    Rectangle<int> getMinimumContentBounds() const;
    
    void updateLayout();
    
protected:
    
    PeerViewInfo * createPeerViewInfo();
    
    OwnedArray<PeerViewInfo> mPeerViews;
    FluxAoOAudioProcessor& processor;
    
    FlexBox peersBox;
    int peersMinHeight = 120;
    int peersMinWidth = 400;
    
    double lastUpdateTimestampMs = 0;
};

