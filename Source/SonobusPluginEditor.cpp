

#include "SonobusPluginProcessor.h"
#include "SonobusPluginEditor.h"

#include "BeatToggleGrid.h"
#include "DebugLogC.h"

#include "PeersContainerView.h"

enum {
    PeriodicUpdateTimerId = 0
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


static void configLevelSlider(Slider * slider)
{
    //slider->setTextValueSuffix(" dB");
    slider->setRange(0.0, 1.0, 0.0);
    slider->setSkewFactor(0.25);
    slider->setDoubleClickReturnValue(true, 1.0);
    slider->setTextBoxIsEditable(false);
    slider->setSliderSnapsToMousePosition(false);
    slider->setScrollWheelEnabled(false);
    slider->valueFromTextFunction = [](const String& s) -> float { return Decibels::decibelsToGain(s.getFloatValue()); };
    slider->textFromValueFunction = [](float v) -> String { return Decibels::toString(Decibels::gainToDecibels(v), 1); };
}


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

static void configEditor(TextEditor *editor)
{
    editor->setIndents(8, 6);
}


//==============================================================================
SonobusAudioProcessorEditor::SonobusAudioProcessorEditor (SonobusAudioProcessor& p)
    : AudioProcessorEditor (&p), processor (p),  sonoSliderLNF(14)
{
    LookAndFeel::setDefaultLookAndFeel(&sonoLookAndFeel);
    
    sonoLookAndFeel.setUsingNativeAlertWindows(true);

    mTitleLabel = std::make_unique<Label>("title", TRANS("SonoBus"));
    mTitleLabel->setFont(18);
    mTitleLabel->setColour(Label::textColourId, Colour::fromFloatRGBA(0.4f, 0.6f, 0.8f, 1.0f));

    mMainGroupLabel = std::make_unique<Label>("group", "");
    mMainGroupLabel->setJustificationType(Justification::centred);
    mMainGroupLabel->setFont(16);

    mMainPeerLabel = std::make_unique<Label>("peers", "");
    mMainPeerLabel->setJustificationType(Justification::centred);
    mMainPeerLabel->setFont(14);
    
    
    
    mInGainSlider     = std::make_unique<Slider>(Slider::LinearBar,  Slider::TextBoxBelow); 
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

    
    
    mDrySlider     = std::make_unique<Slider>(Slider::LinearBar,  Slider::TextBoxBelow); 
    mDrySlider->setName("dry");
    mDrySlider->setSliderSnapsToMousePosition(false);
    mDrySlider->setTextBoxIsEditable(false);
    mDrySlider->setScrollWheelEnabled(false);

    mOutGainSlider     = std::make_unique<Slider>(Slider::LinearBar,  Slider::TextBoxBelow); 
    mOutGainSlider->setName("wet");
    mOutGainSlider->setSliderSnapsToMousePosition(false);
    mOutGainSlider->setTextBoxIsEditable(false);
    mOutGainSlider->setScrollWheelEnabled(false);

    //configKnobSlider(mInGainSlider.get());
    //configKnobSlider(mDrySlider.get());
    //configKnobSlider(mWetSlider.get());
    
    //mBufferTimeSlider    = std::make_unique<Slider>(Slider::LinearBar,  Slider::TextBoxRight); 
    //mBufferTimeSlider->setName("time");
    //mBufferTimeSlider->setSliderSnapsToMousePosition(false);
    //mBufferTimeSlider->setChangeNotificationOnlyOnRelease(true);

    mInGainLabel = std::make_unique<Label>(SonobusAudioProcessor::paramDry, TRANS("Input Level"));
    configLabel(mInGainLabel.get(), false);

    mDryLabel = std::make_unique<Label>(SonobusAudioProcessor::paramDry, TRANS("Input Monitor"));
    configLabel(mDryLabel.get(), false);

    //mWetLabel = std::make_unique<Label>(SonobusAudioProcessor::paramWet, TRANS("Wet"));
    //configLabel(mWetLabel.get(), false);

    mOutGainLabel = std::make_unique<Label>("outgain", TRANS("Output Level"));
    configLabel(mOutGainLabel.get(), false);

    
    
    mInGainAttachment     = std::make_unique<AudioProcessorValueTreeState::SliderAttachment> (p.getValueTreeState(), SonobusAudioProcessor::paramInGain, *mInGainSlider);
    mDryAttachment     = std::make_unique<AudioProcessorValueTreeState::SliderAttachment> (p.getValueTreeState(), SonobusAudioProcessor::paramDry, *mDrySlider);
    mWetAttachment     = std::make_unique<AudioProcessorValueTreeState::SliderAttachment> (p.getValueTreeState(), SonobusAudioProcessor::paramWet, *mOutGainSlider);
    //mBufferTimeAttachment     = std::make_unique<AudioProcessorValueTreeState::SliderAttachment> (p.getValueTreeState(), SonobusAudioProcessor::paramBufferTime, *mBufferTimeSlider);
    mInMonPan1Attachment     = std::make_unique<AudioProcessorValueTreeState::SliderAttachment> (p.getValueTreeState(), SonobusAudioProcessor::paramInMonitorPan1, *mInMonPanSlider1);
    mInMonPan2Attachment     = std::make_unique<AudioProcessorValueTreeState::SliderAttachment> (p.getValueTreeState(), SonobusAudioProcessor::paramInMonitorPan2, *mInMonPanSlider2);
    

    mConnectTab = std::make_unique<TabbedComponent>(TabbedButtonBar::Orientation::TabsAtTop);
    mConnectTab->setOutline(0);
    mConnectTab->getTabbedButtonBar().setMinimumTabScaleFactor(0.1f);
    mConnectTab->getTabbedButtonBar().setColour(TabbedButtonBar::frontTextColourId, Colour::fromFloatRGBA(0.4, 0.8, 1.0, 1.0));
    mConnectTab->getTabbedButtonBar().setColour(TabbedButtonBar::frontOutlineColourId, Colour::fromFloatRGBA(0.4, 0.8, 1.0, 0.5));


    mDirectConnectContainer = std::make_unique<Component>();
    mServerConnectContainer = std::make_unique<Component>();
    serverContainer = std::make_unique<Component>();
    
    mConnectTab->addTab(TRANS("Group Connect"), Colour::fromFloatRGBA(0.1, 0.1, 0.1, 1.0), mServerConnectContainer.get(), false);
    mConnectTab->addTab(TRANS("Direct Connect"), Colour::fromFloatRGBA(0.1, 0.1, 0.1, 1.0), mDirectConnectContainer.get(), false);
    

    
    mLocalAddressLabel = std::make_unique<Label>("localaddr", TRANS("--"));
    mLocalAddressLabel->setJustificationType(Justification::centredLeft);
    mLocalAddressStaticLabel = std::make_unique<Label>("localaddrst", TRANS("Local Address:"));
    mLocalAddressStaticLabel->setJustificationType(Justification::centredRight);


    mRemoteAddressStaticLabel = std::make_unique<Label>("remaddrst", TRANS("Host: "));
    mRemoteAddressStaticLabel->setJustificationType(Justification::centredRight);
    
    mAddRemoteHostEditor = std::make_unique<TextEditor>("remaddredit");
    mAddRemoteHostEditor->setFont(Font(16));
    mAddRemoteHostEditor->setText("100.36.128.246:11000"); // 100.36.128.246

    
    //mAddRemotePortEditor = std::make_unique<TextEditor>("remportedit");
    //mAddRemotePortEditor->setFont(Font(16));
    //mAddRemotePortEditor->setText("11000");
    
    
    mConnectButton = std::make_unique<TextButton>("directconnect");
    mConnectButton->setButtonText(TRANS("Connect..."));
    mConnectButton->addListener(this);

    mDirectConnectButton = std::make_unique<TextButton>("directconnect");
    mDirectConnectButton->setButtonText(TRANS("Direct Connect"));
    mDirectConnectButton->addListener(this);

    
    mServerConnectButton = std::make_unique<TextButton>("serverconnect");
    mServerConnectButton->setButtonText(TRANS("Connect to Group"));
    mServerConnectButton->addListener(this);
    mServerConnectButton->setColour(TextButton::buttonColourId, Colour::fromFloatRGBA(0.1, 0.4, 0.6, 0.6));
    
    mServerHostEditor = std::make_unique<TextEditor>("srvaddredit");
    mServerHostEditor->setFont(Font(14));
    mServerHostEditor->setText("aoo.sonobus.net:10998"); // 100.36.128.246 // 23.23.205.37
    configEditor(mServerHostEditor.get());
    
    mServerUsernameEditor = std::make_unique<TextEditor>("srvaddredit");
    mServerUsernameEditor->setFont(Font(16));
#if JUCE_IOS
    String username = SystemStats::getComputerName(); //SystemStats::getLogonName();
#else
    String username = SystemStats::getFullUserName(); //SystemStats::getLogonName();    
    if (username.length() > 0) username = username.replaceSection(0, 1, username.substring(0, 1).toUpperCase());
#endif
    
    mServerUsernameEditor->setText(username);
    configEditor(mServerUsernameEditor.get());

    mServerUserPasswordEditor = std::make_unique<TextEditor>("userpass", 0x25cf);
    mServerUserPasswordEditor->setFont(Font(14));
    mServerUserPasswordEditor->setTextToShowWhenEmpty(TRANS("optional"), Colour(0x44ffffff));
    mServerUserPasswordEditor->setText("");
    configEditor(mServerUserPasswordEditor.get());

    
    mServerUserStaticLabel = std::make_unique<Label>("localaddrst", TRANS("Your Displayed Name:"));
    configServerLabel(mServerUserStaticLabel.get());
    
    mServerUserPassStaticLabel = std::make_unique<Label>("localaddrst", TRANS("Password:"));
    configServerLabel(mServerUserPassStaticLabel.get());

    mServerGroupStaticLabel = std::make_unique<Label>("localaddrst", TRANS("Group Name:"));
    configServerLabel(mServerGroupStaticLabel.get());

    mServerGroupPassStaticLabel = std::make_unique<Label>("localaddrst", TRANS("Password:"));
    configServerLabel(mServerGroupPassStaticLabel.get());

    mServerHostStaticLabel = std::make_unique<Label>("localaddrst", TRANS("Using Server:"));
    configServerLabel(mServerHostStaticLabel.get());
    

    mServerGroupEditor = std::make_unique<TextEditor>("groupedit");
    mServerGroupEditor->setFont(Font(16));
    mServerGroupEditor->setText("Default");
    configEditor(mServerGroupEditor.get());

    mServerGroupPasswordEditor = std::make_unique<TextEditor>("grouppass", 0x25cf);
    mServerGroupPasswordEditor->setFont(Font(14));
    mServerGroupPasswordEditor->setTextToShowWhenEmpty(TRANS("optional"), Colour(0x44ffffff));
    mServerGroupPasswordEditor->setText("");
    configEditor(mServerGroupPasswordEditor.get());

    
    mServerStatusLabel = std::make_unique<Label>("servstat", "");
    mServerStatusLabel->setJustificationType(Justification::centredRight);
    mServerStatusLabel->setFont(14);
    mServerStatusLabel->setColour(Label::textColourId, Colour(0x99ffaaaa));

    mMainStatusLabel = std::make_unique<Label>("servstat", "");
    mMainStatusLabel->setJustificationType(Justification::centredRight);
    mMainStatusLabel->setFont(14);
    mMainStatusLabel->setColour(Label::textColourId, Colour(0x66ffffff));
    


    

    mPatchbayButton = std::make_unique<TextButton>("patch");
    mPatchbayButton->setButtonText(TRANS("Patchbay"));
    mPatchbayButton->addListener(this);

    auto flags = foleys::LevelMeter::Horizontal|foleys::LevelMeter::Minimal|foleys::LevelMeter::MaxNumber;
    
    inputMeter = std::make_unique<foleys::LevelMeter>(flags);
    inputMeter->setLookAndFeel(&inMeterLnf);
    inputMeter->setRefreshRateHz(10);
    inputMeter->setMeterSource (&processor.getInputMeterSource());   
    
    outputMeter = std::make_unique<foleys::LevelMeter>(flags);
    outputMeter->setLookAndFeel(&outMeterLnf);
    outputMeter->setRefreshRateHz(10);
    outputMeter->setMeterSource (&processor.getOutputMeterSource());        
    
    
    mSettingsButton = std::make_unique<SonoDrawableButton>("settings",  DrawableButton::ButtonStyle::ImageFitted);
    std::unique_ptr<Drawable> setimg(Drawable::createFromImageData(BinaryData::settings_icon_svg, BinaryData::settings_icon_svgSize));
    mSettingsButton->setImages(setimg.get());
    mSettingsButton->addListener(this);
    mSettingsButton->setAlpha(0.7);
    mSettingsButton->addMouseListener(this, false);

    mPeerContainer = std::make_unique<PeersContainerView>(processor);
    
    mPeerViewport = std::make_unique<Viewport>();
    mPeerViewport->setViewedComponent(mPeerContainer.get(), false);
    
#if JUCE_IOS
    mPeerViewport->setScrollOnDragEnabled(true);
#endif
    
    addAndMakeVisible(mTitleLabel.get());

    addAndMakeVisible(mMainGroupLabel.get());
    addAndMakeVisible(mMainPeerLabel.get());
    
    //addAndMakeVisible(mConnectTab.get());    
    addAndMakeVisible(mDrySlider.get());
    addAndMakeVisible(mOutGainSlider.get());
    //addAndMakeVisible(mWetLabel.get());
    addAndMakeVisible(mInGainSlider.get());
//    addAndMakeVisible(mBufferTimeSlider.get());

    addAndMakeVisible(mLocalAddressLabel.get());
    addAndMakeVisible(mLocalAddressStaticLabel.get());
    addAndMakeVisible(mPatchbayButton.get());
    addAndMakeVisible(mConnectButton.get());
    addAndMakeVisible(mMainStatusLabel.get());
    
    
    mDirectConnectContainer->addAndMakeVisible(mDirectConnectButton.get());
    mDirectConnectContainer->addAndMakeVisible(mAddRemoteHostEditor.get());

    mServerConnectContainer->addAndMakeVisible(mServerConnectButton.get());
    mServerConnectContainer->addAndMakeVisible(mServerHostEditor.get());
    mServerConnectContainer->addAndMakeVisible(mServerUsernameEditor.get());
    mServerConnectContainer->addAndMakeVisible(mServerUserStaticLabel.get());
    mServerConnectContainer->addAndMakeVisible(mServerGroupEditor.get());
    mServerConnectContainer->addAndMakeVisible(mServerGroupStaticLabel.get());
    mServerConnectContainer->addAndMakeVisible(mServerHostStaticLabel.get());
    mServerConnectContainer->addAndMakeVisible(mServerUserPassStaticLabel.get());
    mServerConnectContainer->addAndMakeVisible(mServerGroupPassStaticLabel.get());
    mServerConnectContainer->addAndMakeVisible(mServerGroupPasswordEditor.get());
    mServerConnectContainer->addAndMakeVisible(mServerUserPasswordEditor.get());
    mServerConnectContainer->addAndMakeVisible(mServerStatusLabel.get());
    
    //addAndMakeVisible(mAddRemotePortEditor.get());
    //addAndMakeVisible(mRemoteAddressStaticLabel.get());
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

        processor.startAooServer();
        
        setResizable(true, false);
    } else  {
        setResizable(true, true);    
    }
    
    processor.addClientListener(this);
    
    startTimer(PeriodicUpdateTimerId, 500);
    
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
    
}


void SonobusAudioProcessorEditor::updatePeerState(bool force)
{
    if (force || mPeerContainer->getPeerViewCount() != processor.getNumberRemotePeers()) {
        mPeerContainer->rebuildPeerViews();
        resized();
        
        if (patchbayCalloutBox) {
            mPatchMatrixView->updateGridLayout();
            mPatchMatrixView->updateGrid();
        }

    }
    else {
        mPeerContainer->updatePeerViews();

        if (patchbayCalloutBox) {
            mPatchMatrixView->updateGrid();
        }
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

void SonobusAudioProcessorEditor::timerCallback(int timerid)
{
    if (timerid == PeriodicUpdateTimerId) {
        
        updatePeerState();
        
        updateChannelState();
        
        if (currGroup != processor.getCurrentJoinedGroup() || currConnected != processor.isConnectedToServer()) {
            updateState();
        }
        
        double nowstamp = Time::getMillisecondCounterHiRes() * 1e-3;
        if (serverStatusFadeTimestamp > 0.0 && nowstamp >= serverStatusFadeTimestamp) {
            Desktop::getInstance().getAnimator().fadeOut(mMainStatusLabel.get(), 500);
            serverStatusFadeTimestamp = 0.0;
        }
        
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
            processor.connectRemotePeer(host, port);
        }
    }
    else if (buttonThatWasClicked == mConnectButton.get()) {
        
        if (processor.isConnectedToServer()) {
            processor.disconnectFromServer();
            updateState();
        }
        else {
            if (!serverCalloutBox) {
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
            
            
            currConnectionInfo = AooServerConnectionInfo();
            currConnectionInfo.userName = mServerUsernameEditor->getText();
            currConnectionInfo.userPassword = mServerUserPasswordEditor->getText();
            currConnectionInfo.groupName = mServerGroupEditor->getText();
            currConnectionInfo.groupPassword = mServerGroupPasswordEditor->getText();
            currConnectionInfo.serverHost = host;
            currConnectionInfo.serverPort = port;
            
            if (host.isNotEmpty() && port != 0) {
                processor.connectToServer(currConnectionInfo.serverHost, currConnectionInfo.serverPort, currConnectionInfo.userName, currConnectionInfo.userPassword);
                updateState();
            }
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
    else {
        
       
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
        
        wrap->setSize(jmin(defWidth + 8, dw->getWidth() - 20), jmin(defHeight + 8, dw->getHeight() - 24));
        
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
        
        mServerStatusLabel->setText("", dontSendNotification);
        
        remoteBox.performLayout(mDirectConnectContainer->getLocalBounds());
        serverBox.performLayout(mServerConnectContainer->getLocalBounds());

        //wrap->addAndMakeVisible(mSettingsPanel.get());
        wrap->setViewedComponent(mConnectTab.get(), false);
        
                
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
            showOrHideSettings();        
        }
    }
}

void SonobusAudioProcessorEditor::showOrHideSettings()
{
    DebugLogC("Got settings click");

    if (settingsCalloutBox == nullptr) {
        
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
        const int defHeight = 330;                
        
        wrap->setSize(jmin(defWidth + 8, dw->getWidth() - 20), jmin(defHeight + 8, dw->getHeight() - 24));
        
        //Rectangle<int> setbounds = Rectangle<int>(5, mTitleImage->getBottom() + 2, std::min(100, getLocalBounds().getWidth() - 10), 80);

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

            mAudioDeviceSelector->setBounds(Rectangle<int>(0,0,defWidth,defHeight));
            
            //wrap->addAndMakeVisible(mSettingsPanel.get());
            wrap->setViewedComponent(mAudioDeviceSelector.get(), false);
               
            //settingsBox.performLayout(mAudioDeviceSelector->getLocalBounds());

        }
        
        /*
        
        mSettingsPanel->setBounds(Rectangle<int>(0,0,defWidth,defHeight));
        
        //wrap->addAndMakeVisible(mSettingsPanel.get());
        wrap->setViewedComponent(mSettingsPanel.get(), false);
           
        settingsBox.performLayout(mSettingsPanel->getLocalBounds());
         */
        
        Rectangle<int> bounds =  dw->getLocalArea(nullptr, mSettingsButton->getScreenBounds());
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

    currGroup = processor.getCurrentJoinedGroup();
    if (!currGroup.isEmpty() && currConnected) {
        //mServerGroupEditor->setEnabled(false);
        mMainGroupLabel->setText(currGroup, dontSendNotification);
        String userstr;
        if (processor.getNumberRemotePeers() == 1) {
            userstr = String::formatted(TRANS("%d other user"), processor.getNumberRemotePeers());
        }
        else if (processor.getNumberRemotePeers() > 1) {
            userstr = String::formatted(TRANS("%d other users"), processor.getNumberRemotePeers());
        } else {
            userstr = TRANS("No other users");
        }

        mMainPeerLabel->setText(userstr, dontSendNotification);

    }
    else {
        //mServerGroupEditor->setEnabled(currConnected);
        mMainGroupLabel->setText("", dontSendNotification);
        mMainPeerLabel->setText("", dontSendNotification);
    }

}


void SonobusAudioProcessorEditor::parameterChanged (const String& pname, float newValue)
{
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
                if (ev.message == "access denied") {
                    statstr = TRANS("Already connected with this name");
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
 
    const int narrowthresh = 520;
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
    
    
    
    mInGainLabel->setBounds(mInGainSlider->getBounds().removeFromLeft(mInGainSlider->getWidth()*0.5));
    mDryLabel->setBounds(mDrySlider->getBounds().removeFromLeft(mDrySlider->getWidth()*0.5));
    mOutGainLabel->setBounds(mOutGainSlider->getBounds().removeFromLeft(mOutGainSlider->getWidth()*0.5));
}


void SonobusAudioProcessorEditor::updateLayout()
{
    int minKnobWidth = 50;
    int minPannerWidth = 40;
    int maxPannerWidth = 80;
    int minitemheight = 36;
    int minpassheight = 30;
    int setitemheight = 36;
    int minButtonWidth = 90;
    int sliderheight = 44;
    int meterheight = 32 ;
    int servLabelWidth = 100;
    
    inGainBox.items.clear();
    inGainBox.flexDirection = FlexBox::Direction::column;
    //inGainBox.items.add(FlexItem(minKnobWidth, 16, *mInGainLabel).withMargin(0));
    inGainBox.items.add(FlexItem(minKnobWidth, minitemheight, *mInGainSlider).withMargin(0).withFlex(1));

    dryBox.items.clear();
    dryBox.flexDirection = FlexBox::Direction::row;
    //dryBox.items.add(FlexItem(minKnobWidth, 16, *mDryLabel).withMargin(0));
    dryBox.items.add(FlexItem(minKnobWidth, minitemheight, *mDrySlider).withMargin(0).withFlex(4));
    dryBox.items.add(FlexItem(4, 20).withMargin(0).withFlex(0));
    dryBox.items.add(FlexItem(minKnobWidth, minitemheight, *mPanButton).withMargin(0).withFlex(1).withMaxWidth(50));

    
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
    titleBox.items.add(FlexItem(6, 20).withMargin(1).withFlex(1));
    titleBox.items.add(FlexItem(120, 18, *mLocalAddressStaticLabel).withMargin(2));
    titleBox.items.add(FlexItem(160, minitemheight, *mLocalAddressLabel).withMargin(2).withFlex(0));
    //titleBox.items.add(FlexItem(3, 20).withMargin(1).withFlex(0.2));
    //titleBox.items.add(FlexItem(40, 20, *mDummyVisual).withMargin(1).withFlex(4).withMaxWidth(200.0f));
    
    paramsBox.items.clear();
    paramsBox.flexDirection = FlexBox::Direction::column;
    paramsBox.items.add(FlexItem(minKnobWidth, minitemheight, inGainBox).withMargin(2).withFlex(0));
    paramsBox.items.add(FlexItem(6, 2).withMargin(1).withFlex(0));
    paramsBox.items.add(FlexItem(100, meterheight, *inputMeter).withMargin(2).withFlex(0));
    paramsBox.items.add(FlexItem(6, 2).withMargin(1).withFlex(0));
    paramsBox.items.add(FlexItem(minKnobWidth, minitemheight, dryBox).withMargin(2).withFlex(0));
    //paramsBox.items.add(FlexItem(minKnobWidth, 60, wetBox).withMargin(0).withFlex(1));


    addressBox.items.clear();
    addressBox.flexDirection = FlexBox::Direction::row;
    //addressBox.items.add(FlexItem(40, 18, *mRemoteAddressStaticLabel).withMargin(2));
    addressBox.items.add(FlexItem(180, minitemheight, *mAddRemoteHostEditor).withMargin(2).withFlex(1));
    //addressBox.items.add(FlexItem(55, minitemheight, *mAddRemotePortEditor).withMargin(2).withFlex(1));

    
    remoteBox.items.clear();
    remoteBox.flexDirection = FlexBox::Direction::column;
    remoteBox.items.add(FlexItem(5, 3).withFlex(0));
    remoteBox.items.add(FlexItem(minButtonWidth, minitemheight, *mDirectConnectButton).withMargin(2).withFlex(0).withMaxWidth(200).withMinWidth(100).withAlignSelf(FlexItem::AlignSelf::center));
    remoteBox.items.add(FlexItem(180, minitemheight, addressBox).withMargin(2).withFlex(0).withMaxWidth(240).withAlignSelf(FlexItem::AlignSelf::center));
    remoteBox.items.add(FlexItem(10, 0).withFlex(1));
    //remoteBox.items.add(FlexItem(60, minitemheight, *mPatchbayButton).withMargin(2).withFlex(0.5).withMaxWidth(120));

    servAddressBox.items.clear();
    servAddressBox.flexDirection = FlexBox::Direction::row;
    servAddressBox.items.add(FlexItem(servLabelWidth, minitemheight, *mServerHostStaticLabel).withMargin(2).withFlex(1));
    servAddressBox.items.add(FlexItem(172, minitemheight, *mServerHostEditor).withMargin(2).withFlex(1));

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
    serverBox.items.add(FlexItem(180, minpassheight, servUserPassBox).withMargin(2).withFlex(0));
    serverBox.items.add(FlexItem(5, 6).withFlex(1));
    serverBox.items.add(FlexItem(minButtonWidth, minitemheight, servButtonBox).withMargin(2).withFlex(0));
    serverBox.items.add(FlexItem(5, 6).withFlex(1));
    serverBox.items.add(FlexItem(100, minitemheight, servAddressBox).withMargin(2).withFlex(0));
    serverBox.items.add(FlexItem(80, minpassheight, *mServerStatusLabel).withMargin(2).withFlex(0));
    serverBox.items.add(FlexItem(5, 8).withFlex(0));
    //remoteBox.items.add(FlexItem(60, minitemheight, *mPatchbayButton).withMargin(2).withFlex(0.5).withMaxWidth(120));


    connectBox.items.clear();
    connectBox.flexDirection = FlexBox::Direction::column;
    connectBox.items.add(FlexItem(minButtonWidth, minitemheight, *mConnectButton).withMargin(2).withFlex(0));
    connectBox.items.add(FlexItem(minButtonWidth, minitemheight/2, *mMainGroupLabel).withMargin(2).withFlex(0));
    connectBox.items.add(FlexItem(minButtonWidth, minitemheight/2, *mMainPeerLabel).withMargin(2).withFlex(0));
    connectBox.items.add(FlexItem(80, minitemheight, *mMainStatusLabel).withMargin(2).withFlex(1));
    
    
    middleBox.items.clear();
    int midboxminheight = (minitemheight*3 + 20);
    int midboxminwidth = 296;
    
    if (isNarrow) {
        middleBox.flexDirection = FlexBox::Direction::column;
        middleBox.items.add(FlexItem(100, minitemheight*3 + 12, paramsBox).withMargin(2).withFlex(1));
        middleBox.items.add(FlexItem(5, 2));
        middleBox.items.add(FlexItem(150, minitemheight*2 + 12, connectBox).withMargin(2).withFlex(0));
        middleBox.items.add(FlexItem(6, 2));

        midboxminheight = (minitemheight*5 + 40);
        midboxminwidth = 190;
    } else {
        middleBox.flexDirection = FlexBox::Direction::row;        
        middleBox.items.add(FlexItem(minKnobWidth + minPannerWidth*2, minitemheight*2 + meterheight + 10, paramsBox).withMargin(2).withFlex(1));
        middleBox.items.add(FlexItem(3, 1).withMaxWidth(6));
        middleBox.items.add(FlexItem(150, minitemheight*2, connectBox).withMargin(2).withFlex(0).withMaxWidth(400));
    }
    
    toolbarBox.items.clear();
    toolbarBox.flexDirection = FlexBox::Direction::row;
    toolbarBox.items.add(FlexItem(100, minitemheight, *mOutGainSlider).withMargin(2).withFlex(1));
    toolbarBox.items.add(FlexItem(100, minitemheight, *outputMeter).withMargin(2).withFlex(1));
    toolbarBox.items.add(FlexItem(6, 6).withMargin(1).withFlex(0.2));
    toolbarBox.items.add(FlexItem(60, minitemheight, *mPatchbayButton).withMargin(2).withFlex(0.5).withMaxWidth(120));
    toolbarBox.items.add(FlexItem(12, 6).withMargin(0).withFlex(0));
    

    //middleBox.items.clear();
    //middleBox.flexDirection = FlexBox::Direction::row;
    //middleBox.items.add(FlexItem(minButtonWidth*2, minitemheight, remoteSourceBox).withMargin(2).withFlex(1));
    //middleBox.items.add(FlexItem(10, 0).withFlex(0.5).withMaxWidth(60));
    //middleBox.items.add(FlexItem(minButtonWidth*2, minitemheight, remoteSinkBox).withMargin(2).withFlex(1));

    
    int minheight = 0;
    
    mainBox.items.clear();
    mainBox.flexDirection = FlexBox::Direction::column;    
    mainBox.items.add(FlexItem(120, 20, titleBox).withMargin(3).withFlex(1).withMaxHeight(30)); minheight += 22;
    mainBox.items.add(FlexItem(midboxminwidth, midboxminheight, middleBox).withMargin(3).withFlex(0)); minheight += midboxminheight;
    //mainBox.items.add(FlexItem(10, 0).withFlex(0.5).withMaxHeight(2.0));
    mainBox.items.add(FlexItem(120, 50, *mPeerViewport).withMargin(3).withFlex(3)); minheight += 50 + 6;
    //mainBox.items.add(FlexItem(120, 50, *mPeerContainer).withMargin(3).withFlex(3)); minheight += 50;
    //mainBox.items.add(FlexItem(10, 0).withFlex(0.5).withMaxHeight(2.0));
    mainBox.items.add(FlexItem(100, minitemheight, toolbarBox).withMargin(3).withFlex(0)); minheight += minitemheight + 4;
    mainBox.items.add(FlexItem(10, 8).withFlex(0));
        
    
    //if (getHeight() < minheight) {
    //    setSize(getWidth(), minheight);
    //}
}


