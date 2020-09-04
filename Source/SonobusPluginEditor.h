

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"

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

class PeersContainerView;
class RandomSentenceGenerator;
class WaveformTransportComponent;

class SonobusAudioProcessorEditor;



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
public CompressorView::Listener,
public ExpanderView::Listener,
public ParametricEqView::Listener,
public EffectsBaseView::HeaderListener
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
    
    void sliderValueChanged (Slider* slider) override;

    void parameterChanged (const String&, float newValue) override;
    void handleAsyncUpdate() override;

    void changeListenerCallback (ChangeBroadcaster* source) override;

    // ApplicationCommandTarget
    void getCommandInfo (CommandID cmdID, ApplicationCommandInfo& info) override;
    void getAllCommands (Array<CommandID>& cmds) override;
    bool perform (const InvocationInfo& info) override;
    ApplicationCommandTarget* getNextCommandTarget() override {
        return findFirstTargetParentComponent();
    }

    

    
    void textEditorReturnKeyPressed (TextEditor&) override;
    void textEditorEscapeKeyPressed (TextEditor&) override;
    void textEditorTextChanged (TextEditor&) override;
    void textEditorFocusLost (TextEditor&) override;
    
    void choiceButtonSelected(SonoChoiceButton *comp, int index, int ident) override;

    void genericItemChooserSelected(GenericItemChooser *comp, int index) override;

    void compressorParamsChanged(CompressorView *comp, SonobusAudioProcessor::CompressorParams & params) override;
    void expanderParamsChanged(ExpanderView *comp, SonobusAudioProcessor::CompressorParams & params) override;
    void parametricEqParamsChanged(ParametricEqView *comp, SonobusAudioProcessor::ParametricEqParams & params) override;
    void effectsHeaderClicked(EffectsBaseView *comp, const MouseEvent & ev) override;

    
    void connectWithInfo(const AooServerConnectionInfo & info);

    void showPopTip(const String & message, int timeoutMs, Component * target, int maxwidth=100);

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
    void aooClientPeerJoined(SonobusAudioProcessor *comp, const String & group, const String & user) override;
    void aooClientPeerLeft(SonobusAudioProcessor *comp, const String & group, const String & user) override;
    void aooClientError(SonobusAudioProcessor *comp, const String & errmesg) override;
    void aooClientPeerChangedState(SonobusAudioProcessor *comp, const String & mesg) override;

    std::function<AudioDeviceManager*()> getAudioDeviceManager; // = []() { return 0; };
    std::function<bool()> isInterAppAudioConnected; // = []() { return 0; };
    std::function<Image(int)> getIAAHostIcon; // = []() { return 0; };
    std::function<void()> switchToHostApplication; // = []() { return 0; };

    void handleURL(const String & urlstr);
    
private:

    void updateLayout();

    void updateState();
    
    void configKnobSlider(Slider *);
    //void configLabel(Label * lab, bool val);
    //void configLevelSlider(Slider *);
    void configEditor(TextEditor *editor, bool passwd = false);

    void showPatchbay(bool flag);
    void showInPanners(bool flag);
    void showMetConfig(bool flag);
    void showEffectsConfig(bool flag);
    void showInEffectsConfig(bool flag, Component * fromView=nullptr);

    void showConnectPopup(bool flag);

    bool handleSonobusURL(const URL & url);

    void showFormatChooser(int peerindex);
    
    void showSettings(bool flag);
    
    void updateServerStatusLabel(const String & mesg, bool mainonly=true);
    void updateChannelState(bool force=false);
    bool updatePeerState(bool force=false);
    void updateTransportState();
    
    void updateOptionsState(bool ignorecheck=false);

    void changeUdpPort(int port);
    
    String generateNewUsername(const AooServerConnectionInfo & info);

    bool attemptToPasteConnectionFromClipboard();
    bool copyInfoToClipboard(bool singleURL=false, String * retmessage = nullptr);
    void updateServerFieldsFromConnectionInfo();

    void openFileBrowser();

    bool loadAudioFromURL(URL fileurl);
    
    class TrimFileJob;

    void trimCurrentAudioFile(bool replaceExisting);
    
    void trimAudioFile(const String & fname, double startPos, double lenSecs, bool replaceExisting);
    void trimFinished(const String & trimmedFile);

    void showFilePopupMenu(Component * source);

    
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    SonobusAudioProcessor& processor;

    SonoLookAndFeel sonoLookAndFeel;
    SonoBigTextLookAndFeel sonoSliderLNF;
    SonoBigTextLookAndFeel smallLNF;
    SonoBigTextLookAndFeel teensyLNF;

    class PatchMatrixView;

    std::unique_ptr<Label> mTitleLabel;
    std::unique_ptr<ImageComponent> mTitleImage;
    

    std::unique_ptr<Label> mLocalAddressStaticLabel;
    std::unique_ptr<Label> mLocalAddressLabel;

    std::unique_ptr<TextButton> mDirectConnectButton;

    std::unique_ptr<SonoTextButton> mConnectButton;

    std::unique_ptr<Label> mMainGroupLabel;
    std::unique_ptr<Label> mMainUserLabel;
    std::unique_ptr<Label> mMainPeerLabel;
    std::unique_ptr<ImageComponent> mMainGroupImage;
    std::unique_ptr<ImageComponent> mMainPersonImage;
    std::unique_ptr<Label> mMainMessageLabel;

    
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
    std::unique_ptr<SonoDrawableButton> mServerPasteButton;
    std::unique_ptr<SonoDrawableButton> mServerCopyButton;
    std::unique_ptr<SonoDrawableButton> mServerShareButton;
    std::unique_ptr<Label> mServerAudioInfoLabel;


    std::unique_ptr<Label> mServerStatusLabel;
    std::unique_ptr<Label> mServerInfoLabel;
    std::unique_ptr<Label> mMainStatusLabel;

    std::unique_ptr<Label> mConnectionTimeLabel;
    std::unique_ptr<Label> mFileRecordingLabel;

    
    std::unique_ptr<TextButton> mPatchbayButton;
    std::unique_ptr<SonoDrawableButton> mSettingsButton;
    std::unique_ptr<SonoDrawableButton> mMainLinkButton;

    std::unique_ptr<Slider> mInGainSlider;

    std::unique_ptr<TextButton> mPanButton;

    std::unique_ptr<Slider> mInMonPanSlider1;
    std::unique_ptr<Slider> mInMonPanSlider2;
    std::unique_ptr<Label> mInMonPanLabel1;
    std::unique_ptr<Label> mInMonPanLabel2;

    std::unique_ptr<Slider> mDrySlider;
    std::unique_ptr<Slider> mOutGainSlider;
    
    std::unique_ptr<SonoDrawableButton> mMainMuteButton;
    std::unique_ptr<SonoDrawableButton> mMainRecvMuteButton;

    std::unique_ptr<SonoDrawableButton> mMetConfigButton;
    std::unique_ptr<SonoDrawableButton> mMetEnableButton;
    std::unique_ptr<SonoDrawableButton> mMetSendButton;
    std::unique_ptr<Label> mMetTempoSliderLabel;
    std::unique_ptr<Slider> mMetTempoSlider;
    std::unique_ptr<Label> mMetLevelSliderLabel;
    std::unique_ptr<Slider> mMetLevelSlider;
    std::unique_ptr<DrawableRectangle> mMetButtonBg;

    std::unique_ptr<DrawableRectangle> mDragDropBg;

    std::unique_ptr<Slider> mBufferTimeSlider;
    
    std::unique_ptr<Label> mInGainLabel;
    std::unique_ptr<Label> mDryLabel;
    std::unique_ptr<Label> mWetLabel;
    std::unique_ptr<Label> mOutGainLabel;

    std::unique_ptr<TabbedComponent> mConnectTab;
    std::unique_ptr<Component> mDirectConnectContainer;
    std::unique_ptr<Viewport> mServerConnectViewport;
    std::unique_ptr<Component> mServerConnectContainer;
    std::unique_ptr<Component> mRecentsContainer;
    std::unique_ptr<GroupComponent> mRecentsGroup;

    std::unique_ptr<Component> mConnectComponent;
    std::unique_ptr<DrawableRectangle> mConnectComponentBg;
    std::unique_ptr<Label> mConnectTitle;
    std::unique_ptr<SonoDrawableButton> mConnectCloseButton;

    
    std::unique_ptr<AudioDeviceSelectorComponent> mAudioDeviceSelector;
    std::unique_ptr<Viewport> mAudioOptionsViewport;
    std::unique_ptr<Viewport> mOtherOptionsViewport;

    std::unique_ptr<TabbedComponent> mSettingsTab;

    std::unique_ptr<Component> mHelpComponent;
    std::unique_ptr<Component> mOptionsComponent;
    int minOptionsHeight = 0;
    int minServerConnectHeight = 0;
    
    std::unique_ptr<Component> mMetContainer;

    std::unique_ptr<Component> mEffectsContainer;
    std::unique_ptr<Component> mInEffectsContainer;

    
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

    std::unique_ptr<SonoDrawableButton> mRecordingButton;
    std::unique_ptr<SonoDrawableButton> mFileBrowseButton;
    std::unique_ptr<SonoDrawableButton> mPlayButton;
    std::unique_ptr<SonoDrawableButton> mDismissTransportButton;
    std::unique_ptr<SonoDrawableButton> mLoopButton;
    std::unique_ptr<SonoDrawableButton> mFileSendAudioButton;
    std::unique_ptr<SonoDrawableButton> mFileMenuButton;
    std::unique_ptr<Slider> mPlaybackSlider;
    std::unique_ptr<WaveformTransportComponent> mWaveformThumbnail;

    // effects
    std::unique_ptr<TextButton> mEffectsButton;
    std::unique_ptr<TextButton> mInEffectsButton;
    std::unique_ptr<CompressorView> mInCompressorView;
    std::unique_ptr<DrawableRectangle> mCompressorBg;
    std::unique_ptr<DrawableRectangle> mReverbHeaderBg;

    std::unique_ptr<ExpanderView> mInExpanderView;
    std::unique_ptr<DrawableRectangle> mExpanderBg;

    std::unique_ptr<ParametricEqView> mInEqView;

    
    std::unique_ptr<SonoChoiceButton> mSendChannelsChoice;
    std::unique_ptr<Label>  mSendChannelsLabel;

    
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

    
    std::unique_ptr<Component> inPannersContainer;
    WeakReference<Component> inPannerCalloutBox;

    //std::unique_ptr<Component> serverContainer;
    WeakReference<Component> serverCalloutBox;

    WeakReference<Component> metCalloutBox;
    WeakReference<Component> effectsCalloutBox;
    WeakReference<Component> inEffectsCalloutBox;

    
    
    std::unique_ptr<PatchMatrixView> mPatchMatrixView;
    //std::unique_ptr<DialogWindow> mPatchbayWindow;
    WeakReference<Component> patchbayCalloutBox;


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
    uint32 settingsClosedTimestamp = 0;

    std::unique_ptr<ListBox> mRecentsListBox;

    
    bool peerStateUpdated = false;
    double serverStatusFadeTimestamp = 0;
    
    std::unique_ptr<Viewport> mPeerViewport;
    std::unique_ptr<PeersContainerView> mPeerContainer;

    int peersHeight = 0;
    bool isNarrow = false;
    bool settingsWasShownOnDown = false;
    bool firstShowInEffects = true;
    WeakReference<Component> settingsCalloutBox;

    int inChannels = 0;
    int outChannels = 0;
    
    String currGroup;
    bool  currConnected = false;
    
    bool mSendChannelsOverridden = false;
    
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


    std::unique_ptr<ConcertinaPanel> mInEffectsConcertina;

    
    FlexBox mainBox;
    FlexBox titleBox;
    FlexBox titleVBox;

    FlexBox mainGroupUserBox;
    
    FlexBox knobBox;    
    
    FlexBox remoteBox;
    FlexBox inputBox;
    FlexBox addressBox;

    FlexBox serverBox;
    FlexBox servStatusBox;
    FlexBox servGroupBox;
    FlexBox servGroupPassBox;
    FlexBox servUserBox;
    FlexBox servUserPassBox;
    //FlexBox inputBox;
    FlexBox servAddressBox;
    FlexBox servButtonBox;
    FlexBox localAddressBox;
    
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
    
    FlexBox metBox;
    FlexBox metVolBox;
    FlexBox metTempoBox;
    FlexBox metSendBox;

    FlexBox effectsBox;
    FlexBox reverbBox;
    FlexBox reverbKnobBox;
    FlexBox reverbCheckBox;
    FlexBox reverbLevelBox;
    FlexBox reverbSizeBox;
    FlexBox reverbDampBox;
    FlexBox reverbPreDelayBox;
    
    FlexBox inEffectsBox;
    
    FlexBox connectMainBox;
    FlexBox connectHorizBox;
    FlexBox connectTitleBox;

    FlexBox connectBox;

    FlexBox recentsBox;
    
    FlexBox inPannerMainBox;
    FlexBox inPannerLabelBox;
    FlexBox inPannerBox;

    FlexBox optionsBox;
    FlexBox optionsNetbufBox;
    FlexBox optionsSendQualBox;
    FlexBox optionsHearlatBox;
    FlexBox optionsMetRecordBox;
    FlexBox optionsUdpBox;
    FlexBox optionsChangeAllQualBox;

    Image iaaHostIcon;

    class CustomTooltipWindow : public TooltipWindow
    {
    public:
        CustomTooltipWindow(SonobusAudioProcessorEditor * parent_) : TooltipWindow(parent_), parent(parent_) {}
        
        String getTipFor (Component& c) override
        {
            if (parent->popTip && parent->popTip->isShowing()) {
                return {};
            }
            return TooltipWindow::getTipFor(c);
        }
                
        SonobusAudioProcessorEditor * parent;
    };
    
    CustomTooltipWindow tooltipWindow{ this };
    
    std::unique_ptr<AudioProcessorValueTreeState::SliderAttachment> mInGainAttachment;
    std::unique_ptr<AudioProcessorValueTreeState::SliderAttachment> mInMonPan1Attachment;
    std::unique_ptr<AudioProcessorValueTreeState::SliderAttachment> mInMonPan2Attachment;
    std::unique_ptr<AudioProcessorValueTreeState::SliderAttachment> mDryAttachment;
    std::unique_ptr<AudioProcessorValueTreeState::SliderAttachment> mWetAttachment;
    std::unique_ptr<AudioProcessorValueTreeState::SliderAttachment> mBufferTimeAttachment;
    std::unique_ptr<AudioProcessorValueTreeState::ButtonAttachment> mMainSendMuteAttachment;
    std::unique_ptr<AudioProcessorValueTreeState::ButtonAttachment> mMainRecvMuteAttachment;
    std::unique_ptr<AudioProcessorValueTreeState::SliderAttachment> mMetTempoAttachment;
    std::unique_ptr<AudioProcessorValueTreeState::SliderAttachment> mMetLevelAttachment;
    std::unique_ptr<AudioProcessorValueTreeState::ButtonAttachment> mMetEnableAttachment;
    std::unique_ptr<AudioProcessorValueTreeState::ButtonAttachment> mMetSendAttachment;
    std::unique_ptr<AudioProcessorValueTreeState::ButtonAttachment> mFileSendAttachment;
    std::unique_ptr<AudioProcessorValueTreeState::ButtonAttachment> mHearLatencyTestAttachment;
    std::unique_ptr<AudioProcessorValueTreeState::ButtonAttachment> mMetRecordedAttachment;
    std::unique_ptr<AudioProcessorValueTreeState::ButtonAttachment> mReverbEnableAttachment;
    std::unique_ptr<AudioProcessorValueTreeState::SliderAttachment> mReverbSizeAttachment;
    std::unique_ptr<AudioProcessorValueTreeState::SliderAttachment> mReverbLevelAttachment;
    std::unique_ptr<AudioProcessorValueTreeState::SliderAttachment> mReverbDampingAttachment;
    std::unique_ptr<AudioProcessorValueTreeState::SliderAttachment> mReverbPreDelayAttachment;

    // keep this down here, so it gets destroyed early
    std::unique_ptr<BubbleMessageComponent> popTip;

    bool iaaConnected = false;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SonobusAudioProcessorEditor)
};
