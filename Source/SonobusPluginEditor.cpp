

#include "SonobusPluginProcessor.h"
#include "SonobusPluginEditor.h"

#include "BeatToggleGrid.h"
#include "DebugLogC.h"

#include "PeersContainerView.h"

#include "RandomSentenceGenerator.h"

#include <sstream>

enum {
    PeriodicUpdateTimerId = 0
};


enum {
    nameTextColourId = 0x1002830,
    selectedColourId = 0x1002840,
    separatorColourId = 0x1002850,
};


class SonobusAudioProcessorEditor::PatchMatrixView : public Component, public BeatToggleGridDelegate
{
public:
    PatchMatrixView(SonobusAudioProcessor& p) : Component(), processor(p) {
        
        grid = std::make_unique<BeatToggleGrid>();
        grid->setDelegate(this);
        
        addAndMakeVisible(grid.get());
        
        updateGrid();
    }
    virtual ~PatchMatrixView() {
    }
    
    bool beatToggleGridPressed(BeatToggleGrid* grid, int index, const MouseEvent & event) override { 
        int count = processor.getNumberRemotePeers();
        if (count == 0) return false;
        
        int src = index/count;
        int dest = index % count;        
        bool currval = processor.getPatchMatrixValue(src, dest);
        valonpress = currval;
        
        processor.setPatchMatrixValue(src, dest, !currval);

        updateGrid();

        return true;
    }
    
    bool beatToggleGridMoved(BeatToggleGrid* grid, int fromIndex, int toIndex, const MouseEvent & event) override {
        int count = processor.getNumberRemotePeers();
        if (count == 0) return false;
        
        int tosrc = toIndex/count;
        int todest = toIndex % count;        
        bool currval = processor.getPatchMatrixValue(tosrc, todest);
        
        processor.setPatchMatrixValue(tosrc, todest, !valonpress);
        
        updateGrid();
        return true;
    }
    
    bool beatToggleGridReleased(BeatToggleGrid* grid, int index, const MouseEvent & event) override {
        return true;
    }
    
    void updateGrid()
    {
        int count = processor.getNumberRemotePeers();
        if ( count*count != grid->getItems()) {
            updateGridLayout();
        }
        
        for (int i=0; i < count; ++i) {
            for (int j=0; j < count; ++j) {
                int item = i*count + j;
                
                grid->setSelected(processor.getPatchMatrixValue(i, j), item);
                if (i == j) {
                    grid->setAccented(processor.getPatchMatrixValue(i, j), item);                    
                }
            }            
        }
        
        grid->refreshGrid(false);
        repaint();
    }
    
    Rectangle<int> getPreferredBounds() const
    {
        int count = processor.getNumberRemotePeers();
        int labwidth = 30;
        int labheight= 18;
        int buttwidth = 60;
        int buttheight = 44;

        Rectangle<int> prefsize;
        prefsize.setX(0);
        prefsize.setY(0);
        prefsize.setWidth(count * buttwidth + labwidth);
        prefsize.setHeight(count * buttheight + labheight);

        return prefsize;
    }

    
    void updateGridLayout() {

        int count = processor.getNumberRemotePeers();
        
        /*
        if (count == 0) {
            grid->setVisible(false);
            return;
        } else {
            grid->setVisible(true);
        }
        */
        
        
        topbox.items.clear();
        topbox.flexDirection = FlexBox::Direction::row;    
        leftbox.items.clear();
        leftbox.flexDirection = FlexBox::Direction::column;    

        while (leftLabels.size() < count) {
            Label * leftlab = new Label("lab", String::formatted("%d", leftLabels.size() + 1));
            leftlab->setJustificationType(Justification::centred);
            leftLabels.add(leftlab);

            Label * toplab = new Label("lab", String::formatted("%d", topLabels.size() + 1));
            toplab->setJustificationType(Justification::centred);
            topLabels.add(toplab);

            addAndMakeVisible( leftLabels.getUnchecked(leftLabels.size()-1));
            addAndMakeVisible( topLabels.getUnchecked(topLabels.size()-1));
        }
        
        for (int i=0; i < leftLabels.size(); ++i) {
            leftLabels.getUnchecked(i)->setVisible(i < count);
            topLabels.getUnchecked(i)->setVisible(i < count);
        }
        
        int labwidth = 30;
        int labheight= 18;
        int buttheight = 36;
        
        topbox.items.add(FlexItem(labwidth, labheight));

        grid->setItems(count * count);
        grid->setSegments(count);
        
        for (int i=0; i < count; ++i) {
            grid->setSegmentSize(count, i);
        }
        
        grid->refreshGrid(true);
        
        for (int i=0; i < count; ++i) {

            for (int j=0; j < count; ++j) {
                int item = i*count + j;
                grid->setLabel(String::formatted("%d>%d", i+1, j+1), item);                
            }
            
            topbox.items.add(FlexItem(20, labheight, *topLabels.getUnchecked(i)).withMargin(2).withFlex(1));
            leftbox.items.add(FlexItem(20, labheight, *leftLabels.getUnchecked(i)).withMargin(2).withFlex(1));            
        }
        

        middlebox.items.clear();
        middlebox.flexDirection = FlexBox::Direction::row;    
        middlebox.items.add(FlexItem(labwidth, labheight, leftbox).withMargin(2).withFlex(0));
        middlebox.items.add(FlexItem(labwidth, buttheight, *grid).withMargin(2).withFlex(1));
        

        
        mainbox.items.clear();
        mainbox.flexDirection = FlexBox::Direction::column;    
        mainbox.items.add(FlexItem(labwidth*2, labheight, topbox).withMargin(2).withFlex(0).withMaxHeight(30));
        mainbox.items.add(FlexItem(labwidth*2, buttheight, middlebox).withMargin(2).withFlex(1));
        
        //grid->refreshGrid(false);
        
        resized();
    }
    
    void paint (Graphics& g) override
    {
        // (Our component is opaque, so we must completely fill the background with a solid colour)
        //g.fillAll (getLookAndFeel().findColour (ColourScheme::UIColour::widgetBackground));

        //g.setColour (Colours::white);
        //g.setFont (15.0f);
        //g.drawFittedText ("Hello World!", getLocalBounds(), Justification::centred, 1);
    }

    void resized() override
    {
        Component::resized();
        
        //updateGridLayout();
        
        mainbox.performLayout(getLocalBounds());    

    }
    
    std::unique_ptr<BeatToggleGrid> grid;
    OwnedArray<Label> leftLabels;
    OwnedArray<Label> topLabels;

    FlexBox mainbox;
    FlexBox middlebox;
    FlexBox leftbox;
    FlexBox topbox;

    
    bool valonpress = false;
    
    SonobusAudioProcessor & processor;
};




static void configLabel(Label *label, bool val)
{
    if (val) {
        label->setFont(12);
        label->setColour(Label::textColourId, Colour(0x90eeeeee));
        label->setJustificationType(Justification::centred);        
    }
    else {
        label->setFont(14);
        //label->setColour(Label::textColourId, Colour(0xaaeeeeee));
        label->setJustificationType(Justification::centredLeft);
    }
}

static void configServerLabel(Label *label)
{
    label->setFont(14);
    label->setColour(Label::textColourId, Colour(0x90eeeeee));
    label->setJustificationType(Justification::centredRight);        
}

static void configEditor(TextEditor *editor, bool passwd = false)
{
    if (passwd)  {
        editor->setIndents(8, 6);        
    } else {
        editor->setIndents(8, 8);
    }
}


//==============================================================================
SonobusAudioProcessorEditor::SonobusAudioProcessorEditor (SonobusAudioProcessor& p)
    : AudioProcessorEditor (&p), processor (p),  sonoSliderLNF(14),
recentsListModel(this),
recentsGroupFont (17.0, Font::bold), recentsNameFont(15, Font::plain), recentsInfoFont(13, Font::plain)
{
    LookAndFeel::setDefaultLookAndFeel(&sonoLookAndFeel);
    
    sonoLookAndFeel.setUsingNativeAlertWindows(true);

    setColour (nameTextColourId, Colour::fromFloatRGBA(1.0f, 1.0f, 1.0f, 0.9f));
    setColour (selectedColourId, Colour::fromFloatRGBA(0.0f, 0.4f, 0.8f, 0.5f));
    setColour (separatorColourId, Colour::fromFloatRGBA(0.3f, 0.3f, 0.3f, 0.3f));

    
    Array<AooServerConnectionInfo> recents;
    processor.getRecentServerConnectionInfos(recents);
    if (recents.size() > 0) {
        currConnectionInfo = recents.getReference(0);
    }
    else {
        // defaults
        currConnectionInfo.groupName = "Default";
#if JUCE_IOS
        String username = SystemStats::getComputerName(); //SystemStats::getLogonName();
#else
        String username = SystemStats::getFullUserName(); //SystemStats::getLogonName();    
        if (username.length() > 0) username = username.replaceSection(0, 1, username.substring(0, 1).toUpperCase());
#endif

        currConnectionInfo.userName = username;

        currConnectionInfo.serverHost = "aoo.sonobus.net";
        currConnectionInfo.serverPort = 10998;
    }
    
    mTitleLabel = std::make_unique<Label>("title", TRANS("SonoBus"));
    mTitleLabel->setFont(18);
    mTitleLabel->setColour(Label::textColourId, Colour::fromFloatRGBA(0.4f, 0.6f, 0.8f, 1.0f));
    mTitleLabel->setInterceptsMouseClicks(true, false);
    mTitleLabel->addMouseListener(this, false);
    
    mMainGroupLabel = std::make_unique<Label>("group", "");
    mMainGroupLabel->setJustificationType(Justification::centredLeft);
    mMainGroupLabel->setFont(16);

    mMainUserLabel = std::make_unique<Label>("user", "");
    mMainUserLabel->setJustificationType(Justification::centredRight);
    mMainUserLabel->setFont(14);

    mMainPeerLabel = std::make_unique<Label>("peers", "");
    mMainPeerLabel->setJustificationType(Justification::centred);
    mMainPeerLabel->setFont(14);

    //mMainPersonImage.reset(Drawable::createFromImageData(BinaryData::person_svg, BinaryData::person_svgSize));
    //mMainGroupImage.reset(Drawable::createFromImageData(BinaryData::people_svg, BinaryData::people_svgSize));

    mMainPersonImage = std::make_unique<ImageComponent>();
    mMainPersonImage->setImage(ImageCache::getFromMemory(BinaryData::person_png, BinaryData::person_pngSize));
    mMainGroupImage = std::make_unique<ImageComponent>();
    mMainGroupImage->setImage(ImageCache::getFromMemory(BinaryData::people_png, BinaryData::people_pngSize));

    
    
    //mInGainSlider     = std::make_unique<Slider>(Slider::LinearBar,  Slider::TextBoxBelow);
    mInGainSlider     = std::make_unique<Slider>(Slider::RotaryHorizontalVerticalDrag,  Slider::TextBoxAbove);
    mInGainSlider->setName("ingain");
    mInGainSlider->setSliderSnapsToMousePosition(false);
    mInGainSlider->setTextBoxIsEditable(false);
    mInGainSlider->setScrollWheelEnabled(false);

    
    mInMonPanSlider1     = std::make_unique<Slider>(Slider::LinearBar,  Slider::TextBoxBelow); 
    mInMonPanSlider1->setName("inpan1");
    mInMonPanSlider1->getProperties().set ("fromCentre", true);
    mInMonPanSlider1->getProperties().set ("noFill", true);
    mInMonPanSlider1->setSliderSnapsToMousePosition(false);
    mInMonPanSlider1->setTextBoxIsEditable(false);
    mInMonPanSlider1->setScrollWheelEnabled(false);
    mInMonPanSlider1->setMouseDragSensitivity(100);

    mInMonPanLabel1 = std::make_unique<Label>(SonobusAudioProcessor::paramDry, TRANS("Pan 1"));
    configLabel(mInMonPanLabel1.get(), true);
    
    mInMonPanSlider2     = std::make_unique<Slider>(Slider::LinearBar,  Slider::TextBoxBelow); 
    mInMonPanSlider2->setName("inpan2");
    mInMonPanSlider2->getProperties().set ("fromCentre", true);
    mInMonPanSlider2->getProperties().set ("noFill", true);
    mInMonPanSlider2->setSliderSnapsToMousePosition(false);
    mInMonPanSlider2->setTextBoxIsEditable(false);
    mInMonPanSlider2->setScrollWheelEnabled(false);
    mInMonPanSlider2->setMouseDragSensitivity(100);

    mInMonPanLabel2 = std::make_unique<Label>(SonobusAudioProcessor::paramDry, TRANS("Pan 2"));
    configLabel(mInMonPanLabel2.get(), true);
    
    
    inPannersContainer = std::make_unique<Component>();
    
    mPanButton = std::make_unique<TextButton>("pan");
    mPanButton->setButtonText(TRANS("Pan"));
    mPanButton->addListener(this);

    
    mMasterMuteButton = std::make_unique<SonoDrawableButton>("sendmute", DrawableButton::ButtonStyle::ImageOnButtonBackground);
    std::unique_ptr<Drawable> sendallowimg(Drawable::createFromImageData(BinaryData::outgoing_allowed_svg, BinaryData::outgoing_allowed_svgSize));
    std::unique_ptr<Drawable> senddisallowimg(Drawable::createFromImageData(BinaryData::outgoing_disallowed_svg, BinaryData::outgoing_disallowed_svgSize));
    mMasterMuteButton->setImages(sendallowimg.get(), nullptr, nullptr, nullptr, senddisallowimg.get());
    mMasterMuteButton->addListener(this);
    mMasterMuteButton->setClickingTogglesState(true);
    //pvf->sendMutedButton->setColour(TextButton::buttonOnColourId, Colour::fromFloatRGBA(0.15, 0.15, 0.15, 0.7));
    mMasterMuteButton->setColour(TextButton::buttonOnColourId, Colour::fromFloatRGBA(0.2, 0.2, 0.2, 0.7));
    mMasterMuteButton->setColour(TextButton::buttonColourId, Colours::transparentBlack);
    mMasterMuteButton->setTooltip(TRANS("Toggles input muting, preventing or allowing audio to be sent all users"));
    
    
    //mDrySlider     = std::make_unique<Slider>(Slider::LinearBar,  Slider::TextBoxBelow);
    mDrySlider     = std::make_unique<Slider>(Slider::RotaryHorizontalVerticalDrag,  Slider::TextBoxAbove);
    mDrySlider->setName("dry");
    mDrySlider->setSliderSnapsToMousePosition(false);
    mDrySlider->setTextBoxIsEditable(false);
    mDrySlider->setScrollWheelEnabled(false);

    mOutGainSlider     = std::make_unique<Slider>(Slider::LinearBar,  Slider::TextBoxBelow); 
    mOutGainSlider->setName("wet");
    mOutGainSlider->setSliderSnapsToMousePosition(false);
    mOutGainSlider->setTextBoxIsEditable(false);
    mOutGainSlider->setScrollWheelEnabled(false);

    configKnobSlider(mInGainSlider.get());
    configKnobSlider(mDrySlider.get());
    //configKnobSlider(mWetSlider.get());
    
    mBufferTimeSlider    = std::make_unique<Slider>(Slider::LinearBar,  Slider::TextBoxRight);
    mBufferTimeSlider->setName("time");
    mBufferTimeSlider->setSliderSnapsToMousePosition(false);
    mBufferTimeSlider->setChangeNotificationOnlyOnRelease(true);
    mBufferTimeSlider->setDoubleClickReturnValue(true, 15.0);
    mBufferTimeSlider->setTextBoxIsEditable(false);
    mBufferTimeSlider->setScrollWheelEnabled(false);

    mInGainLabel = std::make_unique<Label>(SonobusAudioProcessor::paramDry, TRANS("Input Level"));
    configLabel(mInGainLabel.get(), false);
    mInGainLabel->setJustificationType(Justification::centred);
    
    mDryLabel = std::make_unique<Label>(SonobusAudioProcessor::paramDry, TRANS("Input Monitor"));
    configLabel(mDryLabel.get(), false);
    mDryLabel->setJustificationType(Justification::centred);

    //mWetLabel = std::make_unique<Label>(SonobusAudioProcessor::paramWet, TRANS("Wet"));
    //configLabel(mWetLabel.get(), false);

    mOutGainLabel = std::make_unique<Label>("outgain", TRANS("Output Level"));
    configLabel(mOutGainLabel.get(), false);

    
    
    
    
    mInGainAttachment     = std::make_unique<AudioProcessorValueTreeState::SliderAttachment> (p.getValueTreeState(), SonobusAudioProcessor::paramInGain, *mInGainSlider);
    mDryAttachment     = std::make_unique<AudioProcessorValueTreeState::SliderAttachment> (p.getValueTreeState(), SonobusAudioProcessor::paramDry, *mDrySlider);
    mWetAttachment     = std::make_unique<AudioProcessorValueTreeState::SliderAttachment> (p.getValueTreeState(), SonobusAudioProcessor::paramWet, *mOutGainSlider);
    mBufferTimeAttachment     = std::make_unique<AudioProcessorValueTreeState::SliderAttachment> (p.getValueTreeState(), SonobusAudioProcessor::paramDefaultNetbufMs, *mBufferTimeSlider);
    mInMonPan1Attachment     = std::make_unique<AudioProcessorValueTreeState::SliderAttachment> (p.getValueTreeState(), SonobusAudioProcessor::paramInMonitorPan1, *mInMonPanSlider1);
    mInMonPan2Attachment     = std::make_unique<AudioProcessorValueTreeState::SliderAttachment> (p.getValueTreeState(), SonobusAudioProcessor::paramInMonitorPan2, *mInMonPanSlider2);
    mMasterSendMuteAttachment = std::make_unique<AudioProcessorValueTreeState::ButtonAttachment> (p.getValueTreeState(), SonobusAudioProcessor::paramMasterSendMute, *mMasterMuteButton);

    processor.getValueTreeState().addParameterListener (SonobusAudioProcessor::paramMasterSendMute, this);

    
    mConnectTab = std::make_unique<TabbedComponent>(TabbedButtonBar::Orientation::TabsAtTop);
    mConnectTab->setOutline(0);
    mConnectTab->setTabBarDepth(36);
    mConnectTab->getTabbedButtonBar().setMinimumTabScaleFactor(0.1f);
    mConnectTab->getTabbedButtonBar().setColour(TabbedButtonBar::frontTextColourId, Colour::fromFloatRGBA(0.4, 0.8, 1.0, 1.0));
    mConnectTab->getTabbedButtonBar().setColour(TabbedButtonBar::frontOutlineColourId, Colour::fromFloatRGBA(0.4, 0.8, 1.0, 0.5));


    mDirectConnectContainer = std::make_unique<Component>();
    mServerConnectContainer = std::make_unique<Component>();
    mRecentsContainer = std::make_unique<Component>();
    
    mConnectTab->addTab(TRANS("RECENTS"), Colour::fromFloatRGBA(0.1, 0.1, 0.1, 1.0), mRecentsContainer.get(), false);
    mConnectTab->addTab(TRANS("GROUP"), Colour::fromFloatRGBA(0.1, 0.1, 0.1, 1.0), mServerConnectContainer.get(), false);
    mConnectTab->addTab(TRANS("DIRECT"), Colour::fromFloatRGBA(0.1, 0.1, 0.1, 1.0), mDirectConnectContainer.get(), false);
    

    
    mLocalAddressLabel = std::make_unique<Label>("localaddr", TRANS("--"));
    mLocalAddressLabel->setJustificationType(Justification::centredLeft);
    mLocalAddressStaticLabel = std::make_unique<Label>("localaddrst", TRANS("Local Address:"));
    mLocalAddressStaticLabel->setJustificationType(Justification::centredRight);


    mRemoteAddressStaticLabel = std::make_unique<Label>("remaddrst", TRANS("Host: "));
    mRemoteAddressStaticLabel->setJustificationType(Justification::centredRight);

    mDirectConnectDescriptionLabel = std::make_unique<Label>("remaddrst", TRANS("Connect directly to other instances of SonoBus on your local network with the local address that they advertise."));
    mDirectConnectDescriptionLabel->setJustificationType(Justification::topLeft);
    
    mAddRemoteHostEditor = std::make_unique<TextEditor>("remaddredit");
    mAddRemoteHostEditor->setFont(Font(16));
    mAddRemoteHostEditor->setText(""); // 100.36.128.246:11000
    mAddRemoteHostEditor->setTextToShowWhenEmpty(TRANS("IPaddress:port"), Colour(0x44ffffff));
    
    //mAddRemotePortEditor = std::make_unique<TextEditor>("remportedit");
    //mAddRemotePortEditor->setFont(Font(16));
    //mAddRemotePortEditor->setText("11000");
    
    
    mConnectButton = std::make_unique<TextButton>("directconnect");
    mConnectButton->setButtonText(TRANS("Connect..."));
    mConnectButton->addListener(this);
    mConnectButton->setColour(TextButton::buttonColourId, Colour::fromFloatRGBA(0.6, 0.4, 0.6, 0.6));

    mDirectConnectButton = std::make_unique<TextButton>("directconnect");
    mDirectConnectButton->setButtonText(TRANS("Direct Connect"));
    mDirectConnectButton->addListener(this);
    mDirectConnectButton->setColour(TextButton::buttonColourId, Colour::fromFloatRGBA(0.1, 0.4, 0.6, 0.6));

    
    mServerConnectButton = std::make_unique<TextButton>("serverconnect");
    mServerConnectButton->setButtonText(TRANS("Connect to Group"));
    mServerConnectButton->addListener(this);
    mServerConnectButton->setColour(TextButton::buttonColourId, Colour::fromFloatRGBA(0.1, 0.4, 0.6, 0.6));
    
    mServerHostEditor = std::make_unique<TextEditor>("srvaddredit");
    mServerHostEditor->setFont(Font(14));
    mServerHostEditor->setText( String::formatted("%s:%d", currConnectionInfo.serverHost.toRawUTF8(), currConnectionInfo.serverPort)); // 100.36.128.246 // 23.23.205.37
    configEditor(mServerHostEditor.get());
    
    mServerUsernameEditor = std::make_unique<TextEditor>("srvaddredit");
    mServerUsernameEditor->setFont(Font(16));
    mServerUsernameEditor->setText(currConnectionInfo.userName);
    configEditor(mServerUsernameEditor.get());

    mServerUserPasswordEditor = std::make_unique<TextEditor>("userpass"); // 0x25cf
    mServerUserPasswordEditor->setFont(Font(14));
    mServerUserPasswordEditor->setTextToShowWhenEmpty(TRANS("optional"), Colour(0x44ffffff));
   // mServerUserPasswordEditor->setText(currConnectionInfo.userPassword);
    configEditor(mServerUserPasswordEditor.get());

    
    mServerUserStaticLabel = std::make_unique<Label>("localaddrst", TRANS("Your Displayed Name:"));
    configServerLabel(mServerUserStaticLabel.get());
    mServerUserStaticLabel->setMinimumHorizontalScale(0.8);
    
    mServerUserPassStaticLabel = std::make_unique<Label>("localaddrst", TRANS("Password:"));
    configServerLabel(mServerUserPassStaticLabel.get());

    mServerGroupStaticLabel = std::make_unique<Label>("localaddrst", TRANS("Group Name:"));
    configServerLabel(mServerGroupStaticLabel.get());
    mServerGroupStaticLabel->setMinimumHorizontalScale(0.8);

    mServerGroupPassStaticLabel = std::make_unique<Label>("localaddrst", TRANS("Password:"));
    configServerLabel(mServerGroupPassStaticLabel.get());

    mServerHostStaticLabel = std::make_unique<Label>("localaddrst", TRANS("Using Server:"));
    configServerLabel(mServerHostStaticLabel.get());
    

    mServerGroupEditor = std::make_unique<TextEditor>("groupedit");
    mServerGroupEditor->setFont(Font(16));
    mServerGroupEditor->setText(currConnectionInfo.groupName);
    configEditor(mServerGroupEditor.get());

    mServerGroupPasswordEditor = std::make_unique<TextEditor>("grouppass"); // 0x25cf
    mServerGroupPasswordEditor->setFont(Font(14));
    mServerGroupPasswordEditor->setTextToShowWhenEmpty(TRANS("optional"), Colour(0x44ffffff));
    mServerGroupPasswordEditor->setText(currConnectionInfo.groupPassword);
    configEditor(mServerGroupPasswordEditor.get());

    mServerGroupRandomButton = std::make_unique<SonoDrawableButton>("randgroup", DrawableButton::ButtonStyle::ImageFitted);
    std::unique_ptr<Drawable> randimg(Drawable::createFromImageData(BinaryData::dice_icon_128_png, BinaryData::dice_icon_128_pngSize));
    mServerGroupRandomButton->setImages(randimg.get());
    mServerGroupRandomButton->addListener(this);
    
    
    mServerStatusLabel = std::make_unique<Label>("servstat", "");
    mServerStatusLabel->setJustificationType(Justification::centred);
    mServerStatusLabel->setFont(14);
    mServerStatusLabel->setColour(Label::textColourId, Colour(0x99ffaaaa));
    mServerStatusLabel->setMinimumHorizontalScale(0.75);
    
    mMainStatusLabel = std::make_unique<Label>("servstat", "");
    mMainStatusLabel->setJustificationType(Justification::centred);
    mMainStatusLabel->setFont(12);
    mMainStatusLabel->setColour(Label::textColourId, Colour(0x66ffffff));
    


    mRecentsListBox = std::make_unique<ListBox>("recentslist");
    mRecentsListBox->setColour (ListBox::outlineColourId, Colour::fromFloatRGBA(0.3, 0.3, 0.3, 0.0));
    mRecentsListBox->setColour (ListBox::backgroundColourId, Colour::fromFloatRGBA(0.1, 0.12, 0.1, 0.0f));
    mRecentsListBox->setColour (ListBox::textColourId, Colours::whitesmoke.withAlpha(0.8f));
    mRecentsListBox->setOutlineThickness (1);
#if JUCE_IOS
    mRecentsListBox->getViewport()->setScrollOnDragEnabled(true);
#endif
    mRecentsListBox->getViewport()->setScrollBarsShown(true, false);
    mRecentsListBox->setMultipleSelectionEnabled (false);
    mRecentsListBox->setRowHeight(50);
    mRecentsListBox->setModel (&recentsListModel);
    mRecentsListBox->setRowSelectedOnMouseDown(true);
    mRecentsListBox->setRowClickedOnMouseDown(false);
    

    mPatchbayButton = std::make_unique<TextButton>("patch");
    mPatchbayButton->setButtonText(TRANS("Patchbay"));
    mPatchbayButton->addListener(this);

    auto flags = foleys::LevelMeter::Horizontal|foleys::LevelMeter::Minimal; //|foleys::LevelMeter::MaxNumber;
    
    inputMeter = std::make_unique<foleys::LevelMeter>(flags);
    inputMeter->setLookAndFeel(&inMeterLnf);
    inputMeter->setRefreshRateHz(10);
    inputMeter->setMeterSource (&processor.getInputMeterSource());   
    inputMeter->addMouseListener(this, false);
    
    outputMeter = std::make_unique<foleys::LevelMeter>(flags);
    outputMeter->setLookAndFeel(&outMeterLnf);
    outputMeter->setRefreshRateHz(10);
    outputMeter->setMeterSource (&processor.getOutputMeterSource());            
    outputMeter->addMouseListener(this, false);

    
    mSettingsButton = std::make_unique<SonoDrawableButton>("settings",  DrawableButton::ButtonStyle::ImageFitted);
    std::unique_ptr<Drawable> setimg(Drawable::createFromImageData(BinaryData::settings_icon_svg, BinaryData::settings_icon_svgSize));
    mSettingsButton->setImages(setimg.get());
    mSettingsButton->addListener(this);
    mSettingsButton->setAlpha(0.7);
    mSettingsButton->addMouseListener(this, false);

    mPeerContainer = std::make_unique<PeersContainerView>(processor);
    
    mPeerViewport = std::make_unique<Viewport>();
    mPeerViewport->setViewedComponent(mPeerContainer.get(), false);
    
    mHelpComponent = std::make_unique<Component>();
    mOptionsComponent = std::make_unique<Component>();

    
    mOptionsAutosizeDefaultChoice = std::make_unique<SonoChoiceButton>();
    mOptionsAutosizeDefaultChoice->addChoiceListener(this);
    mOptionsAutosizeDefaultChoice->addItem(TRANS("Manual"), 1);
    mOptionsAutosizeDefaultChoice->addItem(TRANS("Auto Up"), 2);
    mOptionsAutosizeDefaultChoice->addItem(TRANS("Auto"), 3);
    
    mOptionsFormatChoiceDefaultChoice = std::make_unique<SonoChoiceButton>();
    mOptionsFormatChoiceDefaultChoice->addChoiceListener(this);
    int numformats = processor.getNumberAudioCodecFormats();
    for (int i=0; i < numformats; ++i) {
        mOptionsFormatChoiceDefaultChoice->addItem(processor.getAudioCodeFormatName(i), i+1);
    }

    mOptionsAutosizeStaticLabel = std::make_unique<Label>("", TRANS("Default Jitter Buffer"));
    configLabel(mOptionsAutosizeStaticLabel.get(), false);
    mOptionsAutosizeStaticLabel->setJustificationType(Justification::centredLeft);
    
    mOptionsFormatChoiceStaticLabel = std::make_unique<Label>("", TRANS("Default Send Quality:"));
    configLabel(mOptionsFormatChoiceStaticLabel.get(), false);
    mOptionsFormatChoiceStaticLabel->setJustificationType(Justification::centredRight);

    mIAAHostButton = std::make_unique<SonoDrawableButton>("iaa", DrawableButton::ButtonStyle::ImageFitted);
    mIAAHostButton->addListener(this);

    
    
#if JUCE_IOS
    //mInGainSlider->setPopupDisplayEnabled(true, false, this);
    //mBufferTimeSlider->setPopupDisplayEnabled(true, false, mOptionsComponent.get());
    //mInMonPanSlider1->setPopupDisplayEnabled(true, false, inPannersContainer.get());
    //mInMonPanSlider2->setPopupDisplayEnabled(true, false, inPannersContainer.get());
    //mDrySlider->setPopupDisplayEnabled(true, false, this);
    //mOutGainSlider->setPopupDisplayEnabled(true, false, this);
#endif
    
    if (JUCEApplicationBase::isStandaloneApp()) {
        //mRecordingButton = std::make_unique<SonoDrawableButton>("record", DrawableButton::ButtonStyle::ImageFitted);
        //std::unique_ptr<Drawable> recimg(Drawable::createFromImageData(BinaryData::rec_normal_png, BinaryData::rec_normal_pngSize));
        //std::unique_ptr<Drawable> recselimg(Drawable::createFromImageData(BinaryData::rec_sel_png, BinaryData::rec_sel_pngSize));
        //mRecordingButton->setImages(recimg.get(), nullptr, nullptr, nullptr, recselimg.get());
        mRecordingButton = std::make_unique<SonoTextButton>("record");
        mRecordingButton->setButtonText(TRANS("Rec"));
        mRecordingButton->addListener(this);
        mRecordingButton->setColour(TextButton::buttonOnColourId, Colour::fromFloatRGBA(0.8, 0.3, 0.3, 0.7));
        
    }
    
#if JUCE_IOS
    mPeerViewport->setScrollOnDragEnabled(true);
#endif
    
    mOptionsComponent->addAndMakeVisible(mBufferTimeSlider.get());
    mOptionsComponent->addAndMakeVisible(mOptionsAutosizeDefaultChoice.get());
    mOptionsComponent->addAndMakeVisible(mOptionsFormatChoiceDefaultChoice.get());
    mOptionsComponent->addAndMakeVisible(mOptionsAutosizeStaticLabel.get());
    mOptionsComponent->addAndMakeVisible(mOptionsFormatChoiceStaticLabel.get());
    
    
    addAndMakeVisible(mTitleLabel.get());

    addAndMakeVisible(mMainGroupLabel.get());
    addAndMakeVisible(mMainPeerLabel.get());
    addAndMakeVisible(mMainUserLabel.get());
    addAndMakeVisible(mMainPersonImage.get());
    addAndMakeVisible(mMainGroupImage.get());

    //addAndMakeVisible(mConnectTab.get());    
    addAndMakeVisible(mDrySlider.get());
    addAndMakeVisible(mOutGainSlider.get());
    //addAndMakeVisible(mWetLabel.get());
    addAndMakeVisible(mInGainSlider.get());
    addAndMakeVisible(mMasterMuteButton.get());
    
    addAndMakeVisible(mPatchbayButton.get());
    addAndMakeVisible(mConnectButton.get());
    addAndMakeVisible(mMainStatusLabel.get());

    addChildComponent(mIAAHostButton.get());
    
    if (mRecordingButton) {
        addAndMakeVisible(mRecordingButton.get());
    }

    
    mDirectConnectContainer->addAndMakeVisible(mDirectConnectButton.get());
    mDirectConnectContainer->addAndMakeVisible(mAddRemoteHostEditor.get());
    mDirectConnectContainer->addAndMakeVisible(mRemoteAddressStaticLabel.get());
    mDirectConnectContainer->addAndMakeVisible(mDirectConnectDescriptionLabel.get());
    mDirectConnectContainer->addAndMakeVisible(mLocalAddressLabel.get());
    mDirectConnectContainer->addAndMakeVisible(mLocalAddressStaticLabel.get());

    mServerConnectContainer->addAndMakeVisible(mServerConnectButton.get());
    mServerConnectContainer->addAndMakeVisible(mServerHostEditor.get());
    mServerConnectContainer->addAndMakeVisible(mServerUsernameEditor.get());
    mServerConnectContainer->addAndMakeVisible(mServerUserStaticLabel.get());
    mServerConnectContainer->addAndMakeVisible(mServerGroupEditor.get());
    mServerConnectContainer->addAndMakeVisible(mServerGroupRandomButton.get());
    mServerConnectContainer->addAndMakeVisible(mServerGroupStaticLabel.get());
    mServerConnectContainer->addAndMakeVisible(mServerHostStaticLabel.get());
    mServerConnectContainer->addAndMakeVisible(mServerGroupPassStaticLabel.get());
    mServerConnectContainer->addAndMakeVisible(mServerGroupPasswordEditor.get());
    //mServerConnectContainer->addAndMakeVisible(mServerUserPasswordEditor.get());
    //mServerConnectContainer->addAndMakeVisible(mServerUserPassStaticLabel.get());
    mServerConnectContainer->addAndMakeVisible(mServerStatusLabel.get());

    mRecentsContainer->addAndMakeVisible(mRecentsListBox.get());

    
    //addAndMakeVisible(mAddRemotePortEditor.get());
    addAndMakeVisible(mInGainLabel.get());
    addAndMakeVisible(mDryLabel.get());
    addAndMakeVisible(mOutGainLabel.get());
    addAndMakeVisible(inputMeter.get());
    addAndMakeVisible(outputMeter.get());
    addAndMakeVisible(mPeerViewport.get());
    //addAndMakeVisible(mPeerContainer.get());
    addAndMakeVisible (mSettingsButton.get());
    addAndMakeVisible (mPanButton.get());
    
    inPannersContainer->addAndMakeVisible (mInMonPanLabel1.get());
    inPannersContainer->addAndMakeVisible (mInMonPanLabel2.get());
    inPannersContainer->addAndMakeVisible (mInMonPanSlider1.get());
    inPannersContainer->addAndMakeVisible (mInMonPanSlider2.get());

    inChannels = processor.getTotalNumInputChannels();
    outChannels = processor.getTotalNumOutputChannels();
    
    updateLayout();
    
    updateState();

    
    //setResizeLimits(400, 300, 2000, 1000);

    if (JUCEApplicationBase::isStandaloneApp()) {
#if !JUCE_IOS
        processor.startAooServer();
#endif
        setResizable(true, false);
    } else  {
        setResizable(true, true);    
    }
    
    processor.addClientListener(this);
    
    std::istringstream gramstream(BinaryData::wordmaker_g, BinaryData::wordmaker_gSize);
    mRandomSentence = std::make_unique<RandomSentenceGenerator>(gramstream);
    mRandomSentence->capEveryWord = true;
    
    startTimer(PeriodicUpdateTimerId, 1000);
    
   // Make sure that before the constructor has finished, you've set the
   // editor's size to whatever you need it to be.
   setSize (700, 440);
   
}

SonobusAudioProcessorEditor::~SonobusAudioProcessorEditor()
{
}



void SonobusAudioProcessorEditor::configKnobSlider(Slider * slider)
{
    //slider->setVelocityBasedMode(true);
    //slider->setVelocityModeParameters(2.5, 1, 0.05);
    //slider->setTextBoxStyle(Slider::NoTextBox, true, 40, 18);
    slider->setSliderStyle(Slider::SliderStyle::RotaryVerticalDrag);
    slider->setTextBoxStyle(Slider::TextBoxAbove, true, 60, 14);
    slider->setMouseDragSensitivity(128);
    slider->setScrollWheelEnabled(false);
    //slider->setPopupDisplayEnabled(true, false, this);
    slider->setColour(Slider::textBoxBackgroundColourId, Colours::transparentBlack);
    slider->setColour(Slider::textBoxOutlineColourId, Colours::transparentBlack);
    slider->setColour(Slider::textBoxTextColourId, Colour(0x90eeeeee));
    slider->setLookAndFeel(&sonoSliderLNF);
}


//////////////////
// these client listener callbacks will be from a different thread

void SonobusAudioProcessorEditor::aooClientConnected(SonobusAudioProcessor *comp, bool success, const String & errmesg) 
{
    DebugLogC("Client connect success: %d  mesg: %s", success, errmesg.toRawUTF8());
    {
        const ScopedLock sl (clientStateLock);        
        clientEvents.add(ClientEvent(ClientEvent::ConnectEvent, success, errmesg));
    }
    triggerAsyncUpdate();
}

void SonobusAudioProcessorEditor::aooClientDisconnected(SonobusAudioProcessor *comp, bool success, const String & errmesg) 
{
    DebugLogC("Client disconnect success: %d  mesg: %s", success, errmesg.toRawUTF8());
    {
        const ScopedLock sl (clientStateLock);        
        clientEvents.add(ClientEvent(ClientEvent::DisconnectEvent, success, errmesg));
    }
    triggerAsyncUpdate();
}

void SonobusAudioProcessorEditor::aooClientLoginResult(SonobusAudioProcessor *comp, bool success, const String & errmesg)  
{
    DebugLogC("Client login success: %d  mesg: %s", success, errmesg.toRawUTF8());
    {
        const ScopedLock sl (clientStateLock);        
        clientEvents.add(ClientEvent(ClientEvent::LoginEvent, success, errmesg));
    }
    triggerAsyncUpdate();
}

void SonobusAudioProcessorEditor::aooClientGroupJoined(SonobusAudioProcessor *comp, bool success, const String & group,  const String & errmesg) 
{
    DebugLogC("Client join group %s success: %d  mesg: %s", group.toRawUTF8(), success, errmesg.toRawUTF8());
    {
        const ScopedLock sl (clientStateLock);        
        clientEvents.add(ClientEvent(ClientEvent::GroupJoinEvent, group, success, errmesg));
    }
    triggerAsyncUpdate();
}

void SonobusAudioProcessorEditor::aooClientGroupLeft(SonobusAudioProcessor *comp, bool success, const String & group, const String & errmesg)  
{
    DebugLogC("Client leave group %s success: %d   mesg: %s", group.toRawUTF8(), success, errmesg.toRawUTF8());
    {
        const ScopedLock sl (clientStateLock);        
        clientEvents.add(ClientEvent(ClientEvent::GroupLeaveEvent, group, success, errmesg));
    }
    triggerAsyncUpdate();
}

void SonobusAudioProcessorEditor::aooClientPeerJoined(SonobusAudioProcessor *comp, const String & group, const String & user)  
{
    DebugLogC("Client peer '%s' joined group '%s'", user.toRawUTF8(), group.toRawUTF8());
    {
        const ScopedLock sl (clientStateLock);        
        clientEvents.add(ClientEvent(ClientEvent::PeerJoinEvent, group, true, "", user));
    }
    triggerAsyncUpdate();
}

void SonobusAudioProcessorEditor::aooClientPeerLeft(SonobusAudioProcessor *comp, const String & group, const String & user)  
{
    DebugLogC("Client peer '%s' left group '%s'", user.toRawUTF8(), group.toRawUTF8());
    {
        const ScopedLock sl (clientStateLock);        
        clientEvents.add(ClientEvent(ClientEvent::PeerLeaveEvent, group, true, "", user));
    }
    triggerAsyncUpdate();

}

void SonobusAudioProcessorEditor::aooClientError(SonobusAudioProcessor *comp, const String & errmesg)  
{
    DebugLogC("Client error: %s", errmesg.toRawUTF8());
    {
        const ScopedLock sl (clientStateLock);        
        clientEvents.add(ClientEvent(ClientEvent::Error, errmesg));
    }
    
    triggerAsyncUpdate();
}

void SonobusAudioProcessorEditor::aooClientPeerChangedState(SonobusAudioProcessor *comp, const String & mesg)
{
    {
        const ScopedLock sl (clientStateLock);        
        clientEvents.add(ClientEvent(ClientEvent::PeerChangedState, mesg));
    }
    
    triggerAsyncUpdate();    
}



//////////////////////////

void SonobusAudioProcessorEditor::choiceButtonSelected(SonoChoiceButton *comp, int index, int ident)
{
    if (comp == mOptionsFormatChoiceDefaultChoice.get()) {
        processor.setDefaultAudioCodecFormat(index);
    }
    else if (comp == mOptionsAutosizeDefaultChoice.get()) {
        processor.setDefaultAutoresizeBufferMode((SonobusAudioProcessor::AutoNetBufferMode) index);
    }
}


bool SonobusAudioProcessorEditor::updatePeerState(bool force)
{
    if (force || mPeerContainer->getPeerViewCount() != processor.getNumberRemotePeers()) {
        mPeerContainer->rebuildPeerViews();
        updateLayout();
        resized();
        
        if (patchbayCalloutBox) {
            mPatchMatrixView->updateGridLayout();
            mPatchMatrixView->updateGrid();
        }

        updateState();
        return true;
    }
    else {
        mPeerContainer->updatePeerViews();

        if (patchbayCalloutBox) {
            mPatchMatrixView->updateGrid();
        }
        return false;
    }        
}


void SonobusAudioProcessorEditor::updateChannelState(bool force)
{
    if (force || inChannels != processor.getTotalNumInputChannels() || outChannels != processor.getTotalNumOutputChannels()) {
        inChannels = processor.getTotalNumInputChannels();            
        outChannels = processor.getTotalNumOutputChannels();
        updateLayout();
        updateState();
        resized();
    }    
}

void SonobusAudioProcessorEditor::updateOptionsState()
{
    mOptionsFormatChoiceDefaultChoice->setSelectedItemIndex(processor.getDefaultAudioCodecFormat(), dontSendNotification);
    mOptionsAutosizeDefaultChoice->setSelectedItemIndex((int)processor.getDefaultAutoresizeBufferMode(), dontSendNotification);
}


void SonobusAudioProcessorEditor::timerCallback(int timerid)
{
    if (timerid == PeriodicUpdateTimerId) {
        
        bool stateUpdated = updatePeerState();
        
        updateChannelState();
        
        if (!stateUpdated && (currGroup != processor.getCurrentJoinedGroup() || currConnected != processor.isConnectedToServer())) {
            updateState();
        }
        
        double nowstamp = Time::getMillisecondCounterHiRes() * 1e-3;
        if (serverStatusFadeTimestamp > 0.0 && nowstamp >= serverStatusFadeTimestamp) {
            Desktop::getInstance().getAnimator().fadeOut(mMainStatusLabel.get(), 500);
            serverStatusFadeTimestamp = 0.0;
        }
        
#if JUCE_IOS
        if (JUCEApplicationBase::isStandaloneApp()) {
            bool iaaconn = isInterAppAudioConnected();
            if (iaaconn != iaaConnected) {
                iaaConnected = iaaconn;
                DebugLogC("Interapp audio state changed: %d", iaaConnected);
                if (iaaConnected) {
                    iaaHostIcon = getIAAHostIcon(78);
                    DrawableImage randimg;
                    randimg.setImage(iaaHostIcon);
                    mIAAHostButton->setImages(&randimg);
                }
                updateLayout();
                updateState();
            }
            
        }
#endif
    }
}

void SonobusAudioProcessorEditor::buttonClicked (Button* buttonThatWasClicked)
{
    if (buttonThatWasClicked == mDirectConnectButton.get()) {
        String hostport = mAddRemoteHostEditor->getText();        
        //String port = mAddRemotePortEditor->getText();
        // parse it
        StringArray toks = StringArray::fromTokens(hostport, ":/ ", "");
        String host;
        int port = 11000;
        
        if (toks.size() >= 1) {
            host = toks[0].trim();
        }
        if (toks.size() >= 2) {
            port = toks[1].trim().getIntValue();
        }
        
        if (host.isNotEmpty() && port != 0) {
            if (processor.connectRemotePeer(host, port)) {
                if (serverCalloutBox) {
                    showConnectPopup(false);
                }
            }
        }
    }
    else if (buttonThatWasClicked == mConnectButton.get()) {
        
        if (processor.isConnectedToServer()) {
            processor.disconnectFromServer();
            updateState();
        }
        else {
            if (!serverCalloutBox) {
                mMainPeerLabel->setText("", dontSendNotification);
                showConnectPopup(true);
            } else {
                showConnectPopup(false);
            }
        }
        
    }
    else if (buttonThatWasClicked == mServerConnectButton.get()) {

        if (processor.isConnectedToServer()) {
            processor.disconnectFromServer();    
            updateState();
        }
        else {
            String hostport = mServerHostEditor->getText();        
            //String port = mAddRemotePortEditor->getText();
            // parse it
            StringArray toks = StringArray::fromTokens(hostport, ":", "");
            String host;
            int port = 10999;
            
            if (toks.size() >= 1) {
                host = toks[0].trim();
            }
            if (toks.size() >= 2) {
                port = toks[1].trim().getIntValue();
            }
            
            AooServerConnectionInfo info;
            info.userName = mServerUsernameEditor->getText();
            //currConnectionInfo.userPassword = mServerUserPasswordEditor->getText();
            info.groupName = mServerGroupEditor->getText();
            info.groupPassword = mServerGroupPasswordEditor->getText();
            info.serverHost = host;
            info.serverPort = port;
            
            connectWithInfo(info);                       
            
        }
    }
    else if (buttonThatWasClicked == mPatchbayButton.get()) {
        if (!patchbayCalloutBox) {
            showPatchbay(true);
        } else {
            showPatchbay(false);
        }        
    }
    else if (buttonThatWasClicked == mPanButton.get()) {
        if (!inPannerCalloutBox) {
            showInPanners(true);
        } else {
            showInPanners(false);
        }        
    }
    else if (buttonThatWasClicked == mMasterMuteButton.get()) {
        // allow or disallow sending to all peers, handled by button attachment

        if (mMasterMuteButton->getToggleState()) {
            showPopTip(TRANS("Not sending your audio anywhere"), 2000, mMasterMuteButton.get());
        } else {
            showPopTip(TRANS("Sending your audio to all"), 2000, mMasterMuteButton.get());
        }
    }
    else if (buttonThatWasClicked == mServerGroupRandomButton.get()) {
        // randomize group name
        String rgroup = mRandomSentence->randomSentence();
        mServerGroupEditor->setText(rgroup, dontSendNotification);
    }
    else if (buttonThatWasClicked == mSettingsButton.get()) {
        if (!settingsWasShownOnDown) {
            showSettings(true);
        }
    }
    else if (buttonThatWasClicked == mIAAHostButton.get()) {
        if (iaaConnected) {
            switchToHostApplication();
        }
    }
    else if (buttonThatWasClicked == mRecordingButton.get()) {
        if (processor.isRecordingToFile()) {
            processor.stopRecordingToFile();
            mRecordingButton->setToggleState(false, dontSendNotification);
            //updateServerStatusLabel("Stopped Recording");

            String filepath;
#if (JUCE_IOS)
            filepath = lastRecordedFile.getRelativePathFrom(File::getSpecialLocation (File::userDocumentsDirectory));
            showPopTip(TRANS("Finished recording to ") + filepath, 4000, mRecordingButton.get(), 130);
#else
            filepath = lastRecordedFile.getRelativePathFrom(File::getSpecialLocation (File::userHomeDirectory));
#endif

            mRecordingButton->setTooltip(TRANS("Last recorded file: ") + filepath);
            
        } else {
            // create new timestamped filename
            String filename = Time::getCurrentTime().formatted("SonoBusSession_%Y-%m-%d_%H.%M.%S");

#if (JUCE_IOS)
            auto parentDir = File::getSpecialLocation (File::userDocumentsDirectory);
#else
            auto parentDir = File::getSpecialLocation (File::userDocumentsDirectory);
            parentDir = parentDir.getChildFile("SonoBus");
            parentDir.createDirectory();
#endif

            const File file (parentDir.getNonexistentChildFile (filename, ".flac"));

            if (processor.startRecordingToFile(file)) {
                mRecordingButton->setToggleState(true, dontSendNotification);
                //updateServerStatusLabel("Started recording...");
                lastRecordedFile = file;
                String filepath;
#if (JUCE_IOS)
                showPopTip(TRANS("Started recording output"), 2000, mRecordingButton.get());
                filepath = lastRecordedFile.getRelativePathFrom(File::getSpecialLocation (File::userDocumentsDirectory));
#else
                filepath = lastRecordedFile.getRelativePathFrom(File::getSpecialLocation (File::userHomeDirectory));
#endif
                mRecordingButton->setTooltip(TRANS("Recording audio to: ") + filepath);
            }
            
        }
    }
    else {
        
       
    }
}

void SonobusAudioProcessorEditor::connectWithInfo(const AooServerConnectionInfo & info)
{
    currConnectionInfo = info;
    
    if (currConnectionInfo.groupName.isEmpty()) {
        mServerStatusLabel->setText(TRANS("You need to specify a group name!"), dontSendNotification);
        return;
    }
    
    if (currConnectionInfo.userName.isEmpty()) {
        mServerStatusLabel->setText(TRANS("You need to specify a user name!"), dontSendNotification);
        return;
    }
    
    if (currConnectionInfo.serverHost.isNotEmpty() && currConnectionInfo.serverPort != 0) 
    {
        processor.connectToServer(currConnectionInfo.serverHost, currConnectionInfo.serverPort, currConnectionInfo.userName, currConnectionInfo.userPassword);
        updateState();
    }
    else {
        mServerStatusLabel->setText(TRANS("Server address is invalid!"), dontSendNotification);
    }
}


void SonobusAudioProcessorEditor::showInPanners(bool flag)
{
    
    if (flag && inPannerCalloutBox == nullptr) {
        
        Viewport * wrap = new Viewport();
        
        Component* dw = this->findParentComponentOfClass<DocumentWindow>();
        
        if (!dw) {
            dw = this->findParentComponentOfClass<AudioProcessorEditor>();        
        }
        if (!dw) {
            dw = this->findParentComponentOfClass<Component>();        
        }
        if (!dw) {
            dw = this;
        }
        
        // calculate based on how many peers we have
        const int defWidth = 140; 
        const int defHeight = 70;
        
        wrap->setSize(jmin(defWidth, dw->getWidth() - 20), jmin(defHeight, dw->getHeight() - 24));
        
        //Rectangle<int> setbounds = Rectangle<int>(5, mTitleImage->getBottom() + 2, std::min(100, getLocalBounds().getWidth() - 10), 80);
        
        inPannersContainer->setBounds(Rectangle<int>(0,0,defWidth,defHeight));
        
        //wrap->addAndMakeVisible(mSettingsPanel.get());
        wrap->setViewedComponent(inPannersContainer.get(), false);
        inPannersContainer->setVisible(true);
        
        inPannerMainBox.performLayout(inPannersContainer->getLocalBounds());
        
        
        Rectangle<int> bounds =  dw->getLocalArea(nullptr, mPanButton->getScreenBounds());
        DebugLogC("callout bounds: %s", bounds.toString().toRawUTF8());
        inPannerCalloutBox = & CallOutBox::launchAsynchronously (wrap, bounds , dw);
        if (CallOutBox * box = dynamic_cast<CallOutBox*>(inPannerCalloutBox.get())) {
            box->setDismissalMouseClicksAreAlwaysConsumed(true);
        }
    }
    else {
        // dismiss it
        if (CallOutBox * box = dynamic_cast<CallOutBox*>(inPannerCalloutBox.get())) {
            box->dismiss();
            inPannerCalloutBox = nullptr;
        }
    }
}

void SonobusAudioProcessorEditor::showPatchbay(bool flag)
{
    if (!mPatchMatrixView) {
        mPatchMatrixView = std::make_unique<PatchMatrixView>(processor);
    }
    
    if (flag && patchbayCalloutBox == nullptr) {
        
        Viewport * wrap = new Viewport();
        
        Component* dw = this->findParentComponentOfClass<DocumentWindow>();
        
        if (!dw) {
            dw = this->findParentComponentOfClass<AudioProcessorEditor>();        
        }
        if (!dw) {
            dw = this->findParentComponentOfClass<Component>();        
        }
        if (!dw) {
            dw = this;
        }
        
        // calculate based on how many peers we have
        auto prefbounds = mPatchMatrixView->getPreferredBounds();
        const int defWidth = prefbounds.getWidth(); 
        const int defHeight = prefbounds.getHeight();
        
        wrap->setSize(jmin(defWidth + 8, dw->getWidth() - 20), jmin(defHeight + 8, dw->getHeight() - 24));
        
        //Rectangle<int> setbounds = Rectangle<int>(5, mTitleImage->getBottom() + 2, std::min(100, getLocalBounds().getWidth() - 10), 80);
        
        mPatchMatrixView->setBounds(Rectangle<int>(0,0,defWidth,defHeight));
        
        //wrap->addAndMakeVisible(mSettingsPanel.get());
        wrap->setViewedComponent(mPatchMatrixView.get(), false);
        
        //patchbayBox.performLayout(mPatchMatrixView->getLocalBounds());
        
        mPatchMatrixView->updateGridLayout();
        mPatchMatrixView->updateGrid();
        
        Rectangle<int> bounds =  dw->getLocalArea(nullptr, mPatchbayButton->getScreenBounds());
        DebugLogC("callout bounds: %s", bounds.toString().toRawUTF8());
        patchbayCalloutBox = & CallOutBox::launchAsynchronously (wrap, bounds , dw);
        if (CallOutBox * box = dynamic_cast<CallOutBox*>(patchbayCalloutBox.get())) {
            box->setDismissalMouseClicksAreAlwaysConsumed(true);
        }
    }
    else {
        // dismiss it
        if (CallOutBox * box = dynamic_cast<CallOutBox*>(patchbayCalloutBox.get())) {
            box->dismiss();
            patchbayCalloutBox = nullptr;
        }
    }
}


void SonobusAudioProcessorEditor::showConnectPopup(bool flag)
{
    
    if (flag && serverCalloutBox == nullptr) {
        
        Viewport * wrap = new Viewport();
        
        Component* dw = this->findParentComponentOfClass<DocumentWindow>();
        
        if (!dw) {
            dw = this->findParentComponentOfClass<AudioProcessorEditor>();        
        }
        if (!dw) {
            dw = this->findParentComponentOfClass<Component>();        
        }
        if (!dw) {
            dw = this;
        }
        
        auto prefbounds = Rectangle<int>(0,0, 300, 320);
        const int defWidth = prefbounds.getWidth(); 
        const int defHeight = prefbounds.getHeight();
        
        wrap->setSize(jmin(defWidth + 8, dw->getWidth() - 20), jmin(defHeight + 8, dw->getHeight() - 24));
        
        //Rectangle<int> setbounds = Rectangle<int>(5, mTitleImage->getBottom() + 2, std::min(100, getLocalBounds().getWidth() - 10), 80);
        
        mConnectTab->setBounds(Rectangle<int>(0,0,defWidth,defHeight));
        
        mServerStatusLabel->setText(TRANS("All who join the same Group will be able to connect with each other."), dontSendNotification);

        mServerHostEditor->setText( String::formatted("%s:%d", currConnectionInfo.serverHost.toRawUTF8(), currConnectionInfo.serverPort)); // 100.36.128.246 // 23.23.205.37
        mServerUsernameEditor->setText(currConnectionInfo.userName);
        mServerGroupEditor->setText(currConnectionInfo.groupName);
        mServerGroupPasswordEditor->setText(currConnectionInfo.groupPassword);


        
        recentsListModel.updateState();
        mRecentsListBox->updateContent();
        mRecentsListBox->deselectAllRows();
        
        remoteBox.performLayout(mDirectConnectContainer->getLocalBounds());
        serverBox.performLayout(mServerConnectContainer->getLocalBounds());
        recentsBox.performLayout(mRecentsContainer->getLocalBounds());

        //wrap->addAndMakeVisible(mSettingsPanel.get());
        wrap->setViewedComponent(mConnectTab.get(), false);
        

        if (firstTimeConnectShow) {
            if (recentsListModel.getNumRows() > 0) {
                // show recents tab first
                mConnectTab->setCurrentTabIndex(0);
            } else {
                mConnectTab->setCurrentTabIndex(1);                
            }
            firstTimeConnectShow = false;
        }
        
        Rectangle<int> bounds =  dw->getLocalArea(nullptr, mConnectButton->getScreenBounds());
        DebugLogC("callout bounds: %s", bounds.toString().toRawUTF8());
        serverCalloutBox = & CallOutBox::launchAsynchronously (wrap, bounds , dw);
        if (CallOutBox * box = dynamic_cast<CallOutBox*>(serverCalloutBox.get())) {
            box->setDismissalMouseClicksAreAlwaysConsumed(true);
        }
    }
    else {
        // dismiss it
        if (CallOutBox * box = dynamic_cast<CallOutBox*>(serverCalloutBox.get())) {
            box->dismiss();
            serverCalloutBox = nullptr;
        }
    }
}


void SonobusAudioProcessorEditor::sliderValueChanged (Slider* slider)
{

}

void SonobusAudioProcessorEditor::mouseDown (const MouseEvent& event) 
{
    
    if (event.eventComponent == mSettingsButton.get()) {
        settingsWasShownOnDown = settingsCalloutBox != nullptr;
        if (!settingsWasShownOnDown) {
          //  showOrHideSettings();
        }
    }
    else if (event.eventComponent == mTitleLabel.get()) {
        settingsWasShownOnDown = settingsCalloutBox != nullptr;
        if (settingsWasShownOnDown) {
            showSettings(false);
        }
    }
    else if (event.eventComponent == inputMeter.get()) {
        inputMeter->clearClipIndicator(-1);
    }
    else if (event.eventComponent == outputMeter.get()) {
        outputMeter->clearClipIndicator(-1);
    }
}

void SonobusAudioProcessorEditor::mouseUp (const MouseEvent& event)
{
    if (event.eventComponent == mTitleLabel.get()) {
        showSettings(true);
    }
}

void SonobusAudioProcessorEditor::showSettings(bool flag)
{
    DebugLogC("Got settings click");

    if (flag && settingsCalloutBox == nullptr) {
        
        Viewport * wrap = new Viewport();
        
        Component* dw = this->findParentComponentOfClass<DocumentWindow>();
        
        if (!dw) {
            dw = this->findParentComponentOfClass<AudioProcessorEditor>();        
        }
        if (!dw) {
            dw = this->findParentComponentOfClass<Component>();        
        }
        if (!dw) {
            dw = this;
        }

        const int defWidth = 300;
        const int defHeight = 350;
        
        wrap->setSize(jmin(defWidth + 8, dw->getWidth() - 20), jmin(defHeight + 8, dw->getHeight() - 24));
        
        //Rectangle<int> setbounds = Rectangle<int>(5, mTitleImage->getBottom() + 2, std::min(100, getLocalBounds().getWidth() - 10), 80);
        bool firsttime = false;
        if (!mSettingsTab) {
            mSettingsTab = std::make_unique<TabbedComponent>(TabbedButtonBar::Orientation::TabsAtTop);
            mSettingsTab->setTabBarDepth(36);
            mSettingsTab->setOutline(0);
            mSettingsTab->getTabbedButtonBar().setMinimumTabScaleFactor(0.1f);
            firsttime = true;
        }

        
        if (JUCEApplicationBase::isStandaloneApp() && getAudioDeviceManager())
        {
            if (!mAudioDeviceSelector) {
                int minNumInputs  = std::numeric_limits<int>::max(), maxNumInputs  = 0,
                minNumOutputs = std::numeric_limits<int>::max(), maxNumOutputs = 0;
                
                auto updateMinAndMax = [] (int newValue, int& minValue, int& maxValue)
                {
                    minValue = jmin (minValue, newValue);
                    maxValue = jmax (maxValue, newValue);
                };
                
                /*
                if (channelConfiguration.size() > 0)
                {
                    auto defaultConfig =  channelConfiguration.getReference (0);
                    updateMinAndMax ((int) defaultConfig.numIns,  minNumInputs,  maxNumInputs);
                    updateMinAndMax ((int) defaultConfig.numOuts, minNumOutputs, maxNumOutputs);
                }
                 */
                
                if (auto* bus = processor.getBus (true, 0)) {
                    updateMinAndMax (bus->getDefaultLayout().size(), minNumInputs, maxNumInputs);
                    if (bus->isNumberOfChannelsSupported(1)) {
                        updateMinAndMax (1, minNumInputs, maxNumInputs);                
                    }
                }
                
                if (auto* bus = processor.getBus (false, 0)) {
                    updateMinAndMax (bus->getDefaultLayout().size(), minNumOutputs, maxNumOutputs);
                    if (bus->isNumberOfChannelsSupported(1)) {
                        updateMinAndMax (1, minNumOutputs, maxNumOutputs);                
                    }
                }
                
                
                minNumInputs  = jmin (minNumInputs,  maxNumInputs);
                minNumOutputs = jmin (minNumOutputs, maxNumOutputs);
                
            
                mAudioDeviceSelector = std::make_unique<AudioDeviceSelectorComponent>(*getAudioDeviceManager(),
                                                                                      minNumInputs, maxNumInputs,
                                                                                      minNumOutputs, maxNumOutputs,
                                                                                      false, // show MIDI input
                                                                                      false,
                                                                                      false, false);
            }

            if (firsttime) {
                mSettingsTab->addTab(TRANS("AUDIO"), Colour::fromFloatRGBA(0.1, 0.1, 0.1, 1.0), mAudioDeviceSelector.get(), false);
            }
            
            //mAudioDeviceSelector->setBounds(Rectangle<int>(0,0,defWidth,defHeight));

            //wrap->setViewedComponent(mAudioDeviceSelector.get(), false);
               
            //settingsBox.performLayout(mAudioDeviceSelector->getLocalBounds());

        }


        if (firsttime) {
            mSettingsTab->addTab(TRANS("OPTIONS"),Colour::fromFloatRGBA(0.1, 0.1, 0.1, 1.0), mOptionsComponent.get(), false);
            mSettingsTab->addTab(TRANS("HELP"), Colour::fromFloatRGBA(0.1, 0.1, 0.1, 1.0), mHelpComponent.get(), false);
        }
        
        mSettingsTab->setBounds(Rectangle<int>(0,0,defWidth,defHeight));

        //wrap->setViewedComponent(mAudioDeviceSelector.get(), false);
        wrap->setViewedComponent(mSettingsTab.get(), false);

        
        optionsBox.performLayout(mOptionsComponent->getLocalBounds());

        mOptionsAutosizeStaticLabel->setBounds(mBufferTimeSlider->getBounds().removeFromLeft(mBufferTimeSlider->getWidth()*0.75));

        
        updateOptionsState();
        
        /*
        
        mSettingsPanel->setBounds(Rectangle<int>(0,0,defWidth,defHeight));
        
        //wrap->addAndMakeVisible(mSettingsPanel.get());
        wrap->setViewedComponent(mSettingsPanel.get(), false);
           
        settingsBox.performLayout(mSettingsPanel->getLocalBounds());
         */
        
        Rectangle<int> bounds =  dw->getLocalArea(nullptr, mTitleLabel->getScreenBounds().reduced(10));
        DebugLogC("callout bounds: %s", bounds.toString().toRawUTF8());
        settingsCalloutBox = & CallOutBox::launchAsynchronously (wrap, bounds , dw);
        if (CallOutBox * box = dynamic_cast<CallOutBox*>(settingsCalloutBox.get())) {
            box->setDismissalMouseClicksAreAlwaysConsumed(true);
        }
    }
    else {
        // dismiss it
        if (CallOutBox * box = dynamic_cast<CallOutBox*>(settingsCalloutBox.get())) {
            box->dismiss();
            settingsCalloutBox = nullptr;
        }
    }
}

void SonobusAudioProcessorEditor::updateState()
{
    mLocalAddressLabel->setText(String::formatted("%s : %d", processor.getLocalIPAddress().toString().toRawUTF8(), processor.getUdpLocalPort()), dontSendNotification);

    currConnected = processor.isConnectedToServer();
    if (currConnected) {
        mConnectButton->setButtonText(TRANS("Disconnect"));
        //mServerGroupEditor->setEnabled(true);
        //mServerGroupEditor->setAlpha(1.0f);
    }
    else {
        mConnectButton->setButtonText(TRANS("Connect..."));
        //mServerGroupEditor->setEnabled(false);
        //mServerGroupEditor->setAlpha(0.5f);
    }

    if (mRecordingButton) {
        mRecordingButton->setToggleState(processor.isRecordingToFile(), dontSendNotification);
    }

    
    currGroup = processor.getCurrentJoinedGroup();
    if (!currGroup.isEmpty() && currConnected) {
        //mServerGroupEditor->setEnabled(false);
        mMainGroupLabel->setText(currGroup, dontSendNotification);
        String userstr;
        //if (processor.getNumberRemotePeers() == 1) {
        //    userstr = String::formatted(TRANS("%d other user"), processor.getNumberRemotePeers() + 1);
       // }
        if (processor.getNumberRemotePeers() > 0) {
            userstr = String::formatted(TRANS("%d total users"), processor.getNumberRemotePeers() + 1);
        } else {
            userstr = TRANS("No other users yet");
        }

        mMainPeerLabel->setText(userstr, dontSendNotification);
        mMainUserLabel->setText(currConnectionInfo.userName, dontSendNotification);

        mPatchbayButton->setVisible(false);
    }
    else {
        //mServerGroupEditor->setEnabled(currConnected);
        mMainGroupLabel->setText("", dontSendNotification);
        mMainUserLabel->setText("", dontSendNotification);

        if (processor.getNumberRemotePeers() == 0 || !currConnected) {
            mMainPeerLabel->setText(TRANS("Press Connect button to start!"), dontSendNotification);
        } else {
            mMainPeerLabel->setText("", dontSendNotification);
        }

        
#if JUCE_IOS
        mPatchbayButton->setVisible(false);

#else
        if (processor.getNumberRemotePeers() > 1 && !currConnected) {
            mPatchbayButton->setVisible(true);
        } else {
            mPatchbayButton->setVisible(false);            
        }
#endif   
    }
    
#if JUCE_IOS
    mIAAHostButton->setVisible(iaaConnected);
#endif
    
}


void SonobusAudioProcessorEditor::parameterChanged (const String& pname, float newValue)
{
    
    if (pname == SonobusAudioProcessor::paramMasterSendMute) {
        {
            const ScopedLock sl (clientStateLock);
            clientEvents.add(ClientEvent(ClientEvent::PeerChangedState, ""));
        }
        triggerAsyncUpdate();
    }
    
    
    /*
    if (pname == YaleDAudioProcessor::paramLPFilterCutoff) {
        lastLPCutoff = newValue;
        mLPCutoffChanged = true;
        if (MessageManager::getInstance()->isThisTheMessageThread())
        {
            cancelPendingUpdate();
            updateLPFilterCutoff(lastLPCutoff);
        }
        else {
            triggerAsyncUpdate();
        }        
    }
*/
}

void SonobusAudioProcessorEditor::updateServerStatusLabel(const String & mesg, bool mainonly)
{
    const double fadeAfterSec = 5.0;
    mMainStatusLabel->setText(mesg, dontSendNotification);
    Desktop::getInstance().getAnimator().fadeIn(mMainStatusLabel.get(), 200);

    serverStatusFadeTimestamp = Time::getMillisecondCounterHiRes() * 1e-3 + fadeAfterSec;   
    
    if (!mainonly) {
        mServerStatusLabel->setText(mesg, dontSendNotification);
    }
}

void SonobusAudioProcessorEditor::handleAsyncUpdate()
{ 
    const ScopedLock sl (clientStateLock);        
    for (auto & ev : clientEvents) {
        if (ev.type == ClientEvent::PeerChangedState) {
            peerStateUpdated = true;
            mPeerContainer->updateLayout();
            mPeerContainer->resized();
            updatePeerState();
            updateState();
        }
        else if (ev.type == ClientEvent::ConnectEvent) {
            String statstr;
            if (ev.success) {
                statstr = TRANS("Connected to server");                
            } else {
                if (String(ev.message).contains("access denied")) {
                    statstr = TRANS("Already connected with this user name");

                    // try again with different username (auto-incremented)
                    //if (currConnectionInfo.userName.)
                    
                    //connectWithInfo(currConnectionInfo);

                } else {
                    statstr = TRANS("Connect failed: ") + ev.message;
                }

            }
            
            // now attempt to join group
            if (ev.success) {
                currConnectionInfo.timestamp = Time::getCurrentTime().toMilliseconds();
                processor.addRecentServerConnectionInfo(currConnectionInfo);

                processor.joinServerGroup(currConnectionInfo.groupName, currConnectionInfo.groupPassword);                
            }
            else {
                // switch to group page
                mConnectTab->setCurrentTabIndex(1);
            }
            
            updateServerStatusLabel(statstr, false);
            updateState();
        }
        else if (ev.type == ClientEvent::DisconnectEvent) {
            String statstr;
            if (ev.success) {
                statstr = TRANS("Disconnected from server");
            } else {
                //statstr = TRANS("Disconnect failed: ") + ev.message;
            }
            updateServerStatusLabel(statstr);
            updateState();
        }
        else if (ev.type == ClientEvent::GroupJoinEvent) {
            String statstr;
            if (ev.success) {
                statstr = TRANS("Joined Group: ") + ev.group;
                
                if (serverCalloutBox) {
                    showConnectPopup(false);
                }
            } else {
                statstr = TRANS("Failed to join group: ") + ev.message;
                // disconnect
                processor.disconnectFromServer();
                
            }
            updateServerStatusLabel(statstr, false);
            updateState();
        }
        else if (ev.type == ClientEvent::GroupLeaveEvent) {
            String statstr;
            if (ev.success) {
                statstr = TRANS("Left Group: ") + ev.group;
            } else {
                statstr = TRANS("Failed to leave group: ") + ev.message;
            }
            updateServerStatusLabel(statstr);
            updatePeerState(true);
            updateState();
        }
        else if (ev.type == ClientEvent::PeerJoinEvent) {
            updatePeerState(true);
            updateState();
        }
        else if (ev.type == ClientEvent::PeerLeaveEvent) {
            updatePeerState(true);
            updateState();
        }

    }
    clientEvents.clearQuick();
}

//==============================================================================
void SonobusAudioProcessorEditor::paint (Graphics& g)
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll (getLookAndFeel().findColour (ResizableWindow::backgroundColourId));

    //g.setColour (Colours::white);
    //g.setFont (15.0f);
    //g.drawFittedText ("Hello World!", getLocalBounds(), Justification::centred, 1);
}

void SonobusAudioProcessorEditor::resized()
{
    // This is generally where you'll want to lay out the positions of any
    // subcomponents in your editor..

    Component::resized();
 
    const int narrowthresh = 588; //520;
    bool nownarrow = getWidth() < narrowthresh;
    if (nownarrow != isNarrow) {
        isNarrow = nownarrow;
        mPeerContainer->setNarrowMode(isNarrow);
        updateLayout();
    }
    
    DebugLogC("RESIZED to %d %d", getWidth(), getHeight());
    
    mainBox.performLayout(getLocalBounds());    

    Rectangle<int> peersminbounds = mPeerContainer->getMinimumContentBounds();
    
    mPeerContainer->setBounds(Rectangle<int>(0, 0, std::max(peersminbounds.getWidth(), mPeerViewport->getWidth() - 10), std::max(peersminbounds.getHeight() + 5, mPeerViewport->getHeight())));
    
    //mMainGroupImage->setTransformToFit(mMainGroupImage->getBounds().toFloat().translated(-mMainGroupImage->getWidth()*0.75, -mMainGroupImage->getHeight()*0.25), RectanglePlacement::fillDestination);
    //mMainPersonImage->setTransformToFit(mMainPersonImage->getBounds().toFloat().translated(-mMainPersonImage->getWidth()*0.75, -mMainPersonImage->getHeight()*0.25), RectanglePlacement::fillDestination);

    
    //mInGainLabel->setBounds(mInGainSlider->getBounds().removeFromLeft(mInGainSlider->getWidth()*0.5));
    //mDryLabel->setBounds(mDrySlider->getBounds().removeFromLeft(mDrySlider->getWidth()*0.5));
    mOutGainLabel->setBounds(mOutGainSlider->getBounds().removeFromLeft(mOutGainSlider->getWidth()*0.5));
}


void SonobusAudioProcessorEditor::updateLayout()
{
    int minKnobWidth = 50;
    int minPannerWidth = 40;
    int maxPannerWidth = 80;
    int minitemheight = 36;
    int knobitemheight = 80;
    int minpassheight = 30;
    int setitemheight = 36;
    int minButtonWidth = 90;
    int sliderheight = 44;
    int meterheight = 20 ;
    int servLabelWidth = 72;
    int iconheight = 24;
    int iconwidth = iconheight;
    int knoblabelheight = 18;
    
    inGainBox.items.clear();
    inGainBox.flexDirection = FlexBox::Direction::column;
    //inGainBox.items.add(FlexItem(4, 20).withMargin(0).withFlex(0));
    inGainBox.items.add(FlexItem(minKnobWidth, knoblabelheight, *mInGainLabel).withMargin(0).withFlex(0));
    inGainBox.items.add(FlexItem(minKnobWidth, minitemheight, *mInGainSlider).withMargin(0).withFlex(1));

    dryBox.items.clear();
    dryBox.flexDirection = FlexBox::Direction::column;
    dryBox.items.add(FlexItem(minKnobWidth, knoblabelheight, *mDryLabel).withMargin(0).withFlex(0));
    dryBox.items.add(FlexItem(minKnobWidth, minitemheight, *mDrySlider).withMargin(0).withFlex(1));
    //dryBox.items.add(FlexItem(4, 20).withMargin(0).withFlex(0));

    knobBox.items.clear();
    knobBox.flexDirection = FlexBox::Direction::row;
    knobBox.items.add(FlexItem(80, minitemheight, *mMasterMuteButton).withMargin(0).withFlex(0).withMaxHeight(minitemheight).withAlignSelf(FlexItem::AlignSelf::flexEnd));
    knobBox.items.add(FlexItem(4, 5).withMargin(0).withFlex(0));
    knobBox.items.add(FlexItem(80, minitemheight, inGainBox).withMargin(0).withFlex(1).withMaxWidth(120));
    knobBox.items.add(FlexItem(4, 5).withMargin(0).withFlex(0));
    knobBox.items.add(FlexItem(80, minitemheight, dryBox).withMargin(0).withFlex(1).withMaxWidth(120));
    knobBox.items.add(FlexItem(4, 5).withMargin(0).withFlex(0));
    knobBox.items.add(FlexItem(40, minitemheight, *mPanButton).withMargin(0).withFlex(1).withMaxWidth(54).withMaxHeight(minitemheight).withAlignSelf(FlexItem::AlignSelf::flexEnd));

    
    // options
    optionsNetbufBox.items.clear();
    optionsNetbufBox.flexDirection = FlexBox::Direction::row;
    //optionsNetbufBox.items.add(FlexItem(minButtonWidth, minitemheight, *mOptionsAutosizeStaticLabel).withMargin(0).withFlex(1));
    optionsNetbufBox.items.add(FlexItem(12, 12));
    optionsNetbufBox.items.add(FlexItem(minButtonWidth, minitemheight, *mBufferTimeSlider).withMargin(0).withFlex(3));
    optionsNetbufBox.items.add(FlexItem(4, 12));
    optionsNetbufBox.items.add(FlexItem(80, minitemheight, *mOptionsAutosizeDefaultChoice).withMargin(0).withFlex(0));

    optionsSendQualBox.items.clear();
    optionsSendQualBox.flexDirection = FlexBox::Direction::row;
    optionsSendQualBox.items.add(FlexItem(minButtonWidth, minitemheight, *mOptionsFormatChoiceStaticLabel).withMargin(0).withFlex(1));
    optionsSendQualBox.items.add(FlexItem(minButtonWidth, minitemheight, *mOptionsFormatChoiceDefaultChoice).withMargin(0).withFlex(1));

    
    optionsBox.items.clear();
    optionsBox.flexDirection = FlexBox::Direction::column;
    optionsBox.items.add(FlexItem(4, 12));
    optionsBox.items.add(FlexItem(100, minitemheight, optionsSendQualBox).withMargin(2).withFlex(0));
    optionsBox.items.add(FlexItem(4, 4));
    optionsBox.items.add(FlexItem(100, minitemheight, optionsNetbufBox).withMargin(2).withFlex(0));

    
    
    if (outChannels > 1) {
        inPannerMainBox.items.clear();
        inPannerMainBox.flexDirection = FlexBox::Direction::column;

        inPannerBox.items.clear();
        inPannerBox.flexDirection = FlexBox::Direction::row;
        inPannerLabelBox.items.clear();
        inPannerLabelBox.flexDirection = FlexBox::Direction::row;

        inPannerBox.items.add(FlexItem(minPannerWidth, minitemheight, *mInMonPanSlider1).withMargin(0).withFlex(1)); //.withMaxWidth(maxPannerWidth));
        inPannerLabelBox.items.add(FlexItem(minPannerWidth, 18, *mInMonPanLabel1).withMargin(0).withFlex(1)); //q.withMaxWidth(maxPannerWidth));

        mInMonPanSlider1->setVisible(true);
        mInMonPanLabel1->setVisible(true);
        
        if (inChannels > 1) {
            inPannerBox.items.add(FlexItem(4, 20).withMargin(1).withFlex(0));
            inPannerBox.items.add(FlexItem(minPannerWidth, minitemheight, *mInMonPanSlider2).withMargin(0).withFlex(1)); //.withMaxWidth(maxPannerWidth));

            inPannerLabelBox.items.add(FlexItem(4, 18).withMargin(1).withFlex(0));
            inPannerLabelBox.items.add(FlexItem(minPannerWidth, 18, *mInMonPanLabel2).withMargin(0).withFlex(1)); //.withMaxWidth(maxPannerWidth));

            mInMonPanSlider2->setVisible(true);
            mInMonPanLabel2->setVisible(true);
            
            mInMonPanSlider1->setDoubleClickReturnValue(true, -1.0);
            mInMonPanSlider2->setDoubleClickReturnValue(true, 1.0);
        }
        else {
            mInMonPanSlider1->setDoubleClickReturnValue(true, 0.0);

            mInMonPanSlider2->setVisible(false);
            mInMonPanLabel2->setVisible(false);
        }

        inPannerMainBox.items.add(FlexItem(6, 20, inPannerLabelBox).withMargin(1).withFlex(0));
        inPannerMainBox.items.add(FlexItem(6, minitemheight, inPannerBox).withMargin(1).withFlex(1));
        
        mPanButton->setVisible(true);
    }
    else {
        mPanButton->setVisible(false);
        //mInMonPanSlider1->setVisible(false);
    }
    
    //wetBox.items.clear();
    //wetBox.flexDirection = FlexBox::Direction::column;
    //wetBox.items.add(FlexItem(minKnobWidth, 18, *mWetLabel).withMargin(0));
    //wetBox.items.add(FlexItem(minKnobWidth, minitemheight, *mWetSlider).withMargin(0).withFlex(1));

    //bufferTimeBox.items.clear();
    //bufferTimeBox.flexDirection = FlexBox::Direction::row;
    //bufferTimeBox.items.add(FlexItem(80, 18, *mBufferTimeLabel).withMargin(2));
    //bufferTimeBox.items.add(FlexItem(200, minitemheight, *mBufferTimeSlider).withMargin(3).withFlex(1));

    titleBox.items.clear();
    titleBox.flexDirection = FlexBox::Direction::row;
    titleBox.items.add(FlexItem(2, 20).withMargin(1).withFlex(0));
    titleBox.items.add(FlexItem(minitemheight - 8, minitemheight - 12, *mSettingsButton).withMargin(1).withMaxWidth(40).withFlex(0));
    //titleBox.items.add(FlexItem(5, 20).withMargin(1).withFlex(0));
    titleBox.items.add(FlexItem(90, 20, *mTitleLabel).withMargin(1).withFlex(0));
    titleBox.items.add(FlexItem(6, 20).withMargin(1).withFlex(0));
    titleBox.items.add(FlexItem(100, meterheight, *inputMeter).withMargin(1).withFlex(4).withMaxWidth(250).withMaxHeight(meterheight).withAlignSelf(FlexItem::AlignSelf::center));
    if (iaaConnected) {
        titleBox.items.add(FlexItem(4, 20).withMargin(1).withFlex(0.1));
        titleBox.items.add(FlexItem(minitemheight, minitemheight, *mIAAHostButton).withMargin(0).withFlex(0));
    }
    titleBox.items.add(FlexItem(4, 20).withMargin(1).withFlex(0.1));

    
    //titleBox.items.add(FlexItem(3, 20).withMargin(1).withFlex(0.2));
    //titleBox.items.add(FlexItem(40, 20, *mDummyVisual).withMargin(1).withFlex(4).withMaxWidth(200.0f));
    
    paramsBox.items.clear();
    paramsBox.flexDirection = FlexBox::Direction::column;
    paramsBox.items.add(FlexItem(minKnobWidth, minitemheight, titleBox).withMargin(2).withFlex(0));
    //paramsBox.items.add(FlexItem(minKnobWidth, minitemheight, inGainBox).withMargin(2).withFlex(0));
    paramsBox.items.add(FlexItem(minKnobWidth*4, knobitemheight, knobBox).withMargin(2).withFlex(0));
    //paramsBox.items.add(FlexItem(6, 2).withMargin(0).withFlex(0));
    //paramsBox.items.add(FlexItem(100, meterheight, *inputMeter).withMargin(2).withFlex(0));
    //paramsBox.items.add(FlexItem(6, 2).withMargin(0).withFlex(0));
    //paramsBox.items.add(FlexItem(minKnobWidth, minitemheight, dryBox).withMargin(2).withFlex(0));

    localAddressBox.items.clear();
    localAddressBox.flexDirection = FlexBox::Direction::row;
    localAddressBox.items.add(FlexItem(60, 18, *mLocalAddressStaticLabel).withMargin(2).withFlex(1).withMaxWidth(110));
    localAddressBox.items.add(FlexItem(100, minitemheight, *mLocalAddressLabel).withMargin(2).withFlex(1).withMaxWidth(160));


    addressBox.items.clear();
    addressBox.flexDirection = FlexBox::Direction::row;
    addressBox.items.add(FlexItem(servLabelWidth, 18, *mRemoteAddressStaticLabel).withMargin(2).withFlex(0.25));
    addressBox.items.add(FlexItem(172, minitemheight, *mAddRemoteHostEditor).withMargin(2).withFlex(1));
    //addressBox.items.add(FlexItem(55, minitemheight, *mAddRemotePortEditor).withMargin(2).withFlex(1));

    
    remoteBox.items.clear();
    remoteBox.flexDirection = FlexBox::Direction::column;
    remoteBox.items.add(FlexItem(5, 8).withFlex(0));
    remoteBox.items.add(FlexItem(180, minitemheight, addressBox).withMargin(2).withFlex(0));
    remoteBox.items.add(FlexItem(minButtonWidth, minitemheight, *mDirectConnectButton).withMargin(8).withFlex(0).withMinWidth(100));
    remoteBox.items.add(FlexItem(180, minitemheight, *mDirectConnectDescriptionLabel).withMargin(2).withFlex(1));
    //remoteBox.items.add(FlexItem(10, 0).withFlex(1));
    remoteBox.items.add(FlexItem(60, minitemheight, localAddressBox).withMargin(2).withFlex(0));

    servAddressBox.items.clear();
    servAddressBox.flexDirection = FlexBox::Direction::row;
    servAddressBox.items.add(FlexItem(servLabelWidth, minitemheight, *mServerHostStaticLabel).withMargin(2).withFlex(1));
    servAddressBox.items.add(FlexItem(172, minpassheight, *mServerHostEditor).withMargin(2).withFlex(1));

    servUserBox.items.clear();
    servUserBox.flexDirection = FlexBox::Direction::row;
    servUserBox.items.add(FlexItem(servLabelWidth, minitemheight, *mServerUserStaticLabel).withMargin(2).withFlex(0.25));
    servUserBox.items.add(FlexItem(120, minitemheight, *mServerUsernameEditor).withMargin(2).withFlex(1));

    servUserPassBox.items.clear();
    servUserPassBox.flexDirection = FlexBox::Direction::row;
    servUserPassBox.items.add(FlexItem(servLabelWidth, minpassheight, *mServerUserPassStaticLabel).withMargin(2).withFlex(1));
    servUserPassBox.items.add(FlexItem(90, minpassheight, *mServerUserPasswordEditor).withMargin(2).withFlex(1));

    servGroupBox.items.clear();
    servGroupBox.flexDirection = FlexBox::Direction::row;
    servGroupBox.items.add(FlexItem(servLabelWidth, minitemheight, *mServerGroupStaticLabel).withMargin(2).withFlex(0.25));
    servGroupBox.items.add(FlexItem(120, minitemheight, *mServerGroupEditor).withMargin(2).withFlex(1));
    servGroupBox.items.add(FlexItem(minPannerWidth, minitemheight, *mServerGroupRandomButton).withMargin(2).withFlex(0));

    servGroupPassBox.items.clear();
    servGroupPassBox.flexDirection = FlexBox::Direction::row;
    servGroupPassBox.items.add(FlexItem(servLabelWidth, minpassheight, *mServerGroupPassStaticLabel).withMargin(2).withFlex(1));
    servGroupPassBox.items.add(FlexItem(90, minpassheight, *mServerGroupPasswordEditor).withMargin(2).withFlex(1));

    
    servButtonBox.items.clear();
    servButtonBox.flexDirection = FlexBox::Direction::row;
    servButtonBox.items.add(FlexItem(minButtonWidth, minitemheight, *mServerConnectButton).withMargin(2).withFlex(1));


    
    serverBox.items.clear();
    serverBox.flexDirection = FlexBox::Direction::column;
    serverBox.items.add(FlexItem(5, 3).withFlex(0));
    serverBox.items.add(FlexItem(180, minitemheight, servGroupBox).withMargin(2).withFlex(0));
    serverBox.items.add(FlexItem(180, minpassheight, servGroupPassBox).withMargin(2).withFlex(0));
    serverBox.items.add(FlexItem(5, 6).withFlex(0));
    serverBox.items.add(FlexItem(180, minitemheight, servUserBox).withMargin(2).withFlex(0));
    //serverBox.items.add(FlexItem(180, minpassheight, servUserPassBox).withMargin(2).withFlex(0));
    serverBox.items.add(FlexItem(5, 6).withFlex(1));
    serverBox.items.add(FlexItem(minButtonWidth, minitemheight, servButtonBox).withMargin(2).withFlex(0));
    serverBox.items.add(FlexItem(5, 6).withFlex(0));
    serverBox.items.add(FlexItem(80, minpassheight, *mServerStatusLabel).withMargin(2).withFlex(1));
    serverBox.items.add(FlexItem(100, minitemheight, servAddressBox).withMargin(2).withFlex(0));
    serverBox.items.add(FlexItem(5, 8).withFlex(0));
    //remoteBox.items.add(FlexItem(60, minitemheight, *mPatchbayButton).withMargin(2).withFlex(0.5).withMaxWidth(120));


    mainGroupUserBox.items.clear();
    mainGroupUserBox.flexDirection = FlexBox::Direction::row;
    mainGroupUserBox.items.add(FlexItem(iconwidth, iconheight, *mMainGroupImage).withMargin(0).withFlex(0));
    mainGroupUserBox.items.add(FlexItem(minButtonWidth, minitemheight/2, *mMainGroupLabel).withMargin(2).withFlex(1));
    mainGroupUserBox.items.add(FlexItem(minButtonWidth, minitemheight/2, *mMainUserLabel).withMargin(2).withFlex(1));
    mainGroupUserBox.items.add(FlexItem(iconwidth, iconheight, *mMainPersonImage).withMargin(0).withFlex(0));

    connectBox.items.clear();
    connectBox.flexDirection = FlexBox::Direction::column;
    connectBox.items.add(FlexItem(minButtonWidth, minitemheight, *mConnectButton).withMargin(2).withFlex(0));
    connectBox.items.add(FlexItem(minButtonWidth, minitemheight/2, mainGroupUserBox).withMargin(2).withFlex(0));
    connectBox.items.add(FlexItem(minButtonWidth, minitemheight/2, *mMainPeerLabel).withMargin(2).withFlex(0));
    //connectBox.items.add(FlexItem(80, minitemheight, *mMainStatusLabel).withMargin(2).withFlex(1));

    recentsBox.items.clear();
    recentsBox.flexDirection = FlexBox::Direction::column;
    recentsBox.items.add(FlexItem(minButtonWidth, minitemheight, *mRecentsListBox).withMargin(6).withFlex(1));
    
    
    middleBox.items.clear();
    int midboxminheight = (minitemheight + knobitemheight + 8);
    int midboxminwidth = 296;
    
    if (isNarrow) {
        middleBox.flexDirection = FlexBox::Direction::column;
        middleBox.items.add(FlexItem(100, minitemheight + knobitemheight + 6, paramsBox).withMargin(2).withFlex(0));
        middleBox.items.add(FlexItem(5, 2));
        middleBox.items.add(FlexItem(150, minitemheight*2 + 12, connectBox).withMargin(2).withFlex(0));
        middleBox.items.add(FlexItem(6, 2));

        midboxminheight = (minitemheight*3 + knobitemheight + 24);
        midboxminwidth = 190;
    } else {
        middleBox.flexDirection = FlexBox::Direction::row;        
        middleBox.items.add(FlexItem(minKnobWidth + minPannerWidth*2, minitemheight + knobitemheight + 12, paramsBox).withMargin(2).withFlex(1));
        middleBox.items.add(FlexItem(4, 1).withMaxWidth(6));
        middleBox.items.add(FlexItem(260, minitemheight*2 + 20, connectBox).withMargin(2).withFlex(0).withMaxWidth(400));
    }

    outmeterBox.items.clear();
    outmeterBox.flexDirection = FlexBox::Direction::column;
    outmeterBox.items.add(FlexItem(80, 14, *mMainStatusLabel).withMargin(0).withFlex(0));
    outmeterBox.items.add(FlexItem(6, 2));
    outmeterBox.items.add(FlexItem(100, meterheight, *outputMeter).withMargin(0).withFlex(1).withMaxHeight(meterheight)); //.withAlignSelf(FlexItem::AlignSelf::center));

    
    toolbarBox.items.clear();
    toolbarBox.flexDirection = FlexBox::Direction::row;
    toolbarBox.items.add(FlexItem(4, 6).withMargin(0).withFlex(0));
    toolbarBox.items.add(FlexItem(100, minitemheight, *mOutGainSlider).withMargin(2).withFlex(1));
    //toolbarBox.items.add(FlexItem(100, meterheight, *outputMeter).withMargin(2).withFlex(1).withMaxHeight(meterheight).withAlignSelf(FlexItem::AlignSelf::center));
    toolbarBox.items.add(FlexItem(100, meterheight, outmeterBox).withMargin(2).withFlex(1));
#if JUCE_IOS
#else
    if (processor.getCurrentJoinedGroup().isEmpty() && processor.getNumberRemotePeers() > 1) {
        toolbarBox.items.add(FlexItem(6, 6).withMargin(1).withFlex(0.2));
        toolbarBox.items.add(FlexItem(60, minitemheight, *mPatchbayButton).withMargin(2).withFlex(0.5).withMaxWidth(120));
    }
#endif
    if (mRecordingButton) {
        toolbarBox.items.add(FlexItem(40, minitemheight, *mRecordingButton).withMargin(2).withFlex(0));
    }

    toolbarBox.items.add(FlexItem(14, 6).withMargin(0).withFlex(0));
    

    //middleBox.items.clear();
    //middleBox.flexDirection = FlexBox::Direction::row;
    //middleBox.items.add(FlexItem(minButtonWidth*2, minitemheight, remoteSourceBox).withMargin(2).withFlex(1));
    //middleBox.items.add(FlexItem(10, 0).withFlex(0.5).withMaxWidth(60));
    //middleBox.items.add(FlexItem(minButtonWidth*2, minitemheight, remoteSinkBox).withMargin(2).withFlex(1));

    
    int minheight = 0;
    
    mainBox.items.clear();
    mainBox.flexDirection = FlexBox::Direction::column;    
    //mainBox.items.add(FlexItem(120, 20, titleBox).withMargin(3).withFlex(1).withMaxHeight(30)); minheight += 22;
    mainBox.items.add(FlexItem(midboxminwidth, midboxminheight, middleBox).withMargin(3).withFlex(0)); minheight += midboxminheight;
    //mainBox.items.add(FlexItem(10, 0).withFlex(0.5).withMaxHeight(2.0));
    mainBox.items.add(FlexItem(120, 50, *mPeerViewport).withMargin(3).withFlex(3)); minheight += 50 + 6;
    //mainBox.items.add(FlexItem(120, 50, *mPeerContainer).withMargin(3).withFlex(3)); minheight += 50;
    //mainBox.items.add(FlexItem(10, 0).withFlex(0.5).withMaxHeight(2.0));
    mainBox.items.add(FlexItem(10, 1).withFlex(0));
    mainBox.items.add(FlexItem(100, minitemheight, toolbarBox).withMargin(0).withFlex(0)); minheight += minitemheight + 4;
    mainBox.items.add(FlexItem(10, 8).withFlex(0));
        
    
    //if (getHeight() < minheight) {
    //    setSize(getWidth(), minheight);
    //}
}

void SonobusAudioProcessorEditor::showPopTip(const String & message, int timeoutMs, Component * target, int maxwidth)
{

    popTip = std::make_unique<BubbleMessageComponent>();
    popTip->setAllowedPlacement(BubbleMessageComponent::BubblePlacement::above | BubbleMessageComponent::BubblePlacement::below);
    
    if (target) {
        target->getTopLevelComponent()->addChildComponent (popTip.get());
    }
    else {
        addChildComponent(popTip.get());
    }
    
    AttributedString text(message);
    text.setJustification (Justification::centred);
    text.setColour (findColour (TextButton::textColourOffId));
    text.setFont(Font(12));
    if (target) {
        popTip->showAt(target, text, timeoutMs);
    }
    else {
        Rectangle<int> topbox(getWidth()/2 - maxwidth/2, 0, maxwidth, 2);
        popTip->showAt(topbox, text, timeoutMs);
    }
}

#pragma RecentsListModel

SonobusAudioProcessorEditor::RecentsListModel::RecentsListModel(SonobusAudioProcessorEditor * parent_) : parent(parent_)
{
    groupImage = ImageCache::getFromMemory(BinaryData::people_png, BinaryData::people_pngSize);
    personImage = ImageCache::getFromMemory(BinaryData::person_png, BinaryData::person_pngSize);
}


void SonobusAudioProcessorEditor::RecentsListModel::updateState()
{
    parent->processor.getRecentServerConnectionInfos(recents);
    
}

int SonobusAudioProcessorEditor::RecentsListModel::getNumRows()
{
    return recents.size();
}

void SonobusAudioProcessorEditor::RecentsListModel::paintListBoxItem (int rowNumber, Graphics &g, int width, int height, bool rowIsSelected)
{
    if (rowNumber >= recents.size()) return;
    
    if (rowIsSelected) {
        g.setColour (parent->findColour(selectedColourId));
        g.fillRect(Rectangle<int>(0,0,width,height));
    }
    
    g.setColour(parent->findColour(separatorColourId));
    g.drawLine(0, height-1, width, height);
    
    
    g.setColour (parent->findColour(nameTextColourId));
    g.setFont (parent->recentsGroupFont);
    
    AooServerConnectionInfo & info = recents.getReference(rowNumber);
    
    float xratio = 0.5;
    float yratio = 0.6;
    
    // DebugLogC("Paint %s", text.toRawUTF8());
    float iconsize = height*yratio;
    float groupheight = height*yratio;
    g.drawImageWithin(groupImage, 0, 0, iconsize, iconsize, RectanglePlacement::fillDestination);
    g.drawFittedText (info.groupName, iconsize + 4, 0, width*xratio - 8 - iconsize, groupheight, Justification::centredLeft, true);

    g.setFont (parent->recentsNameFont);    
    g.setColour (parent->findColour(nameTextColourId).withAlpha(0.8f));
    g.drawImageWithin(personImage, width*xratio, 0, iconsize, iconsize, RectanglePlacement::fillDestination);
    g.drawFittedText (info.userName, width*xratio + iconsize, 0, width*(1.0f - xratio) - 4 - iconsize, groupheight, Justification::centredLeft, true);

    String infostr;
    if (info.groupPassword.isNotEmpty()) {
        infostr = TRANS("password protected, ");
    }

    infostr += String::formatted(TRANS("on %s "), Time(info.timestamp).toString(true, true, false).toRawUTF8());
    
    if (info.serverHost != "aoo.sonobus.net") {
        infostr += String::formatted(TRANS("to %s"), info.serverHost.toRawUTF8());
    }

    g.setColour (parent->findColour(nameTextColourId).withAlpha(0.5f));
    g.setFont (parent->recentsInfoFont);    
    
    g.drawFittedText (infostr, 14, height * yratio, width - 24, height * (1.0f - yratio), Justification::centredTop, true);

}

void SonobusAudioProcessorEditor::RecentsListModel::listBoxItemClicked (int rowNumber, const MouseEvent& e)
{
    // use this
    DebugLogC("Clicked %d", rowNumber);
    
    parent->connectWithInfo(recents.getReference(rowNumber));
}

void SonobusAudioProcessorEditor::RecentsListModel::selectedRowsChanged(int rowNumber)
{

}
