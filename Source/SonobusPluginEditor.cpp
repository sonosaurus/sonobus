

#include "SonobusPluginProcessor.h"
#include "SonobusPluginEditor.h"

#include "BeatToggleGrid.h"
#include "DebugLogC.h"

#include "PeersContainerView.h"
#include "WaveformTransportComponent.h"
#include "RandomSentenceGenerator.h"
#include "SonoUtility.h"
#include "SonobusTypes.h"

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

void SonobusAudioProcessorEditor::configEditor(TextEditor *editor, bool passwd)
{
    editor->addListener(this);
    if (passwd)  {
        editor->setIndents(8, 6);        
    } else {
        editor->setIndents(8, 8);
    }
}


//==============================================================================
SonobusAudioProcessorEditor::SonobusAudioProcessorEditor (SonobusAudioProcessor& p)
    : AudioProcessorEditor (&p), processor (p),  sonoSliderLNF(14), smallLNF(14), teensyLNF(11),
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
        currConnectionInfo.groupName = "";
#if JUCE_IOS
        String username = SystemStats::getFullUserName(); //SystemStats::getLogonName();
        //String username = SystemStats::getComputerName(); //SystemStats::getLogonName();
#else
        String username = SystemStats::getFullUserName(); //SystemStats::getLogonName();    
        //if (username.length() > 0) username = username.replaceSection(0, 1, username.substring(0, 1).toUpperCase());
#endif

        if (username.isEmpty()) {
            username = SystemStats::getComputerName();
        }
        
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
    mMainUserLabel->setJustificationType(Justification::centredLeft);
    mMainUserLabel->setFont(14);

    mMainPeerLabel = std::make_unique<Label>("peers", "");
    mMainPeerLabel->setJustificationType(Justification::centred);
    mMainPeerLabel->setFont(18);

    mMainMessageLabel = std::make_unique<Label>("mesg", "");
    mMainMessageLabel->setJustificationType(Justification::centred);
    mMainMessageLabel->setFont(14);

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
    mInGainSlider->setTextBoxIsEditable(true);
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
    mPanButton->setButtonText(TRANS("In Pan"));
    mPanButton->setLookAndFeel(&smallLNF);
    mPanButton->addListener(this);
    
    
    
    
    mMasterMuteButton = std::make_unique<SonoDrawableButton>("sendmute", DrawableButton::ButtonStyle::ImageFitted);
    std::unique_ptr<Drawable> sendallowimg(Drawable::createFromImageData(BinaryData::mic_svg, BinaryData::mic_svgSize));
    std::unique_ptr<Drawable> senddisallowimg(Drawable::createFromImageData(BinaryData::mic_disabled_svg, BinaryData::mic_disabled_svgSize));
    mMasterMuteButton->setImages(sendallowimg.get(), nullptr, nullptr, nullptr, senddisallowimg.get());
    mMasterMuteButton->addListener(this);
    mMasterMuteButton->setClickingTogglesState(true);
    //pvf->sendMutedButton->setColour(TextButton::buttonOnColourId, Colour::fromFloatRGBA(0.15, 0.15, 0.15, 0.7));
    mMasterMuteButton->setColour(TextButton::buttonOnColourId, Colour::fromFloatRGBA(0.2, 0.2, 0.2, 0.7));
    mMasterMuteButton->setColour(TextButton::buttonColourId, Colours::transparentBlack);
    mMasterMuteButton->setColour(SonoTextButton::outlineColourId, Colours::transparentBlack);
    mMasterMuteButton->setColour(DrawableButton::backgroundOnColourId, Colour::fromFloatRGBA(0.2, 0.2, 0.2, 0.7));
    mMasterMuteButton->setColour(DrawableButton::backgroundColourId, Colours::transparentBlack);
    mMasterMuteButton->setTooltip(TRANS("Mutes/Unmutes your Input, none of your audio will be sent to users when muted"));

    mMasterRecvMuteButton = std::make_unique<SonoDrawableButton>("sendmute", DrawableButton::ButtonStyle::ImageFitted);
    std::unique_ptr<Drawable> recvallowimg(Drawable::createFromImageData(BinaryData::speaker_svg, BinaryData::speaker_svgSize));
    std::unique_ptr<Drawable> recvdisallowimg(Drawable::createFromImageData(BinaryData::speaker_disabled_svg, BinaryData::speaker_disabled_svgSize));
    mMasterRecvMuteButton->setImages(recvallowimg.get(), nullptr, nullptr, nullptr, recvdisallowimg.get());
    mMasterRecvMuteButton->addListener(this);
    mMasterRecvMuteButton->setClickingTogglesState(true);
    mMasterRecvMuteButton->setColour(TextButton::buttonOnColourId, Colour::fromFloatRGBA(0.2, 0.2, 0.2, 0.7));
    mMasterRecvMuteButton->setColour(TextButton::buttonColourId, Colours::transparentBlack);
    mMasterRecvMuteButton->setColour(SonoTextButton::outlineColourId, Colours::transparentBlack);
    mMasterRecvMuteButton->setColour(DrawableButton::backgroundOnColourId, Colour::fromFloatRGBA(0.2, 0.2, 0.2, 0.7));
    mMasterRecvMuteButton->setColour(DrawableButton::backgroundColourId, Colours::transparentBlack);
    mMasterRecvMuteButton->setTooltip(TRANS("Mutes/Unmutes all users, no audio data will be received when users are muted"));

    
    mMetContainer = std::make_unique<Component>();

    mMetButtonBg = std::make_unique<DrawableRectangle>();
    mMetButtonBg->setCornerSize(Point<float>(8,8));
    mMetButtonBg->setFill (Colour::fromFloatRGBA(0.0, 0.0, 0.0, 1.0));
    mMetButtonBg->setStrokeFill (Colour::fromFloatRGBA(0.5, 0.5, 0.5, 0.25));
    mMetButtonBg->setStrokeThickness(0.5);

    mDragDropBg = std::make_unique<DrawableRectangle>();
    //mDragDropBg->setCornerSize(Point<float>(8,8));
    mDragDropBg->setFill (Colour::fromFloatRGBA(0.5, 0.5, 0.5, 0.2));
    //mDragDropBg->setStrokeFill (Colour::fromFloatRGBA(0.5, 0.5, 0.5, 0.25));
    //mDragDropBg->setStrokeThickness(0.5);
    
    
    mMetEnableButton = std::make_unique<SonoDrawableButton>("sendmute", DrawableButton::ButtonStyle::ImageFitted);
    std::unique_ptr<Drawable> metimg(Drawable::createFromImageData(BinaryData::met_svg, BinaryData::met_svgSize));
    //std::unique_ptr<Drawable> senddisallowimg(Drawable::createFromImageData(BinaryData::mic_disabled_svg, BinaryData::mic_disabled_svgSize));
    mMetEnableButton->setImages(metimg.get(), nullptr, nullptr, nullptr, nullptr);
    mMetEnableButton->addListener(this);
    mMetEnableButton->setClickingTogglesState(true);
    //pvf->sendMutedButton->setColour(TextButton::buttonOnColourId, Colour::fromFloatRGBA(0.15, 0.15, 0.15, 0.7));
    mMetEnableButton->setColour(TextButton::buttonOnColourId, Colour::fromFloatRGBA(0.2, 0.2, 0.2, 0.7));
    mMetEnableButton->setColour(TextButton::buttonColourId, Colours::transparentBlack);
    mMetEnableButton->setTooltip(TRANS("Metronome On/Off"));
    
    mMetConfigButton = std::make_unique<SonoDrawableButton>("metconf", DrawableButton::ButtonStyle::ImageFitted);
    std::unique_ptr<Drawable> metcfgimg(Drawable::createFromImageData(BinaryData::dots_svg, BinaryData::dots_svgSize));
    mMetConfigButton->setImages(metcfgimg.get(), nullptr, nullptr, nullptr, nullptr);
    mMetConfigButton->addListener(this);
    //mMetConfigButton->setColour(TextButton::buttonOnColourId, Colour::fromFloatRGBA(0.2, 0.2, 0.2, 0.7));
    mMetConfigButton->setColour(TextButton::buttonOnColourId, Colours::transparentBlack);
    mMetConfigButton->setColour(TextButton::buttonColourId, Colours::transparentBlack);
    mMetConfigButton->setTooltip(TRANS("Metronome Options"));
    
    mMetTempoSlider     = std::make_unique<Slider>(Slider::RotaryHorizontalVerticalDrag,  Slider::TextBoxAbove);
    mMetTempoSlider->setName("mettempo");
    mMetTempoSlider->setSliderSnapsToMousePosition(false);
    mMetTempoSlider->setScrollWheelEnabled(false);
    configKnobSlider(mMetTempoSlider.get());
    //mMetTempoSlider->setTextBoxStyle(Slider::NoTextBox, true, 60, 14);
    mMetTempoSlider->setTextBoxStyle(Slider::TextBoxAbove, true, 80, 18);
    mMetTempoSlider->setTextBoxIsEditable(true);
    //mMetTempoSlider->setLookAndFeel(&teensyLNF);

    
    mMetLevelSlider     = std::make_unique<Slider>(Slider::RotaryHorizontalVerticalDrag,  Slider::NoTextBox);
    mMetLevelSlider->setName("metvol");
    mMetLevelSlider->setSliderSnapsToMousePosition(false);
    mMetLevelSlider->setScrollWheelEnabled(false);
    configKnobSlider(mMetLevelSlider.get());
    mMetLevelSlider->setTextBoxStyle(Slider::TextBoxAbove, true, 80, 18);
    mMetLevelSlider->setTextBoxIsEditable(true);
    //mMetLevelSlider->setLookAndFeel(&teensyLNF);

    mMetLevelSliderLabel = std::make_unique<Label>(SonobusAudioProcessor::paramDry, TRANS("Level"));
    configLabel(mMetLevelSliderLabel.get(), false);
    mMetLevelSliderLabel->setJustificationType(Justification::centred);

    mMetTempoSliderLabel = std::make_unique<Label>(SonobusAudioProcessor::paramDry, TRANS("Tempo"));
    configLabel(mMetTempoSliderLabel.get(), false);
    mMetTempoSliderLabel->setJustificationType(Justification::centred);

    mMetSendButton = std::make_unique<SonoDrawableButton>("sendmute", DrawableButton::ButtonStyle::ImageOnButtonBackground);
    std::unique_ptr<Drawable> metsendimg(Drawable::createFromImageData(BinaryData::send_group_svg, BinaryData::send_group_svgSize));
    //std::unique_ptr<Drawable> senddisallowimg(Drawable::createFromImageData(BinaryData::mic_disabled_svg, BinaryData::mic_disabled_svgSize));
    mMetSendButton->setImages(metsendimg.get(), nullptr, nullptr, nullptr, nullptr);
    mMetSendButton->addListener(this);
    mMetSendButton->setClickingTogglesState(true);
    //pvf->sendMutedButton->setColour(TextButton::buttonOnColourId, Colour::fromFloatRGBA(0.15, 0.15, 0.15, 0.7));
    //mMetSendButton->setColour(TextButton::buttonOnColourId, Colour::fromFloatRGBA(0.2, 0.2, 0.2, 0.7));
    mMetSendButton->setColour(TextButton::buttonOnColourId, Colour::fromFloatRGBA(0.2, 0.5, 0.7, 0.65));
    mMetSendButton->setColour(TextButton::buttonColourId, Colours::transparentBlack);
    mMetSendButton->setTooltip(TRANS("Send Metronome to All"));

    
    //mDrySlider     = std::make_unique<Slider>(Slider::LinearBar,  Slider::TextBoxBelow);
    mDrySlider     = std::make_unique<Slider>(Slider::RotaryHorizontalVerticalDrag,  Slider::TextBoxAbove);
    mDrySlider->setName("dry");
    mDrySlider->setSliderSnapsToMousePosition(false);
    mDrySlider->setScrollWheelEnabled(false);

    mOutGainSlider     = std::make_unique<Slider>(Slider::RotaryHorizontalVerticalDrag,  Slider::TextBoxRight);
    mOutGainSlider->setName("wet");
    mOutGainSlider->setSliderSnapsToMousePosition(false);
    mOutGainSlider->setScrollWheelEnabled(false);

    configKnobSlider(mInGainSlider.get());
    configKnobSlider(mDrySlider.get());
    configKnobSlider(mOutGainSlider.get());

    mOutGainSlider->setTextBoxIsEditable(true);
    mDrySlider->setTextBoxIsEditable(true);
    mInGainSlider->setTextBoxIsEditable(true);

    //configKnobSlider(mWetSlider.get());
    
    mBufferTimeSlider    = std::make_unique<Slider>(Slider::LinearBar,  Slider::TextBoxRight);
    mBufferTimeSlider->setName("time");
    mBufferTimeSlider->setSliderSnapsToMousePosition(false);
    mBufferTimeSlider->setChangeNotificationOnlyOnRelease(true);
    mBufferTimeSlider->setDoubleClickReturnValue(true, 15.0);
    mBufferTimeSlider->setTextBoxIsEditable(false);
    mBufferTimeSlider->setScrollWheelEnabled(false);

    mInGainLabel = std::make_unique<Label>(SonobusAudioProcessor::paramDry, TRANS("In Level"));
    configLabel(mInGainLabel.get(), false);
    mInGainLabel->setJustificationType(Justification::centred);
    
    mDryLabel = std::make_unique<Label>(SonobusAudioProcessor::paramDry, TRANS("In Monitor"));
    configLabel(mDryLabel.get(), false);
    mDryLabel->setJustificationType(Justification::centred);

    //mWetLabel = std::make_unique<Label>(SonobusAudioProcessor::paramWet, TRANS("Wet"));
    //configLabel(mWetLabel.get(), false);

    mOutGainLabel = std::make_unique<Label>("outgain", TRANS("Master Out"));
    configLabel(mOutGainLabel.get(), false);
    mOutGainLabel->setJustificationType(Justification::centred);

    
    int largeEditorFontsize = 16;
    int smallerEditorFontsize = 14;

#if JUCE_IOS
    largeEditorFontsize = 18;
    smallerEditorFontsize = 16;
#endif
    
    mInGainAttachment     = std::make_unique<AudioProcessorValueTreeState::SliderAttachment> (p.getValueTreeState(), SonobusAudioProcessor::paramInGain, *mInGainSlider);
    mDryAttachment     = std::make_unique<AudioProcessorValueTreeState::SliderAttachment> (p.getValueTreeState(), SonobusAudioProcessor::paramDry, *mDrySlider);
    mWetAttachment     = std::make_unique<AudioProcessorValueTreeState::SliderAttachment> (p.getValueTreeState(), SonobusAudioProcessor::paramWet, *mOutGainSlider);
    mBufferTimeAttachment     = std::make_unique<AudioProcessorValueTreeState::SliderAttachment> (p.getValueTreeState(), SonobusAudioProcessor::paramDefaultNetbufMs, *mBufferTimeSlider);
    mInMonPan1Attachment     = std::make_unique<AudioProcessorValueTreeState::SliderAttachment> (p.getValueTreeState(), SonobusAudioProcessor::paramInMonitorPan1, *mInMonPanSlider1);
    mInMonPan2Attachment     = std::make_unique<AudioProcessorValueTreeState::SliderAttachment> (p.getValueTreeState(), SonobusAudioProcessor::paramInMonitorPan2, *mInMonPanSlider2);
    mMasterSendMuteAttachment = std::make_unique<AudioProcessorValueTreeState::ButtonAttachment> (p.getValueTreeState(), SonobusAudioProcessor::paramMasterSendMute, *mMasterMuteButton);
    mMasterRecvMuteAttachment = std::make_unique<AudioProcessorValueTreeState::ButtonAttachment> (p.getValueTreeState(), SonobusAudioProcessor::paramMasterRecvMute, *mMasterRecvMuteButton);

    mMetEnableAttachment = std::make_unique<AudioProcessorValueTreeState::ButtonAttachment> (p.getValueTreeState(), SonobusAudioProcessor::paramMetEnabled, *mMetEnableButton);
    mMetLevelAttachment     = std::make_unique<AudioProcessorValueTreeState::SliderAttachment> (p.getValueTreeState(), SonobusAudioProcessor::paramMetGain, *mMetLevelSlider);
    mMetTempoAttachment     = std::make_unique<AudioProcessorValueTreeState::SliderAttachment> (p.getValueTreeState(), SonobusAudioProcessor::paramMetTempo, *mMetTempoSlider);
    mMetSendAttachment = std::make_unique<AudioProcessorValueTreeState::ButtonAttachment> (p.getValueTreeState(), SonobusAudioProcessor::paramSendMetAudio, *mMetSendButton);
 
    
    processor.getValueTreeState().addParameterListener (SonobusAudioProcessor::paramMasterSendMute, this);
    processor.getValueTreeState().addParameterListener (SonobusAudioProcessor::paramMetEnabled, this);
    processor.getValueTreeState().addParameterListener (SonobusAudioProcessor::paramMasterRecvMute, this);
    processor.getValueTreeState().addParameterListener (SonobusAudioProcessor::paramSendMetAudio, this);
    processor.getValueTreeState().addParameterListener (SonobusAudioProcessor::paramSendFileAudio, this);
    processor.getValueTreeState().addParameterListener (SonobusAudioProcessor::paramHearLatencyTest, this);
    processor.getValueTreeState().addParameterListener (SonobusAudioProcessor::paramMetIsRecorded, this);

    
    mConnectTab = std::make_unique<TabbedComponent>(TabbedButtonBar::Orientation::TabsAtTop);
    mConnectTab->setOutline(0);
    mConnectTab->setTabBarDepth(36);
    mConnectTab->getTabbedButtonBar().setMinimumTabScaleFactor(0.1f);
    mConnectTab->getTabbedButtonBar().setColour(TabbedButtonBar::frontTextColourId, Colour::fromFloatRGBA(0.4, 0.8, 1.0, 1.0));
    mConnectTab->getTabbedButtonBar().setColour(TabbedButtonBar::frontOutlineColourId, Colour::fromFloatRGBA(0.4, 0.8, 1.0, 0.5));


    mDirectConnectContainer = std::make_unique<Component>();
    mServerConnectContainer = std::make_unique<Component>();
    mRecentsContainer = std::make_unique<Component>();

    mRecentsGroup = std::make_unique<GroupComponent>("", TRANS("RECENTS"));
    mRecentsGroup->setColour(GroupComponent::textColourId, Colour::fromFloatRGBA(0.8, 0.8, 0.8, 0.8));
    mRecentsGroup->setColour(GroupComponent::outlineColourId, Colour::fromFloatRGBA(0.8, 0.8, 0.8, 0.1));
    mRecentsGroup->setTextLabelPosition(Justification::centred);
    
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
    
    
    mConnectButton = std::make_unique<SonoTextButton>("directconnect");
    mConnectButton->setButtonText(TRANS("Connect..."));
    mConnectButton->addListener(this);
    mConnectButton->setColour(TextButton::buttonColourId, Colour::fromFloatRGBA(0.6, 0.4, 0.6, 0.6));
    mConnectButton->setColour(SonoTextButton::outlineColourId, Colour::fromFloatRGBA(0.6, 0.6, 0.6, 0.4));
    mConnectButton->setTextJustification(Justification::centred);
    
    mDirectConnectButton = std::make_unique<TextButton>("directconnect");
    mDirectConnectButton->setButtonText(TRANS("Direct Connect"));
    mDirectConnectButton->addListener(this);
    mDirectConnectButton->setColour(TextButton::buttonColourId, Colour::fromFloatRGBA(0.1, 0.4, 0.6, 0.6));
    mDirectConnectButton->setWantsKeyboardFocus(true);

    
    mServerConnectButton = std::make_unique<TextButton>("serverconnect");
    mServerConnectButton->setButtonText(TRANS("Connect to Group"));
    mServerConnectButton->addListener(this);
    mServerConnectButton->setColour(TextButton::buttonColourId, Colour::fromFloatRGBA(0.1, 0.4, 0.6, 0.6));
    mServerConnectButton->setWantsKeyboardFocus(true);
    
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
    mServerStatusLabel->setFont(16);
    mServerStatusLabel->setColour(Label::textColourId, Colour(0x99ffaaaa));
    mServerStatusLabel->setMinimumHorizontalScale(0.75);

    mServerInfoLabel = std::make_unique<Label>("servinfo", "");
    mServerInfoLabel->setJustificationType(Justification::centred);
    mServerInfoLabel->setFont(16);
    mServerInfoLabel->setColour(Label::textColourId, Colour(0x99dddddd));
    mServerInfoLabel->setMinimumHorizontalScale(0.85);

    mServerAudioInfoLabel = std::make_unique<Label>("servinfo", TRANS("The server is only used to help users find each other, no audio passes through it. All audio is sent directly between users (peer to peer)."));
    mServerAudioInfoLabel->setJustificationType(Justification::centredTop);
    mServerAudioInfoLabel->setFont(14);
    mServerAudioInfoLabel->setColour(Label::textColourId, Colour(0x99aaaaaa));
    mServerAudioInfoLabel->setMinimumHorizontalScale(0.75);

    mMainStatusLabel = std::make_unique<Label>("servstat", "");
    mMainStatusLabel->setJustificationType(Justification::centredRight);
    mMainStatusLabel->setFont(13);
    mMainStatusLabel->setColour(Label::textColourId, Colour(0x66ffffff));

    mConnectionTimeLabel = std::make_unique<Label>("conntime", "");
    mConnectionTimeLabel->setJustificationType(Justification::centredBottom);
    mConnectionTimeLabel->setFont(13);
    mConnectionTimeLabel->setColour(Label::textColourId, Colour(0x66ffffff));



    mRecentsListBox = std::make_unique<ListBox>("recentslist");
    mRecentsListBox->setColour (ListBox::outlineColourId, Colour::fromFloatRGBA(0.7, 0.7, 0.7, 0.0));
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

    //auto flags = foleys::LevelMeter::Horizontal|foleys::LevelMeter::Minimal; //|foleys::LevelMeter::MaxNumber;
    auto flags = foleys::LevelMeter::Minimal; //|foleys::LevelMeter::MaxNumber;
    
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
    mConnectComponent = std::make_unique<Component>();
    mConnectComponent->setWantsKeyboardFocus(true);
    
    mConnectTitle = std::make_unique<Label>("conntime", TRANS("Connect"));
    mConnectTitle->setJustificationType(Justification::centred);
    mConnectTitle->setFont(Font(20, Font::bold));
    mConnectTitle->setColour(Label::textColourId, Colour(0x66ffffff));

    mConnectComponentBg = std::make_unique<DrawableRectangle>();
    mConnectComponentBg->setFill (Colour::fromFloatRGBA(0.1, 0.1, 0.1, 1.0));
    
    mConnectCloseButton = std::make_unique<SonoDrawableButton>("x", DrawableButton::ButtonStyle::ImageFitted);
    std::unique_ptr<Drawable> ximg(Drawable::createFromImageData(BinaryData::x_icon_svg, BinaryData::x_icon_svgSize));
    mConnectCloseButton->setImages(ximg.get());
    mConnectCloseButton->addListener(this);
    mConnectCloseButton->setColour(DrawableButton::backgroundColourId, Colours::transparentBlack);
    
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

    mOptionsAutosizeStaticLabel = std::make_unique<Label>("", TRANS("Default Safety Buffer"));
    configLabel(mOptionsAutosizeStaticLabel.get(), false);
    mOptionsAutosizeStaticLabel->setJustificationType(Justification::centredLeft);
    
    mOptionsFormatChoiceStaticLabel = std::make_unique<Label>("", TRANS("Default Send Quality:"));
    configLabel(mOptionsFormatChoiceStaticLabel.get(), false);
    mOptionsFormatChoiceStaticLabel->setJustificationType(Justification::centredRight);

    mOptionsHearLatencyButton = std::make_unique<ToggleButton>(TRANS("Make Latency Test Audible"));
    mOptionsHearLatencyButton->addListener(this);
    mHearLatencyTestAttachment = std::make_unique<AudioProcessorValueTreeState::ButtonAttachment> (p.getValueTreeState(), SonobusAudioProcessor::paramHearLatencyTest, *mOptionsHearLatencyButton);

    mOptionsMetRecordedButton = std::make_unique<ToggleButton>(TRANS("Metronome output recorded"));
    mOptionsMetRecordedButton->addListener(this);
    mMetRecordedAttachment = std::make_unique<AudioProcessorValueTreeState::ButtonAttachment> (p.getValueTreeState(), SonobusAudioProcessor::paramMetIsRecorded, *mOptionsMetRecordedButton);

    
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
    
    //if (JUCEApplicationBase::isaoneApp())
    {
        mRecordingButton = std::make_unique<SonoDrawableButton>("record", DrawableButton::ButtonStyle::ImageFitted);
        std::unique_ptr<Drawable> recimg(Drawable::createFromImageData(BinaryData::record_svg, BinaryData::record_svgSize));
        std::unique_ptr<Drawable> recselimg(Drawable::createFromImageData(BinaryData::record_active_alt_svg, BinaryData::record_active_alt_svgSize));
        mRecordingButton->setImages(recimg.get(), nullptr, nullptr, nullptr, recselimg.get());
        //mRecordingButton = std::make_unique<SonoTextButton>("record");
        //mRecordingButton->setButtonText(TRANS("Record"));
        mRecordingButton->addListener(this);
        //mRecordingButton->setColour(DrawableButton::backgroundOnColourId, Colour::fromFloatRGBA(0.8, 0.3, 0.3, 0.5));
        mRecordingButton->setColour(DrawableButton::backgroundOnColourId, Colours::transparentBlack);
        mRecordingButton->setTooltip(TRANS("Start/Stop recording audio to file"));
        //mRecordingButton->setAlpha(0.8f);
        
        mFileRecordingLabel = std::make_unique<Label>("rectime", "");
        mFileRecordingLabel->setJustificationType(Justification::centredBottom);
        mFileRecordingLabel->setFont(12);
        mFileRecordingLabel->setColour(Label::textColourId, Colour(0x88ffbbbb));
        
        mFileBrowseButton = std::make_unique<SonoDrawableButton>("browse", DrawableButton::ButtonStyle::ImageFitted);
        std::unique_ptr<Drawable> folderimg(Drawable::createFromImageData(BinaryData::folder_icon_svg, BinaryData::folder_icon_svgSize));
        mFileBrowseButton->setImages(folderimg.get());
        mFileBrowseButton->addListener(this);
        //mFileBrowseButton->setColour(DrawableButton::backgroundOnColourId, Colour::fromFloatRGBA(0.2, 0.2, 0.2, 0.7));
        mFileBrowseButton->setColour(DrawableButton::backgroundColourId, Colours::transparentBlack);
        mFileBrowseButton->setTooltip(TRANS("Load audio file for playback"));
        //mFileBrowseButton->setAlpha(0.8f);

        mPlayButton = std::make_unique<SonoDrawableButton>("play", DrawableButton::ButtonStyle::ImageFitted);
        std::unique_ptr<Drawable> playimg(Drawable::createFromImageData(BinaryData::play_icon_svg, BinaryData::play_icon_svgSize));
        std::unique_ptr<Drawable> pauseimg(Drawable::createFromImageData(BinaryData::pause_icon_svg, BinaryData::pause_icon_svgSize));
        mPlayButton->setImages(playimg.get(), nullptr, nullptr, nullptr, pauseimg.get());
        mPlayButton->setClickingTogglesState(true);
        mPlayButton->addListener(this);
        mPlayButton->setColour(DrawableButton::backgroundOnColourId, Colours::transparentBlack);
        // mPlayButton->setColour(DrawableButton::backgroundOnColourId, Colour::fromFloatRGBA(0.2, 0.5, 0.7, 0.5));
        mPlayButton->setColour(DrawableButton::backgroundColourId, Colours::transparentBlack);

        mLoopButton = std::make_unique<SonoDrawableButton>("play", DrawableButton::ButtonStyle::ImageFitted);
        std::unique_ptr<Drawable> loopimg(Drawable::createFromImageData(BinaryData::loop_icon_svg, BinaryData::loop_icon_svgSize));
        //std::unique_ptr<Drawable> loopselimg(Drawable::createFromImageData(BinaryData::loop_button_selected_png, BinaryData::loop_button_selected_pngSize));
        mLoopButton->setImages(loopimg.get(), nullptr, nullptr, nullptr, nullptr);
        mLoopButton->setClickingTogglesState(true);
        mLoopButton->addListener(this);
        mLoopButton->setColour(DrawableButton::backgroundOnColourId, Colour::fromFloatRGBA(0.6, 0.3, 0.6, 0.5));
        //mLoopButton->setColour(DrawableButton::backgroundOnColourId, Colours::transparentBlack);
        mLoopButton->setColour(DrawableButton::backgroundColourId, Colours::transparentBlack);

        mDismissTransportButton = std::make_unique<SonoDrawableButton>("x", DrawableButton::ButtonStyle::ImageFitted);
        std::unique_ptr<Drawable> ximg(Drawable::createFromImageData(BinaryData::x_icon_svg, BinaryData::x_icon_svgSize));
        mDismissTransportButton->setImages(ximg.get());
        mDismissTransportButton->addListener(this);
        //mDismissTransportButton->setColour(TextButton::buttonOnColourId, Colour::fromFloatRGBA(0.2, 0.4, 0.2, 0.7));
        mDismissTransportButton->setColour(DrawableButton::backgroundColourId, Colours::transparentBlack);
        //mDismissTransportButton->setAlpha(0.8f);

        mWaveformThumbnail.reset (new WaveformTransportComponent (processor.getFormatManager(), processor.getTransportSource(), commandManager));
        mWaveformThumbnail->addChangeListener (this);
        mWaveformThumbnail->setFollowsTransport(false);
        
        mPlaybackSlider     = std::make_unique<Slider>(Slider::RotaryHorizontalVerticalDrag,  Slider::TextBoxRight);
        mPlaybackSlider->setRange(0.0, 2.0, 0.0);
        mPlaybackSlider->setSkewFactor(0.25);
        mPlaybackSlider->setName("plevel");
        mPlaybackSlider->setSliderSnapsToMousePosition(false);
        mPlaybackSlider->setDoubleClickReturnValue(true, 1.0);
        mPlaybackSlider->setTextBoxIsEditable(false);
        mPlaybackSlider->setScrollWheelEnabled(false);
        configKnobSlider(mPlaybackSlider.get());
        mPlaybackSlider->setMouseDragSensitivity(80);
        mPlaybackSlider->setTextBoxStyle(Slider::NoTextBox, true, 60, 14);
        mPlaybackSlider->setPopupDisplayEnabled(true, true, this);
        mPlaybackSlider->valueFromTextFunction = [](const String& s) -> float { return Decibels::decibelsToGain(s.getFloatValue()); };
        mPlaybackSlider->textFromValueFunction = [](float v) -> String { return Decibels::toString(Decibels::gainToDecibels(v), 1); };
        mPlaybackSlider->onValueChange = [this] { processor.getTransportSource().setGain(mPlaybackSlider->getValue()); };
        
        mFileSendAudioButton = std::make_unique<SonoDrawableButton>("sendmute", DrawableButton::ButtonStyle::ImageFitted);
        std::unique_ptr<Drawable> filesendimg(Drawable::createFromImageData(BinaryData::send_group_small_svg, BinaryData::send_group_small_svgSize));
        //std::unique_ptr<Drawable> senddisallowimg(Drawable::createFromImageData(BinaryData::mic_disabled_svg, BinaryData::mic_disabled_svgSize));
        mFileSendAudioButton->setImages(filesendimg.get(), nullptr, nullptr, nullptr, nullptr);
        mFileSendAudioButton->addListener(this);
        mFileSendAudioButton->setClickingTogglesState(true);
        //pvf->sendMutedButton->setColour(TextButton::buttonOnColourId, Colour::fromFloatRGBA(0.15, 0.15, 0.15, 0.7));
        //mMetSendButton->setColour(TextButton::buttonOnColourId, Colour::fromFloatRGBA(0.2, 0.2, 0.2, 0.7));
        mFileSendAudioButton->setColour(DrawableButton::backgroundOnColourId, Colour::fromFloatRGBA(0.2, 0.5, 0.7, 0.65));
        mFileSendAudioButton->setColour(TextButton::buttonColourId, Colours::transparentBlack);
        mFileSendAudioButton->setTooltip(TRANS("Send File Playback to All"));

        mFileSendAttachment = std::make_unique<AudioProcessorValueTreeState::ButtonAttachment> (p.getValueTreeState(), SonobusAudioProcessor::paramSendFileAudio, *mFileSendAudioButton);

    }
    
#if JUCE_IOS
    mPeerViewport->setScrollOnDragEnabled(true);
#endif
    
    mOptionsComponent->addAndMakeVisible(mBufferTimeSlider.get());
    mOptionsComponent->addAndMakeVisible(mOptionsAutosizeDefaultChoice.get());
    mOptionsComponent->addAndMakeVisible(mOptionsFormatChoiceDefaultChoice.get());
    mOptionsComponent->addAndMakeVisible(mOptionsAutosizeStaticLabel.get());
    mOptionsComponent->addAndMakeVisible(mOptionsFormatChoiceStaticLabel.get());
    mOptionsComponent->addAndMakeVisible(mOptionsHearLatencyButton.get());
    mOptionsComponent->addAndMakeVisible(mOptionsMetRecordedButton.get());
    
    addAndMakeVisible(mPeerViewport.get());
    addAndMakeVisible(mTitleLabel.get());

    addAndMakeVisible(mMainGroupLabel.get());
    addAndMakeVisible(mMainPeerLabel.get());
    addAndMakeVisible(mMainMessageLabel.get());
    addAndMakeVisible(mMainUserLabel.get());
    addAndMakeVisible(mMainPersonImage.get());
    addAndMakeVisible(mMainGroupImage.get());

    //addAndMakeVisible(mConnectTab.get());    
    addAndMakeVisible(mDrySlider.get());
    addAndMakeVisible(mOutGainSlider.get());
    //addAndMakeVisible(mWetLabel.get());
    addAndMakeVisible(mInGainSlider.get());
    addAndMakeVisible(mMasterMuteButton.get());
    addAndMakeVisible(mMasterRecvMuteButton.get());
    
    addAndMakeVisible(mPatchbayButton.get());
    addAndMakeVisible(mConnectButton.get());
    addAndMakeVisible(mMainStatusLabel.get());
    addAndMakeVisible(mConnectionTimeLabel.get());

    addAndMakeVisible(mMetButtonBg.get());
    addAndMakeVisible(mMetEnableButton.get());
    addAndMakeVisible(mMetConfigButton.get());


    
    mMetContainer->addAndMakeVisible(mMetLevelSlider.get());
    mMetContainer->addAndMakeVisible(mMetTempoSlider.get());
    mMetContainer->addAndMakeVisible(mMetLevelSliderLabel.get());
    mMetContainer->addAndMakeVisible(mMetTempoSliderLabel.get());
    mMetContainer->addAndMakeVisible(mMetSendButton.get());

    addChildComponent(mIAAHostButton.get());


    if (mRecordingButton) {
        addAndMakeVisible(mRecordingButton.get());
        addAndMakeVisible(mFileBrowseButton.get());
        addAndMakeVisible(mFileRecordingLabel.get());
        addAndMakeVisible(mPlayButton.get());
        addAndMakeVisible(mLoopButton.get());
        addAndMakeVisible(mDismissTransportButton.get());
        addAndMakeVisible(mWaveformThumbnail.get());
        addAndMakeVisible(mPlaybackSlider.get());
        addAndMakeVisible(mFileSendAudioButton.get());
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
    mServerConnectContainer->addAndMakeVisible(mServerInfoLabel.get());
    mServerConnectContainer->addAndMakeVisible(mServerAudioInfoLabel.get());

    mRecentsContainer->addAndMakeVisible(mRecentsListBox.get());

    mConnectComponent->addAndMakeVisible(mConnectComponentBg.get());
    mConnectComponent->addAndMakeVisible(mConnectTab.get());
    mConnectComponent->addAndMakeVisible(mConnectTitle.get());
    mConnectComponent->addAndMakeVisible(mConnectCloseButton.get());

    //addAndMakeVisible(mAddRemotePortEditor.get());
    addAndMakeVisible(mInGainLabel.get());
    addAndMakeVisible(mDryLabel.get());
    addAndMakeVisible(mOutGainLabel.get());
    addAndMakeVisible(inputMeter.get());
    addAndMakeVisible(outputMeter.get());
    //addAndMakeVisible(mPeerContainer.get());
    addAndMakeVisible (mSettingsButton.get());
    addAndMakeVisible (mPanButton.get());
    
    addChildComponent(mConnectComponent.get());

    // over everything
    addChildComponent(mDragDropBg.get());

    
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

        menuBarModel = std::make_unique<SonobusMenuBarModel>(*this);

        menuBarModel->setApplicationCommandManagerToWatch(&commandManager);

        
#if JUCE_MAC
        auto extraAppleMenuItems = PopupMenu();
        extraAppleMenuItems.addCommandItem(&commandManager, SonobusCommands::ShowOptions);
        MenuBarModel::setMacMainMenu(menuBarModel.get(), &extraAppleMenuItems);
#endif
      
        
    } else  {
        setResizable(true, true);    
    }
    
    processor.addClientListener(this);
    processor.getTransportSource().addChangeListener (this);

    std::istringstream gramstream(std::string(BinaryData::wordmaker_g, BinaryData::wordmaker_gSize));
    mRandomSentence = std::make_unique<RandomSentenceGenerator>(gramstream);
    mRandomSentence->capEveryWord = true;
    

    commandManager.registerAllCommandsForTarget (this);
    
    // this will use the command manager to initialise the KeyPressMappingSet with
    // the default keypresses that were specified when the targets added their commands
    // to the manager.
    commandManager.getKeyMappings()->resetToDefaultMappings();
    
    // having set up the default key-mappings, you might now want to load the last set
    // of mappings that the user configured.
    //myCommandManager->getKeyMappings()->restoreFromXml (lastSavedKeyMappingsXML);
    
    // Now tell our top-level window to send any keypresses that arrive to the
    // KeyPressMappingSet, which will use them to invoke the appropriate commands.
    addKeyListener (commandManager.getKeyMappings());
    
        
    commandManager.commandStatusChanged();
    
    setWantsKeyboardFocus(true);
    
    startTimer(PeriodicUpdateTimerId, 1000);
    
   // Make sure that before the constructor has finished, you've set the
   // editor's size to whatever you need it to be.
   setSize (720, 440);
       
}

SonobusAudioProcessorEditor::~SonobusAudioProcessorEditor()
{
    if (menuBarModel) {
        menuBarModel->setApplicationCommandManagerToWatch(nullptr);
#if JUCE_MAC
        MenuBarModel::setMacMainMenu(nullptr);
#endif
    }
    
    processor.removeClientListener(this);
    processor.getTransportSource().removeChangeListener(this);
    
    if (mWaveformThumbnail) {
        mWaveformThumbnail->removeChangeListener (this);
    }

}


bool SonobusAudioProcessorEditor::isInterestedInFileDrag (const StringArray& /*files*/) 
{
    return true;
}

void SonobusAudioProcessorEditor::filesDropped (const StringArray& files, int /*x*/, int /*y*/) 
{
    mDragDropBg->setVisible(false);    

    URL fileDropped = URL (File (files[0]));
    loadAudioFromURL(fileDropped);
}

void  SonobusAudioProcessorEditor::fileDragEnter (const StringArray& files, int x, int y) 
{
    // todo check to see if it's an audio file and highlight something
    mDragDropBg->setVisible(true);
}

void SonobusAudioProcessorEditor::fileDragExit (const StringArray& files)
{
    mDragDropBg->setVisible(false);    
}


void SonobusAudioProcessorEditor::configKnobSlider(Slider * slider)
{
    //slider->setVelocityBasedMode(true);
    //slider->setVelocityModeParameters(2.5, 1, 0.05);
    //slider->setTextBoxStyle(Slider::NoTextBox, true, 40, 18);
    slider->setSliderStyle(Slider::SliderStyle::RotaryHorizontalVerticalDrag);
    slider->setTextBoxStyle(Slider::TextBoxAbove, true, 60, 14);
    slider->setMouseDragSensitivity(128);
    slider->setScrollWheelEnabled(false);
    //slider->setPopupDisplayEnabled(true, false, this);
    slider->setColour(Slider::textBoxBackgroundColourId, Colours::transparentBlack);
    slider->setColour(Slider::textBoxOutlineColourId, Colours::transparentBlack);
    slider->setColour(Slider::textBoxTextColourId, Colour(0x90eeeeee));
    slider->setColour(TooltipWindow::textColourId, Colour(0xf0eeeeee));

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


void SonobusAudioProcessorEditor::updateTransportState()
{
    if (mPlayButton) {
        if (!mCurrentAudioFile.isEmpty()) {

            mPlayButton->setVisible(true);
            mLoopButton->setVisible(true);
            mDismissTransportButton->setVisible(true);
            mWaveformThumbnail->setVisible(true);
            mPlaybackSlider->setVisible(true);
            mFileSendAudioButton->setVisible(true);
        } else {
            mPlayButton->setVisible(false);
            mLoopButton->setVisible(false);
            mDismissTransportButton->setVisible(false);
            mWaveformThumbnail->setVisible(false);
            mPlaybackSlider->setVisible(false);
            mFileSendAudioButton->setVisible(false);
        }

        mPlayButton->setToggleState(processor.getTransportSource().isPlaying(), dontSendNotification);

        mPlaybackSlider->setValue(processor.getTransportSource().getGain(), dontSendNotification);
        
    }
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
        
        if (processor.isRecordingToFile() && mFileRecordingLabel) {
            mFileRecordingLabel->setText(SonoUtility::durationToString(processor.getElapsedRecordTime(), true), dontSendNotification);
        }

        if (processor.isConnectedToServer()) {
            mConnectionTimeLabel->setText(SonoUtility::durationToString(processor.getElapsedConnectedTime(), true), dontSendNotification);
        }

#if 0
        if (JUCEApplicationBase::isStandaloneApp() && getAudioDeviceManager())
        {
            int inlat = getAudioDeviceManager()->getCurrentAudioDevice()->getInputLatencyInSamples();
            int outlat = getAudioDeviceManager()->getCurrentAudioDevice()->getOutputLatencyInSamples();
            int bufsize = getAudioDeviceManager()->getCurrentAudioDevice()->getCurrentBufferSizeSamples();

            DebugLogC("InLat: %d  OutLat: %d  bufsize: %d", inlat, outlat, bufsize);
        }
#endif
        
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

void SonobusAudioProcessorEditor::textEditorReturnKeyPressed (TextEditor& ed)
{
    DebugLogC("Return pressed");
    
    
    if (mConnectComponent->isVisible()) {
        //mServerConnectButton->setWantsKeyboardFocus(true);
        mServerConnectButton->grabKeyboardFocus();
        //mServerConnectButton->setWantsKeyboardFocus(false);
    }
}

void SonobusAudioProcessorEditor::textEditorEscapeKeyPressed (TextEditor& ed)
{
    DebugLogC("escape pressed");
    if (mConnectComponent->isVisible()) {
        //mServerConnectButton->setWantsKeyboardFocus(true);
        mServerConnectButton->grabKeyboardFocus();
        //mServerConnectButton->setWantsKeyboardFocus(false);
    }
    //grabKeyboardFocus();
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
            if (processor.connectRemotePeer(host, port, "", "", !mMasterRecvMuteButton->getToggleState())) {
                showConnectPopup(false);
            }
        }
    }
    else if (buttonThatWasClicked == mConnectButton.get()) {
        
        if (processor.isConnectedToServer()) {
            mConnectionTimeLabel->setText(TRANS("Last Session: ") + SonoUtility::durationToString(processor.getElapsedConnectedTime(), true), dontSendNotification);
            mConnectButton->setTextJustification(Justification::centredTop);

            processor.disconnectFromServer();
            updateState();
        }
        else {
            mConnectionTimeLabel->setText("", dontSendNotification);
            mConnectButton->setTextJustification(Justification::centred);

            if (!mConnectComponent->isVisible()) {
                //mMainPeerLabel->setText("", dontSendNotification);
                showConnectPopup(true);
            } else {
                showConnectPopup(false);
            }
        }
        
    }
    else if (buttonThatWasClicked == mServerConnectButton.get()) {

        if (processor.isConnectedToServer()) {
            mConnectionTimeLabel->setText(TRANS("Total: ") + SonoUtility::durationToString(processor.getElapsedConnectedTime(), true), dontSendNotification);

            processor.disconnectFromServer();    
            updateState();
        }
        else {
            String hostport = mServerHostEditor->getText();        
            //String port = mAddRemotePortEditor->getText();
            // parse it
            StringArray toks = StringArray::fromTokens(hostport, ":", "");
            String host = "aoo.sonobus.net";
            int port = 10998;
            
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
            
            mConnectionTimeLabel->setText("", dontSendNotification);

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
    else if (buttonThatWasClicked == mMetConfigButton.get()) {
        if (!metCalloutBox) {
            showMetConfig(true);
        } else {
            showMetConfig(false);
        }        
    }
    else if (buttonThatWasClicked == mMasterMuteButton.get()) {
        // allow or disallow sending to all peers, handled by button attachment

        if (mMasterMuteButton->getToggleState()) {
            showPopTip(TRANS("Not sending your audio anywhere"), 3000, mMasterMuteButton.get());
        } else {
            showPopTip(TRANS("Sending your audio to others"), 3000, mMasterMuteButton.get());
        }
    }
    else if (buttonThatWasClicked == mMasterRecvMuteButton.get()) {
        // allow or disallow sending to all peers, handled by button attachment
        
        if (mMasterRecvMuteButton->getToggleState()) {
            showPopTip(TRANS("Muted everyone"), 3000, mMasterRecvMuteButton.get());
        } else {
            showPopTip(TRANS("Unmuted all who were not muted previously"), 3000, mMasterRecvMuteButton.get());
        }
    }
    else if (buttonThatWasClicked == mMetSendButton.get()) {
        // handled by button attachment
        if (mMetSendButton->getToggleState()) {
            showPopTip(TRANS("Sending your metronome to all users"), 3000, mMetSendButton.get());
        } else {
            showPopTip(TRANS("Now only you will hear your metronome"), 3000, mMetSendButton.get());
        }
    }
    else if (buttonThatWasClicked == mFileSendAudioButton.get()) {
        // handled by button attachment
        if (mFileSendAudioButton->getToggleState()) {
            showPopTip(TRANS("Sending file playback to all users"), 3000, mFileSendAudioButton.get());
        } else {
            showPopTip(TRANS("Now only you will hear the file playback"), 3000, mFileSendAudioButton.get());
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
            //showPopTip(TRANS("Finished recording to ") + filepath, 4000, mRecordingButton.get(), 130);
#else
            filepath = lastRecordedFile.getRelativePathFrom(File::getSpecialLocation (File::userHomeDirectory));
#endif

            mRecordingButton->setTooltip(TRANS("Last recorded file: ") + filepath);

            //mFileRecordingLabel->setText("Total: " + SonoUtility::durationToString(processor.getElapsedRecordTime(), true), dontSendNotification);
            mFileRecordingLabel->setText("", dontSendNotification);

            // load up recording
            loadAudioFromURL(URL(lastRecordedFile));
            updateLayout();
            resized();
            
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
            
            mFileRecordingLabel->setText("", dontSendNotification);

        }
    }
    else if (buttonThatWasClicked == mFileBrowseButton.get()) {
        if (mFileChooser.get() == nullptr) {

            SafePointer<SonobusAudioProcessorEditor> safeThis (this);
            
            if (! RuntimePermissions::isGranted (RuntimePermissions::readExternalStorage))
            {
                RuntimePermissions::request (RuntimePermissions::readExternalStorage,
                                             [safeThis] (bool granted) mutable
                                             {
                    if (granted)
                        safeThis->buttonClicked (safeThis->mFileBrowseButton.get());
                });
                return;
            }

            if (FileChooser::isPlatformDialogAvailable())
            {
#if !(JUCE_IOS)
                if (mCurrOpenDir.getFullPathName().isEmpty()) {
                    mCurrOpenDir = File::getSpecialLocation (File::userDocumentsDirectory).getChildFile("SonoBus");
                }
#endif
                
                mFileChooser.reset(new FileChooser(TRANS("Choose an audio file to open..."),
                                                   mCurrOpenDir,
#if (JUCE_IOS || JUCE_MAC)
                                                   "*.wav;*.flac;*.aif;*.ogg;*.mp3;*.m4a;*.caf",
#else
                                                   "*.wav;*.flac;*.aif;*.ogg;*.mp3",
#endif
                                                   true, false, getTopLevelComponent()));
            
            
                
                mFileChooser->launchAsync (FileBrowserComponent::openMode | FileBrowserComponent::canSelectFiles,
                                           [safeThis] (const FileChooser& chooser) mutable
                                           {
                    auto results = chooser.getURLResults();
                    if (safeThis != nullptr && results.size() > 0)
                    {
                        auto url = results.getReference (0);

                        DebugLogC("Attempting to load from: %s", url.toString(false).toRawUTF8());
                        
                        if (url.isLocalFile()) {
                            safeThis->mCurrOpenDir = url.getLocalFile().getParentDirectory();
                        }

                        safeThis->loadAudioFromURL(std::move(url));
                        
                    }
                    
                    if (safeThis) {
                        safeThis->mFileChooser.reset();
                    }
                    
                    safeThis->mDragDropBg->setVisible(false);    
                    
                }, nullptr);
               
            }
            else {
                DebugLogC("Need to enable code signing");
            }
        }
    }
    else if (buttonThatWasClicked == mDismissTransportButton.get()) {
        processor.getTransportSource().stop();
        loadAudioFromURL(URL());
        updateLayout();
        resized();
    }
    else if (buttonThatWasClicked == mPlayButton.get()) {
        if (mPlayButton->getToggleState()) {
            //processor.getTransportSource().setLooping(mLoopButton->getToggleState());
            //int64 lstart, llength;
            //processor.getTransportSource().getLoopRange(lstart, llength);
            //if (llength == 0) {
            //    processor.getTransportSource().setLoopRange(0, processor.getTransportSource().getTotalLength());
            //}
            processor.getTransportSource().start();
        } else {
            processor.getTransportSource().stop();
        }
        
        commandManager.commandStatusChanged();
    }
    else if (buttonThatWasClicked == mLoopButton.get()) {

        if (mLoopButton->getToggleState()) {
            int64 lstart, llength;
            processor.getTransportSource().getLoopRange(lstart, llength);
            if (llength == 0 || llength == processor.getTransportSource().getTotalLength()) {
                processor.getTransportSource().setLoopRange(0, processor.getTransportSource().getTotalLength());
                mWaveformThumbnail->updateSelectionFromLoop();
            } else {
                mWaveformThumbnail->setLoopFromSelection();
            }
        }

        processor.getTransportSource().setLooping(mLoopButton->getToggleState());

        mWaveformThumbnail->updateState();
        commandManager.commandStatusChanged();
    }
    else if (buttonThatWasClicked == mConnectCloseButton.get()) {
        mConnectComponent->setVisible(false);
    }
    else {
        
       
    }
}

bool SonobusAudioProcessorEditor::loadAudioFromURL(URL fileurl)
{
    bool ret = false;
    
    if (processor.loadURLIntoTransport (fileurl)) {
        mCurrentAudioFile = std::move (fileurl);
        processor.getTransportSource().setLooping(mLoopButton->getToggleState());
        updateLayout();
        resized();
    }
    else {
        mCurrentAudioFile = std::move(fileurl);
    }

    updateTransportState();

    //zoomSlider.setValue (0, dontSendNotification);
    
    mWaveformThumbnail->setURL (mCurrentAudioFile);
    commandManager.commandStatusChanged();

    return ret;
}


void SonobusAudioProcessorEditor::connectWithInfo(const AooServerConnectionInfo & info)
{
    currConnectionInfo = info;
    
    if (currConnectionInfo.groupName.isEmpty()) {
        mServerStatusLabel->setText(TRANS("You need to specify a group name!"), dontSendNotification);
        mServerGroupEditor->setColour(TextEditor::backgroundColourId, Colour(0xff880000));
        mServerGroupEditor->repaint();
        mServerInfoLabel->setVisible(false);
        mServerStatusLabel->setVisible(true);
        return;
    }
    else {
        mServerGroupEditor->setColour(TextEditor::backgroundColourId, Colour(0xff050505));
        mServerGroupEditor->repaint();
    }
    
    if (currConnectionInfo.userName.isEmpty()) {
        mServerStatusLabel->setText(TRANS("You need to specify a user name!"), dontSendNotification);
        mServerUsernameEditor->setColour(TextEditor::backgroundColourId, Colour(0xff880000));
        mServerUsernameEditor->repaint();
        mServerInfoLabel->setVisible(false);
        mServerStatusLabel->setVisible(true);
        return;
    }
    else {
        mServerUsernameEditor->setColour(TextEditor::backgroundColourId, Colour(0xff050505));
        mServerUsernameEditor->repaint();
    }
    
    if (currConnectionInfo.serverHost.isNotEmpty() && currConnectionInfo.serverPort != 0) 
    {
        processor.connectToServer(currConnectionInfo.serverHost, currConnectionInfo.serverPort, currConnectionInfo.userName, currConnectionInfo.userPassword);
        updateState();

        mServerHostEditor->setColour(TextEditor::backgroundColourId, Colour(0xff050505));
        mServerHostEditor->repaint();
    }
    else {
        mServerStatusLabel->setText(TRANS("Server address is invalid!"), dontSendNotification);
        mServerHostEditor->setColour(TextEditor::backgroundColourId, Colour(0xff880000));
        mServerHostEditor->repaint();
        mServerInfoLabel->setVisible(false);
        mServerStatusLabel->setVisible(true);
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

void SonobusAudioProcessorEditor::showMetConfig(bool flag)
{
    
    if (flag && metCalloutBox == nullptr) {
        
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
        
#if JUCE_IOS
        const int defWidth = 230; 
        const int defHeight = 96;
#else
        const int defWidth = 210; 
        const int defHeight = 86;
#endif
        
        
        wrap->setSize(jmin(defWidth, dw->getWidth() - 20), jmin(defHeight, dw->getHeight() - 24));
        
        //Rectangle<int> setbounds = Rectangle<int>(5, mTitleImage->getBottom() + 2, std::min(100, getLocalBounds().getWidth() - 10), 80);
        
        mMetContainer->setBounds(Rectangle<int>(0,0,defWidth,defHeight));
        
        //wrap->addAndMakeVisible(mSettingsPanel.get());
        wrap->setViewedComponent(mMetContainer.get(), false);
        mMetContainer->setVisible(true);
        
        metBox.performLayout(mMetContainer->getLocalBounds());
        
        
        Rectangle<int> bounds =  dw->getLocalArea(nullptr, mMetConfigButton->getScreenBounds());
        DebugLogC("callout bounds: %s", bounds.toString().toRawUTF8());
        inPannerCalloutBox = & CallOutBox::launchAsynchronously (wrap, bounds , dw);
        if (CallOutBox * box = dynamic_cast<CallOutBox*>(metCalloutBox.get())) {
            box->setDismissalMouseClicksAreAlwaysConsumed(true);
        }
    }
    else {
        // dismiss it
        if (CallOutBox * box = dynamic_cast<CallOutBox*>(metCalloutBox.get())) {
            box->dismiss();
            metCalloutBox = nullptr;
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
    
#if 1
    
    if (flag) {
        mConnectComponent->toFront(true);
        
        mServerInfoLabel->setText(TRANS("All who join the same Group will be able to connect with each other."), dontSendNotification);
        mServerInfoLabel->setVisible(true);
        mServerStatusLabel->setVisible(false);

        if (currConnectionInfo.serverPort == 10998) {
            mServerHostEditor->setText( currConnectionInfo.serverHost);
        } else {
            mServerHostEditor->setText( String::formatted("%s:%d", currConnectionInfo.serverHost.toRawUTF8(), currConnectionInfo.serverPort));
        }
        mServerUsernameEditor->setText(currConnectionInfo.userName);
        mServerGroupEditor->setText(currConnectionInfo.groupName);
        mServerGroupPasswordEditor->setText(currConnectionInfo.groupPassword);

        recentsListModel.updateState();
        mRecentsListBox->updateContent();
        mRecentsListBox->deselectAllRows();

        
        if (firstTimeConnectShow) {
            if (mConnectTab->getNumTabs() > 2) {
                if (recentsListModel.getNumRows() > 0) {
                    // show recents tab first
                    mConnectTab->setCurrentTabIndex(0);
                } else {
                    mConnectTab->setCurrentTabIndex(1);
                }
            }
            firstTimeConnectShow = false;
        }
        
        mConnectComponent->setVisible(true);
    }
    else {
        mConnectComponent->setVisible(false);
    }
    
#else
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
#endif
}


void SonobusAudioProcessorEditor::sliderValueChanged (Slider* slider)
{

}

void SonobusAudioProcessorEditor::mouseDown (const MouseEvent& event) 
{
    
    if (event.eventComponent == mSettingsButton.get()) {
        settingsWasShownOnDown = settingsCalloutBox != nullptr || (Time::getMillisecondCounter() < settingsClosedTimestamp + 500);

        if (!settingsWasShownOnDown) {
          //  showOrHideSettings();
        }
    }
    else if (event.eventComponent == mTitleLabel.get()) {
        settingsWasShownOnDown = settingsCalloutBox != nullptr;
        settingsClosedTimestamp = 0; // reset on a down
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
        if (Time::getMillisecondCounter() > settingsClosedTimestamp + 1000) {
            // only show if it wasn't just dismissed
            showSettings(true);
        }
    }
}

void SonobusAudioProcessorEditor::componentVisibilityChanged (Component& component)
{
    //if (&component == mSettingsTab.get()) {
    //    DebugLogC("setting vis changed: %d", component.isVisible());
    //}
}

void SonobusAudioProcessorEditor::componentParentHierarchyChanged (Component& component)
{
    if (&component == mSettingsTab.get()) {
        if (component.getParentComponent() == nullptr) {
            DebugLogC("setting parent changed: %p", component.getParentComponent());
            settingsClosedTimestamp = Time::getMillisecondCounter();
        }
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

        
#if JUCE_IOS
        const int defWidth = 300;
        const int defHeight = 420;
#else
        const int defWidth = 320;
        const int defHeight = 360;
#endif
        
        wrap->setSize(jmin(defWidth + 8, dw->getWidth() - 20), jmin(defHeight + 8, dw->getHeight() - 24));
        
        //Rectangle<int> setbounds = Rectangle<int>(5, mTitleImage->getBottom() + 2, std::min(100, getLocalBounds().getWidth() - 10), 80);
        bool firsttime = false;
        if (!mSettingsTab) {
            mSettingsTab = std::make_unique<TabbedComponent>(TabbedButtonBar::Orientation::TabsAtTop);
            mSettingsTab->setTabBarDepth(36);
            mSettingsTab->setOutline(0);
            mSettingsTab->getTabbedButtonBar().setMinimumTabScaleFactor(0.1f);
            mSettingsTab->addComponentListener(this);
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
                
#if JUCE_IOS
                mAudioDeviceSelector->setItemHeight(44);
#endif
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
        
        settingsClosedTimestamp = 0;
        
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
        mConnectButton->setTextJustification(Justification::centredTop);
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
            userstr = String::formatted("%d", processor.getNumberRemotePeers() + 1);
        } else {
            userstr = "1";
        }

        mMainPeerLabel->setText(userstr, dontSendNotification);
        mMainUserLabel->setText(currConnectionInfo.userName, dontSendNotification);

        mMainGroupImage->setVisible(true);
        mMainPersonImage->setVisible(true);
        mMainPeerLabel->setVisible(true);

        if (processor.getNumberRemotePeers() == 0) {
            mMainMessageLabel->setText(TRANS("Waiting for other users to join..."), dontSendNotification);
            mMainMessageLabel->setVisible(true);
        } else {
            mMainMessageLabel->setText("", dontSendNotification);
            mMainMessageLabel->setVisible(false);
        }
        
        mPatchbayButton->setVisible(false);
    }
    else {
        //mServerGroupEditor->setEnabled(currConnected);
        mMainGroupLabel->setText("", dontSendNotification);
        mMainUserLabel->setText("", dontSendNotification);

        mMainGroupImage->setVisible(false);
        mMainPersonImage->setVisible(false);
        mMainPeerLabel->setVisible(false);

        mMainMessageLabel->setVisible(true);

        if (processor.getNumberRemotePeers() == 0 /* || !currConnected */ ) {
            mMainMessageLabel->setText(TRANS("Press Connect button to start!"), dontSendNotification);
        } else {
            mMainMessageLabel->setText("", dontSendNotification);
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
    
    commandManager.commandStatusChanged();

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
    else if (pname == SonobusAudioProcessor::paramMasterRecvMute) {
        {
            const ScopedLock sl (clientStateLock);
            clientEvents.add(ClientEvent(ClientEvent::PeerChangedState, ""));
        }
        triggerAsyncUpdate();
    }
    else if (pname == SonobusAudioProcessor::paramMetEnabled) {
        {
            const ScopedLock sl (clientStateLock);
            clientEvents.add(ClientEvent(ClientEvent::PeerChangedState, ""));
        }
        triggerAsyncUpdate();
    }
    else if (pname == SonobusAudioProcessor::paramHearLatencyTest) {
        {
            const ScopedLock sl (clientStateLock);
            clientEvents.add(ClientEvent(ClientEvent::PeerChangedState, ""));
        }
        triggerAsyncUpdate();
    }
    else if (pname == SonobusAudioProcessor::paramMetIsRecorded) {
        {
            const ScopedLock sl (clientStateLock);
            clientEvents.add(ClientEvent(ClientEvent::PeerChangedState, ""));
        }
        triggerAsyncUpdate();
    }
    else if (pname == SonobusAudioProcessor::paramSendFileAudio) {
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
    //mMainStatusLabel->setText(mesg, dontSendNotification);
    //Desktop::getInstance().getAnimator().fadeIn(mMainStatusLabel.get(), 200);

    //serverStatusFadeTimestamp = Time::getMillisecondCounterHiRes() * 1e-3 + fadeAfterSec;
    
    if (!mainonly) {
        mServerStatusLabel->setText(mesg, dontSendNotification);
        mServerInfoLabel->setVisible(false);
        mServerStatusLabel->setVisible(true);
    }
}


String SonobusAudioProcessorEditor::generateNewUsername(const AooServerConnectionInfo & info)
{
    String newname = info.userName;
    
    // look for number as last word
    int spacepos = newname.lastIndexOf(" ");
    if (spacepos >= 0) {
        String lastword = newname.substring(spacepos+1);
        int ival = lastword.getIntValue();
        if (ival > 0) {
            // found a number, increment it
            newname = newname.substring(0, spacepos) + String::formatted(" %d", ival+1);
        }
        else {
            // no number, add one
            newname += " 2";
        }
    } else {
        // nothing, append a number 2
        newname += " 2";
    }
    
    return newname;
}


void SonobusAudioProcessorEditor::handleAsyncUpdate()
{
    Array<ClientEvent> newevents;
    {
        const ScopedLock sl (clientStateLock);
        newevents = clientEvents;
        clientEvents.clearQuick();
    }
    
    for (auto & ev : newevents) {
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
                    currConnectionInfo.userName = generateNewUsername(currConnectionInfo);

                    DebugLogC("Trying again with name: %s", currConnectionInfo.userName.toRawUTF8());
                    connectWithInfo(currConnectionInfo);
                    
                    return;
                    
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
            updatePeerState(true);
            updateState();
        }
        else if (ev.type == ClientEvent::GroupJoinEvent) {
            String statstr;
            if (ev.success) {
                statstr = TRANS("Joined Group: ") + ev.group;
                
                showConnectPopup(false);
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
 
#if JUCE_IOS
    const int narrowthresh = 480; //520;
#else
    const int narrowthresh = 588; //520;
#endif
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

    //mMainMessageLabel->setBounds(mMainGroupImage->getX(), mMainGroupImage->getY(), mMainUserLabel->getRight() - mMainGroupImage->getX(), mMainUserLabel->getHeight());
    mMainMessageLabel->setBounds(mPeerViewport->getX(), mPeerViewport->getY() + 20, mPeerViewport->getRight() - mPeerViewport->getX(), mMainUserLabel->getHeight());
    
    auto metbgbounds = Rectangle<int>(mMetEnableButton->getX(), mMetEnableButton->getY(), mMetConfigButton->getRight() - mMetEnableButton->getX(),  mMetEnableButton->getHeight()).expanded(2, 2);
    mMetButtonBg->setRectangle (metbgbounds.toFloat());

    
    mDragDropBg->setRectangle (getLocalBounds().toFloat());

    
    // connect component stuff
    mConnectComponent->setBounds(getLocalBounds());
    mConnectComponentBg->setRectangle (mConnectComponent->getLocalBounds().toFloat());

#if 1
    if (getWidth() > 700) {
        if (mConnectTab->getNumTabs() > 2) {
            // move recents to main connect component, out of tab
            mConnectTab->removeTab(0);
            mRecentsGroup->addAndMakeVisible(mRecentsContainer.get());
            mConnectComponent->addAndMakeVisible(mRecentsGroup.get());

            mConnectTab->setCurrentTabIndex(0);
            updateLayout();
        }
    } else {
        if (mConnectTab->getNumTabs() < 3) {
            int tabsel = mConnectTab->getCurrentTabIndex();
            mRecentsGroup->removeChildComponent(mRecentsContainer.get());
            mRecentsGroup->setVisible(false);
            
            mConnectTab->addTab(TRANS("RECENTS"), Colour::fromFloatRGBA(0.1, 0.1, 0.1, 1.0), mRecentsContainer.get(), false);
            //mConnectTab->addTab(TRANS("GROUP"), Colour::fromFloatRGBA(0.1, 0.1, 0.1, 1.0), mServerConnectContainer.get(), false);
            //mConnectTab->addTab(TRANS("DIRECT"), Colour::fromFloatRGBA(0.1, 0.1, 0.1, 1.0), mDirectConnectContainer.get(), false);
            mConnectTab->moveTab(2, 0);
            mConnectTab->setCurrentTabIndex(tabsel + 1);
            updateLayout();
        }
    }
#endif
    connectMainBox.performLayout(mConnectComponent->getLocalBounds());
    remoteBox.performLayout(mDirectConnectContainer->getLocalBounds().withSizeKeepingCentre(jmin(400, mDirectConnectContainer->getWidth()), mDirectConnectContainer->getHeight()));
    serverBox.performLayout(mServerConnectContainer->getLocalBounds().withSizeKeepingCentre(jmin(400, mServerConnectContainer->getWidth()), mServerConnectContainer->getHeight()));

    if (mConnectTab->getNumTabs() < 3) {
        mRecentsContainer->setBounds(mRecentsGroup->getLocalBounds().reduced(4).withTrimmedTop(10));
    }
    recentsBox.performLayout(mRecentsContainer->getLocalBounds());

    mServerInfoLabel->setBounds(mServerStatusLabel->getBounds());

    mConnectionTimeLabel->setBounds(mConnectButton->getBounds().removeFromBottom(16));
    
    if (mRecordingButton) {
        mFileRecordingLabel->setBounds(mRecordingButton->getBounds().removeFromBottom(14).translated(0, 1));
    }


    if (mConnectComponent->isVisible()) {
        mRecentsListBox->updateContent();
    }
    
    //mInGainLabel->setBounds(mInGainSlider->getBounds().removeFromLeft(mInGainSlider->getWidth()*0.5));
    //mDryLabel->setBounds(mDrySlider->getBounds().removeFromLeft(mDrySlider->getWidth()*0.5));
    //mOutGainLabel->setBounds(mOutGainSlider->getBounds().removeFromLeft(mOutGainSlider->getWidth()*0.5));
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
    
#if JUCE_IOS
    // make the button heights a bit more for touchscreen purposes
    minitemheight = 44;
    knobitemheight = 90;
    minpassheight = 38;
#endif
    
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

    
    outBox.items.clear();
    outBox.flexDirection = FlexBox::Direction::column;
    outBox.items.add(FlexItem(minKnobWidth, knoblabelheight, *mOutGainLabel).withMargin(0).withFlex(0));
    outBox.items.add(FlexItem(minKnobWidth, minitemheight, *mOutGainSlider).withMargin(0).withFlex(1)); //.withAlignSelf(FlexItem::AlignSelf::center));

    knobBox.items.clear();
    knobBox.flexDirection = FlexBox::Direction::row;
    knobBox.items.add(FlexItem(4, 5).withMargin(0).withFlex(isNarrow ? 0.0 : 0.1));
    knobBox.items.add(FlexItem(meterheight, minitemheight, *inputMeter).withMargin(1).withFlex(0));
    knobBox.items.add(FlexItem(2, 5).withMargin(0).withFlex(0));
    knobBox.items.add(FlexItem(minKnobWidth, minitemheight, inGainBox).withMargin(0).withFlex(1).withMaxWidth(isNarrow ? 160 : 120));
    knobBox.items.add(FlexItem(2, 5).withMargin(0).withFlex(0));
    knobBox.items.add(FlexItem(minKnobWidth, minitemheight, dryBox).withMargin(0).withFlex(1).withMaxWidth(isNarrow ? 160 : 120));
    knobBox.items.add(FlexItem(4, 5).withMargin(0).withFlex(0));
    knobBox.items.add(FlexItem(40, minitemheight, *mPanButton).withMargin(0).withFlex(1).withMaxWidth(54).withMaxHeight(minitemheight).withAlignSelf(FlexItem::AlignSelf::center));
    knobBox.items.add(FlexItem(2, 5).withMargin(0).withFlex(0));
    knobBox.items.add(FlexItem(minKnobWidth, minitemheight, outBox).withMargin(0).withFlex(1).withMaxWidth(isNarrow ? 160 : 120));
    knobBox.items.add(FlexItem(2, 5).withMargin(0).withFlex(0));
    knobBox.items.add(FlexItem(meterheight, minitemheight, *outputMeter).withMargin(0).withFlex(0)); //.withAlignSelf(FlexItem::AlignSelf::center));
    knobBox.items.add(FlexItem(isNarrow ? 14 : 16, 5).withMargin(0).withFlex(0));

    
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

    optionsHearlatBox.items.clear();
    optionsHearlatBox.flexDirection = FlexBox::Direction::row;
    optionsHearlatBox.items.add(FlexItem(10, 12));
    optionsHearlatBox.items.add(FlexItem(minButtonWidth, minitemheight, *mOptionsHearLatencyButton).withMargin(0).withFlex(1));

    optionsMetRecordBox.items.clear();
    optionsMetRecordBox.flexDirection = FlexBox::Direction::row;
    optionsMetRecordBox.items.add(FlexItem(10, 12));
    optionsMetRecordBox.items.add(FlexItem(minButtonWidth, minitemheight, *mOptionsMetRecordedButton).withMargin(0).withFlex(1));

    
    optionsBox.items.clear();
    optionsBox.flexDirection = FlexBox::Direction::column;
    optionsBox.items.add(FlexItem(4, 12));
    optionsBox.items.add(FlexItem(100, minitemheight, optionsSendQualBox).withMargin(2).withFlex(0));
    optionsBox.items.add(FlexItem(4, 4));
    optionsBox.items.add(FlexItem(100, minitemheight, optionsNetbufBox).withMargin(2).withFlex(0));
    optionsBox.items.add(FlexItem(4, 2));
    optionsBox.items.add(FlexItem(100, minpassheight, optionsHearlatBox).withMargin(2).withFlex(0));
    optionsBox.items.add(FlexItem(4, 2));
    optionsBox.items.add(FlexItem(100, minpassheight, optionsMetRecordBox).withMargin(2).withFlex(0));

    
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



    
    //titleBox.items.add(FlexItem(3, 20).withMargin(1).withFlex(0.2));
    //titleBox.items.add(FlexItem(40, 20, *mDummyVisual).withMargin(1).withFlex(4).withMaxWidth(200.0f));
    
    paramsBox.items.clear();
    paramsBox.flexDirection = FlexBox::Direction::column;
    //paramsBox.items.add(FlexItem(minKnobWidth, minitemheight, titleBox).withMargin(2).withFlex(0));
    //paramsBox.items.add(FlexItem(minKnobWidth, minitemheight, inGainBox).withMargin(2).withFlex(0));
    paramsBox.items.add(FlexItem(minKnobWidth*4 + 2*meterheight, knobitemheight, knobBox).withMargin(2).withFlex(0));
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
    remoteBox.items.add(FlexItem(180, minitemheight, *mDirectConnectDescriptionLabel).withMargin(2).withFlex(1).withMaxHeight(100));
    remoteBox.items.add(FlexItem(60, minitemheight, localAddressBox).withMargin(2).withFlex(0));
    remoteBox.items.add(FlexItem(10, 0).withFlex(1));

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
    servButtonBox.items.add(FlexItem(5, 3).withFlex(0.1));
    servButtonBox.items.add(FlexItem(minButtonWidth, minitemheight, *mServerConnectButton).withMargin(2).withFlex(1).withMaxWidth(300));
    servButtonBox.items.add(FlexItem(5, 3).withFlex(0.1));


    
    int maxservboxwidth = 400;
    
    serverBox.items.clear();
    serverBox.flexDirection = FlexBox::Direction::column;
    serverBox.items.add(FlexItem(5, 3).withFlex(0));
    serverBox.items.add(FlexItem(80, minpassheight, *mServerStatusLabel).withMargin(2).withFlex(1).withMaxWidth(maxservboxwidth).withMaxHeight(60));
    serverBox.items.add(FlexItem(5, 4).withFlex(0));
    serverBox.items.add(FlexItem(180, minitemheight, servGroupBox).withMargin(2).withFlex(0).withMaxWidth(maxservboxwidth));
    serverBox.items.add(FlexItem(180, minpassheight, servGroupPassBox).withMargin(2).withFlex(0).withMaxWidth(maxservboxwidth));
    serverBox.items.add(FlexItem(5, 6).withFlex(0));
    serverBox.items.add(FlexItem(180, minitemheight, servUserBox).withMargin(2).withFlex(0).withMaxWidth(maxservboxwidth));
    //serverBox.items.add(FlexItem(180, minpassheight, servUserPassBox).withMargin(2).withFlex(0));
    serverBox.items.add(FlexItem(5, 6).withFlex(1).withMaxHeight(14));
    serverBox.items.add(FlexItem(minButtonWidth, minitemheight, servButtonBox).withMargin(2).withFlex(0).withMaxWidth(maxservboxwidth));
    serverBox.items.add(FlexItem(5, 10).withFlex(0));
    serverBox.items.add(FlexItem(100, minitemheight, servAddressBox).withMargin(2).withFlex(0).withMaxWidth(maxservboxwidth));
    serverBox.items.add(FlexItem(80, minpassheight, *mServerAudioInfoLabel).withMargin(2).withFlex(1).withMaxWidth(maxservboxwidth).withMaxHeight(60));
    //serverBox.items.add(FlexItem(5, 2).withFlex(0));
    //serverBox.items.add(FlexItem(80, minpassheight, *mServerStatusLabel).withMargin(2).withFlex(1).withMaxWidth(maxservboxwidth));
    serverBox.items.add(FlexItem(5, 8).withFlex(1));
    //remoteBox.items.add(FlexItem(60, minitemheight, *mPatchbayButton).withMargin(2).withFlex(0.5).withMaxWidth(120));


    mainGroupUserBox.items.clear();
    mainGroupUserBox.flexDirection = FlexBox::Direction::row;
    mainGroupUserBox.items.add(FlexItem(0, 6).withFlex(0));
    mainGroupUserBox.items.add(FlexItem(24, minitemheight/2, *mMainPeerLabel).withMargin(0).withFlex(0));
    mainGroupUserBox.items.add(FlexItem(iconwidth, iconheight, *mMainGroupImage).withMargin(0).withFlex(0));
    mainGroupUserBox.items.add(FlexItem(minButtonWidth, minitemheight/2, *mMainGroupLabel).withMargin(2).withFlex(1));
    mainGroupUserBox.items.add(FlexItem(iconwidth, iconheight, *mMainPersonImage).withMargin(0).withFlex(0));
    mainGroupUserBox.items.add(FlexItem(minButtonWidth, minitemheight/2, *mMainUserLabel).withMargin(2).withFlex(1));

    connectBox.items.clear();
    connectBox.flexDirection = FlexBox::Direction::column;
    //connectBox.items.add(FlexItem(minButtonWidth, minitemheight, *mConnectButton).withMargin(2).withFlex(0));
    connectBox.items.add(FlexItem(minButtonWidth, minitemheight, titleBox).withMargin(2).withFlex(0));
    connectBox.items.add(FlexItem(minButtonWidth, minitemheight/2, mainGroupUserBox).withMargin(2).withFlex(0));
    //connectBox.items.add(FlexItem(80, minitemheight, *mMainStatusLabel).withMargin(2).withFlex(1));

    titleBox.items.clear();
    titleBox.flexDirection = FlexBox::Direction::row;
    titleBox.items.add(FlexItem(2, 20).withMargin(1).withFlex(0));
    titleBox.items.add(FlexItem(minitemheight - 8, minitemheight - 12, *mSettingsButton).withMargin(1).withMaxWidth(40).withFlex(0));
    //titleBox.items.add(FlexItem(5, 20).withMargin(1).withFlex(0));
    titleBox.items.add(FlexItem(90, 20, *mTitleLabel).withMargin(1).withFlex(0));
    if (iaaConnected) {
        titleBox.items.add(FlexItem(4, 4).withMargin(1).withFlex(0.0));
        titleBox.items.add(FlexItem(minitemheight, minitemheight, *mIAAHostButton).withMargin(0).withFlex(0));
    }
    titleBox.items.add(FlexItem(4, 4).withMargin(1).withFlex(0.0));
    titleBox.items.add(FlexItem(minButtonWidth, minitemheight, *mConnectButton).withMargin(0).withFlex(1));

    
    
    recentsBox.items.clear();
    recentsBox.flexDirection = FlexBox::Direction::column;
    recentsBox.items.add(FlexItem(minButtonWidth, minitemheight, *mRecentsListBox).withMargin(6).withFlex(1));
    
    // JLC
    
    middleBox.items.clear();
    int midboxminheight = (knobitemheight + 4);
    int midboxminwidth = 296;
    
    if (isNarrow) {
        middleBox.flexDirection = FlexBox::Direction::column;
        middleBox.items.add(FlexItem(150, minitemheight*1.5 + 8, connectBox).withMargin(2).withFlex(0));
        middleBox.items.add(FlexItem(5, 2));
        middleBox.items.add(FlexItem(100, knobitemheight + 6, paramsBox).withMargin(2).withFlex(0));
        middleBox.items.add(FlexItem(6, 2));

        midboxminheight = (minitemheight*1.5 + knobitemheight + 18);
        midboxminwidth = 190;
    } else {
        middleBox.flexDirection = FlexBox::Direction::row;        
        middleBox.items.add(FlexItem(260, minitemheight*1.5 , connectBox).withMargin(2).withFlex(1).withMaxWidth(400));
        middleBox.items.add(FlexItem(4, 1).withMaxWidth(6).withFlex(0.5));
        middleBox.items.add(FlexItem(minKnobWidth*4 + meterheight*2, knobitemheight, paramsBox).withMargin(2).withFlex(4));
    }


    toolbarTextBox.items.clear();
    toolbarTextBox.flexDirection = FlexBox::Direction::column;
    toolbarTextBox.items.add(FlexItem(40, minitemheight/2, *mMainStatusLabel).withMargin(0).withFlex(1));
    //toolbarTextBox.items.add(FlexItem(40, minitemheight/2, *mConnectionTimeLabel).withMargin(0).withFlex(1));


    metVolBox.items.clear();
    metVolBox.flexDirection = FlexBox::Direction::column;
    metVolBox.items.add(FlexItem(minKnobWidth, knoblabelheight, *mMetLevelSliderLabel).withMargin(0).withFlex(0));
    metVolBox.items.add(FlexItem(minKnobWidth, minitemheight, *mMetLevelSlider).withMargin(0).withFlex(1));

    metTempoBox.items.clear();
    metTempoBox.flexDirection = FlexBox::Direction::column;
    metTempoBox.items.add(FlexItem(minKnobWidth, knoblabelheight, *mMetTempoSliderLabel).withMargin(0).withFlex(0));
    metTempoBox.items.add(FlexItem(minKnobWidth, minitemheight, *mMetTempoSlider).withMargin(0).withFlex(1));

    metSendBox.items.clear();
    metSendBox.flexDirection = FlexBox::Direction::column;
    metSendBox.items.add(FlexItem(5, 5).withMargin(0).withFlex(1));
    metSendBox.items.add(FlexItem(60, minitemheight, *mMetSendButton).withMargin(0).withFlex(0));

    metBox.items.clear();
    metBox.flexDirection = FlexBox::Direction::row;
    metBox.items.add(FlexItem(minKnobWidth, minitemheight, metTempoBox).withMargin(0).withFlex(1));
    metBox.items.add(FlexItem(minKnobWidth, minitemheight, metVolBox).withMargin(0).withFlex(1));
    metBox.items.add(FlexItem(3, 6).withMargin(0).withFlex(0));
    metBox.items.add(FlexItem(60, minitemheight, metSendBox).withMargin(2).withFlex(1));
    
    
    toolbarBox.items.clear();
    toolbarBox.flexDirection = FlexBox::Direction::row;
    //toolbarBox.items.add(FlexItem(4, 5).withMargin(0).withFlex(0));
    //toolbarBox.items.add(FlexItem(120, minitemheight, titleBox).withMargin(0).withFlex(1).withMaxWidth(120));
    toolbarBox.items.add(FlexItem(7, 6).withMargin(0).withFlex(0));
    toolbarBox.items.add(FlexItem(44, minitemheight, *mMasterMuteButton).withMargin(0).withFlex(0));
    toolbarBox.items.add(FlexItem(4, 6).withMargin(0).withFlex(0));
    toolbarBox.items.add(FlexItem(44, minitemheight, *mMasterRecvMuteButton).withMargin(0).withFlex(0));
    toolbarBox.items.add(FlexItem(4, 6).withMargin(0).withFlex(0.1));
    toolbarBox.items.add(FlexItem(46, minitemheight, *mMetEnableButton).withMargin(0).withFlex(0).withMaxHeight(minitemheight+2).withAlignSelf(FlexItem::AlignSelf::center));
    toolbarBox.items.add(FlexItem(36, minitemheight, *mMetConfigButton).withMargin(0).withFlex(0).withMaxHeight(minitemheight+2).withAlignSelf(FlexItem::AlignSelf::center));
    toolbarBox.items.add(FlexItem(4, 5).withMargin(0).withFlex(0.1));

    //toolbarBox.items.add(FlexItem(100, minitemheight, *mOutGainSlider).withMargin(2).withFlex(1));
    //toolbarBox.items.add(FlexItem(100, meterheight, *outputMeter).withMargin(2).withFlex(1).withMaxHeight(meterheight).withAlignSelf(FlexItem::AlignSelf::center));
    //toolbarBox.items.add(FlexItem(100, meterheight, outmeterBox).withMargin(2).withFlex(1));
#if JUCE_IOS
#else
    if (processor.getCurrentJoinedGroup().isEmpty() && processor.getNumberRemotePeers() > 1) {
        toolbarBox.items.add(FlexItem(6, 6).withMargin(1).withFlex(0.2));
        toolbarBox.items.add(FlexItem(60, minitemheight, *mPatchbayButton).withMargin(2).withFlex(0.5).withMaxWidth(120));
    }
#endif
    //toolbarBox.items.add(FlexItem(40, minitemheight, toolbarTextBox).withMargin(1).withFlex(1));

    if (mRecordingButton) {

        transportVBox.items.clear();
        transportVBox.flexDirection = FlexBox::Direction::column;

        transportBox.items.clear();
        transportBox.flexDirection = FlexBox::Direction::row;

        transportWaveBox.items.clear();
        transportWaveBox.flexDirection = FlexBox::Direction::row;

        transportWaveBox.items.add(FlexItem(isNarrow ? 7 : 3, 6).withMargin(0).withFlex(0));
        transportWaveBox.items.add(FlexItem(100, minitemheight, *mWaveformThumbnail).withMargin(0).withFlex(1));
        transportWaveBox.items.add(FlexItem(isNarrow ? 15 : 3, 6).withMargin(0).withFlex(0));

        transportBox.items.add(FlexItem(7, 6).withMargin(0).withFlex(0));
        transportBox.items.add(FlexItem(44, minitemheight, *mPlayButton).withMargin(0).withFlex(0));
        transportBox.items.add(FlexItem(4, 6).withMargin(0).withFlex(0));
        transportBox.items.add(FlexItem(44, minitemheight, *mLoopButton).withMargin(0).withFlex(0));
        
        if ( ! isNarrow) {
            transportBox.items.add(FlexItem(3, 6, transportWaveBox).withMargin(0).withFlex(1));  

            transportVBox.items.add(FlexItem(3, minitemheight, transportBox).withMargin(0).withFlex(0));  
        }
        else {
            transportBox.items.add(FlexItem(3, 6).withMargin(0).withFlex(0.1));

            transportVBox.items.add(FlexItem(3, minitemheight, transportWaveBox).withMargin(0).withFlex(0));  
            transportVBox.items.add(FlexItem(3, 3).withMargin(0).withFlex(0));
            transportVBox.items.add(FlexItem(3, minitemheight, transportBox).withMargin(0).withFlex(0));  
        }
            
        transportBox.items.add(FlexItem(minKnobWidth, minitemheight, *mPlaybackSlider).withMargin(0).withFlex(0));
        transportBox.items.add(FlexItem(3, 6).withMargin(0).withFlex(0));
        transportBox.items.add(FlexItem(44, minitemheight, *mFileSendAudioButton).withMargin(0).withFlex(0));
        transportBox.items.add(FlexItem(3, 6).withMargin(0).withFlex(0));
        transportBox.items.add(FlexItem(44, minitemheight, *mDismissTransportButton).withMargin(0).withFlex(0));
        transportBox.items.add(FlexItem(14, 6).withMargin(1).withFlex(0));
        
        //recBox.items.clear();
        //recBox.flexDirection = FlexBox::Direction::column;
        //recBox.items.add(FlexItem(44, 12, *mFileRecordingLabel).withMargin(0).withFlex(0));
        //recBox.items.add(FlexItem(44, minitemheight, *mRecordingButton).withMargin(0).withFlex(1));

        //toolbarBox.items.add(FlexItem(44, minitemheight, recBox).withMargin(1).withFlex(0));
        toolbarBox.items.add(FlexItem(44, minitemheight, *mRecordingButton).withMargin(0).withFlex(0));
        toolbarBox.items.add(FlexItem(3, 6).withMargin(1).withFlex(0));
        toolbarBox.items.add(FlexItem(44, minitemheight, *mFileBrowseButton).withMargin(1).withFlex(0));
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
    if (!mCurrentAudioFile.isEmpty()) {
        if (isNarrow) {
            mainBox.items.add(FlexItem(100, 2*minitemheight + 3, transportVBox).withMargin(0).withFlex(0)); minheight += 2*minitemheight + 8;
        }
        else {
            mainBox.items.add(FlexItem(100, minitemheight, transportVBox).withMargin(0).withFlex(0)); minheight += minitemheight + 5;
        }
        mainBox.items.add(FlexItem(4, 5).withMargin(0).withFlex(0));
    }
    
    mainBox.items.add(FlexItem(100, minitemheight + 4, toolbarBox).withMargin(0).withFlex(0)); minheight += minitemheight + 10;
    mainBox.items.add(FlexItem(10, 6).withFlex(0));
        
    
    // Connect component
    
    connectTitleBox.items.clear();
    connectTitleBox.flexDirection = FlexBox::Direction::row;
    connectTitleBox.items.add(FlexItem(50, minitemheight));
    connectTitleBox.items.add(FlexItem(80, minitemheight, *mConnectTitle).withMargin(2).withFlex(1));
    connectTitleBox.items.add(FlexItem(50, minitemheight, *mConnectCloseButton).withMargin(2));

    connectHorizBox.items.clear();
    connectHorizBox.flexDirection = FlexBox::Direction::row;
    if (mConnectTab->getNumTabs() < 3) {
        connectHorizBox.items.add(FlexItem(320, 100, *mRecentsGroup).withMargin(3).withFlex(0));
    }
    connectHorizBox.items.add(FlexItem(100, 100, *mConnectTab).withMargin(3).withFlex(1));

    connectMainBox.items.clear();
    connectMainBox.flexDirection = FlexBox::Direction::column;
    connectMainBox.items.add(FlexItem(100, minitemheight, connectTitleBox).withMargin(3).withFlex(0));
    connectMainBox.items.add(FlexItem(100, 100, connectHorizBox).withMargin(3).withFlex(1));

    
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

void SonobusAudioProcessorEditor::changeListenerCallback (ChangeBroadcaster* source)
{
    if (source == mWaveformThumbnail.get()) {
        loadAudioFromURL(URL (mWaveformThumbnail->getLastDroppedFile()));
    } else if (source == &(processor.getTransportSource())) {
        //if (!processor.getTransportSource().isPlaying() && processor.getTransportSource().getCurrentPosition() >= processor.getTransportSource().getLengthInSeconds()) {
            // at end, return to start
//            processor.getTransportSource().setPosition(0.0);
 //       }
        updateTransportState();
    }
}

#pragma mark - ApplicationCommandTarget

void SonobusAudioProcessorEditor::getCommandInfo (CommandID cmdID, ApplicationCommandInfo& info) {
    switch (cmdID) {
        case SonobusCommands::MuteAllInput:
            info.setInfo (TRANS("Mute All Input"),
                          TRANS("Toggle Mute all input"),
                          TRANS("Popup"), 0);
            info.setActive(true);
            info.addDefaultKeypress ('m', ModifierKeys::noModifiers);
            break;
        case SonobusCommands::MuteAllPeers:
            info.setInfo (TRANS("Mute All Users"),
                          TRANS("Toggle Mute all users"),
                          TRANS("Popup"), 0);
            info.setActive(true);
            info.addDefaultKeypress ('u', ModifierKeys::commandModifier);
            break;
        case SonobusCommands::TogglePlayPause:
            info.setInfo (TRANS("Play/Pause"),
                          TRANS("Toggle file playback"),
                          TRANS("Popup"), 0);
            info.setActive(mCurrentAudioFile.getFileName().isNotEmpty());
            info.addDefaultKeypress (' ', ModifierKeys::noModifiers);
            break;
        case SonobusCommands::ToggleLoop:
            info.setInfo (TRANS("Loop"),
                          TRANS("Toggle file looping"),
                          TRANS("Popup"), 0);
            info.setActive(mCurrentAudioFile.getFileName().isNotEmpty());
            info.addDefaultKeypress ('l', ModifierKeys::noModifiers);
            break;
        case SonobusCommands::TrimSelectionToNewFile:
            info.setInfo (TRANS("Trim to New"),
                          TRANS("Trim file from selection to new file"),
                          TRANS("Popup"), 0);
            info.setActive(mCurrentAudioFile.getFileName().isNotEmpty());
            info.addDefaultKeypress ('t', ModifierKeys::commandModifier);
            break;
        case SonobusCommands::CloseFile:
            info.setInfo (TRANS("Close File"),
                          TRANS("Close file"),
                          TRANS("Popup"), 0);
            info.setActive(mCurrentAudioFile.getFileName().isNotEmpty());
            info.addDefaultKeypress ('w', ModifierKeys::commandModifier);
            break;
        case SonobusCommands::OpenFile:
            info.setInfo (TRANS("Open File"),
                          TRANS("Open Audio file"),
                          TRANS("Popup"), 0);
            info.setActive(true);
            info.addDefaultKeypress ('o', ModifierKeys::commandModifier);
            break;
        case SonobusCommands::ShareFile:
            info.setInfo (TRANS("Share File"),
                          TRANS("Share file"),
                          TRANS("Popup"), 0);
            info.setActive(mCurrentAudioFile.getFileName().isNotEmpty());
            info.addDefaultKeypress ('e', ModifierKeys::commandModifier);
            break;
        case SonobusCommands::Connect:
            info.setInfo (TRANS("Connect"),
                          TRANS("Connect"),
                          TRANS("Popup"), 0);
            info.setActive(!currConnected);
            info.addDefaultKeypress ('n', ModifierKeys::commandModifier);
            break;
        case SonobusCommands::Disconnect:
            info.setInfo (TRANS("Disconnect"),
                          TRANS("Disconnect"),
                          TRANS("Popup"), 0);
            info.setActive(currConnected);
            info.addDefaultKeypress ('d', ModifierKeys::commandModifier);
            break;
        case SonobusCommands::ShowOptions:
            info.setInfo (TRANS("Show Options"),
                          TRANS("Show Options"),
                          TRANS("Popup"), 0);
            info.setActive(true);
            info.addDefaultKeypress (',', ModifierKeys::commandModifier);
            break;
        case SonobusCommands::RecordToggle:
            info.setInfo (TRANS("Record"),
                          TRANS("Toggle Record"),
                          TRANS("Popup"), 0);
            info.setActive(true);
            info.addDefaultKeypress ('r', ModifierKeys::commandModifier);
            break;
    }
}

void SonobusAudioProcessorEditor::getAllCommands (Array<CommandID>& cmds) {
    cmds.add(SonobusCommands::MuteAllInput);
    cmds.add(SonobusCommands::MuteAllPeers);
    cmds.add(SonobusCommands::TogglePlayPause);
    cmds.add(SonobusCommands::ToggleLoop);
    cmds.add(SonobusCommands::TrimSelectionToNewFile);
    cmds.add(SonobusCommands::CloseFile);
    cmds.add(SonobusCommands::ShareFile);
    cmds.add(SonobusCommands::Connect);
    cmds.add(SonobusCommands::Disconnect);
    cmds.add(SonobusCommands::ShowOptions);
    cmds.add(SonobusCommands::OpenFile);
    cmds.add(SonobusCommands::RecordToggle);
}

bool SonobusAudioProcessorEditor::perform (const InvocationInfo& info) {
    bool ret = true;
    
    switch (info.commandID) {
        case SonobusCommands::MuteAllInput:
            DebugLogC("got mute toggle!");
            mMasterMuteButton->setToggleState(!mMasterMuteButton->getToggleState(), sendNotification);
            break;
        case SonobusCommands::MuteAllPeers:
            DebugLogC("got mute peers toggle!");
            mMasterRecvMuteButton->setToggleState(!mMasterRecvMuteButton->getToggleState(), sendNotification);
            break;
        case SonobusCommands::TogglePlayPause:
            DebugLogC("got play pause!");
            if (mPlayButton->isVisible()) {
                mPlayButton->setToggleState(!mPlayButton->getToggleState(), sendNotification);
            }
            break;
        case SonobusCommands::ToggleLoop:
            DebugLogC("got loop toggle!");
            if (mLoopButton->isVisible()) {
                mLoopButton->setToggleState(!mLoopButton->getToggleState(), sendNotification);
            }
            break;
        case SonobusCommands::TrimSelectionToNewFile:
            DebugLogC("Got trim!");
            break;
        case SonobusCommands::CloseFile:
            DebugLogC("got close file!");
            if (mDismissTransportButton->isVisible()) {
                buttonClicked(mDismissTransportButton.get());
            }

            break;
        case SonobusCommands::ShareFile:
            DebugLogC("got share file!");

            break;
        case SonobusCommands::OpenFile:
            DebugLogC("got open file!");
            buttonClicked(mFileBrowseButton.get());
            break;
        case SonobusCommands::Connect:
            DebugLogC("got connect!");
            if (!currConnected) {
                buttonClicked(mConnectButton.get());
            }
            break;
        case SonobusCommands::Disconnect:
            DebugLogC("got disconnect!");
            
            if (currConnected) {
                buttonClicked(mConnectButton.get());
            }

            break;
        case SonobusCommands::ShowOptions:
            DebugLogC("got show options!");
            buttonClicked(mSettingsButton.get());

            break;
        case SonobusCommands::RecordToggle:
            DebugLogC("got record toggle!");
            buttonClicked(mRecordingButton.get());

            break;
        default:
            ret = false;
    }
    
    return ret;
}

#pragma MenuBarModel

enum
{
    MenuFileIndex = 0,
    MenuConnectIndex,
    MenuTransportIndex,
    MenuHelpIndex
};

StringArray SonobusAudioProcessorEditor::SonobusMenuBarModel::getMenuBarNames()
{
    return StringArray(TRANS("File"),
                       TRANS("Connect"),
                       TRANS("Transport"),
                       TRANS("Help"));
}

PopupMenu SonobusAudioProcessorEditor::SonobusMenuBarModel::getMenuForIndex (int topLevelMenuIndex, const String& /*menuName*/)
{
     PopupMenu retval;
        
    switch (topLevelMenuIndex) {
        case MenuFileIndex:
            retval.addCommandItem (&parent.commandManager, SonobusCommands::OpenFile);
            retval.addCommandItem (&parent.commandManager, SonobusCommands::CloseFile);
            retval.addCommandItem (&parent.commandManager, SonobusCommands::ShareFile);
            retval.addSeparator();
            retval.addCommandItem (&parent.commandManager, SonobusCommands::TrimSelectionToNewFile);
#if JUCE_WINDOWS
            retval.addCommandItem (&parent.commandManager, SonobusCommands::ShowOptions);
            retval.addSeparator();
            retval.addCommandItem (&parent.commandManager, StandardApplicationCommandIDs::quit);
#endif
            break;
        case MenuConnectIndex:
            retval.addCommandItem (&parent.commandManager, SonobusCommands::Connect);
            retval.addCommandItem (&parent.commandManager, SonobusCommands::Disconnect);
            retval.addSeparator();
            retval.addCommandItem (&parent.commandManager, SonobusCommands::MuteAllInput);
            retval.addCommandItem (&parent.commandManager, SonobusCommands::MuteAllPeers);
            break;
        case MenuTransportIndex:
            retval.addCommandItem (&parent.commandManager, SonobusCommands::TogglePlayPause);
            retval.addCommandItem (&parent.commandManager, SonobusCommands::ToggleLoop);
            retval.addSeparator();
            retval.addCommandItem (&parent.commandManager, SonobusCommands::RecordToggle);
            break;
            
        case MenuHelpIndex:
#if JUCE_WINDOWS
            //retval.addCommandItem (&commandManager, aboutTonalEnergyTuner);
#endif
            //retval.addCommandItem (&commandManager, SonobusCommands::ShowHelp);
            break;
    }
    
    return retval;
}

void SonobusAudioProcessorEditor::SonobusMenuBarModel::menuItemSelected (int menuItemID, int topLevelMenuIndex)
{
#if JUCE_MAC
    if (topLevelMenuIndex == -1) {
        switch (menuItemID) {
            case 1:
                //commandManager.invokeDirectly(aboutTonalEnergyTuner, true);
                break;
            case 2:
                parent.commandManager.invokeDirectly(SonobusCommands::ShowOptions, true);
                break;
        }
    }
#endif
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
