

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"

#include "SonobusPluginProcessor.h"
#include "SonoLookAndFeel.h"
#include "SonoChoiceButton.h"
#include "SonoDrawableButton.h"

class PeersContainerView;
class RandomSentenceGenerator;

//==============================================================================
/**
*/
class SonobusAudioProcessorEditor  : public AudioProcessorEditor, public MultiTimer,
public Button::Listener,
public AudioProcessorValueTreeState::Listener,
public Slider::Listener,
public SonoChoiceButton::Listener, 
public SonobusAudioProcessor::ClientListener,
public AsyncUpdater
{
public:
    SonobusAudioProcessorEditor (SonobusAudioProcessor&);
    ~SonobusAudioProcessorEditor();

    //==============================================================================
    void paint (Graphics&) override;
    void resized() override;

    void mouseDown (const MouseEvent& event)  override;
    void mouseUp (const MouseEvent& event)  override;

    void timerCallback(int timerid) override;

    void buttonClicked (Button* buttonThatWasClicked) override;

    void sliderValueChanged (Slider* slider) override;

    void parameterChanged (const String&, float newValue) override;
    void handleAsyncUpdate() override;

    void choiceButtonSelected(SonoChoiceButton *comp, int index, int ident) override;

    void connectWithInfo(const AooServerConnectionInfo & info);

    void showPopTip(const String & message, int timeoutMs, Component * target, int maxwidth=100);

    
    // client listener
    void aooClientConnected(SonobusAudioProcessor *comp, bool success, const String & errmesg="") override;
    void aooClientDisconnected(SonobusAudioProcessor *comp, bool success, const String & errmesg="") override;
    void aooClientLoginResult(SonobusAudioProcessor *comp, bool success, const String & errmesg="") override;
    void aooClientGroupJoined(SonobusAudioProcessor *comp, bool success, const String & group,  const String & errmesg="") override;
    void aooClientGroupLeft(SonobusAudioProcessor *comp, bool success, const String & group, const String & errmesg="") override;
    void aooClientPeerJoined(SonobusAudioProcessor *comp, const String & group, const String & user) override;
    void aooClientPeerLeft(SonobusAudioProcessor *comp, const String & group, const String & user) override;
    void aooClientError(SonobusAudioProcessor *comp, const String & errmesg) override;
    void aooClientPeerChangedState(SonobusAudioProcessor *comp, const String & mesg) override;

    std::function<AudioDeviceManager*()> getAudioDeviceManager; // = []() { return 0; };
    
private:

    void updateLayout();

    void updateState();
    
    void configKnobSlider(Slider *);
    //void configLabel(Label * lab, bool val);
    //void configLevelSlider(Slider *);
    
    void showPatchbay(bool flag);
    void showInPanners(bool flag);

    void showConnectPopup(bool flag);

    
    void showFormatChooser(int peerindex);
    
    void showSettings(bool flag);
    
    void updateServerStatusLabel(const String & mesg, bool mainonly=true);
    void updateChannelState(bool force=false);
    bool updatePeerState(bool force=false);

    void updateOptionsState();

    
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    SonobusAudioProcessor& processor;

    SonoLookAndFeel sonoLookAndFeel;
    SonoBigTextLookAndFeel sonoSliderLNF;

    class PatchMatrixView;

    std::unique_ptr<Label> mTitleLabel;
    

    std::unique_ptr<Label> mLocalAddressStaticLabel;
    std::unique_ptr<Label> mLocalAddressLabel;

    std::unique_ptr<TextButton> mDirectConnectButton;

    std::unique_ptr<TextButton> mConnectButton;

    std::unique_ptr<Label> mMainGroupLabel;
    std::unique_ptr<Label> mMainUserLabel;
    std::unique_ptr<Label> mMainPeerLabel;
    std::unique_ptr<ImageComponent> mMainGroupImage;
    std::unique_ptr<ImageComponent> mMainPersonImage;

    
    std::unique_ptr<Label> mRemoteAddressStaticLabel;
    std::unique_ptr<TextEditor> mAddRemoteHostEditor;
    std::unique_ptr<Label> mDirectConnectDescriptionLabel;

    std::unique_ptr<TextButton> mServerConnectButton;

    std::unique_ptr<Label> mServerHostStaticLabel;
    std::unique_ptr<TextEditor> mServerHostEditor;

    std::unique_ptr<Label> mServerUserStaticLabel;
    std::unique_ptr<TextEditor> mServerUsernameEditor;
    std::unique_ptr<Label> mServerUserPassStaticLabel;
    std::unique_ptr<TextEditor> mServerUserPasswordEditor;

    std::unique_ptr<Label> mServerGroupStaticLabel;
    std::unique_ptr<TextEditor> mServerGroupEditor;
    std::unique_ptr<Label> mServerGroupPassStaticLabel;
    std::unique_ptr<TextEditor> mServerGroupPasswordEditor;

    std::unique_ptr<SonoDrawableButton> mServerGroupRandomButton;


    std::unique_ptr<Label> mServerStatusLabel;
    std::unique_ptr<Label> mMainStatusLabel;

    
    std::unique_ptr<TextButton> mPatchbayButton;
    std::unique_ptr<SonoDrawableButton> mSettingsButton;

    std::unique_ptr<Slider> mInGainSlider;

    std::unique_ptr<TextButton> mPanButton;

    std::unique_ptr<Slider> mInMonPanSlider1;
    std::unique_ptr<Slider> mInMonPanSlider2;
    std::unique_ptr<Label> mInMonPanLabel1;
    std::unique_ptr<Label> mInMonPanLabel2;

    std::unique_ptr<Slider> mDrySlider;
    std::unique_ptr<Slider> mOutGainSlider;
    
    std::unique_ptr<SonoDrawableButton> mMasterMuteButton;

    std::unique_ptr<Slider> mBufferTimeSlider;
    
    std::unique_ptr<Label> mInGainLabel;
    std::unique_ptr<Label> mDryLabel;
    std::unique_ptr<Label> mWetLabel;
    std::unique_ptr<Label> mOutGainLabel;

    std::unique_ptr<TabbedComponent> mConnectTab;
    std::unique_ptr<Component> mDirectConnectContainer;
    std::unique_ptr<Component> mServerConnectContainer;
    std::unique_ptr<Component> mRecentsContainer;

    std::unique_ptr<AudioDeviceSelectorComponent> mAudioDeviceSelector;

    std::unique_ptr<TabbedComponent> mSettingsTab;

    std::unique_ptr<Component> mHelpComponent;
    std::unique_ptr<Component> mOptionsComponent;

    std::unique_ptr<SonoChoiceButton> mOptionsAutosizeDefaultChoice;
    std::unique_ptr<SonoChoiceButton> mOptionsFormatChoiceDefaultChoice;
    std::unique_ptr<Label>  mOptionsAutosizeStaticLabel;
    std::unique_ptr<Label>  mOptionsFormatChoiceStaticLabel;

    std::unique_ptr<SonoTextButton> mRecordingButton;

    
    SonoLookAndFeel inMeterLnf;
    SonoLookAndFeel outMeterLnf;

    std::unique_ptr<foleys::LevelMeter> inputMeter;
    std::unique_ptr<foleys::LevelMeter> outputMeter;

    
    std::unique_ptr<Component> inPannersContainer;
    WeakReference<Component> inPannerCalloutBox;

    //std::unique_ptr<Component> serverContainer;
    WeakReference<Component> serverCalloutBox;

    
    
    std::unique_ptr<PatchMatrixView> mPatchMatrixView;
    //std::unique_ptr<DialogWindow> mPatchbayWindow;
    WeakReference<Component> patchbayCalloutBox;

    std::unique_ptr<BubbleMessageComponent> popTip;

    File lastRecordedFile;
    
    // client state stuff
    CriticalSection clientStateLock;
    struct ClientEvent {
        enum Type {
            None,
            ConnectEvent,
            DisconnectEvent,
            LoginEvent,
            GroupJoinEvent,
            GroupLeaveEvent,
            PeerJoinEvent,
            PeerLeaveEvent,
            PeerChangedState,
            Error
        };
        
        ClientEvent() : type(None) {}
        ClientEvent(Type type_, const String & mesg) : type(type_), success(true), message(mesg) {}
        ClientEvent(Type type_, bool success_, const String & mesg) : type(type_), success(success_), message(mesg) {}
        ClientEvent(Type type_, const String & group_, bool success_, const String & mesg, const String & user_="") : type(type_), success(success_), message(mesg), user(user_), group(group_) {}

        Type type = None;
        bool success = false;
        String message;
        String user;
        String group;
    };
    Array<ClientEvent> clientEvents;
    
    class RecentsListModel : public ListBoxModel
    {
    public:
        RecentsListModel(SonobusAudioProcessorEditor * parent_);
        int getNumRows() override;
        void     paintListBoxItem (int rowNumber, Graphics &g, int width, int height, bool rowIsSelected) override;
        void listBoxItemClicked (int rowNumber, const MouseEvent& e) override;
        void selectedRowsChanged(int lastRowSelected) override;

        void updateState();

    protected:
        SonobusAudioProcessorEditor * parent;

        Image groupImage;
        Image personImage;
        
        Array<AooServerConnectionInfo> recents;
    };
    RecentsListModel recentsListModel;
    Font recentsGroupFont;
    Font recentsNameFont;
    Font recentsInfoFont;
    bool firstTimeConnectShow = true;
    
    std::unique_ptr<ListBox> mRecentsListBox;

    
    bool peerStateUpdated = false;
    double serverStatusFadeTimestamp = 0;
    
    std::unique_ptr<Viewport> mPeerViewport;
    std::unique_ptr<PeersContainerView> mPeerContainer;

    int peersHeight = 0;
    bool isNarrow = false;
    bool settingsWasShownOnDown = false;
    WeakReference<Component> settingsCalloutBox;

    int inChannels = 0;
    int outChannels = 0;
    
    String currGroup;
    bool  currConnected = false;
    
    AooServerConnectionInfo currConnectionInfo;
    
    std::unique_ptr<TableListBox> mRemoteSinkListBox;
    std::unique_ptr<TableListBox> mRemoteSourceListBox;
    
    std::unique_ptr<RandomSentenceGenerator> mRandomSentence;

    
    FlexBox mainBox;
    FlexBox titleBox;

    FlexBox mainGroupUserBox;
    
    FlexBox remoteBox;
    FlexBox inputBox;
    FlexBox addressBox;

    FlexBox serverBox;
    FlexBox servGroupBox;
    FlexBox servGroupPassBox;
    FlexBox servUserBox;
    FlexBox servUserPassBox;
    //FlexBox inputBox;
    FlexBox servAddressBox;
    FlexBox servButtonBox;

    
    FlexBox middleBox;

    FlexBox remoteSourceBox;
    FlexBox remoteSinkBox;
    FlexBox paramsBox;
    FlexBox inGainBox;
    FlexBox dryBox;
    FlexBox wetBox;
    FlexBox toolbarBox;

    FlexBox connectBox;

    FlexBox recentsBox;
    
    FlexBox inPannerMainBox;
    FlexBox inPannerLabelBox;
    FlexBox inPannerBox;

    FlexBox optionsBox;
    FlexBox optionsNetbufBox;
    FlexBox optionsSendQualBox;

    
    std::unique_ptr<AudioProcessorValueTreeState::SliderAttachment> mInGainAttachment;
    std::unique_ptr<AudioProcessorValueTreeState::SliderAttachment> mInMonPan1Attachment;
    std::unique_ptr<AudioProcessorValueTreeState::SliderAttachment> mInMonPan2Attachment;
    std::unique_ptr<AudioProcessorValueTreeState::SliderAttachment> mDryAttachment;
    std::unique_ptr<AudioProcessorValueTreeState::SliderAttachment> mWetAttachment;
    std::unique_ptr<AudioProcessorValueTreeState::SliderAttachment> mBufferTimeAttachment;
    std::unique_ptr<AudioProcessorValueTreeState::ButtonAttachment> mMasterSendMuteAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SonobusAudioProcessorEditor)
};
