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
#include "CompressorView.h"

class JitterBufferMeter;

class PeerViewInfo : public Component{
public:
    PeerViewInfo();
    virtual ~PeerViewInfo();
    
    void paint(Graphics& g) override;
    void resized() override;
    
    //void updateLayout();

    SonoBigTextLookAndFeel smallLnf;
    SonoBigTextLookAndFeel medLnf;
    SonoBigTextLookAndFeel sonoSliderLNF;

    SonoLookAndFeel rmeterLnf;
    SonoLookAndFeel smeterLnf;
    
    std::unique_ptr<Label> nameLabel;
    std::unique_ptr<Label> addrLabel;
    std::unique_ptr<Label> staticAddrLabel;
    std::unique_ptr<ToggleButton> sendMutedButton;
    std::unique_ptr<TextButton> recvMutedButton;
    std::unique_ptr<TextButton> recvSoloButton;
    std::unique_ptr<SonoDrawableButton> latActiveButton;
    std::unique_ptr<SonoDrawableButton> sendOptionsButton;
    std::unique_ptr<SonoDrawableButton> recvOptionsButton;
    std::unique_ptr<TextButton> fxButton;
    std::unique_ptr<TextButton> panButton;
    std::unique_ptr<Label>  statusLabel;
    std::unique_ptr<Label>  levelLabel;
    std::unique_ptr<Slider> levelSlider;
    std::unique_ptr<Slider> panSlider1;
    std::unique_ptr<Slider> panSlider2;
    std::unique_ptr<Label>  bufferTimeLabel;
    std::unique_ptr<Slider> bufferTimeSlider;        
    std::unique_ptr<SonoChoiceButton> autosizeButton;
    std::unique_ptr<SonoChoiceButton> formatChoiceButton;
    std::unique_ptr<ToggleButton> changeAllFormatButton;
    std::unique_ptr<TextButton> optionsResetDropButton;
    std::unique_ptr<TextButton> optionsRemoveButton;
    std::unique_ptr<SonoChoiceButton> remoteSendFormatChoiceButton;
    std::unique_ptr<SonoDrawableButton> bufferMinButton;

    std::unique_ptr<Component> effectsContainer;

    std::unique_ptr<CompressorView> compressorView;
   

    
    std::unique_ptr<Component> pannersContainer;
    std::unique_ptr<Component> sendOptionsContainer;
    std::unique_ptr<Component> recvOptionsContainer;

    std::unique_ptr<Label>  staticLatencyLabel;
    std::unique_ptr<Label>  latencyLabel;
    std::unique_ptr<Label>  staticPingLabel;
    std::unique_ptr<Label>  pingLabel;

    std::unique_ptr<Label>  staticSendQualLabel;
    std::unique_ptr<Label>  sendQualityLabel;
    std::unique_ptr<Label>  staticBufferLabel;
    std::unique_ptr<Label>  bufferLabel;

    
    std::unique_ptr<Label>  staticFormatChoiceLabel;
    std::unique_ptr<Label>  staticRemoteSendFormatChoiceLabel;

    std::unique_ptr<Label>  sendActualBitrateLabel;
    std::unique_ptr<Label>  recvActualBitrateLabel;

    double fillRatio = 0.0;
    std::unique_ptr<JitterBufferMeter> jitterBufferMeter;

    std::unique_ptr<DrawableRectangle> sendStatsBg;
    std::unique_ptr<DrawableRectangle> recvStatsBg;
    std::unique_ptr<DrawableRectangle> pingBg;

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
    FlexBox pannerbox;
    
    FlexBox recvOptionsBox;
    FlexBox optionsNetbufBox;
    FlexBox optionsstatbox;
    FlexBox optionsbuttbox;
    FlexBox optionsaddrbox;
    FlexBox optionsRemoteQualityBox;

    FlexBox sendOptionsBox;
    FlexBox optionsSendQualBox;
    FlexBox optionsChangeAllQualBox;
    FlexBox optionsSendMutedBox;
    
    
    FlexBox effectsBox;

    FlexBox squalbox;
    FlexBox netbufbox;
    FlexBox recvstatbox;
    
    int64_t lastBytesRecv = 0;
    int64_t lastBytesSent = 0;

    int64_t lastDropped = 0;
    uint32 lastDroppedChangedTimestampMs = 0;

    uint32 stopLatencyTestTimestampMs = 0;
    bool wasRecvActiveAtLatencyTest = false;
    bool wasSendActiveAtLatencyTest = false;
    
    bool singlePanner = true;
    
    Colour bgColor;
    Colour borderColor;
    Colour itemColor;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PeerViewInfo)
};

class PendingPeerViewInfo : public Component {
public:
    PendingPeerViewInfo();
    virtual ~PendingPeerViewInfo();
    
    void paint(Graphics& g) override;
    void resized() override;
    
    std::unique_ptr<Label>  nameLabel;
    std::unique_ptr<Label>  messageLabel;

    FlexBox mainbox;

    Colour bgColor;
    Colour borderColor;
};

class PeersContainerView : public Component,
public Button::Listener,
public Slider::Listener,
public SonoChoiceButton::Listener,
public GenericItemChooser::Listener,
public CompressorView::Listener,
public MultiTimer
{
public:
    PeersContainerView(SonobusAudioProcessor&);
    
    void paint(Graphics & g) override;
    
    void resized() override;
    
    void buttonClicked (Button* buttonThatWasClicked) override;        
    void sliderValueChanged (Slider* slider) override;
    void choiceButtonSelected(SonoChoiceButton *comp, int index, int ident) override;
    
    void mouseUp (const MouseEvent& event)  override;

    void timerCallback(int timerId) override;
    
    
    int getPeerViewCount() const { return mPeerViews.size(); }
    
    void resetPendingUsers();
    void peerPendingJoin(String & group, String & user);
    void peerFailedJoin(String & group, String & user);
    int getPendingPeerCount() const { return mPendingUsers.size(); }
    
    void rebuildPeerViews();
    void updatePeerViews();
    
    void setNarrowMode(bool flag) { isNarrow = flag; updateLayout(); }
    bool setNarrowMode() const { return isNarrow; }
    
    void showPopupMenu(Component * source, int index);

    void genericItemChooserSelected(GenericItemChooser *comp, int index) override;
    
    void compressorParamsChanged(CompressorView *comp, SonobusAudioProcessor::CompressorParams & params) override;


    void clearClipIndicators();
    
    Rectangle<int> getMinimumContentBounds() const;
    
    void updateLayout();
    
protected:
    
    void startLatencyTest(int i);
    void stopLatencyTest(int i);
    
    void configLevelSlider(Slider * slider);    
    void configLabel(Label *label, int ltype);
    void configKnobSlider(Slider * slider);    
    
    String generateLatencyMessage(const SonobusAudioProcessor::LatencyInfo &latinfo);

    
    PeerViewInfo * createPeerViewInfo();

    PendingPeerViewInfo * createPendingPeerViewInfo();
    
    void showPopTip(const String & message, int timeoutMs, Component * target, int maxwidth);
    void showPanners(int index, bool flag);
    void showSendOptions(int index, bool flag, Component * fromView=nullptr);
    void showRecvOptions(int index, bool flag, Component * fromView=nullptr);
    void showEffects(int index, bool flag, Component * fromView=nullptr);

    
    OwnedArray<PeerViewInfo> mPeerViews;
    SonobusAudioProcessor& processor;

    
    std::unique_ptr<BubbleMessageComponent> popTip;

    struct PendingUserInfo {
        PendingUserInfo(const String & group_="", const String & user_="") : group(group_), user(user_) {}
        String group;
        String user;
        bool failed = false;
    };
    
    std::map<String,PendingUserInfo> mPendingUsers;

    OwnedArray<PendingPeerViewInfo> mPendingPeerViews;

    
    WeakReference<Component> pannerCalloutBox;
    WeakReference<Component> recvOptionsCalloutBox;
    WeakReference<Component> sendOptionsCalloutBox;
    WeakReference<Component> effectsCalloutBox;

    FlexBox peersBox;
    int peersMinHeight = 120;
    int peersMinWidth = 400;
    
    bool isNarrow = false;

    uint32 lastUpdateTimestampMs = 0;
    
    Colour mutedTextColor;
    Colour soloColor;
    Colour regularTextColor;
    Colour droppedTextColor;
    Colour dimTextColor;
    Colour outlineColor;
    Colour bgColor;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PeersContainerView)

};

