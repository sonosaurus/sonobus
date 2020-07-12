/*
  ==============================================================================

    PeersContainerView.h
    Created: 27 Jun 2020 12:42:15pm
    Author:  Jesse Chappell

  ==============================================================================
*/

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"

#include "SonobusPluginProcessor.h"
#include "SonoLookAndFeel.h"
#include "SonoChoiceButton.h"
#include "SonoDrawableButton.h"
#include "GenericItemChooser.h"


class PeerViewInfo : public Component{
public:
    PeerViewInfo();
    virtual ~PeerViewInfo() {}
    
    void paint(Graphics& g) override;
    void resized() override;
    
    //void updateLayout();

    SonoBigTextLookAndFeel smallLnf;
    
    SonoLookAndFeel rmeterLnf;
    SonoLookAndFeel smeterLnf;
    
    std::unique_ptr<Label> nameLabel;
    std::unique_ptr<Label> addrLabel;
    std::unique_ptr<ToggleButton> sendActiveButton;
    std::unique_ptr<ToggleButton> recvActiveButton;
    std::unique_ptr<TextButton> latActiveButton;
    std::unique_ptr<SonoDrawableButton> menuButton;
    std::unique_ptr<Label>  statusLabel;
    std::unique_ptr<Label>  levelLabel;
    std::unique_ptr<Slider> levelSlider;
    std::unique_ptr<Slider> panSlider1;
    std::unique_ptr<Slider> panSlider2;
    std::unique_ptr<Label>  bufferTimeLabel;
    std::unique_ptr<Slider> bufferTimeSlider;        
    std::unique_ptr<SonoChoiceButton> autosizeButton;
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
    FlexBox mainnarrowbox;
    FlexBox mainsendbox;
    FlexBox mainrecvbox;
    FlexBox namebox;
    FlexBox nameaddrbox;
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
    
    double stopLatencyTestTimestampMs = 0.0;
    bool wasRecvActiveAtLatencyTest = false;
    bool wasSendActiveAtLatencyTest = false;
};

class PeersContainerView : public Component,
public Button::Listener,
public Slider::Listener,
public SonoChoiceButton::Listener,
public GenericItemChooser::Listener 
{
public:
    PeersContainerView(SonobusAudioProcessor&);
    
    void paint(Graphics & g) override;
    
    void resized() override;
    
    void buttonClicked (Button* buttonThatWasClicked) override;        
    void sliderValueChanged (Slider* slider) override;
    void choiceButtonSelected(SonoChoiceButton *comp, int index, int ident) override;
    
    void mouseUp (const MouseEvent& event)  override;

    
    int getPeerViewCount() const { return mPeerViews.size(); }
    
    void rebuildPeerViews();
    void updatePeerViews();
    
    void setNarrowMode(bool flag) { isNarrow = flag; updateLayout(); }
    bool setNarrowMode() const { return isNarrow; }
    
    void showPopupMenu(Component * source, int index);

    void genericItemChooserSelected(GenericItemChooser *comp, int index) override;
    

    Rectangle<int> getMinimumContentBounds() const;
    
    void updateLayout();
    
protected:
    
    void startLatencyTest(int i);
    void stopLatencyTest(int i);
    
    PeerViewInfo * createPeerViewInfo();
    
    OwnedArray<PeerViewInfo> mPeerViews;
    SonobusAudioProcessor& processor;
    
    FlexBox peersBox;
    int peersMinHeight = 120;
    int peersMinWidth = 400;
    
    bool isNarrow = false;

    double lastUpdateTimestampMs = 0;
};

