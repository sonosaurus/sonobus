// SPDX-License-Identifier: GPLv3-or-later WITH Appstore-exception
// Copyright (C) 2020 Jesse Chappell



#pragma once

#include <JuceHeader.h>

#include "SonobusPluginProcessor.h"
#include "SonoLookAndFeel.h"
#include "SonoChoiceButton.h"
#include "SonoDrawableButton.h"
#include "SonoTextButton.h"
#include "SonoUtility.h"
#include "GenericItemChooser.h"
#include "CompressorView.h"
#include "ExpanderView.h"
#include "ParametricEqView.h"
#include "EffectParams.h"
#include "ConnectView.h"
#include "ChannelGroupsView.h"
#include "PeersContainerView.h"
#include "OptionsView.h"
#include "ReverbView.h"
#include "VDONinjaView.h"

class RandomSentenceGenerator;
class WaveformTransportComponent;

class SonobusAudioProcessorEditor;
class ChannelGroupsView;
class MonitorDelayView;
class ChatView;
class SoundboardView;
class LatencyMatchView;
class SuggestNewGroupView;

//==============================================================================
/**
*/
class SonobusAudioProcessorEditor  : public AudioProcessorEditor, public MultiTimer,
public Button::Listener,
public AudioProcessorValueTreeState::Listener,
public Slider::Listener,
public SonoChoiceButton::Listener, 
public SonobusAudioProcessor::ClientListener,
public ComponentListener,
public ChangeListener,
public TextEditor::Listener,
public ApplicationCommandTarget,
public AsyncUpdater,
public FileDragAndDropTarget,
public GenericItemChooser::Listener,
public ConnectView::Listener,
public ChannelGroupsView::Listener,
public PeersContainerView::Listener
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

    void componentVisibilityChanged (Component& component) override;
    void componentParentHierarchyChanged (Component& component) override;
    void componentMovedOrResized (Component&, bool wasmoved, bool wasresized) override;

    void sliderValueChanged (Slider* slider) override;

    void parameterChanged (const String&, float newValue) override;
    void handleAsyncUpdate() override;

    void changeListenerCallback (ChangeBroadcaster* source) override;

    void parentHierarchyChanged() override;

    
    bool keyPressed (const KeyPress & key) override;
    bool keyStateChanged (bool isKeyDown) override;
    
    // ApplicationCommandTarget
    void getCommandInfo (CommandID cmdID, ApplicationCommandInfo& info) override;
    void getAllCommands (Array<CommandID>& cmds) override;
    bool perform (const InvocationInfo& info) override;
    ApplicationCommandTarget* getNextCommandTarget() override {
        return findFirstTargetParentComponent();
    }

    // connectview
    void connectionsChanged(ConnectView *comp) override;

    // channelgroupsview
    void channelLayoutChanged(ChannelGroupsView *comp) override;

    // PeerContainerView
    void internalSizesChanged(PeersContainerView *comp) override;

    void textEditorReturnKeyPressed (TextEditor&) override;
    void textEditorEscapeKeyPressed (TextEditor&) override;
    void textEditorTextChanged (TextEditor&) override;
    void textEditorFocusLost (TextEditor&) override;
    
    void choiceButtonSelected(SonoChoiceButton *comp, int index, int ident) override;

    void genericItemChooserSelected(GenericItemChooser *comp, int index) override;


    void connectWithInfo(const AooServerConnectionInfo & info, bool allowEmptyGroup = false, bool copyInfoOnly=false);

    void showPopTip(const String & message, int timeoutMs, Component * target, int maxwidth=100);

    void updateUseKeybindings();


    // file drop

    bool isInterestedInFileDrag (const StringArray& /*files*/) override;
    void filesDropped (const StringArray& files, int /*x*/, int /*y*/) override;
    void fileDragEnter (const StringArray& files, int x, int y) override;
    void fileDragExit (const StringArray& files) override;

    // client listener
    void aooClientConnected(SonobusAudioProcessor *comp, bool success, const String & errmesg="") override;
    void aooClientDisconnected(SonobusAudioProcessor *comp, bool success, const String & errmesg="") override;
    void aooClientLoginResult(SonobusAudioProcessor *comp, bool success, const String & errmesg="") override;
    void aooClientGroupJoined(SonobusAudioProcessor *comp, bool success, const String & group,  const String & errmesg="") override;
    void aooClientGroupLeft(SonobusAudioProcessor *comp, bool success, const String & group, const String & errmesg="") override;
    void aooClientPublicGroupModified(SonobusAudioProcessor *comp, const String & group, int count, const String & errmesg="") override;
    void aooClientPublicGroupDeleted(SonobusAudioProcessor *comp, const String & group,  const String & errmesg="") override;
    void aooClientPeerJoined(SonobusAudioProcessor *comp, const String & group, const String & user) override;
    void aooClientPeerPendingJoin(SonobusAudioProcessor *comp, const String & group, const String & user) override;
    void aooClientPeerJoinFailed(SonobusAudioProcessor *comp, const String & group, const String & user) override;
    void aooClientPeerJoinBlocked(SonobusAudioProcessor *comp, const String & group, const String & user, const String & address, int port) override;
    void aooClientPeerLeft(SonobusAudioProcessor *comp, const String & group, const String & user) override;
    void aooClientError(SonobusAudioProcessor *comp, const String & errmesg) override;
    void aooClientPeerChangedState(SonobusAudioProcessor *comp, const String & mesg) override;
    void sbChatEventReceived(SonobusAudioProcessor *comp, const SBChatEvent & mesg) override;
    void peerRequestedLatencyMatch(SonobusAudioProcessor *comp, const String & username, float latency) override;
    void peerBlockedInfoChanged(SonobusAudioProcessor *comp, const String & username, bool blocked) override;
    void peerSuggestedNewGroup(SonobusAudioProcessor *comp, const String & username, const String & newgroup, const String & groupPass, bool isPublic, const StringArray & others) override;


    std::function<AudioDeviceManager*()> getAudioDeviceManager; // = []() { return 0; };
    std::function<bool()> isInterAppAudioConnected; // = []() { return 0; };
    std::function<Image(int)> getIAAHostIcon; // = []() { return 0; };
    std::function<void()> switchToHostApplication; // = []() { return 0; };
    std::function<Value*()> getShouldOverrideSampleRateValue; // = []() { return 0; };
    std::function<Value*()> getShouldCheckForNewVersionValue; // = []() { return 0; };
    std::function<Value*()> getAllowBluetoothInputValue; // = []() { return 0; };

    std::function<StringArray*()> getRecentSetupFiles; // = []() { return 0; };
    std::function<String*()> getLastRecentsFolder; // = []() { return 0; };

    std::function<void()> saveSettingsIfNeeded; // = []() { return 0; };

    void handleURL(const String & urlstr);
    

    ChannelGroupsView * getInputChannelGroupsView() { return mInputChannelsContainer.get(); }
    PeersContainerView * getPeersContainerView() { return mPeerContainer.get(); }

    // if returns true signifies go ahead and quit now, otherwise we'll handle it
    bool requestedQuit();


    bool loadSettingsFromFile(const File & file);
    bool saveSettingsToFile(const File & file);

    bool setupLocalisation(const String & overrideLang = {});

    // OSC control registration methods
    void registerAllOSCControls();
    void unregisterAllOSCControls();
    void sendAllOSCState();
    void sendPeerOSCState(int peerIndex);
    void clearPeerOSCState(int peerIndex);

private:

    void updateLayout();

    void updateState(bool rebuildInputChannels = true);
    
    void configKnobSlider(Slider *);
    //void configLabel(Label * lab, bool val);
    void configLevelSlider(Slider *);
    void configEditor(TextEditor *editor, bool passwd = false);

    void showPatchbay(bool flag);
    void showMetConfig(bool flag);
    void showEffectsConfig(bool flag);

    void showGroupMenu(bool show);

    void showConnectPopup(bool flag);

    void showFormatChooser(int peerindex);
    
    void showSettings(bool flag);

    void showMonitorDelayView(bool flag);

    void showInputReverbView(bool flag);

    void updateServerStatusLabel(const String & mesg, bool mainonly=true);
    void updateChannelState(bool force=false);
    bool updatePeerState(bool force=false);
    void updateTransportState();
    
    void updateOptionsState(bool ignorecheck=false);

    
    String generateNewUsername(const AooServerConnectionInfo & info);



    void openFileBrowser();
    void chooseRecDirBrowser();

    bool loadAudioFromURL(const URL & fileurl);
    bool updateTransportWithURL(const URL & fileurl);

    class TrimFileJob;

    void trimCurrentAudioFile(bool replaceExisting);
    
    void trimAudioFile(const String & fname, double startPos, double lenSecs, bool replaceExisting);
    void trimFinished(const String & trimmedFile);

    void showFilePopupMenu(Component * source);

    void showLatencyMatchPrompt(const String & name, float latencyms);
    void showLatencyMatchView(bool show);

    void showSuggestedGroupPrompt(const String & name, const String & group, const String & grouppass, bool ispublic, const StringArray & others);
    void showSuggestGroupView(bool show);

    void showVDONinjaView(bool show, bool fromVideoButton=true);
    void copyGroupLink();

    void resetJitterBufferForAll();
    void recordRoundtripLatencyForAll(const String & fullpath, const String & filename);

    void requestRecordDir(std::function<void (URL)> callback);
    
    void updateSliderSnap();


    void showSaveSettingsPreset();
    void showLoadSettingsPreset();

    void showChatPanel(bool show, bool allowresize=true);
    void showSoundboardPanel(bool show, bool allowresize=true);

    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    SonobusAudioProcessor& processor;

    SonoLookAndFeel sonoLookAndFeel;
    SonoBigTextLookAndFeel sonoSliderLNF;
    SonoBigTextLookAndFeel smallLNF;
    SonoBigTextLookAndFeel teensyLNF;
    SonoPanSliderLookAndFeel panSliderLNF;

    class PatchMatrixView;

    std::unique_ptr<Label> mTitleLabel;
    std::unique_ptr<ImageComponent> mTitleImage;
    

    std::unique_ptr<SonoTextButton> mConnectButton;
    std::unique_ptr<SonoDrawableButton> mAltConnectButton;
    std::unique_ptr<SonoDrawableButton> mVideoButton;

    std::unique_ptr<Label> mMainGroupLabel;
    std::unique_ptr<Label> mMainUserLabel;
    std::unique_ptr<Label> mMainPeerLabel;
    std::unique_ptr<ImageComponent> mMainGroupImage;
    std::unique_ptr<ImageComponent> mMainPersonImage;
    std::unique_ptr<Label> mMainMessageLabel;

    std::unique_ptr<SonoDrawableButton> mPeerLayoutFullButton;
    std::unique_ptr<SonoDrawableButton> mPeerLayoutMinimalButton;


    std::unique_ptr<Label> mServerStatusLabel;
    std::unique_ptr<Label> mServerInfoLabel;
    std::unique_ptr<Label> mMainStatusLabel;

    std::unique_ptr<Label> mConnectionTimeLabel;
    std::unique_ptr<Label> mFileRecordingLabel;

    
    std::unique_ptr<TextButton> mPatchbayButton;
    std::unique_ptr<SonoDrawableButton> mSettingsButton;
    std::unique_ptr<SonoDrawableButton> mMainLinkButton;
    std::unique_ptr<Drawable> mMainLinkArrow;

    std::unique_ptr<Slider> mInGainSlider;

    std::unique_ptr<TextButton> mInMixerButton;


    std::unique_ptr<TextButton> mInMuteButton;
    std::unique_ptr<TextButton> mInSoloButton;

    std::unique_ptr<TextButton> mMonDelayButton;

    
    std::unique_ptr<Slider> mDrySlider;
    std::unique_ptr<Slider> mOutGainSlider;
    
    std::unique_ptr<SonoDrawableButton> mMainMuteButton;
    std::unique_ptr<SonoDrawableButton> mMainRecvMuteButton;
    std::unique_ptr<SonoDrawableButton> mMainPushToTalkButton;

    std::unique_ptr<SonoDrawableButton> mMetConfigButton;
    std::unique_ptr<SonoDrawableButton> mMetEnableButton;
    std::unique_ptr<SonoDrawableButton> mMetSendButton;
    std::unique_ptr<TextButton> mMetSyncButton;
    std::unique_ptr<TextButton> mMetSyncFileButton;
    std::unique_ptr<Label> mMetTempoSliderLabel;
    std::unique_ptr<Slider> mMetTempoSlider;
    std::unique_ptr<Label> mMetLevelSliderLabel;
    std::unique_ptr<Slider> mMetLevelSlider;
    std::unique_ptr<DrawableRectangle> mMetButtonBg;

    std::unique_ptr<DrawableRectangle> mDragDropBg;

    std::unique_ptr<DrawableRectangle> mFileAreaBg;


    std::unique_ptr<Label> mInGainLabel;
    std::unique_ptr<Label> mDryLabel;
    std::unique_ptr<Label> mWetLabel;
    std::unique_ptr<Label> mOutGainLabel;

    std::unique_ptr<ConnectView> mConnectView;

    std::unique_ptr<OptionsView> mOptionsView;

    //std::unique_ptr<TabbedComponent> mSettingsTab;

    uint32 settingsClosedTimestamp = 0;
    int minServerConnectHeight = 0;

    std::unique_ptr<Component> mMetContainer;

    std::unique_ptr<Component> mEffectsContainer;


    std::unique_ptr<SonoDrawableButton> mRecordingButton;
    std::unique_ptr<SonoDrawableButton> mFileBrowseButton;
    std::unique_ptr<SonoDrawableButton> mPlayButton;
    std::unique_ptr<SonoDrawableButton> mSkipBackButton;
    std::unique_ptr<SonoDrawableButton> mDismissTransportButton;
    std::unique_ptr<SonoDrawableButton> mLoopButton;
    std::unique_ptr<SonoDrawableButton> mFileSendAudioButton;
    std::unique_ptr<SonoDrawableButton> mFileMenuButton;
    std::unique_ptr<Slider> mPlaybackSlider;
    std::unique_ptr<WaveformTransportComponent> mWaveformThumbnail;

    std::unique_ptr<Drawable> mPeerRecImage;
    std::unique_ptr<Drawable> mPeerRecStealthImage;


    // effects
    std::unique_ptr<TextButton> mEffectsButton;

    std::unique_ptr<DrawableRectangle> mReverbHeaderBg;

    std::unique_ptr<MonitorDelayView> mMonitorDelayView;

    std::unique_ptr<SonoDrawableButton> mBufferMinButton;
    std::unique_ptr<SonoDrawableButton> mRecvSyncButton;


    
    std::unique_ptr<SonoChoiceButton> mSendChannelsChoice;
    std::unique_ptr<Label>  mSendChannelsLabel;

    std::unique_ptr<SonoTextButton> mSetupAudioButton;

    
    std::unique_ptr<SonoDrawableButton> mReverbEnabledButton;
    std::unique_ptr<Label>  mReverbTitleLabel;
    std::unique_ptr<SonoChoiceButton> mReverbModelChoice;
    std::unique_ptr<Label>  mReverbLevelLabel;
    std::unique_ptr<Slider> mReverbLevelSlider;
    std::unique_ptr<Label>  mReverbSizeLabel;
    std::unique_ptr<Slider> mReverbSizeSlider;
    std::unique_ptr<Label>  mReverbDampingLabel;
    std::unique_ptr<Slider> mReverbDampingSlider;
    std::unique_ptr<Label>  mReverbPreDelayLabel;
    std::unique_ptr<Slider> mReverbPreDelaySlider;


    class ApproveComponent;

    // latency match stuff
    std::unique_ptr<ApproveComponent>  mLatMatchApproveComponent;
    std::unique_ptr<LatencyMatchView> mLatMatchView;

    std::unique_ptr<ApproveComponent>  mSuggestedGroupComponent;
    std::unique_ptr<SuggestNewGroupView> mSuggestNewGroupView;


    // vdo ninja
    std::unique_ptr<VDONinjaView> mVDONinjaView;


    std::unique_ptr<FileChooser> mFileChooser;
    File  mCurrOpenDir;
    URL mCurrentAudioFile;
    bool mReloadFile = false;
    
    std::unique_ptr<ThreadPool> mWorkPool;

    std::unique_ptr<SonoDrawableButton> mIAAHostButton;

    
    SonoLookAndFeel inMeterLnf;
    SonoLookAndFeel outMeterLnf;

    std::unique_ptr<foleys::LevelMeter> inputMeter;
    std::unique_ptr<foleys::LevelMeter> outputMeter;


    //std::unique_ptr<Component> serverContainer;
    WeakReference<Component> serverCalloutBox;

    WeakReference<Component> metCalloutBox;
    WeakReference<Component> effectsCalloutBox;

    WeakReference<Component> monDelayCalloutBox;

    WeakReference<Component> latmatchCalloutBox;
    WeakReference<Component> latmatchViewCalloutBox;

    WeakReference<Component> suggestedGroupCalloutBox;
    WeakReference<Component> suggestNewGroupViewCalloutBox;

    WeakReference<Component> vdoninjaViewCalloutBox;


    std::unique_ptr<PatchMatrixView> mPatchMatrixView;
    //std::unique_ptr<DialogWindow> mPatchbayWindow;
    WeakReference<Component> patchbayCalloutBox;

    bool mPanChanged = false;
    

    URL lastRecordedFile;

    String mActiveLanguageCode;

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
            PeerPendingJoinEvent,
            PeerFailedJoinEvent,
            PeerBlockedJoinEvent,
            PublicGroupModifiedEvent,
            PublicGroupDeletedEvent,
            PeerRequestedLatencyMatchEvent,
            PeerBlockedInfoChangedEvent,
            PeerSuggestedNewGroupEvent,
            Error
        };
        
        ClientEvent() : type(None) {}
        ClientEvent(Type type_, const String & mesg) : type(type_), success(true), message(mesg) {}
        ClientEvent(Type type_, bool success_, const String & mesg) : type(type_), success(success_), message(mesg) {}
        ClientEvent(Type type_, const String & group_, bool success_, const String & mesg, const String & user_="", float fval=0.0f) : type(type_), success(success_), message(mesg), user(user_), group(group_), floatVal(fval) {}
        ClientEvent(Type type_, const String & mesg, float val) : type(type_), success(true), message(mesg), floatVal(val) {}

        static ClientEvent makeSuggestedNewGroupEvent(const String & user, const String & group_, const String & passwd, bool ispublic, const StringArray & others) {
            ClientEvent event(PeerSuggestedNewGroupEvent, group_, ispublic, passwd, user);
            event.array = others;
            return event;
        }

        Type type = None;
        bool success = false;
        String message;
        String user;
        String group;
        float floatVal = 0.0f;
        StringArray array;
    };
    Array<ClientEvent> clientEvents;

    CriticalSection    chatStateLock;
    Array<SBChatEvent> newChatEvents;
    Atomic<bool>  haveNewChatEvents  { false };

    std::unique_ptr<ChatView> mChatView;
    std::unique_ptr<ComponentBoundsConstrainer> mChatSizeConstrainer;
    std::unique_ptr<ResizableEdgeComponent> mChatEdgeResizer;
    std::unique_ptr<SonoDrawableButton> mChatButton;
    bool mAboutToShowChat = false;
    bool mChatShowDidResize = false;
    bool mChatWasVisible = false;
    bool mChatOverlay  = false;
    volatile bool mIgnoreResize = false;

   std::unique_ptr<SoundboardView> mSoundboardView;
   std::unique_ptr<SonoDrawableButton> mSoundboardButton;
   std::unique_ptr<ComponentBoundsConstrainer> mSoundboardSizeConstrainer;
   std::unique_ptr<ResizableEdgeComponent> mSoundboardEdgeResizer;
   bool mAboutToShowSoundboard = false;
   bool mSoundboardShowDidResize = false;
   bool mSoundboardWasVisible = false;

    bool peerStateUpdated = false;
    double serverStatusFadeTimestamp = 0;

    std::unique_ptr<Component> mTopLevelContainer;

    std::unique_ptr<Viewport> mMainViewport;
    std::unique_ptr<Component> mMainContainer;
    std::unique_ptr<PeersContainerView> mPeerContainer;

    std::unique_ptr<Viewport> mInputChannelsViewport;
    std::unique_ptr<ChannelGroupsView> mInputChannelsContainer;

    int peersHeight = 0;
    bool isNarrow = false;
    bool isReallyNarrow = false;
    bool settingsWasShownOnDown = false;
    WeakReference<Component> settingsCalloutBox;

    int inChannels = 0;
    int outChannels = 0;

    String currGroup;
    bool  currConnected = false;
    
    bool mSendChannelsOverridden = false;

    bool mPushToTalkKeyDown = false;
    bool mPushToTalkWasMuted = true;
    
    bool mAltReleaseIsPending = false;
    bool mAltReleaseShouldAct = false;

    AooServerConnectionInfo currConnectionInfo;
    
    std::unique_ptr<TableListBox> mRemoteSinkListBox;
    std::unique_ptr<TableListBox> mRemoteSourceListBox;
    
    
    class SonobusCommandManager : public ApplicationCommandManager {
    public:
        SonobusCommandManager(SonobusAudioProcessorEditor & parent_) : parent(parent_) {}
        ApplicationCommandTarget* getFirstCommandTarget(CommandID commandID) override {
            return &parent;
        }
    private:
        SonobusAudioProcessorEditor & parent;
    };


    void populateRecentSetupsMenu(PopupMenu & popup);
    void addToRecentsSetups(const File & file);

    SonobusCommandManager commandManager { *this };

    
    class SonobusMenuBarModel
     : public MenuBarModel
    {
    public:
        SonobusMenuBarModel(SonobusAudioProcessorEditor & parent_) : parent(parent_) {}
        
        // MenuBarModel
        StringArray getMenuBarNames() override;
        PopupMenu getMenuForIndex (int topLevelMenuIndex, const String& /*menuName*/) override;
        void menuItemSelected (int menuItemID, int topLevelMenuIndex) override;

        
    protected:
        SonobusAudioProcessorEditor & parent;
    };
    
    std::unique_ptr<SonobusMenuBarModel> menuBarModel;
    std::unique_ptr<MenuBarComponent> mMenuBar;


    
    std::unique_ptr<RandomSentenceGenerator> mRandomSentence;



    FlexBox mainBox;
    FlexBox titleBox;
    FlexBox titleVBox;

    FlexBox mainGroupUserBox;
    FlexBox mainGroupBox;
    FlexBox mainUserBox;

    FlexBox mainGroupLayoutBox;


    FlexBox knobBox;    
    
    FlexBox remoteBox;
    FlexBox inputBox;
    FlexBox addressBox;

    FlexBox connectBox;

    FlexBox middleBox;

    FlexBox remoteSourceBox;
    FlexBox remoteSinkBox;
    FlexBox paramsBox;
    FlexBox inGainBox;
    FlexBox dryBox;
    FlexBox wetBox;
    FlexBox toolbarBox;
    FlexBox toolbarTextBox;
    FlexBox outBox;
    FlexBox transportBox;
    FlexBox transportWaveBox;
    FlexBox transportVBox;
    FlexBox recBox;
    FlexBox knobButtonBox;    
    FlexBox inMeterBox;
    FlexBox mainMeterBox;

    FlexBox inputMainBox;
    FlexBox inputButtonBox;
    FlexBox inputRightBox;
    FlexBox inputLeftBox;
    FlexBox inputPannerBox;

    FlexBox outputMainBox;


    FlexBox metBox;
    FlexBox metVolBox;
    FlexBox metTempoBox;
    FlexBox metSendBox;
    FlexBox metSendSyncBox;

    FlexBox effectsBox;
    FlexBox reverbBox;
    FlexBox reverbKnobBox;
    FlexBox reverbCheckBox;
    FlexBox reverbLevelBox;
    FlexBox reverbSizeBox;
    FlexBox reverbDampBox;
    FlexBox reverbPreDelayBox;

    FlexBox inPannerMainBox;
    FlexBox inPannerLabelBox;
    FlexBox inPannerBox;

    FlexBox latMatchBox;
    FlexBox latMatchButtBox;

    Image iaaHostIcon;

    class CustomTooltipWindow : public TooltipWindow
    {
    public:
        CustomTooltipWindow(SonobusAudioProcessorEditor * parent_, Component * viewparent) : TooltipWindow(viewparent), parent(parent_) {}
        virtual ~CustomTooltipWindow() {
            if (parent) {
                // reset our smart pointer without a delete! someone else is deleting it
                parent->tooltipWindow.release();
            }
        }
        
        String getTipFor (Component& c) override
        {
            if (parent->popTip && parent->popTip->isShowing()) {
                return {};
            }
            return TooltipWindow::getTipFor(c);
        }
                
        SonobusAudioProcessorEditor * parent;
    };
    
    std::unique_ptr<CustomTooltipWindow> tooltipWindow;
    
    std::unique_ptr<AudioProcessorValueTreeState::SliderAttachment> mInGainAttachment;
    std::unique_ptr<AudioProcessorValueTreeState::SliderAttachment> mDryAttachment;
    std::unique_ptr<AudioProcessorValueTreeState::SliderAttachment> mWetAttachment;
    std::unique_ptr<AudioProcessorValueTreeState::ButtonAttachment> mMainSendMuteAttachment;
    std::unique_ptr<AudioProcessorValueTreeState::ButtonAttachment> mMainRecvMuteAttachment;
    std::unique_ptr<AudioProcessorValueTreeState::SliderAttachment> mMetTempoAttachment;
    std::unique_ptr<AudioProcessorValueTreeState::ButtonAttachment> mMetSyncAttachment;
    std::unique_ptr<AudioProcessorValueTreeState::ButtonAttachment> mMetSyncFileAttachment;
    std::unique_ptr<AudioProcessorValueTreeState::SliderAttachment> mMetLevelAttachment;
    std::unique_ptr<AudioProcessorValueTreeState::ButtonAttachment> mMetEnableAttachment;
    std::unique_ptr<AudioProcessorValueTreeState::ButtonAttachment> mMetSendAttachment;
    std::unique_ptr<AudioProcessorValueTreeState::ButtonAttachment> mFileSendAttachment;
    std::unique_ptr<AudioProcessorValueTreeState::ButtonAttachment> mHearLatencyTestAttachment;
    std::unique_ptr<AudioProcessorValueTreeState::ButtonAttachment> mReverbEnableAttachment;
    std::unique_ptr<AudioProcessorValueTreeState::SliderAttachment> mReverbSizeAttachment;
    std::unique_ptr<AudioProcessorValueTreeState::SliderAttachment> mReverbLevelAttachment;
    std::unique_ptr<AudioProcessorValueTreeState::SliderAttachment> mReverbDampingAttachment;
    std::unique_ptr<AudioProcessorValueTreeState::SliderAttachment> mReverbPreDelayAttachment;
    std::unique_ptr<AudioProcessorValueTreeState::ButtonAttachment> mInMonSoloAttachment;
    std::unique_ptr<AudioProcessorValueTreeState::ButtonAttachment> mInMonMuteAttachment;

    // keep this down here, so it gets destroyed early
    std::unique_ptr<BubbleMessageComponent> popTip;

    ScopedMessageBox mScopedShareBox;

    bool iaaConnected = false;

    File mSettingsFolder;
    
    bool mOSCControlsRegistered = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SonobusAudioProcessorEditor)
};
