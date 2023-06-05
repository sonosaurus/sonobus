// SPDX-License-Identifier: GPLv3-or-later WITH Appstore-exception
// Copyright (C) 2020 Jesse Chappell


#pragma once

#include "JuceHeader.h"

#include "SonobusPluginProcessor.h"
#include "SonoLookAndFeel.h"
#include "SonoChoiceButton.h"
#include "SonoDrawableButton.h"
#include "GenericItemChooser.h"
#include "CompressorView.h"

#include "ChannelGroupsView.h"

#include <map>

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
    SonoPanSliderLookAndFeel panSliderLNF;

    SonoLookAndFeel rmeterLnf;
    SonoLookAndFeel smeterLnf;

    std::unique_ptr<TextEditor> addrLabel;
    std::unique_ptr<Label> staticAddrLabel;
    std::unique_ptr<ToggleButton> sendMutedButton;
    std::unique_ptr<TextButton> recvMutedButton;
    std::unique_ptr<TextButton> recvSoloButton;
    std::unique_ptr<SonoDrawableButton> latActiveButton;
    std::unique_ptr<SonoDrawableButton> sendOptionsButton;
    std::unique_ptr<SonoDrawableButton> recvOptionsButton;
    std::unique_ptr<Label>  statusLabel;
    std::unique_ptr<Label>  bufferTimeLabel;
    std::unique_ptr<Slider> bufferTimeSlider;        
    std::unique_ptr<SonoChoiceButton> autosizeButton;
    std::unique_ptr<SonoChoiceButton> formatChoiceButton;
    std::unique_ptr<ToggleButton> changeAllFormatButton;
    std::unique_ptr<TextButton> optionsResetDropButton;
    std::unique_ptr<TextButton> optionsRemoveButton;
    std::unique_ptr<TextButton> optionsBlockButton;
    std::unique_ptr<SonoChoiceButton> remoteSendFormatChoiceButton;
    std::unique_ptr<ToggleButton> changeAllRecvFormatButton;
    std::unique_ptr<SonoDrawableButton> bufferMinButton;
    std::unique_ptr<SonoDrawableButton> bufferMinFrontButton;
    std::unique_ptr<Drawable> recvButtonImage;
    std::unique_ptr<Drawable> sendButtonImage;


    
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

    std::unique_ptr<ChannelGroupsView> channelGroups;


    std::unique_ptr<DrawableRectangle> sendStatsBg;
    std::unique_ptr<DrawableRectangle> recvStatsBg;
    std::unique_ptr<DrawableRectangle> pingBg;


    std::unique_ptr<foleys::LevelMeter> recvMeter;
    std::unique_ptr<foleys::LevelMeter> sendMeter;
    
    
    FlexBox mainbox;
    FlexBox mainnarrowbox;
    FlexBox mainsendbox;
    FlexBox mainrecvbox;
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
    bool isNarrow = false;
    bool fullMode = true;
    bool addrClicked = false;
    
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
    std::unique_ptr<TextButton>  removeButton;
    std::unique_ptr<TextButton>  unblockButton;

    FlexBox mainbox;

    Colour bgColor;
    Colour borderColor;
};

class PeersContainerView : public Component,
public Button::Listener,
public Slider::Listener,
public SonoChoiceButton::Listener,
public GenericItemChooser::Listener,
public ChannelGroupsView::Listener,
public MultiTimer
{
public:
    PeersContainerView(SonobusAudioProcessor&);

    class Listener {
    public:
        virtual ~Listener() {}
        virtual void internalSizesChanged(PeersContainerView *comp) {}
    };
    void addListener(Listener * listener) { listeners.add(listener); }
    void removeListener(Listener * listener) { listeners.remove(listener); }

    void paint(Graphics & g) override;
    
    void resized() override;
    
    void buttonClicked (Button* buttonThatWasClicked) override;        
    void sliderValueChanged (Slider* slider) override;
    void choiceButtonSelected(SonoChoiceButton *comp, int index, int ident) override;
    
    void mouseUp (const MouseEvent& event)  override;

    void mouseDown (const MouseEvent& event)  override;
    void mouseDrag (const MouseEvent& event)  override;


    void timerCallback(int timerId) override;
    
    
    int getPeerViewCount() const { return mPeerViews.size(); }
    
    void resetPendingUsers();
    void peerPendingJoin(String & group, String & user);
    void peerFailedJoin(String & group, String & user);
    void peerBlockedJoin(String & group, String & user, String & address, int port);
    void peerLeftGroup(String & group, String & user);
    int getPendingPeerCount() const { return (int)mPendingUsers.size(); }
    
    void rebuildPeerViews();
    void updatePeerViews(int specific=-1);
    
    void setNarrowMode(bool flag) { isNarrow = flag; updateLayout(); }
    bool setNarrowMode() const { return isNarrow; }
    
    void showPopupMenu(Component * source, int index);

    void genericItemChooserSelected(GenericItemChooser *comp, int index) override;
    

    // channelgroupsview
    void channelLayoutChanged(ChannelGroupsView *comp) override;
    void nameLabelClicked(ChannelGroupsView *comp) override;

    void clearClipIndicators();
    
    juce::Rectangle<int> getMinimumContentBounds() const;

    void applyToAllSliders(std::function<void(Slider *)> & routine);

    void updateLayout();

    std::function<AudioDeviceManager*()> getAudioDeviceManager; // = []() { return 0; };

    void setPeerDisplayMode(SonobusAudioProcessor::PeerDisplayMode mode);

    void updateAvailableCodecOptions(const String & username);

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
    void showSendOptions(int index, bool flag, Component * fromView=nullptr);
    void showRecvOptions(int index, bool flag, Component * fromView=nullptr);

    void updatePeerOrdering();
    int getPeerFromIndex(int index);
    juce::Rectangle<int> getBoundsForPeer(int chgroup);
    int getPeerForPoint(Point<int> pos, bool inbetween);

    ListenerList<Listener> listeners;

    OwnedArray<PeerViewInfo> mPeerViews;
    SonobusAudioProcessor& processor;

    // key is username, value is priority (lower is first)
    std::map<String, int> mPeerPriorityOrdering;
    
    std::vector<int> mPeerUpdateOrdering;

    std::unique_ptr<BubbleMessageComponent> popTip;

    struct PendingUserInfo {
        PendingUserInfo(const String & group_="", const String & user_="") : group(group_), user(user_) {}
        String group;
        String user;
        bool failed = false;
        bool blocked = false;
        String address;
        int    port = 0;
    };
    
    std::map<String,PendingUserInfo> mPendingUsers;

    OwnedArray<PendingPeerViewInfo> mPendingPeerViews;

    std::unique_ptr<ChannelGroupEffectsView> mEffectsView;

    std::unique_ptr<DrawableImage> mDragDrawable;
    std::unique_ptr<DrawableRectangle> mInsertLine;

    
    WeakReference<Component> pannerCalloutBox;
    WeakReference<Component> recvOptionsCalloutBox;
    WeakReference<Component> sendOptionsCalloutBox;
    WeakReference<Component> effectsCalloutBox;

    FlexBox peersBox;
    int peersMinHeight = 120;
    int peersMinWidth = 400;

    int mLastWidth = 0;
    bool isNarrow = false;
    bool peerModeFull = true; // default

    uint32 lastUpdateTimestampMs = 0;
    
    Colour mutedBySoloColor;
    Colour mutedTextColor;
    Colour mutedColor;
    Colour soloColor;
    Colour regularTextColor;
    Colour droppedTextColor;
    Colour dimTextColor;
    Colour outlineColor;
    Colour bgColor;

    // dragging state
    bool mDraggingActive = false;
    int mDraggingSourcePeer = -1;
    int mDraggingGroupPos = -1;
    bool mIgnoreNameClick = false;
    Array< juce::Rectangle<int> > mPeerViewBounds;
    Image  mDragImage;
    bool mAutoscrolling = false;


    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PeersContainerView)

};
