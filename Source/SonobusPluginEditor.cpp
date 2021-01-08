// SPDX-License-Identifier: GPLv3-or-later
// Copyright (C) 2020 Jesse Chappell



#include "SonobusPluginProcessor.h"
#include "SonobusPluginEditor.h"

#include "BeatToggleGrid.h"

#include "PeersContainerView.h"
#include "WaveformTransportComponent.h"
#include "RandomSentenceGenerator.h"
#include "SonoUtility.h"
#include "SonobusTypes.h"

#include "AutoUpdater.h"

#include <sstream>

enum {
    PeriodicUpdateTimerId = 0,
    CheckForNewVersionTimerId
};


enum {
    nameTextColourId = 0x1002830,
    selectedColourId = 0x1002840,
    separatorColourId = 0x1002850,
};

#define DEFAULT_SERVER_PORT 10998
#define DEFAULT_SERVER_HOST "aoo.sonobus.net"
#define SONOBUS_SCHEME "sonobus"

class SonobusConnectTabbedComponent : public TabbedComponent
{
public:
    SonobusConnectTabbedComponent(TabbedButtonBar::Orientation orientation, SonobusAudioProcessorEditor & editor_) : TabbedComponent(orientation), editor(editor_) {

    }

    void currentTabChanged (int newCurrentTabIndex, const String& newCurrentTabName) override {

        editor.connectTabChanged(newCurrentTabIndex);
    }

protected:
    SonobusAudioProcessorEditor & editor;

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
        
        resized();
    }
    
    void paint (Graphics& g) override
    {

    }

    void resized() override
    {
        Component::resized();
        
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
    : AudioProcessorEditor (&p), processor (p),  sonoSliderLNF(13), smallLNF(14), teensyLNF(11), panSliderLNF(12),
recentsListModel(this),
recentsGroupFont (17.0, Font::bold), recentsNameFont(15, Font::plain), recentsInfoFont(13, Font::plain),
publicGroupsListModel(this)
{
    LookAndFeel::setDefaultLookAndFeel(&sonoLookAndFeel);
    
    sonoLookAndFeel.setUsingNativeAlertWindows(true);

    // setupLocalisation();  // NOT YET!

    setColour (nameTextColourId, Colour::fromFloatRGBA(1.0f, 1.0f, 1.0f, 0.9f));
    setColour (selectedColourId, Colour::fromFloatRGBA(0.0f, 0.4f, 0.8f, 0.5f));
    setColour (separatorColourId, Colour::fromFloatRGBA(0.3f, 0.3f, 0.3f, 0.3f));

    mWorkPool = std::make_unique<ThreadPool>(1);
    
    sonoSliderLNF.textJustification = Justification::centredRight;
    sonoSliderLNF.sliderTextJustification = Justification::centredRight;
    
    
    Array<AooServerConnectionInfo> recents;
    processor.getRecentServerConnectionInfos(recents);
    if (recents.size() > 0) {
        currConnectionInfo = recents.getReference(0);
    }
    else {
        // defaults
        currConnectionInfo.groupName = "";
#if JUCE_IOS
        //String username = SystemStats::getFullUserName(); //SystemStats::getLogonName();
        String username = SystemStats::getComputerName(); //SystemStats::getLogonName();
#else
        String username = SystemStats::getFullUserName(); //SystemStats::getLogonName();    
        //if (username.length() > 0) username = username.replaceSection(0, 1, username.substring(0, 1).toUpperCase());
#endif

        if (username.isEmpty()) {
            username = SystemStats::getComputerName();
        }
        
        currConnectionInfo.userName = username;

        currConnectionInfo.serverHost = "aoo.sonobus.net";
        currConnectionInfo.serverPort = DEFAULT_SERVER_PORT;
    }
    
    mTitleLabel = std::make_unique<Label>("title", TRANS("SonoBus"));
    mTitleLabel->setFont(20);
    mTitleLabel->setColour(Label::textColourId, Colour(0xff47b0f8));
    mTitleLabel->setInterceptsMouseClicks(true, false);
    mTitleLabel->addMouseListener(this, false);

    mTitleImage = std::make_unique<ImageComponent>("title");
    mTitleImage->setImage(ImageCache::getFromMemory(BinaryData::sonobus_logo_96_png, BinaryData::sonobus_logo_96_pngSize));
    mTitleImage->setInterceptsMouseClicks(true, false);
    mTitleImage->addMouseListener(this, false);

    
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
    mMainMessageLabel->setFont(20);

    mMainPersonImage = std::make_unique<ImageComponent>();
    mMainPersonImage->setImage(ImageCache::getFromMemory(BinaryData::person_png, BinaryData::person_pngSize));
    mMainGroupImage = std::make_unique<ImageComponent>();
    mMainGroupImage->setImage(ImageCache::getFromMemory(BinaryData::people_png, BinaryData::people_pngSize));
    mMainPersonImage->setInterceptsMouseClicks(false, false);
    mMainGroupImage->setInterceptsMouseClicks(false, false);
    
    
    mInGainSlider     = std::make_unique<Slider>(Slider::LinearHorizontal,  Slider::TextBoxAbove);
    mInGainSlider->setName("ingain");
    mInGainSlider->setSliderSnapsToMousePosition(processor.getSlidersSnapToMousePosition());
    mInGainSlider->setTextBoxIsEditable(true);
    mInGainSlider->setScrollWheelEnabled(false);


    mInMonPanMonoSlider     = std::make_unique<Slider>(Slider::LinearHorizontal,  Slider::NoTextBox); 
    mInMonPanMonoSlider->setName("inpanmono");
    mInMonPanMonoSlider->getProperties().set ("fromCentre", true);
    mInMonPanMonoSlider->getProperties().set ("noFill", true);
    mInMonPanMonoSlider->setSliderSnapsToMousePosition(false);
    mInMonPanMonoSlider->setTextBoxIsEditable(false);
    mInMonPanMonoSlider->setScrollWheelEnabled(false);
    mInMonPanMonoSlider->setMouseDragSensitivity(100);
    mInMonPanMonoSlider->addListener(this);
    
    // just create it to set the range stuff, then remove it
    mInMonPanMonoAttachment = std::make_unique<AudioProcessorValueTreeState::SliderAttachment> (p.getValueTreeState(), SonobusAudioProcessor::paramInMonitorMonoPan, *mInMonPanMonoSlider);
    mInMonPanMonoAttachment.reset();
    
    mInMonPanMonoSlider->setDoubleClickReturnValue(true, 0.0);
    mInMonPanMonoSlider->setLookAndFeel(&panSliderLNF);
    mInMonPanMonoSlider->textFromValueFunction =  [](double v) -> String { if (fabs(v) < 0.01) return String(TRANS("Pan: Center")); return String(TRANS("Pan: ")) +  String((int)rint(abs(v*100.0f))) + ((v > 0 ? "% R" : "% L")) ; };
    mInMonPanMonoSlider->valueFromTextFunction =  [](const String& s) -> double { return s.getDoubleValue()*1e-2f; };
    mInMonPanMonoSlider->setValue(0.1, dontSendNotification);
    mInMonPanMonoSlider->setValue(0.0, dontSendNotification);
    mInMonPanMonoSlider->setPopupDisplayEnabled(true, true, this);

    
    mInMonPanStereoSlider     = std::make_unique<Slider>(Slider::TwoValueHorizontal,  Slider::NoTextBox); 
    mInMonPanStereoSlider->setName("inpanstereo");
    mInMonPanStereoSlider->getProperties().set ("fromCentre", true);
    mInMonPanStereoSlider->getProperties().set ("noFill", true);
    mInMonPanStereoSlider->setSliderSnapsToMousePosition(false);
    mInMonPanStereoSlider->setRange(mInMonPanMonoSlider->getMinimum(), mInMonPanMonoSlider->getMaximum(), mInMonPanMonoSlider->getInterval());
    mInMonPanStereoSlider->setSkewFactor(mInMonPanMonoSlider->getSkewFactor());
    mInMonPanStereoSlider->setTextBoxIsEditable(false);
    mInMonPanStereoSlider->setScrollWheelEnabled(false);
    mInMonPanStereoSlider->setMouseDragSensitivity(100);
    //mInMonPanStereoSlider->setDoubleClickReturnValue(true, 0.0);
    mInMonPanStereoSlider->setLookAndFeel(&panSliderLNF);            
    mInMonPanStereoSlider->textFromValueFunction =  [](double v) -> String { if (fabs(v) < 0.01) return String(TRANS("Pan: Center")); return String(TRANS("Pan: ")) +  String((int)rint(abs(v*100.0f))) + ((v > 0 ? "% R" : "% L")) ; };
    mInMonPanStereoSlider->valueFromTextFunction =  [](const String& s) -> double { return s.getDoubleValue()*1e-2f; };
    mInMonPanStereoSlider->setMinAndMaxValues(-1, 1, dontSendNotification);
    mInMonPanStereoSlider->setPopupDisplayEnabled(true, true, this);
    mInMonPanStereoSlider->addListener(this);
    
    
    mInMonPanSlider1     = std::make_unique<Slider>(Slider::LinearBar,  Slider::TextBoxBelow); 
    mInMonPanSlider1->setName("inpan1");
    mInMonPanSlider1->getProperties().set ("fromCentre", true);
    mInMonPanSlider1->getProperties().set ("noFill", true);
    mInMonPanSlider1->setSliderSnapsToMousePosition(false);
    mInMonPanSlider1->setTextBoxIsEditable(false);
    mInMonPanSlider1->setScrollWheelEnabled(false);
    mInMonPanSlider1->setMouseDragSensitivity(100);

    mInMonPanLabel1 = std::make_unique<Label>(SonobusAudioProcessor::paramDry, TRANS("In Pan"));
    configLabel(mInMonPanLabel1.get(), true);
    
    mInMonPanSlider2     = std::make_unique<Slider>(Slider::LinearBar,  Slider::TextBoxBelow); 
    mInMonPanSlider2->setName("inpan2");
    mInMonPanSlider2->getProperties().set ("fromCentre", true);
    mInMonPanSlider2->getProperties().set ("noFill", true);
    mInMonPanSlider2->setSliderSnapsToMousePosition(false);
    mInMonPanSlider2->setTextBoxIsEditable(false);
    mInMonPanSlider2->setScrollWheelEnabled(false);
    mInMonPanSlider2->setMouseDragSensitivity(100);

    mInMonPanLabel2 = std::make_unique<Label>(SonobusAudioProcessor::paramDry, TRANS("In Pan 2"));
    configLabel(mInMonPanLabel2.get(), true);
    
    mInMonPanSlider1->setDoubleClickReturnValue(true, -1.0);
    mInMonPanSlider2->setDoubleClickReturnValue(true, 1.0);

    
    inPannersContainer = std::make_unique<Component>();
    
    mPanButton = std::make_unique<TextButton>("pan");
    mPanButton->setButtonText(TRANS("In Pan"));
    mPanButton->setLookAndFeel(&smallLNF);
    mPanButton->addListener(this);

    mInMuteButton = std::make_unique<TextButton>("mute");
    mInMuteButton->setButtonText(TRANS("MUTE"));
    mInMuteButton->setLookAndFeel(&smallLNF);
    mInMuteButton->addListener(this);
    mInMuteButton->setClickingTogglesState(true);
    mInMuteButton->setColour(TextButton::buttonOnColourId, Colour::fromFloatRGBA(0.6, 0.3, 0.1, 1.0));
    mInMuteButton->setTooltip(TRANS("Mutes your input preventing everyone from hearing you, without any indicator"));
    mInMonMuteAttachment = std::make_unique<AudioProcessorValueTreeState::ButtonAttachment> (p.getValueTreeState(), SonobusAudioProcessor::paramMainInMute, *mInMuteButton);

    mInSoloButton = std::make_unique<TextButton>("solo");
    mInSoloButton->setButtonText(TRANS("SOLO"));
    mInSoloButton->setLookAndFeel(&smallLNF);
    mInSoloButton->addListener(this);
    mInSoloButton->setClickingTogglesState(true);
    mInSoloButton->setColour(TextButton::buttonOnColourId, Colour::fromFloatRGBA(1.0, 1.0, 0.6, 0.7f));
    mInSoloButton->setColour(TextButton::textColourOnId, Colours::darkblue);

    mInSoloButton->setTooltip(TRANS("Listen to only yourself, and other soloed users. Alt-click to exclusively solo yourself."));
    mInMonSoloAttachment = std::make_unique<AudioProcessorValueTreeState::ButtonAttachment> (p.getValueTreeState(), SonobusAudioProcessor::paramMainMonitorSolo, *mInSoloButton);

    
    
    
    mMainMuteButton = std::make_unique<SonoDrawableButton>("sendmute", DrawableButton::ButtonStyle::ImageFitted);
    std::unique_ptr<Drawable> sendallowimg(Drawable::createFromImageData(BinaryData::mic_svg, BinaryData::mic_svgSize));
    std::unique_ptr<Drawable> senddisallowimg(Drawable::createFromImageData(BinaryData::mic_disabled_svg, BinaryData::mic_disabled_svgSize));
    mMainMuteButton->setImages(sendallowimg.get(), nullptr, nullptr, nullptr, senddisallowimg.get());
    mMainMuteButton->addListener(this);
    mMainMuteButton->setClickingTogglesState(true);
    mMainMuteButton->setColour(TextButton::buttonOnColourId, Colour::fromFloatRGBA(0.2, 0.2, 0.2, 0.7));
    mMainMuteButton->setColour(TextButton::buttonColourId, Colours::transparentBlack);
    mMainMuteButton->setColour(SonoTextButton::outlineColourId, Colours::transparentBlack);
    mMainMuteButton->setColour(DrawableButton::backgroundOnColourId, Colour::fromFloatRGBA(0.2, 0.2, 0.2, 0.7));
    mMainMuteButton->setColour(DrawableButton::backgroundColourId, Colours::transparentBlack);
    mMainMuteButton->setTooltip(TRANS("Silences your Input, none of your audio (including file playback) will be sent to users and they will see a muted indicator"));

    mMainRecvMuteButton = std::make_unique<SonoDrawableButton>("recvmute", DrawableButton::ButtonStyle::ImageFitted);
    std::unique_ptr<Drawable> recvallowimg(Drawable::createFromImageData(BinaryData::hearothers_svg, BinaryData::hearothers_svgSize));
    std::unique_ptr<Drawable> recvdisallowimg(Drawable::createFromImageData(BinaryData::muteothers_svg, BinaryData::muteothers_svgSize));
    mMainRecvMuteButton->setImages(recvallowimg.get(), nullptr, nullptr, nullptr, recvdisallowimg.get());
    mMainRecvMuteButton->addListener(this);
    mMainRecvMuteButton->setClickingTogglesState(true);
    mMainRecvMuteButton->setColour(TextButton::buttonOnColourId, Colour::fromFloatRGBA(0.2, 0.2, 0.2, 0.7));
    mMainRecvMuteButton->setColour(TextButton::buttonColourId, Colours::transparentBlack);
    mMainRecvMuteButton->setColour(SonoTextButton::outlineColourId, Colours::transparentBlack);
    mMainRecvMuteButton->setColour(DrawableButton::backgroundOnColourId, Colour::fromFloatRGBA(0.2, 0.2, 0.2, 0.7));
    mMainRecvMuteButton->setColour(DrawableButton::backgroundColourId, Colours::transparentBlack);
    mMainRecvMuteButton->setTooltip(TRANS("Mutes/Unmutes all users, no audio data will be received when users are muted"));

    mMainPushToTalkButton = std::make_unique<SonoDrawableButton>("ptt", DrawableButton::ButtonStyle::ImageFitted);
    std::unique_ptr<Drawable> pttimg(Drawable::createFromImageData(BinaryData::mic_pointing_svg, BinaryData::mic_pointing_svgSize));
    std::unique_ptr<Drawable> pttonimg(Drawable::createFromImageData(BinaryData::speaker_disabled_svg, BinaryData::speaker_disabled_svgSize));
    mMainPushToTalkButton->setImages(pttimg.get()/*, nullptr, pttonimg.get()*/);
    mMainPushToTalkButton->addMouseListener(this, false);
    mMainPushToTalkButton->setColour(TextButton::buttonOnColourId, Colour::fromFloatRGBA(0.2, 0.2, 0.2, 0.7));
    mMainPushToTalkButton->setColour(TextButton::buttonColourId, Colours::transparentBlack);
    mMainPushToTalkButton->setColour(SonoTextButton::outlineColourId, Colours::transparentBlack);
    mMainPushToTalkButton->setColour(DrawableButton::backgroundOnColourId, Colour::fromFloatRGBA(0.2, 0.2, 0.2, 0.7));
    mMainPushToTalkButton->setColour(DrawableButton::backgroundColourId, Colours::transparentBlack);
    String pttmessage = TRANS("When pressed, mutes others and unmutes you momentarily (push to talk).");
#if !(JUCE_IOS || JUCE_ANDROID)
    pttmessage += TRANS(" Use the 'T' key as a shortcut.");
#endif
    mMainPushToTalkButton->setTooltip(pttmessage);
    
    mMetContainer = std::make_unique<Component>();
    mEffectsContainer = std::make_unique<Component>();
    mInEffectsContainer = std::make_unique<Component>();

    
    
    mMetButtonBg = std::make_unique<DrawableRectangle>();
    mMetButtonBg->setCornerSize(Point<float>(8,8));
    mMetButtonBg->setFill (Colour::fromFloatRGBA(0.0, 0.0, 0.0, 1.0));
    mMetButtonBg->setStrokeFill (Colour::fromFloatRGBA(0.5, 0.5, 0.5, 0.25));
    mMetButtonBg->setStrokeThickness(0.5);

    mDragDropBg = std::make_unique<DrawableRectangle>();
    mDragDropBg->setFill (Colour::fromFloatRGBA(0.5, 0.5, 0.5, 0.2));

    mFileAreaBg = std::make_unique<DrawableRectangle>();
    mFileAreaBg->setCornerSize(Point<float>(8,8));
    mFileAreaBg->setFill (Colour::fromFloatRGBA(0.04, 0.04, 0.04, 1.0));
    mFileAreaBg->setStrokeFill (Colour::fromFloatRGBA(0.5, 0.5, 0.5, 0.25));
    mFileAreaBg->setStrokeThickness(0.5);
    
    
    mMetEnableButton = std::make_unique<SonoDrawableButton>("sendmute", DrawableButton::ButtonStyle::ImageFitted);
    std::unique_ptr<Drawable> metimg(Drawable::createFromImageData(BinaryData::met_svg, BinaryData::met_svgSize));
    mMetEnableButton->setImages(metimg.get(), nullptr, nullptr, nullptr, nullptr);
    mMetEnableButton->addListener(this);
    mMetEnableButton->setClickingTogglesState(true);
    mMetEnableButton->setColour(TextButton::buttonOnColourId, Colour::fromFloatRGBA(0.2, 0.2, 0.2, 0.7));
    mMetEnableButton->setColour(TextButton::buttonColourId, Colours::transparentBlack);
    mMetEnableButton->setTooltip(TRANS("Metronome On/Off"));
    
    mMetConfigButton = std::make_unique<SonoDrawableButton>("metconf", DrawableButton::ButtonStyle::ImageFitted);
    std::unique_ptr<Drawable> metcfgimg(Drawable::createFromImageData(BinaryData::dots_svg, BinaryData::dots_svgSize));
    mMetConfigButton->setImages(metcfgimg.get(), nullptr, nullptr, nullptr, nullptr);
    mMetConfigButton->addListener(this);
    mMetConfigButton->setColour(TextButton::buttonOnColourId, Colours::transparentBlack);
    mMetConfigButton->setColour(TextButton::buttonColourId, Colours::transparentBlack);
    mMetConfigButton->setTooltip(TRANS("Metronome Options"));
    
    mMetTempoSlider     = std::make_unique<Slider>(Slider::RotaryHorizontalVerticalDrag,  Slider::TextBoxAbove);
    mMetTempoSlider->setName("mettempo");
    mMetTempoSlider->setSliderSnapsToMousePosition(false);
    mMetTempoSlider->setScrollWheelEnabled(false);
    configKnobSlider(mMetTempoSlider.get());
    mMetTempoSlider->setTextBoxStyle(Slider::TextBoxAbove, true, 80, 18);
    mMetTempoSlider->setTextBoxIsEditable(true);

    
    mMetLevelSlider     = std::make_unique<Slider>(Slider::RotaryHorizontalVerticalDrag,  Slider::NoTextBox);
    mMetLevelSlider->setName("metvol");
    mMetLevelSlider->setSliderSnapsToMousePosition(false);
    mMetLevelSlider->setScrollWheelEnabled(false);
    configKnobSlider(mMetLevelSlider.get());
    mMetLevelSlider->setTextBoxStyle(Slider::TextBoxAbove, true, 80, 18);
    mMetLevelSlider->setTextBoxIsEditable(true);

    mMetLevelSliderLabel = std::make_unique<Label>(SonobusAudioProcessor::paramDry, TRANS("Level"));
    configLabel(mMetLevelSliderLabel.get(), false);
    mMetLevelSliderLabel->setJustificationType(Justification::centred);

    mMetTempoSliderLabel = std::make_unique<Label>(SonobusAudioProcessor::paramDry, TRANS("Tempo"));
    configLabel(mMetTempoSliderLabel.get(), false);
    mMetTempoSliderLabel->setJustificationType(Justification::centred);

    mMetSendButton = std::make_unique<SonoDrawableButton>("sendmute", DrawableButton::ButtonStyle::ImageOnButtonBackground);
    std::unique_ptr<Drawable> metsendimg(Drawable::createFromImageData(BinaryData::send_group_svg, BinaryData::send_group_svgSize));
    mMetSendButton->setImages(metsendimg.get(), nullptr, nullptr, nullptr, nullptr);
    mMetSendButton->addListener(this);
    mMetSendButton->setClickingTogglesState(true);
    mMetSendButton->setColour(TextButton::buttonOnColourId, Colour::fromFloatRGBA(0.2, 0.5, 0.7, 0.65));
    mMetSendButton->setColour(TextButton::buttonColourId, Colours::transparentBlack);
    mMetSendButton->setTooltip(TRANS("Send Metronome to All"));

    
    mDrySlider     = std::make_unique<Slider>(Slider::LinearHorizontal,  Slider::TextBoxAbove);
    mDrySlider->setName("dry");
    mDrySlider->setSliderSnapsToMousePosition(processor.getSlidersSnapToMousePosition());
    mDrySlider->setScrollWheelEnabled(false);

    mOutGainSlider     = std::make_unique<Slider>(Slider::LinearHorizontal,  Slider::TextBoxRight);
    mOutGainSlider->setName("wet");
    mOutGainSlider->setSliderSnapsToMousePosition(processor.getSlidersSnapToMousePosition());
    mOutGainSlider->setScrollWheelEnabled(false);

    configLevelSlider(mInGainSlider.get());
    configLevelSlider(mDrySlider.get());
    configLevelSlider(mOutGainSlider.get());

    mOutGainSlider->setTextBoxIsEditable(true);
    mDrySlider->setTextBoxIsEditable(true);
    mInGainSlider->setTextBoxIsEditable(true);

    
    mBufferTimeSlider    = std::make_unique<Slider>(Slider::LinearBar,  Slider::TextBoxRight);
    mBufferTimeSlider->setName("time");
    mBufferTimeSlider->setSliderSnapsToMousePosition(false);
    mBufferTimeSlider->setChangeNotificationOnlyOnRelease(true);
    mBufferTimeSlider->setDoubleClickReturnValue(true, 15.0);
    mBufferTimeSlider->setTextBoxIsEditable(false);
    mBufferTimeSlider->setScrollWheelEnabled(false);

    mInGainLabel = std::make_unique<Label>(SonobusAudioProcessor::paramDry, TRANS("In Level"));
    configLabel(mInGainLabel.get(), false);
    mInGainLabel->setJustificationType(Justification::topLeft);
    mInGainLabel->setTooltip(TRANS("This reduces or boosts the level of your own audio input, and it will affect the level of your audio being sent to others and your own monitoring"));
    mInGainLabel->setInterceptsMouseClicks(true, false);
    
    mDryLabel = std::make_unique<Label>(SonobusAudioProcessor::paramDry, TRANS("Monitor"));
    configLabel(mDryLabel.get(), false);
    mDryLabel->setJustificationType(Justification::topLeft);
    mDryLabel->setTooltip(TRANS("This adjusts the level of the monitoring of your input, that only you hear"));
    mDryLabel->setInterceptsMouseClicks(true, false);

    mOutGainLabel = std::make_unique<Label>("outgain", TRANS("Out Level"));
    configLabel(mOutGainLabel.get(), false);
    mOutGainLabel->setJustificationType(Justification::topLeft);
    mOutGainLabel->setTooltip(TRANS("This is the main volume control which affects everything you hear"));
    mOutGainLabel->setInterceptsMouseClicks(true, false);

    
    int largeEditorFontsize = 16;
    int smallerEditorFontsize = 14;

#if JUCE_IOS || JUCE_ANDROID
    largeEditorFontsize = 18;
    smallerEditorFontsize = 16;
#endif
    
    mInGainAttachment     = std::make_unique<AudioProcessorValueTreeState::SliderAttachment> (p.getValueTreeState(), SonobusAudioProcessor::paramInGain, *mInGainSlider);
    mDryAttachment     = std::make_unique<AudioProcessorValueTreeState::SliderAttachment> (p.getValueTreeState(), SonobusAudioProcessor::paramDry, *mDrySlider);
    mWetAttachment     = std::make_unique<AudioProcessorValueTreeState::SliderAttachment> (p.getValueTreeState(), SonobusAudioProcessor::paramWet, *mOutGainSlider);
    mBufferTimeAttachment     = std::make_unique<AudioProcessorValueTreeState::SliderAttachment> (p.getValueTreeState(), SonobusAudioProcessor::paramDefaultNetbufMs, *mBufferTimeSlider);
    //mInMonPanMonoAttachment     = std::make_unique<AudioProcessorValueTreeState::SliderAttachment> (p.getValueTreeState(), SonobusAudioProcessor::paramInMonitorMonoPan, *mInMonPanMonoSlider);
    //mInMonPan1Attachment     = std::make_unique<AudioProcessorValueTreeState::SliderAttachment> (p.getValueTreeState(), SonobusAudioProcessor::paramInMonitorPan1, *mInMonPanSlider1);
    //mInMonPan2Attachment     = std::make_unique<AudioProcessorValueTreeState::SliderAttachment> (p.getValueTreeState(), SonobusAudioProcessor::paramInMonitorPan2, *mInMonPanSlider2);
    mMainSendMuteAttachment = std::make_unique<AudioProcessorValueTreeState::ButtonAttachment> (p.getValueTreeState(), SonobusAudioProcessor::paramMainSendMute, *mMainMuteButton);
    mMainRecvMuteAttachment = std::make_unique<AudioProcessorValueTreeState::ButtonAttachment> (p.getValueTreeState(), SonobusAudioProcessor::paramMainRecvMute, *mMainRecvMuteButton);

    mMetEnableAttachment = std::make_unique<AudioProcessorValueTreeState::ButtonAttachment> (p.getValueTreeState(), SonobusAudioProcessor::paramMetEnabled, *mMetEnableButton);
    mMetLevelAttachment     = std::make_unique<AudioProcessorValueTreeState::SliderAttachment> (p.getValueTreeState(), SonobusAudioProcessor::paramMetGain, *mMetLevelSlider);
    mMetTempoAttachment     = std::make_unique<AudioProcessorValueTreeState::SliderAttachment> (p.getValueTreeState(), SonobusAudioProcessor::paramMetTempo, *mMetTempoSlider);
    mMetSendAttachment = std::make_unique<AudioProcessorValueTreeState::ButtonAttachment> (p.getValueTreeState(), SonobusAudioProcessor::paramSendMetAudio, *mMetSendButton);
 
    
    processor.getValueTreeState().addParameterListener (SonobusAudioProcessor::paramMainSendMute, this);
    processor.getValueTreeState().addParameterListener (SonobusAudioProcessor::paramMetEnabled, this);
    processor.getValueTreeState().addParameterListener (SonobusAudioProcessor::paramMainRecvMute, this);
    processor.getValueTreeState().addParameterListener (SonobusAudioProcessor::paramSendMetAudio, this);
    processor.getValueTreeState().addParameterListener (SonobusAudioProcessor::paramSendFileAudio, this);
    processor.getValueTreeState().addParameterListener (SonobusAudioProcessor::paramHearLatencyTest, this);
    processor.getValueTreeState().addParameterListener (SonobusAudioProcessor::paramMetIsRecorded, this);
    processor.getValueTreeState().addParameterListener (SonobusAudioProcessor::paramMainReverbModel, this);
    processor.getValueTreeState().addParameterListener (SonobusAudioProcessor::paramMainReverbEnabled, this);
    processor.getValueTreeState().addParameterListener (SonobusAudioProcessor::paramSendChannels, this);
    processor.getValueTreeState().addParameterListener (SonobusAudioProcessor::paramInMonitorMonoPan, this);
    processor.getValueTreeState().addParameterListener (SonobusAudioProcessor::paramInMonitorPan1, this);
    processor.getValueTreeState().addParameterListener (SonobusAudioProcessor::paramInMonitorPan2, this);

    
    mConnectTab = std::make_unique<SonobusConnectTabbedComponent>(TabbedButtonBar::Orientation::TabsAtTop, *this);
    mConnectTab->setOutline(0);
    mConnectTab->setTabBarDepth(36);
    mConnectTab->getTabbedButtonBar().setMinimumTabScaleFactor(0.1f);
    mConnectTab->getTabbedButtonBar().setColour(TabbedButtonBar::frontTextColourId, Colour::fromFloatRGBA(0.4, 0.8, 1.0, 1.0));
    mConnectTab->getTabbedButtonBar().setColour(TabbedButtonBar::frontOutlineColourId, Colour::fromFloatRGBA(0.4, 0.8, 1.0, 0.5));


    mDirectConnectContainer = std::make_unique<Component>();
    mServerConnectContainer = std::make_unique<Component>();
    mPublicServerConnectContainer = std::make_unique<Component>();
    mServerConnectViewport = std::make_unique<Viewport>();
    mPublicServerConnectViewport = std::make_unique<Viewport>();
    mRecentsContainer = std::make_unique<Component>();

    mRecentsGroup = std::make_unique<GroupComponent>("", TRANS("RECENTS"));
    mRecentsGroup->setColour(GroupComponent::textColourId, Colour::fromFloatRGBA(0.8, 0.8, 0.8, 0.8));
    mRecentsGroup->setColour(GroupComponent::outlineColourId, Colour::fromFloatRGBA(0.8, 0.8, 0.8, 0.1));
    mRecentsGroup->setTextLabelPosition(Justification::centred);
    
    mConnectTab->addTab(TRANS("RECENTS"), Colour::fromFloatRGBA(0.1, 0.1, 0.1, 1.0), mRecentsContainer.get(), false);
    mConnectTab->addTab(TRANS("PRIVATE GROUP"), Colour::fromFloatRGBA(0.1, 0.1, 0.1, 1.0), mServerConnectViewport.get(), false);
    mConnectTab->addTab(TRANS("PUBLIC GROUPS"), Colour::fromFloatRGBA(0.1, 0.1, 0.1, 1.0), mPublicServerConnectContainer.get(), false);
    mConnectTab->addTab(TRANS("DIRECT"), Colour::fromFloatRGBA(0.1, 0.1, 0.1, 1.0), mDirectConnectContainer.get(), false);
    

    
    mLocalAddressLabel = std::make_unique<Label>("localaddr", TRANS("--"));
    mLocalAddressLabel->setJustificationType(Justification::centredLeft);
    mLocalAddressStaticLabel = std::make_unique<Label>("localaddrst", TRANS("Local Address:"));
    mLocalAddressStaticLabel->setJustificationType(Justification::centredRight);


    mRemoteAddressStaticLabel = std::make_unique<Label>("remaddrst", TRANS("Host: "));
    mRemoteAddressStaticLabel->setJustificationType(Justification::centredRight);
    mRemoteAddressStaticLabel->setWantsKeyboardFocus(true);

    mDirectConnectDescriptionLabel = std::make_unique<Label>("remaddrst", TRANS("Connect directly to other instances of SonoBus on your local network with the local address that they advertise."));
    mDirectConnectDescriptionLabel->setJustificationType(Justification::topLeft);
    
    mAddRemoteHostEditor = std::make_unique<TextEditor>("remaddredit");
    mAddRemoteHostEditor->setFont(Font(16));
    mAddRemoteHostEditor->setText(""); // 100.36.128.246:11000
    mAddRemoteHostEditor->setTextToShowWhenEmpty(TRANS("IPaddress:port"), Colour(0x44ffffff));
    
    
    mConnectButton = std::make_unique<SonoTextButton>("directconnect");
    mConnectButton->setButtonText(TRANS("Connect..."));
    mConnectButton->addListener(this);
    mConnectButton->setColour(TextButton::buttonColourId, Colour::fromFloatRGBA(0.1, 0.4, 0.6, 0.6));
    mConnectButton->setColour(TextButton::buttonOnColourId, Colour::fromFloatRGBA(0.6, 0.4, 0.6, 0.6));
    mConnectButton->setColour(SonoTextButton::outlineColourId, Colour::fromFloatRGBA(0.5, 0.5, 0.5, 0.4));
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
    configEditor(mServerHostEditor.get());
    
    mServerUsernameEditor = std::make_unique<TextEditor>("srvaddredit");
    mServerUsernameEditor->setFont(Font(16));
    mServerUsernameEditor->setText(currConnectionInfo.userName);
    configEditor(mServerUsernameEditor.get());

    mServerUserPasswordEditor = std::make_unique<TextEditor>("userpass"); // 0x25cf
    mServerUserPasswordEditor->setFont(Font(14));
    mServerUserPasswordEditor->setTextToShowWhenEmpty(TRANS("optional"), Colour(0x44ffffff));
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

    mServerHostStaticLabel = std::make_unique<Label>("localaddrst", TRANS("Connection Server:"));
    configServerLabel(mServerHostStaticLabel.get());
    

    mServerGroupEditor = std::make_unique<TextEditor>("groupedit");
    mServerGroupEditor->setFont(Font(16));
    mServerGroupEditor->setText(!currConnectionInfo.groupIsPublic ? currConnectionInfo.groupName : "");
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
    mServerGroupRandomButton->setTooltip(TRANS("Generate a random group name"));
    
    mServerCopyButton = std::make_unique<SonoDrawableButton>("copy", DrawableButton::ButtonStyle::ImageFitted);
    std::unique_ptr<Drawable> copyimg(Drawable::createFromImageData(BinaryData::copy_icon_svg, BinaryData::copy_icon_svgSize));
    mServerCopyButton->setImages(copyimg.get());
    mServerCopyButton->addListener(this);
    mServerCopyButton->setTooltip(TRANS("Copy connection information to the clipboard to share"));

    mServerPasteButton = std::make_unique<SonoDrawableButton>("paste", DrawableButton::ButtonStyle::ImageFitted);
    std::unique_ptr<Drawable> pasteimg(Drawable::createFromImageData(BinaryData::paste_icon_svg, BinaryData::paste_icon_svgSize));
    mServerPasteButton->setImages(pasteimg.get());
    mServerPasteButton->addListener(this);
    mServerPasteButton->setTooltip(TRANS("Paste connection information from the clipboard"));

    mServerShareButton = std::make_unique<SonoDrawableButton>("share", DrawableButton::ButtonStyle::ImageFitted);
    std::unique_ptr<Drawable> shareimg(Drawable::createFromImageData(BinaryData::copy_icon_svg, BinaryData::copy_icon_svgSize));
    mServerShareButton->setImages(shareimg.get());
    mServerShareButton->addListener(this);
    
    
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

    String servinfo = TRANS("The connection server is only used to help users find each other, no audio passes through it. All audio is sent directly between users (peer to peer).");
    mServerAudioInfoLabel = std::make_unique<Label>("servinfo", servinfo);
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


    mClearRecentsButton = std::make_unique<SonoTextButton>("clearrecent");
    mClearRecentsButton->setButtonText(TRANS("Clear All"));
    mClearRecentsButton->addListener(this);
    mClearRecentsButton->setTextJustification(Justification::centred);
    
    mRecentsListBox = std::make_unique<ListBox>("recentslist");
    mRecentsListBox->setColour (ListBox::outlineColourId, Colour::fromFloatRGBA(0.7, 0.7, 0.7, 0.0));
    mRecentsListBox->setColour (ListBox::backgroundColourId, Colour::fromFloatRGBA(0.1, 0.12, 0.1, 0.0f));
    mRecentsListBox->setColour (ListBox::textColourId, Colours::whitesmoke.withAlpha(0.8f));
    mRecentsListBox->setOutlineThickness (1);
#if JUCE_IOS || JUCE_ANDROID
    mRecentsListBox->getViewport()->setScrollOnDragEnabled(true);
#endif
    mRecentsListBox->getViewport()->setScrollBarsShown(true, false);
    mRecentsListBox->setMultipleSelectionEnabled (false);
    mRecentsListBox->setRowHeight(50);
    mRecentsListBox->setModel (&recentsListModel);
    mRecentsListBox->setRowSelectedOnMouseDown(true);
    mRecentsListBox->setRowClickedOnMouseDown(false);
    

    mPublicGroupsListBox = std::make_unique<ListBox>("publicgroupslist");
    mPublicGroupsListBox->setColour (ListBox::outlineColourId, Colour::fromFloatRGBA(0.7, 0.7, 0.7, 0.0));
    mPublicGroupsListBox->setColour (ListBox::backgroundColourId, Colour::fromFloatRGBA(0.1, 0.12, 0.1, 0.0f));
    mPublicGroupsListBox->setColour (ListBox::textColourId, Colours::whitesmoke.withAlpha(0.8f));
    mPublicGroupsListBox->setOutlineThickness (1);
#if JUCE_IOS || JUCE_ANDROID
    mPublicGroupsListBox->getViewport()->setScrollOnDragEnabled(true);
#endif
    mPublicGroupsListBox->getViewport()->setScrollBarsShown(true, false);
    mPublicGroupsListBox->setMultipleSelectionEnabled (false);
    mPublicGroupsListBox->setRowHeight(38);
    mPublicGroupsListBox->setModel (&publicGroupsListModel);
    mPublicGroupsListBox->setRowSelectedOnMouseDown(true);
    mPublicGroupsListBox->setRowClickedOnMouseDown(false);


    mPublicServerHostEditor = std::make_unique<TextEditor>("pubsrvaddredit");
    mPublicServerHostEditor->setFont(Font(14));
    configEditor(mPublicServerHostEditor.get());
    mPublicServerHostEditor->setTooltip(servinfo);

    mPublicServerHostStaticLabel = std::make_unique<Label>("pubaddrst", TRANS("Connection Server:"));
    configServerLabel(mPublicServerHostStaticLabel.get());
    mPublicServerHostStaticLabel->setWantsKeyboardFocus(true);

    mPublicServerUserStaticLabel = std::make_unique<Label>("localaddrst", TRANS("Your Displayed Name:"));
    configServerLabel(mPublicServerUserStaticLabel.get());
    mPublicServerUserStaticLabel->setMinimumHorizontalScale(0.8);

    mPublicServerStatusInfoLabel = std::make_unique<Label>("pubsrvinfo", "");
    configServerLabel(mPublicServerStatusInfoLabel.get());


    mPublicServerUsernameEditor = std::make_unique<TextEditor>("pubsrvaddredit");
    mPublicServerUsernameEditor->setFont(Font(16));
    mPublicServerUsernameEditor->setText(currConnectionInfo.userName);
    configEditor(mPublicServerUsernameEditor.get());

    mPublicGroupComponent = std::make_unique<GroupComponent>("", TRANS("Active Public Groups"));
    mPublicGroupComponent->setColour(GroupComponent::textColourId, Colour::fromFloatRGBA(0.8, 0.8, 0.8, 0.8));
    mPublicGroupComponent->setColour(GroupComponent::outlineColourId, Colour::fromFloatRGBA(0.8, 0.8, 0.8, 0.1));
    mPublicGroupComponent->setTextLabelPosition(Justification::centred);

    mPublicServerInfoStaticLabel = std::make_unique<Label>("pubinfo", TRANS("Select existing group below OR "));
    configServerLabel(mPublicServerInfoStaticLabel.get());
    mPublicServerInfoStaticLabel->setFont(16);
    mPublicServerInfoStaticLabel->setMinimumHorizontalScale(0.8);

    mPublicServerAddGroupButton = std::make_unique<TextButton>("addgroup");
    mPublicServerAddGroupButton->setButtonText(TRANS("Create Group..."));
    mPublicServerAddGroupButton->addListener(this);
    mPublicServerAddGroupButton->setColour(TextButton::buttonColourId, Colour::fromFloatRGBA(0.1, 0.4, 0.6, 0.6));
    mPublicServerAddGroupButton->setWantsKeyboardFocus(true);

    mPublicServerGroupEditor = std::make_unique<TextEditor>("pubgroupedit");
    mPublicServerGroupEditor->setFont(Font(16));
    mPublicServerGroupEditor->setText(currConnectionInfo.groupIsPublic ? currConnectionInfo.groupName : "");
    configEditor(mPublicServerGroupEditor.get());
    mPublicServerGroupEditor->setTextToShowWhenEmpty(TRANS("enter group name"), Colour(0x44ffffff));
    mPublicServerGroupEditor->setTooltip(TRANS("Choose a descriptive group name that includes geographic information and genre"));


    mPatchbayButton = std::make_unique<TextButton>("patch");
    mPatchbayButton->setButtonText(TRANS("Patchbay"));
    mPatchbayButton->addListener(this);

    mSetupAudioButton = std::make_unique<SonoTextButton>("directconnect");
    mSetupAudioButton->setButtonText(TRANS("Setup Audio"));
    mSetupAudioButton->addListener(this);
    mSetupAudioButton->setTextJustification(Justification::centred);

    
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

    mMainLinkButton = std::make_unique<SonoDrawableButton>("link",  DrawableButton::ButtonStyle::ImageFitted);
    mMainLinkButton->addListener(this);
    mMainLinkButton->setTooltip(TRANS("Press to copy/share link to group"));
    
    mPeerContainer = std::make_unique<PeersContainerView>(processor);
    
    mPeerViewport = std::make_unique<Viewport>();
    mPeerViewport->setViewedComponent(mPeerContainer.get(), false);
    
    mRecOptionsComponent = std::make_unique<Component>();
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
    mOptionsAutosizeDefaultChoice->addItem(TRANS("Manual"), SonobusAudioProcessor::AutoNetBufferModeOff);
    mOptionsAutosizeDefaultChoice->addItem(TRANS("Auto Up"), SonobusAudioProcessor::AutoNetBufferModeAutoIncreaseOnly);
    mOptionsAutosizeDefaultChoice->addItem(TRANS("Auto"), SonobusAudioProcessor::AutoNetBufferModeAutoFull);
    mOptionsAutosizeDefaultChoice->addItem(TRANS("Initial Auto"), SonobusAudioProcessor::AutoNetBufferModeInitAuto);
    
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

    mOptionsHearLatencyButton = std::make_unique<ToggleButton>(TRANS("Make Latency Test Audible"));
    mOptionsHearLatencyButton->addListener(this);
    mHearLatencyTestAttachment = std::make_unique<AudioProcessorValueTreeState::ButtonAttachment> (p.getValueTreeState(), SonobusAudioProcessor::paramHearLatencyTest, *mOptionsHearLatencyButton);

    mOptionsMetRecordedButton = std::make_unique<ToggleButton>(TRANS("Metronome output recorded in full mix"));
    mOptionsMetRecordedButton->addListener(this);
    mMetRecordedAttachment = std::make_unique<AudioProcessorValueTreeState::ButtonAttachment> (p.getValueTreeState(), SonobusAudioProcessor::paramMetIsRecorded, *mOptionsMetRecordedButton);

    
    mOptionsRecFilesStaticLabel = std::make_unique<Label>("", TRANS("Record feature creates the following files:"));
    configLabel(mOptionsRecFilesStaticLabel.get(), false);
    mOptionsRecFilesStaticLabel->setJustificationType(Justification::centredLeft);

    mOptionsRecMixButton = std::make_unique<ToggleButton>(TRANS("Full Mix"));
    mOptionsRecMixButton->addListener(this);

    mOptionsRecMixMinusButton = std::make_unique<ToggleButton>(TRANS("Full Mix without yourself"));
    mOptionsRecMixMinusButton->addListener(this);

    mOptionsRecSelfButton = std::make_unique<ToggleButton>(TRANS("Yourself"));
    mOptionsRecSelfButton->addListener(this);

    mOptionsRecOthersButton = std::make_unique<ToggleButton>(TRANS("Each Connected User"));
    mOptionsRecOthersButton->addListener(this);

    mRecFormatChoice = std::make_unique<SonoChoiceButton>();
    mRecFormatChoice->addChoiceListener(this);
    mRecFormatChoice->addItem(TRANS("FLAC"), SonobusAudioProcessor::FileFormatFLAC);
    mRecFormatChoice->addItem(TRANS("WAV"), SonobusAudioProcessor::FileFormatWAV);
    mRecFormatChoice->addItem(TRANS("OGG"), SonobusAudioProcessor::FileFormatOGG);

    mRecBitsChoice = std::make_unique<SonoChoiceButton>();
    mRecBitsChoice->addChoiceListener(this);
    mRecBitsChoice->addItem(TRANS("16 bit"), 16);
    mRecBitsChoice->addItem(TRANS("24 bit"), 24);


    mRecFormatStaticLabel = std::make_unique<Label>("", TRANS("Audio File Format:"));
    configLabel(mRecFormatStaticLabel.get(), false);
    mRecFormatStaticLabel->setJustificationType(Justification::centredRight);


    mRecLocationStaticLabel = std::make_unique<Label>("", TRANS("Record Location:"));
    configLabel(mRecLocationStaticLabel.get(), false);
    mRecLocationStaticLabel->setJustificationType(Justification::centredRight);

    mRecLocationButton = std::make_unique<TextButton>("fileloc");
    mRecLocationButton->setButtonText("");
    mRecLocationButton->setLookAndFeel(&smallLNF);
    mRecLocationButton->addListener(this);


    
    
    mOptionsUseSpecificUdpPortButton = std::make_unique<ToggleButton>(TRANS("Use Specific UDP Port"));
    mOptionsUseSpecificUdpPortButton->addListener(this);

    mOptionsDynamicResamplingButton = std::make_unique<ToggleButton>(TRANS("Use Drift Correction"));
    mDynamicResamplingAttachment = std::make_unique<AudioProcessorValueTreeState::ButtonAttachment> (p.getValueTreeState(), SonobusAudioProcessor::paramDynamicResampling, *mOptionsDynamicResamplingButton);

    mOptionsAutoReconnectButton = std::make_unique<ToggleButton>(TRANS("Auto-Reconnect to Last Group"));
    mAutoReconnectAttachment = std::make_unique<AudioProcessorValueTreeState::ButtonAttachment> (p.getValueTreeState(), SonobusAudioProcessor::paramAutoReconnectLast, *mOptionsAutoReconnectButton);

    mOptionsOverrideSamplerateButton = std::make_unique<ToggleButton>(TRANS("Override Device Sample Rate"));
    mOptionsOverrideSamplerateButton->addListener(this);

    mOptionsShouldCheckForUpdateButton = std::make_unique<ToggleButton>(TRANS("Automatically check for updates"));
    mOptionsShouldCheckForUpdateButton->addListener(this);

    mOptionsSliderSnapToMouseButton = std::make_unique<ToggleButton>(TRANS("Sliders Snap to Click Position"));
    mOptionsSliderSnapToMouseButton->addListener(this);


    mOptionsInputLimiterButton = std::make_unique<ToggleButton>(TRANS("Use Input FX Limiter"));
    mOptionsInputLimiterButton->addListener(this);

    mOptionsChangeAllFormatButton = std::make_unique<ToggleButton>(TRANS("Change all connected"));
    mOptionsChangeAllFormatButton->addListener(this);
    mOptionsChangeAllFormatButton->setLookAndFeel(&smallLNF);

    mOptionsUdpPortEditor = std::make_unique<TextEditor>("udp");
    mOptionsUdpPortEditor->addListener(this);
    mOptionsUdpPortEditor->setFont(Font(16));
    mOptionsUdpPortEditor->setText(""); // 100.36.128.246:11000
    mOptionsUdpPortEditor->setKeyboardType(TextEditor::numericKeyboard);
    mOptionsUdpPortEditor->setInputRestrictions(5, "0123456789");
    
    configEditor(mOptionsUdpPortEditor.get());

#if JUCE_IOS
    mVersionLabel = std::make_unique<Label>("", TRANS("Version: ") + String(SONOBUS_BUILD_VERSION)); // temporary
#else
    mVersionLabel = std::make_unique<Label>("", TRANS("Version: ") + ProjectInfo::versionString);
#endif
    configLabel(mVersionLabel.get(), true);
    mVersionLabel->setJustificationType(Justification::centredRight);


    // effects
    
    mEffectsButton = std::make_unique<TextButton>("mainfx");
    mEffectsButton->setButtonText(TRANS("FX"));
    mEffectsButton->setLookAndFeel(&smallLNF);
    mEffectsButton->addListener(this);
    mEffectsButton->setColour(TextButton::buttonOnColourId, Colour::fromFloatRGBA(0.2, 0.5, 0.7, 0.5));

    mInEffectsButton = std::make_unique<TextButton>("mainfx");
    mInEffectsButton->setButtonText(TRANS("In FX"));
    mInEffectsButton->setLookAndFeel(&smallLNF);
    mInEffectsButton->addListener(this);
    mInEffectsButton->setColour(TextButton::buttonOnColourId, Colour::fromFloatRGBA(0.2, 0.5, 0.7, 0.5));
    
    mSendChannelsChoice = std::make_unique<SonoChoiceButton>();
    mSendChannelsChoice->addChoiceListener(this);
    mSendChannelsChoice->addItem(TRANS("Match # Inputs"), 0);
    mSendChannelsChoice->addItem(TRANS("Send Mono"), 1);
    mSendChannelsChoice->addItem(TRANS("Send Stereo"), 2);
    mSendChannelsChoice->setLookAndFeel(&smallLNF);

    mSendChannelsLabel = std::make_unique<Label>("sendch", TRANS("# Send Channels"));
    configLabel(mSendChannelsLabel.get(), true);
    mSendChannelsLabel->setJustificationType(Justification::centred);


    mInEffectsConcertina =  std::make_unique<ConcertinaPanel>();

    
    mInCompressorView =  std::make_unique<CompressorView>();
    mInCompressorView->addListener(this);
    mInCompressorView->addHeaderListener(this);

    
    mCompressorBg = std::make_unique<DrawableRectangle>();
    mCompressorBg->setCornerSize(Point<float>(6,6));
    mCompressorBg->setFill (Colour::fromFloatRGBA(0.07, 0.07, 0.07, 1.0));
    mCompressorBg->setStrokeFill (Colour::fromFloatRGBA(0.5, 0.5, 0.5, 0.25));
    mCompressorBg->setStrokeThickness(0.5);

    mInExpanderView =  std::make_unique<ExpanderView>();
    mInExpanderView->addListener(this);
    mInExpanderView->addHeaderListener(this);
    
    mExpanderBg = std::make_unique<DrawableRectangle>();
    mExpanderBg->setCornerSize(Point<float>(6,6));
    mExpanderBg->setFill (Colour::fromFloatRGBA(0.07, 0.07, 0.07, 1.0));
    mExpanderBg->setStrokeFill (Colour::fromFloatRGBA(0.5, 0.5, 0.5, 0.25));
    mExpanderBg->setStrokeThickness(0.5);

    mInEqView =  std::make_unique<ParametricEqView>();
    mInEqView->addListener(this);
    mInEqView->addHeaderListener(this);

    
    mInEffectsConcertina->addPanel(-1, mInExpanderView.get(), false);    
    mInEffectsConcertina->addPanel(-1, mInCompressorView.get(), false);
    mInEffectsConcertina->addPanel(-1, mInEqView.get(), false);
    
    mInEffectsConcertina->setCustomPanelHeader(mInCompressorView.get(), mInCompressorView->getHeaderComponent(), false);
    mInEffectsConcertina->setCustomPanelHeader(mInExpanderView.get(), mInExpanderView->getHeaderComponent(), false);
    mInEffectsConcertina->setCustomPanelHeader(mInEqView.get(), mInEqView->getHeaderComponent(), false);

    
    mReverbHeaderBg = std::make_unique<DrawableRectangle>();
    mReverbHeaderBg->setCornerSize(Point<float>(6,6));
    mReverbHeaderBg->setFill (Colour::fromFloatRGBA(0.07, 0.07, 0.07, 1.0));
    mReverbHeaderBg->setStrokeFill (Colour::fromFloatRGBA(0.5, 0.5, 0.5, 0.25));
    mReverbHeaderBg->setStrokeThickness(0.5);

    
    mReverbLevelLabel = std::make_unique<Label>(SonobusAudioProcessor::paramMainReverbLevel, TRANS("Level"));
    configLabel(mReverbLevelLabel.get(), false);
    mReverbLevelLabel->setJustificationType(Justification::centred);

    mReverbSizeLabel = std::make_unique<Label>(SonobusAudioProcessor::paramMainReverbSize, TRANS("Size"));
    configLabel(mReverbSizeLabel.get(), false);
    mReverbSizeLabel->setJustificationType(Justification::centred);

    mReverbDampingLabel = std::make_unique<Label>(SonobusAudioProcessor::paramMainReverbDamping, TRANS("Damping"));
    configLabel(mReverbDampingLabel.get(), false);
    mReverbDampingLabel->setJustificationType(Justification::centred);

    mReverbPreDelayLabel = std::make_unique<Label>(SonobusAudioProcessor::paramMainReverbDamping, TRANS("Pre-Delay"));
    configLabel(mReverbPreDelayLabel.get(), false);
    mReverbPreDelayLabel->setJustificationType(Justification::centred);


    mReverbTitleLabel = std::make_unique<Label>("revtitle", TRANS("Reverb"));
    mReverbTitleLabel->setJustificationType(Justification::centredLeft);
    mReverbTitleLabel->setInterceptsMouseClicks(true, false);
    mReverbTitleLabel->addMouseListener(this, false);
    
    mReverbEnabledButton = std::make_unique<SonoDrawableButton>("reven", DrawableButton::ButtonStyle::ImageFitted);
    std::unique_ptr<Drawable> powerimg(Drawable::createFromImageData(BinaryData::power_svg, BinaryData::power_svgSize));
    std::unique_ptr<Drawable> powerselimg(Drawable::createFromImageData(BinaryData::power_sel_svg, BinaryData::power_sel_svgSize));
    mReverbEnabledButton->setImages(powerimg.get(), nullptr, nullptr, nullptr, powerselimg.get());
    mReverbEnabledButton->setClickingTogglesState(true);
    mReverbEnabledButton->setColour(TextButton::buttonColourId, Colours::transparentBlack);
    mReverbEnabledButton->setColour(TextButton::buttonOnColourId, Colours::transparentBlack);
    mReverbEnabledButton->setColour(DrawableButton::backgroundColourId, Colours::transparentBlack);
    mReverbEnabledButton->setColour(DrawableButton::backgroundOnColourId, Colours::transparentBlack);    
    mReverbEnabledButton->addListener(this);
    mReverbEnableAttachment = std::make_unique<AudioProcessorValueTreeState::ButtonAttachment> (p.getValueTreeState(), SonobusAudioProcessor::paramMainReverbEnabled, *mReverbEnabledButton);


    mReverbModelChoice = std::make_unique<SonoChoiceButton>();
    mReverbModelChoice->setColour(SonoTextButton::outlineColourId, Colour::fromFloatRGBA(0.6, 0.6, 0.6, 0.4));
    mReverbModelChoice->addChoiceListener(this);
    mReverbModelChoice->addItem(TRANS("Freeverb"), SonobusAudioProcessor::ReverbModelFreeverb);
    mReverbModelChoice->addItem(TRANS("MVerb"), SonobusAudioProcessor::ReverbModelMVerb);
    mReverbModelChoice->addItem(TRANS("Zita"), SonobusAudioProcessor::ReverbModelZita);

    
    mReverbSizeSlider     = std::make_unique<Slider>(Slider::RotaryHorizontalVerticalDrag,  Slider::NoTextBox);
    mReverbSizeSlider->setName("revsize");
    mReverbSizeSlider->setSliderSnapsToMousePosition(false);
    mReverbSizeSlider->setScrollWheelEnabled(false);
    configKnobSlider(mReverbSizeSlider.get());
    mReverbSizeSlider->setTextBoxStyle(Slider::TextBoxAbove, true, 80, 18);
    mReverbSizeSlider->setTextBoxIsEditable(true);
    
    mReverbSizeAttachment = std::make_unique<AudioProcessorValueTreeState::SliderAttachment> (p.getValueTreeState(), SonobusAudioProcessor::paramMainReverbSize, *mReverbSizeSlider);

    mReverbLevelSlider     = std::make_unique<Slider>(Slider::RotaryHorizontalVerticalDrag,  Slider::NoTextBox);
    mReverbLevelSlider->setName("revsize");
    mReverbLevelSlider->setSliderSnapsToMousePosition(false);
    mReverbLevelSlider->setScrollWheelEnabled(false);
    configKnobSlider(mReverbLevelSlider.get());
    mReverbLevelSlider->setTextBoxStyle(Slider::TextBoxAbove, true, 80, 18);
    mReverbLevelSlider->setTextBoxIsEditable(true);

    mReverbLevelAttachment = std::make_unique<AudioProcessorValueTreeState::SliderAttachment> (p.getValueTreeState(), SonobusAudioProcessor::paramMainReverbLevel, *mReverbLevelSlider);

    mReverbDampingSlider     = std::make_unique<Slider>(Slider::RotaryHorizontalVerticalDrag,  Slider::NoTextBox);
    mReverbDampingSlider->setName("revsize");
    mReverbDampingSlider->setSliderSnapsToMousePosition(false);
    mReverbDampingSlider->setScrollWheelEnabled(false);
    configKnobSlider(mReverbDampingSlider.get());
    mReverbDampingSlider->setTextBoxStyle(Slider::TextBoxAbove, true, 80, 18);
    mReverbDampingSlider->setTextBoxIsEditable(true);
    
    mReverbDampingAttachment = std::make_unique<AudioProcessorValueTreeState::SliderAttachment> (p.getValueTreeState(), SonobusAudioProcessor::paramMainReverbDamping, *mReverbDampingSlider);

    mReverbPreDelaySlider     = std::make_unique<Slider>(Slider::RotaryHorizontalVerticalDrag,  Slider::NoTextBox);
    mReverbPreDelaySlider->setName("revsize");
    mReverbPreDelaySlider->setSliderSnapsToMousePosition(false);
    mReverbPreDelaySlider->setScrollWheelEnabled(false);
    configKnobSlider(mReverbPreDelaySlider.get());
    mReverbPreDelaySlider->setTextBoxStyle(Slider::TextBoxAbove, true, 80, 18);
    mReverbPreDelaySlider->setTextBoxIsEditable(true);
    
    mReverbPreDelayAttachment = std::make_unique<AudioProcessorValueTreeState::SliderAttachment> (p.getValueTreeState(), SonobusAudioProcessor::paramMainReverbPreDelay, *mReverbPreDelaySlider);

    
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
    
    {
        mRecordingButton = std::make_unique<SonoDrawableButton>("record", DrawableButton::ButtonStyle::ImageFitted);
        std::unique_ptr<Drawable> recimg(Drawable::createFromImageData(BinaryData::record_svg, BinaryData::record_svgSize));
        std::unique_ptr<Drawable> recselimg(Drawable::createFromImageData(BinaryData::record_active_alt_svg, BinaryData::record_active_alt_svgSize));
        mRecordingButton->setImages(recimg.get(), nullptr, nullptr, nullptr, recselimg.get());
        mRecordingButton->addListener(this);
        mRecordingButton->setColour(DrawableButton::backgroundOnColourId, Colours::transparentBlack);
        mRecordingButton->setTooltip(TRANS("Start/Stop recording audio to file"));
        
        mFileRecordingLabel = std::make_unique<Label>("rectime", "");
        mFileRecordingLabel->setJustificationType(Justification::centredBottom);
        mFileRecordingLabel->setFont(12);
        mFileRecordingLabel->setColour(Label::textColourId, Colour(0x88ffbbbb));
        
        mFileBrowseButton = std::make_unique<SonoDrawableButton>("browse", DrawableButton::ButtonStyle::ImageFitted);
        std::unique_ptr<Drawable> folderimg(Drawable::createFromImageData(BinaryData::folder_icon_svg, BinaryData::folder_icon_svgSize));
        mFileBrowseButton->setImages(folderimg.get());
        mFileBrowseButton->addListener(this);
        mFileBrowseButton->setColour(DrawableButton::backgroundColourId, Colours::transparentBlack);
        mFileBrowseButton->setTooltip(TRANS("Load audio file for playback"));

        mPlayButton = std::make_unique<SonoDrawableButton>("play", DrawableButton::ButtonStyle::ImageFitted);
        std::unique_ptr<Drawable> playimg(Drawable::createFromImageData(BinaryData::play_icon_svg, BinaryData::play_icon_svgSize));
        std::unique_ptr<Drawable> pauseimg(Drawable::createFromImageData(BinaryData::pause_icon_svg, BinaryData::pause_icon_svgSize));
        mPlayButton->setImages(playimg.get(), nullptr, nullptr, nullptr, pauseimg.get());
        mPlayButton->setClickingTogglesState(true);
        mPlayButton->addListener(this);
        mPlayButton->setColour(DrawableButton::backgroundOnColourId, Colours::transparentBlack);
        mPlayButton->setColour(DrawableButton::backgroundColourId, Colours::transparentBlack);

        mLoopButton = std::make_unique<SonoDrawableButton>("play", DrawableButton::ButtonStyle::ImageFitted);
        std::unique_ptr<Drawable> loopimg(Drawable::createFromImageData(BinaryData::loop_icon_svg, BinaryData::loop_icon_svgSize));
        mLoopButton->setImages(loopimg.get(), nullptr, nullptr, nullptr, nullptr);
        mLoopButton->setClickingTogglesState(true);
        mLoopButton->addListener(this);
        mLoopButton->setColour(DrawableButton::backgroundOnColourId, Colour::fromFloatRGBA(0.6, 0.3, 0.6, 0.5));
        mLoopButton->setColour(DrawableButton::backgroundColourId, Colours::transparentBlack);

        mDismissTransportButton = std::make_unique<SonoDrawableButton>("x", DrawableButton::ButtonStyle::ImageFitted);
        std::unique_ptr<Drawable> ximg(Drawable::createFromImageData(BinaryData::x_icon_svg, BinaryData::x_icon_svgSize));
        mDismissTransportButton->setImages(ximg.get());
        mDismissTransportButton->addListener(this);
        mDismissTransportButton->setColour(DrawableButton::backgroundColourId, Colours::transparentBlack);

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
        mFileSendAudioButton->setImages(filesendimg.get(), nullptr, nullptr, nullptr, nullptr);
        mFileSendAudioButton->addListener(this);
        mFileSendAudioButton->setClickingTogglesState(true);
        mFileSendAudioButton->setColour(DrawableButton::backgroundOnColourId, Colour::fromFloatRGBA(0.2, 0.5, 0.7, 0.65));
        mFileSendAudioButton->setColour(TextButton::buttonColourId, Colours::transparentBlack);
        mFileSendAudioButton->setTooltip(TRANS("Send File Playback to All"));

        mFileSendAttachment = std::make_unique<AudioProcessorValueTreeState::ButtonAttachment> (p.getValueTreeState(), SonobusAudioProcessor::paramSendFileAudio, *mFileSendAudioButton);

        mFileMenuButton = std::make_unique<SonoDrawableButton>("filemen", DrawableButton::ButtonStyle::ImageFitted);
        std::unique_ptr<Drawable> fmenuimg(Drawable::createFromImageData(BinaryData::dots_svg, BinaryData::dots_svgSize));
        mFileMenuButton->setImages(fmenuimg.get(), nullptr, nullptr, nullptr, nullptr);
        mFileMenuButton->addListener(this);
        mFileMenuButton->setColour(TextButton::buttonOnColourId, Colours::transparentBlack);
        mFileMenuButton->setColour(TextButton::buttonColourId, Colours::transparentBlack);

        
    }
    
#if JUCE_IOS || JUCE_ANDROID
    mPeerViewport->setScrollOnDragEnabled(true);
#endif
    
    mOptionsComponent->addAndMakeVisible(mBufferTimeSlider.get());
    mOptionsComponent->addAndMakeVisible(mOptionsAutosizeDefaultChoice.get());
    mOptionsComponent->addAndMakeVisible(mOptionsFormatChoiceDefaultChoice.get());
    mOptionsComponent->addAndMakeVisible(mOptionsAutosizeStaticLabel.get());
    mOptionsComponent->addAndMakeVisible(mOptionsFormatChoiceStaticLabel.get());
    mOptionsComponent->addAndMakeVisible(mOptionsHearLatencyButton.get());
    mOptionsComponent->addAndMakeVisible(mOptionsUdpPortEditor.get());
    mOptionsComponent->addAndMakeVisible(mOptionsUseSpecificUdpPortButton.get());
    mOptionsComponent->addAndMakeVisible(mOptionsDynamicResamplingButton.get());
    mOptionsComponent->addAndMakeVisible(mOptionsAutoReconnectButton.get());
    mOptionsComponent->addAndMakeVisible(mOptionsInputLimiterButton.get());
    mOptionsComponent->addAndMakeVisible(mOptionsChangeAllFormatButton.get());
    mOptionsComponent->addAndMakeVisible(mVersionLabel.get());
    //mOptionsComponent->addAndMakeVisible(mTitleImage.get());
    if (JUCEApplicationBase::isStandaloneApp()) {
        mOptionsComponent->addAndMakeVisible(mOptionsOverrideSamplerateButton.get());
        mOptionsComponent->addAndMakeVisible(mOptionsShouldCheckForUpdateButton.get());
    }
    mOptionsComponent->addAndMakeVisible(mOptionsSliderSnapToMouseButton.get());



    mRecOptionsComponent->addAndMakeVisible(mOptionsMetRecordedButton.get());
    mRecOptionsComponent->addAndMakeVisible(mOptionsRecFilesStaticLabel.get());
    mRecOptionsComponent->addAndMakeVisible(mOptionsRecMixButton.get());
    mRecOptionsComponent->addAndMakeVisible(mOptionsRecSelfButton.get());
    mRecOptionsComponent->addAndMakeVisible(mOptionsRecMixMinusButton.get());
    mRecOptionsComponent->addAndMakeVisible(mOptionsRecOthersButton.get());
    mRecOptionsComponent->addAndMakeVisible(mRecFormatChoice.get());
    mRecOptionsComponent->addAndMakeVisible(mRecBitsChoice.get());
    mRecOptionsComponent->addAndMakeVisible(mRecFormatStaticLabel.get());
    mRecOptionsComponent->addAndMakeVisible(mRecLocationButton.get());
    mRecOptionsComponent->addAndMakeVisible(mRecLocationStaticLabel.get());

    
    addAndMakeVisible(mPeerViewport.get());
    addAndMakeVisible(mTitleLabel.get());
    //addAndMakeVisible(mTitleImage.get());

    addAndMakeVisible(mMainLinkButton.get());
    addAndMakeVisible(mMainGroupLabel.get());
    addAndMakeVisible(mMainPeerLabel.get());
    addAndMakeVisible(mMainMessageLabel.get());
    addAndMakeVisible(mMainUserLabel.get());
    addAndMakeVisible(mMainPersonImage.get());
    addAndMakeVisible(mMainGroupImage.get());

    addAndMakeVisible(mDrySlider.get());
    addAndMakeVisible(mOutGainSlider.get());
    addAndMakeVisible(mInGainSlider.get());
    addAndMakeVisible(mMainMuteButton.get());
    addAndMakeVisible(mMainRecvMuteButton.get());
    addAndMakeVisible(mMainPushToTalkButton.get());
    addAndMakeVisible(mInMuteButton.get());
    addAndMakeVisible(mInSoloButton.get());
    //addAndMakeVisible(mInMonPanStereoSlider.get());
    //addAndMakeVisible(mInMonPanMonoSlider.get());
    
    addAndMakeVisible(mPatchbayButton.get());
    addAndMakeVisible(mConnectButton.get());
    addAndMakeVisible(mMainStatusLabel.get());
    addAndMakeVisible(mConnectionTimeLabel.get());

    addChildComponent(mSetupAudioButton.get());

    
    addAndMakeVisible(mMetButtonBg.get());
    addAndMakeVisible(mFileAreaBg.get());
    addAndMakeVisible(mMetEnableButton.get());
    addAndMakeVisible(mMetConfigButton.get());
    addAndMakeVisible(mEffectsButton.get());
    addAndMakeVisible(mInEffectsButton.get());

    
    mMetContainer->addAndMakeVisible(mMetLevelSlider.get());
    mMetContainer->addAndMakeVisible(mMetTempoSlider.get());
    mMetContainer->addAndMakeVisible(mMetLevelSliderLabel.get());
    mMetContainer->addAndMakeVisible(mMetTempoSliderLabel.get());
    mMetContainer->addAndMakeVisible(mMetSendButton.get());

    mEffectsContainer->addAndMakeVisible(mReverbHeaderBg.get());
    mEffectsContainer->addAndMakeVisible(mReverbTitleLabel.get());
    mEffectsContainer->addAndMakeVisible(mReverbEnabledButton.get());
    mEffectsContainer->addAndMakeVisible(mReverbModelChoice.get());
    mEffectsContainer->addAndMakeVisible(mReverbLevelSlider.get());
    mEffectsContainer->addAndMakeVisible(mReverbSizeSlider.get());
    mEffectsContainer->addAndMakeVisible(mReverbLevelLabel.get());
    mEffectsContainer->addAndMakeVisible(mReverbSizeLabel.get());
    mEffectsContainer->addAndMakeVisible(mReverbDampingLabel.get());
    mEffectsContainer->addAndMakeVisible(mReverbDampingSlider.get());
    mEffectsContainer->addAndMakeVisible(mReverbPreDelayLabel.get());
    mEffectsContainer->addAndMakeVisible(mReverbPreDelaySlider.get());
    
    
    
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
        addAndMakeVisible(mFileMenuButton.get());
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
#if ! (JUCE_IOS || JUCE_ANDROID)
    mServerConnectContainer->addAndMakeVisible(mServerPasteButton.get());
    mServerConnectContainer->addAndMakeVisible(mServerCopyButton.get());
#else
    mServerConnectContainer->addAndMakeVisible(mServerShareButton.get());    
#endif
    mServerConnectContainer->addAndMakeVisible(mServerGroupStaticLabel.get());
    mServerConnectContainer->addAndMakeVisible(mServerHostStaticLabel.get());
    mServerConnectContainer->addAndMakeVisible(mServerGroupPassStaticLabel.get());
    mServerConnectContainer->addAndMakeVisible(mServerGroupPasswordEditor.get());
    mServerConnectContainer->addAndMakeVisible(mServerStatusLabel.get());
    mServerConnectContainer->addAndMakeVisible(mServerInfoLabel.get());
    mServerConnectContainer->addAndMakeVisible(mServerAudioInfoLabel.get());

    mRecentsContainer->addAndMakeVisible(mRecentsListBox.get());
    mRecentsContainer->addAndMakeVisible(mClearRecentsButton.get());

    mPublicServerConnectContainer->addAndMakeVisible(mPublicGroupComponent.get());
    mPublicGroupComponent->addAndMakeVisible(mPublicGroupsListBox.get());

    mPublicServerConnectContainer->addAndMakeVisible(mPublicServerHostEditor.get());
    mPublicServerConnectContainer->addAndMakeVisible(mPublicServerHostStaticLabel.get());
    mPublicServerConnectContainer->addAndMakeVisible(mPublicServerUserStaticLabel.get());
    mPublicServerConnectContainer->addAndMakeVisible(mPublicServerUsernameEditor.get());
    mPublicServerConnectContainer->addAndMakeVisible(mPublicServerAddGroupButton.get());
    mPublicServerConnectContainer->addAndMakeVisible(mPublicServerInfoStaticLabel.get());
    mPublicServerConnectContainer->addAndMakeVisible(mPublicServerStatusInfoLabel.get());
    mPublicServerConnectContainer->addAndMakeVisible(mPublicServerGroupEditor.get());


    mConnectComponent->addAndMakeVisible(mConnectComponentBg.get());
    mConnectComponent->addAndMakeVisible(mConnectTab.get());
    mConnectComponent->addAndMakeVisible(mConnectTitle.get());
    mConnectComponent->addAndMakeVisible(mConnectCloseButton.get());

    addAndMakeVisible(mInGainLabel.get());
    addAndMakeVisible(mDryLabel.get());
    addAndMakeVisible(mOutGainLabel.get());
    addAndMakeVisible(inputMeter.get());
    addAndMakeVisible(outputMeter.get());
    addAndMakeVisible (mSettingsButton.get());
    addAndMakeVisible (mPanButton.get());

    addChildComponent(mConnectComponent.get());

    // over everything
    addChildComponent(mDragDropBg.get());

    mServerConnectViewport->setViewedComponent(mServerConnectContainer.get());

    //mPublicServerConnectViewport->setViewedComponent(mPublicServerConnectContainer.get());


    inPannersContainer->addAndMakeVisible (mInMonPanLabel1.get());
    inPannersContainer->addAndMakeVisible (mInMonPanLabel2.get());
    inPannersContainer->addAndMakeVisible (mInMonPanMonoSlider.get());
    inPannersContainer->addAndMakeVisible (mInMonPanStereoSlider.get());
    //inPannersContainer->addAndMakeVisible (mInMonPanSlider1.get());
    //inPannersContainer->addAndMakeVisible (mInMonPanSlider2.get());
    inPannersContainer->addAndMakeVisible (mSendChannelsChoice.get());
    inPannersContainer->addAndMakeVisible (mSendChannelsLabel.get());
    
    
    mInEffectsContainer->addAndMakeVisible (mInEffectsConcertina.get());
    
    inChannels = processor.getTotalNumInputChannels();
    outChannels = processor.getTotalNumOutputChannels();
    
    updateLayout();
    
    updateState();

    updateServerFieldsFromConnectionInfo();
    
    //setResizeLimits(400, 300, 2000, 1000);

    commandManager.registerAllCommandsForTarget (this);

    if (JUCEApplicationBase::isStandaloneApp()) {
#if !(JUCE_IOS || JUCE_ANDROID)
        processor.startAooServer();
#endif
        setResizable(true, false);


        commandManager.registerAllCommandsForTarget (JUCEApplication::getInstance());

        menuBarModel = std::make_unique<SonobusMenuBarModel>(*this);

        menuBarModel->setApplicationCommandManagerToWatch(&commandManager);

        
#if JUCE_MAC
        auto extraAppleMenuItems = PopupMenu();
        extraAppleMenuItems.addCommandItem(&commandManager, SonobusCommands::ShowOptions);
        
        MenuBarModel::setMacMainMenu(menuBarModel.get(), &extraAppleMenuItems);
#endif

#if (JUCE_WINDOWS || JUCE_LINUX)
        mMenuBar = std::make_unique<MenuBarComponent>(menuBarModel.get());
        addAndMakeVisible(mMenuBar.get());
#endif
        
        // look for link in clipboard        
        if (attemptToPasteConnectionFromClipboard()) {
            // show connect immediately
            showConnectPopup(true);
            // switch to group page
            mConnectTab->setCurrentTabIndex(mConnectTab->getNumTabs() > 3 ? 1 : 0);

            updateServerStatusLabel(TRANS("Filled in Group information from clipboard! Press 'Connect to Group' to join..."), false);
        }
        
        
        // check for update!
        // LatestVersionCheckerAndUpdater::getInstance()->checkForNewVersion (false);
        
    } else  {
        setResizable(true, true);    
    }
    
    processor.addClientListener(this);
    processor.getTransportSource().addChangeListener (this);

    std::istringstream gramstream(std::string(BinaryData::wordmaker_g, BinaryData::wordmaker_gSize));
    mRandomSentence = std::make_unique<RandomSentenceGenerator>(gramstream);
    mRandomSentence->capEveryWord = true;
    

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

#if (JUCE_WINDOWS || JUCE_MAC)
    if (JUCEApplicationBase::isStandaloneApp()) {
        startTimer(CheckForNewVersionTimerId, 5000);
    }
#endif

   // Make sure that before the constructor has finished, you've set the
   // editor's size to whatever you need it to be.

    auto defHeight = 440;
#if JUCE_WINDOWS
    defHeight = 470;
#endif
    
    setSize (760, defHeight);


    if (processor.getAutoReconnectToLast()) {
        updateRecents();
        connectWithInfo(recents.getReference(0));
    }


}

SonobusAudioProcessorEditor::~SonobusAudioProcessorEditor()
{
    if (menuBarModel) {
        menuBarModel->setApplicationCommandManagerToWatch(nullptr);
#if JUCE_MAC
        MenuBarModel::setMacMainMenu(nullptr);
#endif
    }
    
    if (tooltipWindow) {
        tooltipWindow->parent = nullptr;
    }
    
    popTip.reset();
    
    processor.getValueTreeState().removeParameterListener (SonobusAudioProcessor::paramMainSendMute, this);
    processor.getValueTreeState().removeParameterListener (SonobusAudioProcessor::paramMetEnabled, this);
    processor.getValueTreeState().removeParameterListener (SonobusAudioProcessor::paramMainRecvMute, this);
    processor.getValueTreeState().removeParameterListener (SonobusAudioProcessor::paramSendMetAudio, this);
    processor.getValueTreeState().removeParameterListener (SonobusAudioProcessor::paramSendFileAudio, this);
    processor.getValueTreeState().removeParameterListener (SonobusAudioProcessor::paramHearLatencyTest, this);
    processor.getValueTreeState().removeParameterListener (SonobusAudioProcessor::paramMetIsRecorded, this);
    processor.getValueTreeState().removeParameterListener (SonobusAudioProcessor::paramMainReverbModel, this);
    processor.getValueTreeState().removeParameterListener (SonobusAudioProcessor::paramMainReverbEnabled, this);
    processor.getValueTreeState().removeParameterListener (SonobusAudioProcessor::paramSendChannels, this);
    processor.getValueTreeState().removeParameterListener (SonobusAudioProcessor::paramInMonitorMonoPan, this);
    processor.getValueTreeState().removeParameterListener (SonobusAudioProcessor::paramInMonitorPan1, this);
    processor.getValueTreeState().removeParameterListener (SonobusAudioProcessor::paramInMonitorPan2, this);


    
    processor.removeClientListener(this);
    processor.getTransportSource().removeChangeListener(this);
    
    if (mWaveformThumbnail) {
        mWaveformThumbnail->removeChangeListener (this);
    }

}

static void doActuallyQuit (int result, SonobusAudioProcessorEditor* editor)
{
    if (result != 0) {
        JUCEApplicationBase::quit();
    }
}

bool SonobusAudioProcessorEditor::requestedQuit()
{
    // allow quit if we are not connected
    if (currConnected && currGroup.isNotEmpty()) {
        // pop up confirmation
        AlertWindow::showOkCancelBox (AlertWindow::WarningIcon,
        TRANS("Quit Confirmation"),
        TRANS("You are connected, are you sure you want to quit?"),
        TRANS ("Quit"),
        String(),
        nullptr,
        ModalCallbackFunction::create (doActuallyQuit, this));
        
        return false;
    }
    return true;
}


bool SonobusAudioProcessorEditor::copyInfoToClipboard(bool singleURL, String * retmessage)
{
    String message = TRANS("Share this link with others to connect with SonoBus:") + " \n\n";

    String hostport = mServerHostEditor->getText();        
    if (hostport.isEmpty()) {
        hostport = "aoo.sonobus.net";
    }
    
    String groupName;
    String groupPassword;

    if (!currConnected) {
        if (mConnectTab->getCurrentContentComponent() == mServerConnectViewport.get()) {
            groupName = mServerGroupEditor->getText();
            groupPassword = mServerGroupPasswordEditor->getText();
        }
    } else {
        groupName = currConnectionInfo.groupName;
        groupPassword = currConnectionInfo.groupPassword;
    }

    String urlstr1;
    urlstr1 << String("sonobus://") << hostport << String("/");
    URL url(urlstr1);
    URL url2("http://go.sonobus.net/sblaunch");

    if (url.isWellFormed() && groupName.isNotEmpty()) {        

        url2 = url2.withParameter("s", hostport);
        
        url = url.withParameter("g", groupName);
        url2 = url2.withParameter("g", groupName);

        if (groupPassword.isNotEmpty()) {
            url = url.withParameter("p", groupPassword);
            url2 = url2.withParameter("p", groupPassword);
        }

        if (currConnected && currConnectionInfo.groupIsPublic) {
            url = url.withParameter("public", "1");
            url2 = url2.withParameter("public", "1");
        }

        //message += url.toString(true);
        //message += "\n\n";

        //message += TRANS("Or share this link:") + "\n";
        message += url2.toString(true);
        message += "\n";

        if (singleURL) {
            message = url2.toString(true);
        }
        SystemClipboard::copyTextToClipboard(message);

        if (retmessage) {
            *retmessage = message;
        }
        
        return true;
    }
    return false;
}

bool SonobusAudioProcessorEditor::handleSonobusURL(const URL & url)
{
    // look for either  http://go.sonobus.net/sblaunch?  style url
    // or sonobus://host:port/? style
    
    auto & pnames = url.getParameterNames();
    auto & pvals = url.getParameterValues();

    int ind;
    if ((ind = pnames.indexOf("g", true)) >= 0) {
        currConnectionInfo.groupName = pvals[ind];

        if ((ind = pnames.indexOf("p", true)) >= 0) { 
            currConnectionInfo.groupPassword = pvals[ind];
        } else {
            currConnectionInfo.groupPassword = "";                    
        }

        if ((ind = pnames.indexOf("public", true)) >= 0) {
            currConnectionInfo.groupIsPublic = pvals[ind].getIntValue() > 0;
        } else {
            currConnectionInfo.groupIsPublic = false;
        }

    }

    if (url.getScheme() == "sonobus") {
        // use domain part as host:port
        String hostpart = url.getDomain();
        currConnectionInfo.serverHost =  hostpart.upToFirstOccurrenceOf(":", false, true);
        int port = url.getPort();
        if (port > 0) {
            currConnectionInfo.serverPort = port;
        } else {
            currConnectionInfo.serverPort = DEFAULT_SERVER_PORT;
        }
    }
    else {
        if ((ind = pnames.indexOf("s", true)) >= 0) {
            String hostpart = pvals[ind];
            currConnectionInfo.serverHost =  hostpart.upToFirstOccurrenceOf(":", false, true);
            String portpart = hostpart.fromFirstOccurrenceOf(":", false, false);
            int port = portpart.getIntValue();
            if (port > 0) {
                currConnectionInfo.serverPort = port;
            } else {
                currConnectionInfo.serverPort = DEFAULT_SERVER_PORT;                
            }
        }
    }

    
    return true;
}

bool SonobusAudioProcessorEditor::attemptToPasteConnectionFromClipboard()
{
    auto clip = SystemClipboard::getTextFromClipboard();
    
    if (clip.isNotEmpty()) {
        // look for sonobus URL anywhere in it
        String urlpart = clip.fromFirstOccurrenceOf("sonobus://", true, true);
        if (urlpart.isNotEmpty()) {
            // find the end (whitespace) and strip it out
            urlpart = urlpart.upToFirstOccurrenceOf("\n", false, true).trim();
            urlpart = urlpart.upToFirstOccurrenceOf(" ", false, true).trim();
            URL url(urlpart);

            if (url.isWellFormed()) {
                DBG("Got good sonobus URL: " << urlpart);

                return handleSonobusURL(url);                
            }
        }
        else {
            // look for go.sonobus.net/sblaunch  url
            urlpart = clip.fromFirstOccurrenceOf("http://go.sonobus.net/sblaunch?", true, false);
            if (urlpart.isEmpty()) urlpart = clip.fromFirstOccurrenceOf("https://go.sonobus.net/sblaunch?", true, false);
            
            if (urlpart.isNotEmpty()) {
                urlpart = urlpart.upToFirstOccurrenceOf("\n", false, true).trim();
                urlpart = urlpart.upToFirstOccurrenceOf(" ", false, true).trim();
                URL url(urlpart);

                if (url.isWellFormed()) {
                    DBG("Got good http sonobus URL: " << urlpart);

                    return handleSonobusURL(url);                
                }
            }
        }
    }
    
    return false;
}

void SonobusAudioProcessorEditor::updateServerFieldsFromConnectionInfo()
{
    if (currConnectionInfo.serverPort == DEFAULT_SERVER_PORT) {
        mServerHostEditor->setText( currConnectionInfo.serverHost, false);
        mPublicServerHostEditor->setText( currConnectionInfo.serverHost, false);
    } else {
        String hostport;
        hostport << currConnectionInfo.serverHost << ":" << currConnectionInfo.serverPort;
        mServerHostEditor->setText( hostport, false);
        mPublicServerHostEditor->setText( hostport, false);
    }
    mServerUsernameEditor->setText(currConnectionInfo.userName, false);
    mPublicServerUsernameEditor->setText(currConnectionInfo.userName, false);
    if (currConnectionInfo.groupIsPublic) {
        mPublicServerGroupEditor->setText(currConnectionInfo.groupName, false);
    } else {
        mServerGroupEditor->setText(currConnectionInfo.groupName, false);
    }
    mServerGroupPasswordEditor->setText(currConnectionInfo.groupPassword, false);
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

void SonobusAudioProcessorEditor::configLevelSlider(Slider * slider)
{
    //slider->setVelocityBasedMode(true);
    //slider->setVelocityModeParameters(2.5, 1, 0.05);
    //slider->setTextBoxStyle(Slider::NoTextBox, true, 40, 18);
    slider->setSliderStyle(Slider::SliderStyle::LinearHorizontal);
    slider->setTextBoxStyle(Slider::TextBoxAbove, true, 50, 14);
    slider->setMouseDragSensitivity(128);
    slider->setScrollWheelEnabled(false);
    //slider->setPopupDisplayEnabled(true, false, this);
    slider->setColour(Slider::textBoxBackgroundColourId, Colours::transparentBlack);
    slider->setColour(Slider::textBoxOutlineColourId, Colours::transparentBlack);
    slider->setColour(Slider::textBoxTextColourId, Colour(0x90eeeeee));
    slider->setColour(TooltipWindow::textColourId, Colour(0xf0eeeeee));

    slider->setLookAndFeel(&sonoSliderLNF);
}

void SonobusAudioProcessorEditor::connectTabChanged (int newCurrentTabIndex)
{
    // normaliza index to have recents as 0
    int adjindex = mConnectTab->getNumTabs() < 4 ? newCurrentTabIndex + 1 : newCurrentTabIndex;

    // public groups
    if (adjindex == 2) {
        publicGroupLogin();
    }
    else if (adjindex == 1) {
        // private groups
        resetPrivateGroupLabels();
    }
}



//////////////////
// these client listener callbacks will be from a different thread

void SonobusAudioProcessorEditor::aooClientConnected(SonobusAudioProcessor *comp, bool success, const String & errmesg) 
{
    DBG("Client connect success: " <<  (int) success << "  mesg: " << errmesg);
    {
        const ScopedLock sl (clientStateLock);        
        clientEvents.add(ClientEvent(ClientEvent::ConnectEvent, success, errmesg));
    }
    triggerAsyncUpdate();
}

void SonobusAudioProcessorEditor::aooClientDisconnected(SonobusAudioProcessor *comp, bool success, const String & errmesg) 
{
    DBG("Client disconnect success: " << (int) success <<  "  mesg: " << errmesg);
    {
        const ScopedLock sl (clientStateLock);        
        clientEvents.add(ClientEvent(ClientEvent::DisconnectEvent, success, errmesg));
    }
    triggerAsyncUpdate();
}

void SonobusAudioProcessorEditor::aooClientLoginResult(SonobusAudioProcessor *comp, bool success, const String & errmesg)  
{
    DBG("Client login success: " << (int)success << "  mesg: " << errmesg);
    {
        const ScopedLock sl (clientStateLock);        
        clientEvents.add(ClientEvent(ClientEvent::LoginEvent, success, errmesg));
    }
    triggerAsyncUpdate();
}

void SonobusAudioProcessorEditor::aooClientGroupJoined(SonobusAudioProcessor *comp, bool success, const String & group,  const String & errmesg) 
{
    DBG("Client join group " << group << " success: " << (int)success  << "  mesg: " << errmesg);
    {
        const ScopedLock sl (clientStateLock);        
        clientEvents.add(ClientEvent(ClientEvent::GroupJoinEvent, group, success, errmesg));
    }
    triggerAsyncUpdate();
}

void SonobusAudioProcessorEditor::aooClientGroupLeft(SonobusAudioProcessor *comp, bool success, const String & group, const String & errmesg)  
{
    DBG("Client leave group " << group << " success: " << (int)success << "   mesg: " << errmesg);
    {
        const ScopedLock sl (clientStateLock);        
        clientEvents.add(ClientEvent(ClientEvent::GroupLeaveEvent, group, success, errmesg));
    }
    triggerAsyncUpdate();
}

void SonobusAudioProcessorEditor::aooClientPublicGroupModified(SonobusAudioProcessor *comp, const String & group, int count, const String & errmesg)
{
    DBG("Public group add/modified " << group << " count: " << (int)count << "   mesg: " << errmesg);
    {
        const ScopedLock sl (clientStateLock);
        clientEvents.add(ClientEvent(ClientEvent::PublicGroupModifiedEvent, group, true, errmesg));
    }
    triggerAsyncUpdate();
}

void SonobusAudioProcessorEditor::aooClientPublicGroupDeleted(SonobusAudioProcessor *comp, const String & group,  const String & errmesg)
{
    DBG("Public group delete " << group << "   mesg: " << errmesg);
    {
        const ScopedLock sl (clientStateLock);
        clientEvents.add(ClientEvent(ClientEvent::PublicGroupDeletedEvent, group, true, errmesg));
    }
    triggerAsyncUpdate();
}


void SonobusAudioProcessorEditor::aooClientPeerJoined(SonobusAudioProcessor *comp, const String & group, const String & user)  
{
    DBG("Client peer '" << user  << "' joined group '" <<  group << "'");
    {
        const ScopedLock sl (clientStateLock);        
        clientEvents.add(ClientEvent(ClientEvent::PeerJoinEvent, group, true, "", user));
    }
    triggerAsyncUpdate();
}

void SonobusAudioProcessorEditor::aooClientPeerPendingJoin(SonobusAudioProcessor *comp, const String & group, const String & user) 
{
    DBG("Client peer '" << user  << "' pending join group '" <<  group << "'");
    {
        const ScopedLock sl (clientStateLock);        
        clientEvents.add(ClientEvent(ClientEvent::PeerPendingJoinEvent, group, true, "", user));
    }
    triggerAsyncUpdate();    
}

void SonobusAudioProcessorEditor::aooClientPeerJoinFailed(SonobusAudioProcessor *comp, const String & group, const String & user)
{
    DBG("Client peer '" << user  << "' FAILed to join group '" <<  group << "'");
    {
        const ScopedLock sl (clientStateLock);        
        clientEvents.add(ClientEvent(ClientEvent::PeerFailedJoinEvent, group, true, "", user));
    }
    triggerAsyncUpdate();
}


void SonobusAudioProcessorEditor::aooClientPeerLeft(SonobusAudioProcessor *comp, const String & group, const String & user)  
{
    DBG("Client peer '" << user  << "' left group '" <<  group << "'");
    {
        const ScopedLock sl (clientStateLock);        
        clientEvents.add(ClientEvent(ClientEvent::PeerLeaveEvent, group, true, "", user));
    }
    triggerAsyncUpdate();

}

void SonobusAudioProcessorEditor::aooClientError(SonobusAudioProcessor *comp, const String & errmesg)  
{
    DBG("Client error: " <<  errmesg);
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
        processor.setDefaultAutoresizeBufferMode((SonobusAudioProcessor::AutoNetBufferMode) ident);
    }
    else if (comp == mRecFormatChoice.get()) {
        processor.setDefaultRecordingFormat((SonobusAudioProcessor::RecordFileFormat) ident);
    }
    else if (comp == mRecBitsChoice.get()) {
        processor.setDefaultRecordingBitsPerSample(ident);
    }
    else if (comp == mReverbModelChoice.get()) {
        processor.setMainReverbModel((SonobusAudioProcessor::ReverbModel) ident);
    }
    else if (comp == mSendChannelsChoice.get()) {
        float fval = processor.getValueTreeState().getParameter(SonobusAudioProcessor::paramSendChannels)->convertTo0to1(ident);
        processor.getValueTreeState().getParameter(SonobusAudioProcessor::paramSendChannels)->setValueNotifyingHost(fval);
    }
}


bool SonobusAudioProcessorEditor::updatePeerState(bool force)
{
    if (!mPeerContainer) return false;
    
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

void SonobusAudioProcessorEditor::updateOptionsState(bool ignorecheck)
{
    mOptionsFormatChoiceDefaultChoice->setSelectedItemIndex(processor.getDefaultAudioCodecFormat(), dontSendNotification);
    mOptionsAutosizeDefaultChoice->setSelectedId((int)processor.getDefaultAutoresizeBufferMode(), dontSendNotification);

    mOptionsChangeAllFormatButton->setToggleState(processor.getChangingDefaultAudioCodecSetsExisting(), dontSendNotification);
    
    int port = processor.getUseSpecificUdpPort();
    if (port > 0) {
        mOptionsUdpPortEditor->setText(String::formatted("%d", port), dontSendNotification);
        mOptionsUdpPortEditor->setEnabled(true);
        mOptionsUdpPortEditor->setAlpha(1.0);
        if (!ignorecheck) {
            mOptionsUseSpecificUdpPortButton->setToggleState(true, dontSendNotification);
        }
    }
    else {
        port = processor.getUdpLocalPort();
        mOptionsUdpPortEditor->setEnabled(mOptionsUseSpecificUdpPortButton->getToggleState());
        mOptionsUdpPortEditor->setAlpha(mOptionsUseSpecificUdpPortButton->getToggleState() ? 1.0 : 0.6);
        mOptionsUdpPortEditor->setText(String::formatted("%d", port), dontSendNotification);
        if (!ignorecheck) {
            mOptionsUseSpecificUdpPortButton->setToggleState(false, dontSendNotification);        
        }
    }

    
    if (JUCEApplication::isStandaloneApp()) {
        if (getShouldOverrideSampleRateValue) {
            Value * val = getShouldOverrideSampleRateValue();
            mOptionsOverrideSamplerateButton->setToggleState((bool)val->getValue(), dontSendNotification);
        }
        if (getShouldCheckForNewVersionValue) {
            Value * val = getShouldCheckForNewVersionValue();
            mOptionsShouldCheckForUpdateButton->setToggleState((bool)val->getValue(), dontSendNotification);
        }
    }

    mOptionsSliderSnapToMouseButton->setToggleState(processor.getSlidersSnapToMousePosition(), dontSendNotification);

    uint32 recmask = processor.getDefaultRecordingOptions();
    
    mOptionsRecOthersButton->setToggleState((recmask & SonobusAudioProcessor::RecordIndividualUsers) != 0, dontSendNotification);
    mOptionsRecMixButton->setToggleState((recmask & SonobusAudioProcessor::RecordMix) != 0, dontSendNotification);
    mOptionsRecMixMinusButton->setToggleState((recmask & SonobusAudioProcessor::RecordMixMinusSelf) != 0, dontSendNotification);
    mOptionsRecSelfButton->setToggleState((recmask & SonobusAudioProcessor::RecordSelf) != 0, dontSendNotification);
    
    mRecFormatChoice->setSelectedId((int)processor.getDefaultRecordingFormat(), dontSendNotification);
    mRecBitsChoice->setSelectedId((int)processor.getDefaultRecordingBitsPerSample(), dontSendNotification);

    File recdir = File(processor.getDefaultRecordingDirectory());
    String dispath = recdir.getRelativePathFrom(File::getSpecialLocation (File::userHomeDirectory));
    if (dispath.startsWith(".")) dispath = processor.getDefaultRecordingDirectory();
    mRecLocationButton->setButtonText(dispath);
    
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
            mFileMenuButton->setVisible(true);
            mFileAreaBg->setVisible(true);
        } else {
            mPlayButton->setVisible(false);
            mLoopButton->setVisible(false);
            mDismissTransportButton->setVisible(false);
            mWaveformThumbnail->setVisible(false);
            mPlaybackSlider->setVisible(false);
            mFileSendAudioButton->setVisible(false);
            mFileMenuButton->setVisible(false);
            mFileAreaBg->setVisible(false);
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

        if (processor.isConnectedToServer() && processor.getCurrentJoinedGroup().isNotEmpty()) {
            mConnectionTimeLabel->setText(SonoUtility::durationToString(processor.getElapsedConnectedTime(), true), dontSendNotification);
        }


        if (!tooltipWindow && getParentComponent()) {
            Component* dw = this; //this->findParentComponentOfClass<DocumentWindow>();
               if (!dw)
                   dw = this->findParentComponentOfClass<AudioProcessorEditor>();        
               if (!dw)
                   dw = this->findParentComponentOfClass<Component>();        
               //if (!dw)
               //    dw = this;

               if (dw) {
                   tooltipWindow = std::make_unique<CustomTooltipWindow>(this, dw);
               }
           }
        
#if 0
        if (JUCEApplicationBase::isStandaloneApp() && getAudioDeviceManager())
        {
            if (auto * ad = getAudioDeviceManager()->getCurrentAudioDevice()) {
                int inlat = ad->getInputLatencyInSamples();
                int outlat = ad->getOutputLatencyInSamples();
                int bufsize = ad->getCurrentBufferSizeSamples();
                
                DBG("InLat: " << inlat << "  OutLat: " << outlat << "  bufsize: " << bufsize);
            }
        }
#endif
        
#if JUCE_IOS
        if (JUCEApplicationBase::isStandaloneApp()) {
            bool iaaconn = isInterAppAudioConnected();
            if (iaaconn != iaaConnected) {
                iaaConnected = iaaconn;
                DBG("Interapp audio state changed: " << (int) iaaConnected);
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
    else if (timerid == CheckForNewVersionTimerId) {
        if (getShouldCheckForNewVersionValue) {
            Value * val = getShouldCheckForNewVersionValue();
            if (val && (bool)val->getValue()) {
                DBG("Checking for new version");
                LatestVersionCheckerAndUpdater::getInstance()->checkForNewVersion (false);
            }
        }
        stopTimer(CheckForNewVersionTimerId);
    }
}

void SonobusAudioProcessorEditor::publicGroupLogin()
{
    String hostport = mPublicServerHostEditor->getText();
    DBG("Public host enter pressed");
    // parse it
    StringArray toks = StringArray::fromTokens(hostport, ":", "");
    String host = "aoo.sonobus.net";
    int port = DEFAULT_SERVER_PORT;

    if (toks.size() >= 1) {
        host = toks[0].trim();
    }
    if (toks.size() >= 2) {
        port = toks[1].trim().getIntValue();
    }

    AooServerConnectionInfo info;
    info.userName = mPublicServerUsernameEditor->getText();
    info.serverHost = host;
    info.serverPort = port;

    bool connchanged = (info.serverHost != currConnectionInfo.serverHost
                        || info.serverPort != currConnectionInfo.serverPort
                        || info.userName != currConnectionInfo.userName);

    if (connchanged
        || !processor.getWatchPublicGroups()
        || !processor.isConnectedToServer()
        ) {

        if (connchanged && processor.isConnectedToServer()) {
            processor.disconnectFromServer();
        }
        else if (!processor.getWatchPublicGroups() && processor.isConnectedToServer()) {
            processor.setWatchPublicGroups(true);
        }

        if (!processor.isConnectedToServer()) {

            Timer::callAfterDelay(100, [this,info] {
                connectWithInfo(info, true);
                updatePublicGroups();
            });
        }
    }
}

void SonobusAudioProcessorEditor::textEditorReturnKeyPressed (TextEditor& ed)
{
    DBG("Return pressed");
    
    if (&ed == mOptionsUdpPortEditor.get()) {
        int port = mOptionsUdpPortEditor->getText().getIntValue();
        changeUdpPort(port);
    }
    else if (&ed == mPublicServerHostEditor.get() || &ed == mPublicServerUsernameEditor.get()) {
        publicGroupLogin();
    }
    else if (&ed == mPublicServerGroupEditor.get()) {
        buttonClicked(mPublicServerAddGroupButton.get());
    }


    
    if (mConnectComponent->isVisible() && mServerConnectButton->isShowing()) {
        //mServerConnectButton->setWantsKeyboardFocus(true);
        mServerConnectButton->grabKeyboardFocus();
        //mServerConnectButton->setWantsKeyboardFocus(false);
    }
    else if (mConnectComponent->isVisible() && mPublicServerAddGroupButton->isShowing()) {
        //mServerConnectButton->setWantsKeyboardFocus(true);
        mPublicServerAddGroupButton->grabKeyboardFocus();
        //mServerConnectButton->setWantsKeyboardFocus(false);
    }
    else if (mConnectComponent->isVisible() && mDirectConnectButton->isShowing()) {
        //mServerConnectButton->setWantsKeyboardFocus(true);
        mDirectConnectButton->grabKeyboardFocus();
        //mServerConnectButton->setWantsKeyboardFocus(false);
    }
}

void SonobusAudioProcessorEditor::textEditorEscapeKeyPressed (TextEditor& ed)
{
    DBG("escape pressed");
    if (mConnectComponent->isVisible()) {
        //mServerConnectButton->setWantsKeyboardFocus(true);
        mServerConnectButton->grabKeyboardFocus();
        //mServerConnectButton->setWantsKeyboardFocus(false);
    }
    //grabKeyboardFocus();
}

void SonobusAudioProcessorEditor::textEditorTextChanged (TextEditor&)
{
}


void SonobusAudioProcessorEditor::textEditorFocusLost (TextEditor& ed)
{
    // only one we care about live is udp port
    if (&ed == mOptionsUdpPortEditor.get()) {
        int port = mOptionsUdpPortEditor->getText().getIntValue();
        changeUdpPort(port);
    }
    else if (&ed == mPublicServerHostEditor.get() || &ed == mPublicServerUsernameEditor.get()) {
        publicGroupLogin();
    }

}

void SonobusAudioProcessorEditor::changeUdpPort(int port)
{
    if (port >= 0) {
        DBG("changing udp port to: " << port);
        processor.setUseSpecificUdpPort(port);

        updateState();
    } 
    updateOptionsState(true);
    
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
            if (processor.connectRemotePeer(host, port, "", "", !mMainRecvMuteButton->getToggleState())) {
                showConnectPopup(false);
            }
        }
    }
    else if (buttonThatWasClicked == mConnectButton.get()) {
        
        if (processor.isConnectedToServer() && processor.getCurrentJoinedGroup().isNotEmpty()) {
            mConnectionTimeLabel->setText(TRANS("Last Session: ") + SonoUtility::durationToString(processor.getElapsedConnectedTime(), true), dontSendNotification);
            mConnectButton->setTextJustification(Justification::centredTop);

            if (processor.getWatchPublicGroups()) {
                processor.leaveServerGroup(processor.getCurrentJoinedGroup());
            }
            else {
                processor.disconnectFromServer();
            }
            updateState();
        }
        else {
            mConnectionTimeLabel->setText("", dontSendNotification);
            mConnectButton->setTextJustification(Justification::centred);

            if (!mConnectComponent->isVisible()) {
                showConnectPopup(true);
            } else {
                showConnectPopup(false);
            }
        }
        
    }
    else if (buttonThatWasClicked == mServerConnectButton.get()) {
        bool wasconnected = false;
        if (processor.isConnectedToServer()) {
            mConnectionTimeLabel->setText(TRANS("Total: ") + SonoUtility::durationToString(processor.getElapsedConnectedTime(), true), dontSendNotification);

            processor.disconnectFromServer();    
            updateState();
            wasconnected = true;
        }

        String hostport = mServerHostEditor->getText();

        // parse it
        StringArray toks = StringArray::fromTokens(hostport, ":", "");
        String host = "aoo.sonobus.net";
        int port = DEFAULT_SERVER_PORT;

        if (toks.size() >= 1) {
            host = toks[0].trim();
        }
        if (toks.size() >= 2) {
            port = toks[1].trim().getIntValue();
        }

        AooServerConnectionInfo info;
        info.userName = mServerUsernameEditor->getText();
        info.groupName = mServerGroupEditor->getText();
        info.groupPassword = mServerGroupPasswordEditor->getText();
        info.groupIsPublic = false;
        info.serverHost = host;
        info.serverPort = port;

        connectWithInfo(info);

        mConnectionTimeLabel->setText("", dontSendNotification);

    }
    else if (buttonThatWasClicked == mPublicServerAddGroupButton.get()) {

        String hostport = mPublicServerHostEditor->getText();

        // parse it
        StringArray toks = StringArray::fromTokens(hostport, ":", "");
        String host = "aoo.sonobus.net";
        int port = DEFAULT_SERVER_PORT;

        if (toks.size() >= 1) {
            host = toks[0].trim();
        }
        if (toks.size() >= 2) {
            port = toks[1].trim().getIntValue();
        }

        AooServerConnectionInfo info;
        info.userName = mPublicServerUsernameEditor->getText();
        info.groupName = mPublicServerGroupEditor->getText();
        info.groupPassword = "";
        info.groupIsPublic = true;
        info.serverHost = host;
        info.serverPort = port;

        connectWithInfo(info);

        mConnectionTimeLabel->setText("", dontSendNotification);

    }
    else if (buttonThatWasClicked == mSetupAudioButton.get()) {
        if (!settingsCalloutBox) {
            showSettings(true);
            mSettingsTab->setCurrentTabIndex(0);
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
    else if (buttonThatWasClicked == mRecLocationButton.get()) {
        // browse folder chooser
        SafePointer<SonobusAudioProcessorEditor> safeThis (this);

        if (! RuntimePermissions::isGranted (RuntimePermissions::readExternalStorage))
        {
            RuntimePermissions::request (RuntimePermissions::readExternalStorage,
                                         [safeThis] (bool granted) mutable
                                         {
                if (granted)
                    safeThis->buttonClicked (safeThis->mRecLocationButton.get());
            });
            return;
        }

        chooseRecDirBrowser();
    }
    else if (buttonThatWasClicked == mMetConfigButton.get()) {
        if (!metCalloutBox) {
            showMetConfig(true);
        } else {
            showMetConfig(false);
        }        
    }
    else if (buttonThatWasClicked == mEffectsButton.get()) {
        if (!effectsCalloutBox) {
            showEffectsConfig(true);
        } else {
            showEffectsConfig(false);
        }        
    }
    else if (buttonThatWasClicked == mInEffectsButton.get()) {
        if (!inEffectsCalloutBox) {
            showInEffectsConfig(true);
        } else {
            showInEffectsConfig(false);
        }        
    }
    else if (buttonThatWasClicked == mMainMuteButton.get()) {
        // allow or disallow sending to all peers, handled by button attachment

        if (mMainMuteButton->getToggleState()) {
            showPopTip(TRANS("Not sending your audio anywhere"), 3000, mMainMuteButton.get());
        } else {
            showPopTip(TRANS("Sending your audio to others"), 3000, mMainMuteButton.get());
        }
    }
    else if (buttonThatWasClicked == mInSoloButton.get()) {
        if (ModifierKeys::currentModifiers.isAltDown()) {
            // exclusive solo this one
            for (int j=0; j < processor.getNumberRemotePeers(); ++j) {
                processor.setRemotePeerSoloed(j, false);
            }
        }
        updatePeerState();
    }
    else if (buttonThatWasClicked == mMainRecvMuteButton.get()) {
        // allow or disallow sending to all peers, handled by button attachment

        if (processor.getNumberRemotePeers() > 0 && settingsCalloutBox == nullptr) {
            if (mMainRecvMuteButton->getToggleState()) {
                showPopTip(TRANS("Muted everyone"), 3000, mMainRecvMuteButton.get());
            } else {
                showPopTip(TRANS("Unmuted all who were not muted previously"), 3000, mMainRecvMuteButton.get());
            }
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
    else if (buttonThatWasClicked == mServerPasteButton.get()) {
        if (attemptToPasteConnectionFromClipboard()) {
            updateServerFieldsFromConnectionInfo();            
            updateServerStatusLabel(TRANS("Filled in Group information from clipboard! Press 'Connect to Group' to join..."), false);
        }
    }
    else if (buttonThatWasClicked == mServerCopyButton.get()) {
        if (copyInfoToClipboard()) {
            showPopTip(TRANS("Copied connection info to clipboard for you to share with others"), 3000, mServerCopyButton.get());
        }
    }
    else if (buttonThatWasClicked == mMainLinkButton.get()) {
#if JUCE_IOS || JUCE_ANDROID
        String message;
        bool singleurl = true;
        if (copyInfoToClipboard(singleurl, &message)) {
            URL url(message);
            if (url.isWellFormed()) {
                Array<URL> urlarray;
                urlarray.add(url);
                ContentSharer::getInstance()->shareFiles(urlarray, [](bool result, const String& msg){ DBG("url share returned " << (int)result << " : " <<  msg); });
            } else {
                ContentSharer::getInstance()->shareText(message, [](bool result, const String& msg){ DBG("share returned " << (int)result << " : " << msg); });       
            }
        }        
#else
        if (copyInfoToClipboard()) {
            showPopTip(TRANS("Copied group connection info to clipboard for you to share with others"), 3000, mMainLinkButton.get());
        }
#endif
    }
    else if (buttonThatWasClicked == mServerShareButton.get()) {
        String message;
        bool singleurl = false;
#if JUCE_IOS || JUCE_ANDROID
        singleurl = true;
#endif
        if (copyInfoToClipboard(singleurl, &message)) {
            URL url(message);
            if (url.isWellFormed()) {
                Array<URL> urlarray;
                urlarray.add(url);
                ContentSharer::getInstance()->shareFiles(urlarray, [](bool result, const String& msg){ DBG("url share returned " << (int)result << " : " <<  msg); });
            } else {
                ContentSharer::getInstance()->shareText(message, [](bool result, const String& msg){ DBG("share returned " << (int)result << " : " << msg); });       
            }
        }
        
        //copyInfoToClipboard();
        //showPopTip(TRANS("Copied connection info to clipboard for you to share with others"), 3000, mServerCopyButton.get());
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
#if (JUCE_IOS || JUCE_ANDROID)
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

            SafePointer<SonobusAudioProcessorEditor> safeThis (this);

            if (! RuntimePermissions::isGranted (RuntimePermissions::writeExternalStorage))
            {
                RuntimePermissions::request (RuntimePermissions::writeExternalStorage,
                                             [safeThis] (bool granted) mutable
                                             {
                    if (granted)
                        safeThis->buttonClicked (safeThis->mRecordingButton.get());
                });
                return;
            }

            // create new timestamped filename
            String filename = Time::getCurrentTime().formatted("SonoBusSession_%Y-%m-%d_%H.%M.%S");

            auto parentDir = File(processor.getDefaultRecordingDirectory());
            parentDir.createDirectory();

            File file (parentDir.getNonexistentChildFile (filename, ".flac"));

            if (processor.startRecordingToFile(file)) {
                mRecordingButton->setToggleState(true, dontSendNotification);
                //updateServerStatusLabel("Started recording...");
                lastRecordedFile = file;
                String filepath;

#if (JUCE_IOS || JUCE_ANDROID)
                showPopTip(TRANS("Started recording output"), 2000, mRecordingButton.get());
#endif
                if (processor.getDefaultRecordingOptions() == SonobusAudioProcessor::RecordMix) {
                    
#if (JUCE_IOS || JUCE_ANDROID)
                    filepath = lastRecordedFile.getRelativePathFrom(File::getSpecialLocation (File::userDocumentsDirectory));
#else
                    filepath = lastRecordedFile.getRelativePathFrom(File::getSpecialLocation (File::userHomeDirectory));
#endif

                    mRecordingButton->setTooltip(TRANS("Recording audio to: ") + filepath);
                } else 
                {
#if (JUCE_IOS || JUCE_ANDROID)
                    filepath = lastRecordedFile.getParentDirectory().getRelativePathFrom(File::getSpecialLocation (File::userDocumentsDirectory));
#else
                    filepath = lastRecordedFile.getParentDirectory().getRelativePathFrom(File::getSpecialLocation (File::userHomeDirectory));
#endif
                    mRecordingButton->setTooltip(TRANS("Recording multi-track audio to: ") + filepath);
                }
            }
            else {
                // show error starting record
                String lasterr = processor.getLastErrorMessage();
                showPopTip(lasterr, 0, mRecordingButton.get());
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

            if (ModifierKeys::currentModifiers.isCommandDown()) {
                // file file
                if (mCurrentAudioFile.getFileName().isNotEmpty()) {
                    mCurrentAudioFile.getLocalFile().revealToUser();
                }
                else {
                    if (mCurrOpenDir.getFullPathName().isEmpty()) {
                        mCurrOpenDir = File(processor.getDefaultRecordingDirectory());
                        DBG("curr open dir is: " << mCurrOpenDir.getFullPathName());
                    }
                    mCurrOpenDir.revealToUser();
                }
                
            }   
            else {
                openFileBrowser();
            }
                      
        }
    }
    else if (buttonThatWasClicked == mDismissTransportButton.get()) {
        processor.getTransportSource().stop();
        loadAudioFromURL(URL());
        updateLayout();
        resized();
    }
    else if (buttonThatWasClicked == mOptionsInputLimiterButton.get()) {
        SonobusAudioProcessor::CompressorParams params;
        processor.getInputLimiterParams(params);
        params.enabled = mOptionsInputLimiterButton->getToggleState();
        processor.setInputLimiterParams(params);        
    }
    else if (buttonThatWasClicked == mPlayButton.get()) {
        if (mPlayButton->getToggleState()) {
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
    else if (buttonThatWasClicked == mFileMenuButton.get()) {
        // show file extra menu
        showFilePopupMenu(mFileMenuButton.get());
    }
    else if (buttonThatWasClicked == mConnectCloseButton.get()) {
        mConnectComponent->setVisible(false);
        mConnectButton->setTextJustification(Justification::centred);

        processor.setWatchPublicGroups(false);
        
        updateState();
    }
    else if (buttonThatWasClicked == mOptionsRecMixButton.get()
             || buttonThatWasClicked == mOptionsRecSelfButton.get()
             || buttonThatWasClicked == mOptionsRecOthersButton.get()
             || buttonThatWasClicked == mOptionsRecMixMinusButton.get()
             ) {
        uint32 recmask = 0;        
        recmask |= (mOptionsRecMixButton->getToggleState() ? SonobusAudioProcessor::RecordMix : 0);
        recmask |= (mOptionsRecOthersButton->getToggleState() ? SonobusAudioProcessor::RecordIndividualUsers : 0);
        recmask |= (mOptionsRecSelfButton->getToggleState() ? SonobusAudioProcessor::RecordSelf : 0);
        recmask |= (mOptionsRecMixMinusButton->getToggleState() ? SonobusAudioProcessor::RecordMixMinusSelf : 0);

        // ensure at least one is selected
        if (recmask == 0) { 
            recmask = SonobusAudioProcessor::RecordMix;
            mOptionsRecMixButton->setToggleState(true, dontSendNotification);
        }
        
        processor.setDefaultRecordingOptions(recmask);
    }
    else if (buttonThatWasClicked == mOptionsChangeAllFormatButton.get()) {
        processor.setChangingDefaultAudioCodecSetsExisting(mOptionsChangeAllFormatButton->getToggleState());
    }
    else if (buttonThatWasClicked == mOptionsUseSpecificUdpPortButton.get()) {
        if (!mOptionsUseSpecificUdpPortButton->getToggleState()) {
            // toggled off, change back to use system chosen port
            changeUdpPort(0);
        } else {
            updateOptionsState(true);
        }
    }
    else if (buttonThatWasClicked == mClearRecentsButton.get()) {
        processor.clearRecentServerConnectionInfos();
        updateRecents();
    }
    else if (buttonThatWasClicked == mOptionsOverrideSamplerateButton.get()) {

        if (JUCEApplicationBase::isStandaloneApp() && getShouldOverrideSampleRateValue) {
            Value * val = getShouldOverrideSampleRateValue();
            val->setValue((bool)mOptionsOverrideSamplerateButton->getToggleState());
        }
    }
    else if (buttonThatWasClicked == mOptionsShouldCheckForUpdateButton.get()) {

        if (JUCEApplicationBase::isStandaloneApp() && getShouldCheckForNewVersionValue) {
            Value * val = getShouldCheckForNewVersionValue();
            val->setValue((bool)mOptionsShouldCheckForUpdateButton->getToggleState());

            if (mOptionsShouldCheckForUpdateButton->getToggleState()) {
                startTimer(CheckForNewVersionTimerId, 3000);
            }
        }
    }
    else if (buttonThatWasClicked == mOptionsSliderSnapToMouseButton.get()) {
        processor.setSlidersSnapToMousePosition(mOptionsSliderSnapToMouseButton->getToggleState());
        updateSliderSnap();
    }
    else {
        
       
    }
}

void SonobusAudioProcessorEditor::openFileBrowser()
{
    SafePointer<SonobusAudioProcessorEditor> safeThis (this);

    if (FileChooser::isPlatformDialogAvailable())
    {
#if !(JUCE_IOS || JUCE_ANDROID)
        if (mCurrOpenDir.getFullPathName().isEmpty()) {
            mCurrOpenDir = File(processor.getDefaultRecordingDirectory());
            DBG("curr open dir is: " << mCurrOpenDir.getFullPathName());
            
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
                
                DBG("Attempting to load from: " <<  url.toString(false));
                
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
        DBG("Need to enable code signing");
    }
}

void SonobusAudioProcessorEditor::chooseRecDirBrowser()
{
    SafePointer<SonobusAudioProcessorEditor> safeThis (this);

    if (FileChooser::isPlatformDialogAvailable())
    {
        File recdir = File(processor.getDefaultRecordingDirectory());

        mFileChooser.reset(new FileChooser(TRANS("Choose the folder for new recordings"),
                                           recdir,
                                           "",
                                           true, false, getTopLevelComponent()));



        mFileChooser->launchAsync (FileBrowserComponent::openMode | FileBrowserComponent::canSelectDirectories,
                                   [safeThis] (const FileChooser& chooser) mutable
                                   {
            auto results = chooser.getURLResults();
            if (safeThis != nullptr && results.size() > 0)
            {
                auto url = results.getReference (0);

                DBG("Chose directory: " <<  url.toString(false));

                if (url.isLocalFile()) {
                    File lfile = url.getLocalFile();
                    if (lfile.isDirectory()) {
                        safeThis->processor.setDefaultRecordingDirectory(lfile.getFullPathName());
                    } else {
                        safeThis->processor.setDefaultRecordingDirectory(lfile.getParentDirectory().getFullPathName());
                    }

                    safeThis->updateOptionsState();
                }
            }

            if (safeThis) {
                safeThis->mFileChooser.reset();
            }

        }, nullptr);

    }
    else {
        DBG("Need to enable code signing");
    }
}

void SonobusAudioProcessorEditor::updateSliderSnap()
{
    // set level slider snap to mouse property based on processor state and size of slider
    auto snap = processor.getSlidersSnapToMousePosition();

    int minsize = 60;

    std::function<void(Slider *)> snapset = [&](Slider * slider){
        slider->setSliderSnapsToMousePosition(slider->getWidth() > minsize && snap);
    };

    snapset(mInGainSlider.get());
    snapset(mOutGainSlider.get());
    snapset(mDrySlider.get());
    snapset(mInMonPanSlider1.get());
    snapset(mInMonPanSlider2.get());
    snapset(mInMonPanStereoSlider.get());
    snapset(mInMonPanMonoSlider.get());

    mPeerContainer->applyToAllSliders(snapset);
}


void SonobusAudioProcessorEditor::handleURL(const String & urlstr)
{
    URL url(urlstr);
    if (url.isWellFormed()) {
        if (!currConnected || currGroup.isEmpty()) {
            if (handleSonobusURL(url)) {

                // connect immediately
                connectWithInfo(currConnectionInfo);

                //showConnectPopup(true);
                // switch to group page
                //mConnectTab->setCurrentTabIndex(mConnectTab->getNumTabs() > 2 ? 1 : 0);
                //updateServerStatusLabel(TRANS("Filled in Group from link! Press 'Connect to Group' to join..."), false);
            }
        }
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


void SonobusAudioProcessorEditor::connectWithInfo(const AooServerConnectionInfo & info, bool allowEmptyGroup)
{
    currConnectionInfo = info;
    
    if (currConnectionInfo.groupName.isEmpty() && !allowEmptyGroup) {
        if (info.groupIsPublic) {
            mPublicServerStatusInfoLabel->setText(TRANS("You need to specify a group name!"), dontSendNotification);
            mPublicServerGroupEditor->setColour(TextEditor::backgroundColourId, Colour(0xff880000));
            mPublicServerGroupEditor->repaint();
            mPublicServerStatusInfoLabel->setVisible(true);
        }
        else {
            mServerStatusLabel->setText(TRANS("You need to specify a group name!"), dontSendNotification);
            mServerGroupEditor->setColour(TextEditor::backgroundColourId, Colour(0xff880000));
            mServerGroupEditor->repaint();
            mServerInfoLabel->setVisible(false);
            mServerStatusLabel->setVisible(true);
        }
        return;
    }
    else {
        mServerGroupEditor->setColour(TextEditor::backgroundColourId, Colour(0xff050505));
        mServerGroupEditor->repaint();
        mPublicServerGroupEditor->setColour(TextEditor::backgroundColourId, Colour(0xff050505));
        mPublicServerGroupEditor->repaint();
    }
    
    if (currConnectionInfo.userName.isEmpty()) {
        String mesg = TRANS("You need to specify a user name!");

        if (info.groupIsPublic) {
            mPublicServerStatusInfoLabel->setText(mesg, dontSendNotification);
            mPublicServerUsernameEditor->setColour(TextEditor::backgroundColourId, Colour(0xff880000));
            mPublicServerUsernameEditor->repaint();
        } else {
            mServerStatusLabel->setText(mesg, dontSendNotification);
            mServerUsernameEditor->setColour(TextEditor::backgroundColourId, Colour(0xff880000));
            mServerUsernameEditor->repaint();
        }

        mServerInfoLabel->setVisible(false);
        mServerStatusLabel->setVisible(true);
        return;
    }
    else {
        mServerUsernameEditor->setColour(TextEditor::backgroundColourId, Colour(0xff050505));
        mServerUsernameEditor->repaint();
        mPublicServerUsernameEditor->setColour(TextEditor::backgroundColourId, Colour(0xff050505));
        mPublicServerUsernameEditor->repaint();
    }
    
    //mServerGroupPasswordEditor->setColour(TextEditor::backgroundColourId, Colour(0xff880000));
    mServerGroupPasswordEditor->setColour(TextEditor::backgroundColourId, Colour(0xff050505));
    mServerGroupPasswordEditor->repaint();
    
    if (currConnectionInfo.serverHost.isNotEmpty() && currConnectionInfo.serverPort != 0) 
    {
        processor.disconnectFromServer();

        Timer::callAfterDelay(100, [this] {
            processor.connectToServer(currConnectionInfo.serverHost, currConnectionInfo.serverPort, currConnectionInfo.userName, currConnectionInfo.userPassword);
            updateState();
        });

        mServerHostEditor->setColour(TextEditor::backgroundColourId, Colour(0xff050505));
        mServerHostEditor->repaint();

        mPublicServerHostEditor->setColour(TextEditor::backgroundColourId, Colour(0xff050505));
        mPublicServerHostEditor->repaint();
    }
    else {
        String mesg = TRANS("Server address is invalid!");
        if (info.groupIsPublic) {
            mPublicServerStatusInfoLabel->setText(mesg, dontSendNotification);
            mPublicServerHostEditor->setColour(TextEditor::backgroundColourId, Colour(0xff880000));
            mPublicServerHostEditor->repaint();
        } else {
            mServerStatusLabel->setText(mesg, dontSendNotification);
            mServerHostEditor->setColour(TextEditor::backgroundColourId, Colour(0xff880000));
            mServerHostEditor->repaint();
        }

        mServerInfoLabel->setVisible(false);
        mServerStatusLabel->setVisible(true);
    }
}


void SonobusAudioProcessorEditor::showInPanners(bool flag)
{
    
    if (flag && inPannerCalloutBox == nullptr) {
        
        auto wrap = std::make_unique<Viewport>();

        Component* dw = this; // nullptr; //this->findParentComponentOfClass<DocumentWindow>();
        
        if (!dw) {
            dw = this->findParentComponentOfClass<AudioProcessorEditor>();        
        }
        if (!dw) {
            dw = this->findParentComponentOfClass<Component>();        
        }
        if (!dw) {
            dw = this;
        }
        
        const int defWidth = 260; 
        const int defHeight = 60;
        
        wrap->setSize(jmin(defWidth, dw->getWidth() - 20), jmin(defHeight, dw->getHeight() - 24));
                
        inPannersContainer->setBounds(Rectangle<int>(0,0,defWidth,defHeight));
        
        wrap->setViewedComponent(inPannersContainer.get(), false);
        inPannersContainer->setVisible(true);
        
        inPannerMainBox.performLayout(inPannersContainer->getLocalBounds());
        
        
        Rectangle<int> bounds =  dw->getLocalArea(nullptr, mPanButton->getScreenBounds());
        DBG("callout bounds: " << bounds.toString());
        inPannerCalloutBox = & CallOutBox::launchAsynchronously (std::move(wrap), bounds , dw, false);
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
        
        auto wrap = std::make_unique<Viewport>();

        Component* dw = this; 
        
#if JUCE_IOS || JUCE_ANDROID
        const int defWidth = 230; 
        const int defHeight = 96;
#else
        const int defWidth = 210; 
        const int defHeight = 86;
#endif
        
        
        wrap->setSize(jmin(defWidth, dw->getWidth() - 20), jmin(defHeight, dw->getHeight() - 24));
        
        
        mMetContainer->setBounds(Rectangle<int>(0,0,defWidth,defHeight));
        
        wrap->setViewedComponent(mMetContainer.get(), false);
        mMetContainer->setVisible(true);
        
        metBox.performLayout(mMetContainer->getLocalBounds());
        
        
        Rectangle<int> bounds =  dw->getLocalArea(nullptr, mMetConfigButton->getScreenBounds());
        DBG("callout bounds: " << bounds.toString());
        metCalloutBox = & CallOutBox::launchAsynchronously (std::move(wrap), bounds , dw, false);
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

void SonobusAudioProcessorEditor::showEffectsConfig(bool flag)
{
    
    if (flag && effectsCalloutBox == nullptr) {
        
        auto wrap = std::make_unique<Viewport>();

        
        Component* dw = this; 
        
#if JUCE_IOS || JUCE_ANDROID
        const int defWidth = 260; 
        const int defHeight = 154;
#else
        const int defWidth = 260; 
        const int defHeight = 135;
#endif
        
        
        wrap->setSize(jmin(defWidth, dw->getWidth() - 20), jmin(defHeight, dw->getHeight() - 24));
        
        
        mEffectsContainer->setBounds(Rectangle<int>(0,0,defWidth,defHeight));
        
        wrap->setViewedComponent(mEffectsContainer.get(), false);
        mEffectsContainer->setVisible(true);
        
        effectsBox.performLayout(mEffectsContainer->getLocalBounds());

        auto headbgbounds = mReverbEnabledButton->getBounds().withRight(mReverbModelChoice->getRight()).expanded(2);
        mReverbHeaderBg->setRectangle (headbgbounds.toFloat());

        
        Rectangle<int> bounds =  dw->getLocalArea(nullptr, mEffectsButton->getScreenBounds());
        DBG("callout bounds: " << bounds.toString());
        effectsCalloutBox = & CallOutBox::launchAsynchronously (std::move(wrap), bounds , dw, false);
        if (CallOutBox * box = dynamic_cast<CallOutBox*>(effectsCalloutBox.get())) {
            box->setDismissalMouseClicksAreAlwaysConsumed(true);
        }
    }
    else {
        // dismiss it
        if (CallOutBox * box = dynamic_cast<CallOutBox*>(effectsCalloutBox.get())) {
            box->dismiss();
            effectsCalloutBox = nullptr;
        }
    }
}

void SonobusAudioProcessorEditor::showInEffectsConfig(bool flag, Component * fromView)
{
    
    if (flag && inEffectsCalloutBox == nullptr) {
        
        auto wrap = std::make_unique<Viewport>();

        Component* dw = this; 
        
        int defWidth = 260;
#if JUCE_IOS || JUCE_ANDROID
        int defHeight = 180;
#else
        int defHeight = 156;
#endif
        
        
        
        
        auto minbounds = mInCompressorView->getMinimumContentBounds();
        auto minheadbounds = mInCompressorView->getMinimumHeaderBounds();
        auto minexpbounds = mInExpanderView->getMinimumContentBounds();        
        auto mineqbounds = mInEqView->getMinimumContentBounds();        

        auto headerheight = minheadbounds.getHeight();
        
        defWidth = jmax(minbounds.getWidth(), minexpbounds.getWidth(), mineqbounds.getWidth()) + 12;
        defHeight = jmax(minbounds.getHeight(), minexpbounds.getHeight(), mineqbounds.getHeight()) + 3*headerheight + 8;
        
        int extrawidth = 0;
        if (defHeight > dw->getHeight() - 24) {
            extrawidth = wrap->getScrollBarThickness() + 1;
        }
        
        wrap->setSize(jmin(defWidth + extrawidth, dw->getWidth() - 10), jmin(defHeight, dw->getHeight() - 24));

        
        mInEffectsContainer->setBounds(Rectangle<int>(0,0,defWidth,defHeight));
        
        wrap->setViewedComponent(mInEffectsContainer.get(), false);
        mInEffectsContainer->setVisible(true);
        
        SonobusAudioProcessor::CompressorParams params;
        processor.getInputCompressorParams(params);
        mInCompressorView->updateParams(params);

        SonobusAudioProcessor::CompressorParams expparams;
        processor.getInputExpanderParams(expparams);
        mInExpanderView->updateParams(expparams);

        SonobusAudioProcessor::ParametricEqParams eqparams;
        processor.getInputEqParams(eqparams);
        mInEqView->updateParams(eqparams);

        
        inEffectsBox.performLayout(mInEffectsContainer->getLocalBounds().reduced(2, 2));

        // first time only
        if (firstShowInEffects) {
            if (eqparams.enabled && !(params.enabled || expparams.enabled)) {
                mInEffectsConcertina->expandPanelFully(mInEqView.get(), false);
            } 
            else {
                mInEffectsConcertina->setPanelSize(mInEqView.get(), 0, false);
                mInEffectsConcertina->expandPanelFully(mInExpanderView.get(), false);
                mInEffectsConcertina->expandPanelFully(mInCompressorView.get(), false);
            } 

            firstShowInEffects = false;
        }

        
        Rectangle<int> bounds =  dw->getLocalArea(nullptr, fromView ? fromView->getScreenBounds() : mInEffectsButton->getScreenBounds());
        DBG("in effect callout bounds: " << bounds.toString());
        inEffectsCalloutBox = & CallOutBox::launchAsynchronously (std::move(wrap), bounds , dw, false);
        if (CallOutBox * box = dynamic_cast<CallOutBox*>(inEffectsCalloutBox.get())) {
            box->setDismissalMouseClicksAreAlwaysConsumed(true);
        }
    }
    else {
        // dismiss it
        if (CallOutBox * box = dynamic_cast<CallOutBox*>(inEffectsCalloutBox.get())) {
            box->dismiss();
            inEffectsCalloutBox = nullptr;
        }
    }
}


void SonobusAudioProcessorEditor::showPatchbay(bool flag)
{
    if (!mPatchMatrixView) {
        mPatchMatrixView = std::make_unique<PatchMatrixView>(processor);
    }
    
    if (flag && patchbayCalloutBox == nullptr) {
        
        auto wrap = std::make_unique<Viewport>();

        Component* dw = this;
        
        // calculate based on how many peers we have
        auto prefbounds = mPatchMatrixView->getPreferredBounds();
        const int defWidth = prefbounds.getWidth(); 
        const int defHeight = prefbounds.getHeight();
        
        wrap->setSize(jmin(defWidth + 8, dw->getWidth() - 20), jmin(defHeight + 8, dw->getHeight() - 24));
                
        mPatchMatrixView->setBounds(Rectangle<int>(0,0,defWidth,defHeight));
        
        wrap->setViewedComponent(mPatchMatrixView.get(), false);
        
        mPatchMatrixView->updateGridLayout();
        mPatchMatrixView->updateGrid();
        
        Rectangle<int> bounds =  dw->getLocalArea(nullptr, mPatchbayButton->getScreenBounds());
        DBG("callout bounds: " << bounds.toString());
        patchbayCalloutBox = & CallOutBox::launchAsynchronously (std::move(wrap), bounds , dw, false);
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

void SonobusAudioProcessorEditor::updateRecents()
{
    recentsListModel.updateState();
    mRecentsListBox->updateContent();
    mRecentsListBox->deselectAllRows();
}

void SonobusAudioProcessorEditor::updatePublicGroups()
{
    publicGroupsListModel.updateState();
    mPublicGroupsListBox->updateContent();
    mPublicGroupsListBox->repaint();
    mPublicGroupsListBox->deselectAllRows();
}

void SonobusAudioProcessorEditor::resetPrivateGroupLabels()
{
    if (!mServerInfoLabel) return;

    mServerInfoLabel->setText(TRANS("All who join the same Group will be able to connect with each other."), dontSendNotification);
    mServerInfoLabel->setVisible(true);
    mServerStatusLabel->setVisible(false);
}

void SonobusAudioProcessorEditor::showConnectPopup(bool flag)
{
    if (flag) {
        mConnectComponent->toFront(true);
        
        resetPrivateGroupLabels();
        updateServerFieldsFromConnectionInfo();
        
        updateRecents();
        
        if (firstTimeConnectShow) {
            if (mConnectTab->getNumTabs() > 3) {
                if (recentsListModel.getNumRows() > 0) {
                    // show recents tab first
                    mConnectTab->setCurrentTabIndex(0);
                } else {
                    mConnectTab->setCurrentTabIndex(1);
                }
            }
            firstTimeConnectShow = false;
        }

        if (mConnectTab->getCurrentContentComponent() == mPublicServerConnectContainer.get()) {
            publicGroupLogin();
        }

        mConnectComponent->setVisible(true);
    }
    else {
        mConnectComponent->setVisible(false);
    }

}


void SonobusAudioProcessorEditor::sliderValueChanged (Slider* slider)
{
    if (slider == mInMonPanMonoSlider.get()) {
        float panval = slider->getValue();
        float fval = processor.getValueTreeState().getParameter(SonobusAudioProcessor::paramInMonitorMonoPan)->convertTo0to1(panval);
        processor.getValueTreeState().getParameter(SonobusAudioProcessor::paramInMonitorMonoPan)->setValueNotifyingHost(fval);
    }
    else if (slider == mInMonPanStereoSlider.get()) {
        float panval1 = slider->getMinValue();
        float panval2 = slider->getMaxValue();
        float fval1 = processor.getValueTreeState().getParameter(SonobusAudioProcessor::paramInMonitorPan1)->convertTo0to1(panval1);
        processor.getValueTreeState().getParameter(SonobusAudioProcessor::paramInMonitorPan1)->setValueNotifyingHost(fval1);
        float fval2 = processor.getValueTreeState().getParameter(SonobusAudioProcessor::paramInMonitorPan2)->convertTo0to1(panval2);
        processor.getValueTreeState().getParameter(SonobusAudioProcessor::paramInMonitorPan2)->setValueNotifyingHost(fval2);
    }
}

void SonobusAudioProcessorEditor::mouseDown (const MouseEvent& event) 
{
    
    if (event.eventComponent == mSettingsButton.get()) {
        settingsWasShownOnDown = settingsCalloutBox != nullptr || (Time::getMillisecondCounter() < settingsClosedTimestamp + 500);

        if (!settingsWasShownOnDown) {
          //  showOrHideSettings();
        }
    }
    else if (event.eventComponent == mTitleLabel.get() || event.eventComponent == mTitleImage.get()) {
        settingsWasShownOnDown = settingsCalloutBox != nullptr;
        settingsClosedTimestamp = 0; // reset on a down
        if (settingsWasShownOnDown) {
            showSettings(false);
        }
    }
    else if (event.eventComponent == inputMeter.get()) {
        inputMeter->clearClipIndicator(-1);
        outputMeter->clearClipIndicator(-1);
        mPeerContainer->clearClipIndicators();
    }
    else if (event.eventComponent == outputMeter.get()) {
        outputMeter->clearClipIndicator(-1);
        inputMeter->clearClipIndicator(-1);
        mPeerContainer->clearClipIndicators();
    }
    else if (event.eventComponent == mMainPushToTalkButton.get()) {
        // mute others/recv, send self
        if (mMainPushToTalkButton->isEnabled()) {
            mPushToTalkWasMuted = processor.getValueTreeState().getParameter(SonobusAudioProcessor::paramMainSendMute)->getValue() > 0;
            processor.getValueTreeState().getParameter(SonobusAudioProcessor::paramMainRecvMute)->setValueNotifyingHost(1.0);            
            processor.getValueTreeState().getParameter(SonobusAudioProcessor::paramMainSendMute)->setValueNotifyingHost(0.0);
        }
    }
}

void SonobusAudioProcessorEditor::mouseUp (const MouseEvent& event)
{
    if (event.eventComponent == mTitleLabel.get() || event.eventComponent == mTitleImage.get()) {
        if (Time::getMillisecondCounter() > settingsClosedTimestamp + 1000) {
            // only show if it wasn't just dismissed
            showSettings(true);
        }
    }
    else if (event.eventComponent == mMainPushToTalkButton.get()) {
        if (mMainPushToTalkButton->isEnabled()) {
            // back to mute self, hear others
            processor.getValueTreeState().getParameter(SonobusAudioProcessor::paramMainSendMute)->setValueNotifyingHost(mPushToTalkWasMuted ? 1.0 : 0.0);            
            processor.getValueTreeState().getParameter(SonobusAudioProcessor::paramMainRecvMute)->setValueNotifyingHost(0.0);
        }
    }
    else if (event.eventComponent == mReverbTitleLabel.get()) {
        mReverbEnabledButton->setToggleState(!mReverbEnabledButton->getToggleState(), sendNotification);
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
            DBG("setting parent changed: " << (uint64) component.getParentComponent());
            settingsClosedTimestamp = Time::getMillisecondCounter();
        }
    }
}

bool SonobusAudioProcessorEditor::keyPressed (const KeyPress & key)
{
    DBG("Got key: " << key.getTextCharacter() << "  isdown: " << (key.isCurrentlyDown() ? 1 : 0) << " keycode: " << key.getKeyCode() << " pcode: " << (int)'p');
    return false;
}

bool SonobusAudioProcessorEditor::keyStateChanged (bool isKeyDown)
{
    bool pttdown = KeyPress::isKeyCurrentlyDown('T');
    
    if (!pttdown && mPushToTalkKeyDown) {
        // release
        DBG("T release");
        // mute self again, send to others
        processor.getValueTreeState().getParameter(SonobusAudioProcessor::paramMainSendMute)->setValueNotifyingHost(mPushToTalkWasMuted ? 1.0 : 0.0);
        processor.getValueTreeState().getParameter(SonobusAudioProcessor::paramMainRecvMute)->setValueNotifyingHost(0.0);            
        mPushToTalkKeyDown = false;
        return true;
    }
    else if (pttdown && !mPushToTalkKeyDown) {
        // press
        DBG("T press");
        // mute others, send self
        mPushToTalkWasMuted = processor.getValueTreeState().getParameter(SonobusAudioProcessor::paramMainSendMute)->getValue() > 0;
        processor.getValueTreeState().getParameter(SonobusAudioProcessor::paramMainRecvMute)->setValueNotifyingHost(1.0);            
        processor.getValueTreeState().getParameter(SonobusAudioProcessor::paramMainSendMute)->setValueNotifyingHost(0.0);
        mPushToTalkKeyDown = true;
        return true;
    }        
    
    return pttdown;
}


void SonobusAudioProcessorEditor::showSettings(bool flag)
{
    DBG("Got settings click");

    if (flag && settingsCalloutBox == nullptr) {
        
        //Viewport * wrap = new Viewport();
        
        Component* dw = this; 
        
#if JUCE_IOS || JUCE_ANDROID
        int defWidth = 300;
        int defHeight = 420;
#else
        int defWidth = 320;
        int defHeight = 400;
#endif
        
        defWidth = jmin(defWidth + 8, dw->getWidth() - 20);
        defHeight = jmin(defHeight + 8, dw->getHeight() - 24);
        
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
                
#if JUCE_IOS || JUCE_ANDROID
                mAudioDeviceSelector->setItemHeight(44);
#endif
                
                mAudioOptionsViewport = std::make_unique<Viewport>();
                mAudioOptionsViewport->setViewedComponent(mAudioDeviceSelector.get(), false);

                
            }

            if (firsttime) {
                mSettingsTab->addTab(TRANS("AUDIO"), Colour::fromFloatRGBA(0.1, 0.1, 0.1, 1.0), mAudioOptionsViewport.get(), false);
            }
            
        }


        if (firsttime) {
            mOtherOptionsViewport = std::make_unique<Viewport>();
            mOtherOptionsViewport->setViewedComponent(mOptionsComponent.get(), false);

            mSettingsTab->addTab(TRANS("OPTIONS"),Colour::fromFloatRGBA(0.1, 0.1, 0.1, 1.0), mOtherOptionsViewport.get(), false);
           // mSettingsTab->addTab(TRANS("HELP"), Colour::fromFloatRGBA(0.1, 0.1, 0.1, 1.0), mHelpComponent.get(), false);

            mRecordOptionsViewport = std::make_unique<Viewport>();
            mRecordOptionsViewport->setViewedComponent(mRecOptionsComponent.get(), false);

            mSettingsTab->addTab(TRANS("RECORDING"),Colour::fromFloatRGBA(0.1, 0.1, 0.1, 1.0), mRecordOptionsViewport.get(), false);

        }
        

        auto wrap = std::make_unique<Component>();

        wrap->addAndMakeVisible(mSettingsTab.get());

        mSettingsTab->setBounds(Rectangle<int>(0,0,defWidth,defHeight));
        
        if (mAudioDeviceSelector) {
            mAudioDeviceSelector->setBounds(Rectangle<int>(0,0,defWidth - 10,mAudioDeviceSelector->getHeight()));
        }
        mOptionsComponent->setBounds(Rectangle<int>(0,0,defWidth - 10, minOptionsHeight));
        mRecOptionsComponent->setBounds(Rectangle<int>(0,0,defWidth - 10, minRecOptionsHeight));

        wrap->setSize(defWidth,defHeight);
        
        optionsBox.performLayout(mOptionsComponent->getLocalBounds());

        recOptionsBox.performLayout(mRecOptionsComponent->getLocalBounds());

        mOptionsAutosizeStaticLabel->setBounds(mBufferTimeSlider->getBounds().removeFromLeft(mBufferTimeSlider->getWidth()*0.75));

        
        updateOptionsState();
        
       
        Rectangle<int> bounds =  dw->getLocalArea(nullptr, mTitleLabel->getScreenBounds().reduced(10));
        DBG("callout bounds: " << bounds.toString());
        settingsCalloutBox = & CallOutBox::launchAsynchronously (std::move(wrap), bounds , dw, false);
        if (CallOutBox * box = dynamic_cast<CallOutBox*>(settingsCalloutBox.get())) {
            box->setDismissalMouseClicksAreAlwaysConsumed(true);
        }
        
        settingsClosedTimestamp = 0;


#if JUCE_WINDOWS
        if (firsttime && JUCEApplicationBase::isStandaloneApp() && getAudioDeviceManager())
        {
            // on windows, if current audio device type isn't ASIO, prompt them that it should be
            auto devtype = getAudioDeviceManager()->getCurrentAudioDeviceType();
            if (!devtype.equalsIgnoreCase("ASIO")) {
                auto mesg = String(TRANS("Using an ASIO audio device type is strongly recommended. If your audio interface did not come with one, please install ASIO4ALL (asio4all.org) and configure it first."));
                showPopTip(mesg, 0, mSettingsTab->getTabbedButtonBar().getTabButton(0), 320);
            }
        }
#endif

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
    String locstr;
    locstr << processor.getLocalIPAddress().toString() << " : " << processor.getUdpLocalPort();
    mLocalAddressLabel->setText(locstr, dontSendNotification);

    currConnected = processor.isConnectedToServer();
    currGroup = processor.getCurrentJoinedGroup();

    if (currConnected && currGroup.isNotEmpty()) {
        mConnectButton->setButtonText(TRANS("Disconnect"));
        mConnectButton->setTextJustification(Justification::centredTop);
        mConnectButton->setToggleState(true, dontSendNotification);
    }
    else {
        mConnectButton->setButtonText(TRANS("Connect..."));
        mConnectButton->setToggleState(false, dontSendNotification);
        
        mPeerContainer->resetPendingUsers();
    }

    if (mRecordingButton) {
        mRecordingButton->setToggleState(processor.isRecordingToFile(), dontSendNotification);
    }

    mReverbModelChoice->setSelectedId(processor.getMainReverbModel(), dontSendNotification);
    mEffectsButton->setToggleState(processor.getMainReverbEnabled(), dontSendNotification);


    if (mReverbEnabledButton->getToggleState()) {
        mReverbHeaderBg->setFill(Colour::fromFloatRGBA(0.2f, 0.5f, 0.7f, 0.5f));                
    } else {
        mReverbHeaderBg->setFill(Colour(0xff2a2a2a));
    }

    mReverbEnabledButton->setAlpha(mReverbEnabledButton->getToggleState() ? 1.0 : 0.5);

    
    int sendchval = (int) processor.getValueTreeState().getParameter(SonobusAudioProcessor::paramSendChannels)->convertFrom0to1( processor.getValueTreeState().getParameter(SonobusAudioProcessor::paramSendChannels)->getValue());
    mSendChannelsChoice->setSelectedId(sendchval, dontSendNotification);

    int panChannels = jmax(outChannels, sendchval);

    
#if 0
    if (panChannels > 1) {        
        mInMonPanLabel1->setVisible(true);
        if (inChannels > 1) {
            mInMonPanSlider1->setVisible(true);
            mInMonPanMonoSlider->setVisible(false);  
        }
        else {
            mInMonPanSlider1->setVisible(false);
            mInMonPanMonoSlider->setVisible(true);            
        }
    } else {
        mInMonPanSlider1->setVisible(false);
        mInMonPanLabel1->setVisible(false);            
        mInMonPanMonoSlider->setVisible(false);
    }

    if (inChannels > 1) {
        mInMonPanSlider2->setVisible(true);
        mInMonPanLabel2->setVisible(true);
        
    }
    else {        
        mInMonPanSlider2->setVisible(false);
        mInMonPanLabel2->setVisible(false);
    }
#else
    
    if (panChannels > 1) {        
        if (inChannels > 1) {
            mInMonPanMonoSlider->setVisible(false);  
            mInMonPanStereoSlider->setVisible(true);  
        }
        else {
            mInMonPanStereoSlider->setVisible(false);  
            mInMonPanMonoSlider->setVisible(true);            
        }
    } else {
        mInMonPanStereoSlider->setVisible(false);  
        mInMonPanMonoSlider->setVisible(false);
    }

    float monopan = processor.getValueTreeState().getParameter(SonobusAudioProcessor::paramInMonitorMonoPan)->convertFrom0to1( processor.getValueTreeState().getParameter(SonobusAudioProcessor::paramInMonitorMonoPan)->getValue());
    mInMonPanMonoSlider->setValue(monopan, dontSendNotification);

    float span1 = processor.getValueTreeState().getParameter(SonobusAudioProcessor::paramInMonitorPan1)->convertFrom0to1( processor.getValueTreeState().getParameter(SonobusAudioProcessor::paramInMonitorPan1)->getValue());
    mInMonPanSlider1->setValue(span1, dontSendNotification);

    float span2 = processor.getValueTreeState().getParameter(SonobusAudioProcessor::paramInMonitorPan2)->convertFrom0to1( processor.getValueTreeState().getParameter(SonobusAudioProcessor::paramInMonitorPan2)->getValue());
    mInMonPanSlider1->setValue(span2, dontSendNotification);

    
#endif
    
    bool sendmute = processor.getValueTreeState().getParameter(SonobusAudioProcessor::paramMainSendMute)->getValue();
    if (!mMainPushToTalkButton->isMouseButtonDown() && !mPushToTalkKeyDown) {
        mMainPushToTalkButton->setVisible(sendmute);
    }
    
    SonobusAudioProcessor::CompressorParams compParams;
    processor.getInputCompressorParams(compParams);
    mInCompressorView->updateParams(compParams);

    SonobusAudioProcessor::CompressorParams expandParams;
    processor.getInputExpanderParams(expandParams);
    mInExpanderView->updateParams(expandParams);

    SonobusAudioProcessor::ParametricEqParams eqParams;
    processor.getInputEqParams(eqParams);
    mInEqView->updateParams(eqParams);

    SonobusAudioProcessor::CompressorParams limparams;
    processor.getInputLimiterParams(limparams);
    mOptionsInputLimiterButton->setToggleState(limparams.enabled, dontSendNotification);
    
    mInEffectsButton->setToggleState(processor.getInputEffectsActive(), dontSendNotification);
    
    if (processor.getMainReverbModel() == SonobusAudioProcessor::ReverbModelFreeverb) {
        mReverbPreDelaySlider->setVisible(false);
        mReverbPreDelayLabel->setVisible(false);
    } else {
        mReverbPreDelaySlider->setVisible(true);
        mReverbPreDelayLabel->setVisible(true);        
    }
  
    if (!currGroup.isEmpty() && currConnected)
    {
        String grouptext;
        grouptext << (currConnectionInfo.groupIsPublic ? TRANS("[P] ") : "") << currGroup;
        mMainGroupLabel->setText(grouptext, dontSendNotification);
        String userstr;

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
        mMainLinkButton->setVisible(true);
        
        if (processor.getNumberRemotePeers() == 0 && mPeerContainer->getPendingPeerCount() == 0) {
            String labstr;
            labstr << TRANS("Waiting for other users to join group \"") << currGroup << "\"...";
            mMainMessageLabel->setText(labstr, dontSendNotification);
            mMainMessageLabel->setVisible(true);
        } else {
            mMainMessageLabel->setText("", dontSendNotification);
            mMainMessageLabel->setVisible(false);
        }
        
        mSetupAudioButton->setVisible(false);
        mPatchbayButton->setVisible(false);
    }
    else {
        mMainGroupLabel->setText("", dontSendNotification);
        mMainUserLabel->setText("", dontSendNotification);

        mMainGroupImage->setVisible(false);
        mMainPersonImage->setVisible(false);
        mMainPeerLabel->setVisible(false);
        mMainLinkButton->setVisible(false);

        mMainMessageLabel->setVisible(true);

        if (processor.getNumberRemotePeers() == 0 /* || !currConnected */ ) {
            String message;
            message += TRANS("Press Connect button to start.") + "\n\n" + TRANS("Please use headphones if you are using a microphone!");
            mMainMessageLabel->setText(message, dontSendNotification);
        } else {
            mMainMessageLabel->setText("", dontSendNotification);
        }

        
        
#if JUCE_IOS || JUCE_ANDROID
        mPatchbayButton->setVisible(false);
        mSetupAudioButton->setVisible(false);
#else


        if (JUCEApplication::isStandaloneApp()) {

            mSetupAudioButton->setVisible(processor.getNumberRemotePeers() == 0);
            
            if (getShouldOverrideSampleRateValue) {
                Value * val = getShouldOverrideSampleRateValue();
                mOptionsOverrideSamplerateButton->setToggleState((bool)val->getValue(), dontSendNotification);
            }
            if (getShouldCheckForNewVersionValue) {
                Value * val = getShouldCheckForNewVersionValue();
                mOptionsShouldCheckForUpdateButton->setToggleState((bool)val->getValue(), dontSendNotification);
            }
        } else {
            mSetupAudioButton->setVisible(false);            
        }

        
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
    
    if (pname == SonobusAudioProcessor::paramMainSendMute) {
        {
            const ScopedLock sl (clientStateLock);
            clientEvents.add(ClientEvent(ClientEvent::PeerChangedState, ""));
        }
        triggerAsyncUpdate();
    }
    else if (pname == SonobusAudioProcessor::paramMainRecvMute) {
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
    else if (pname == SonobusAudioProcessor::paramMainReverbModel) {
        {
            const ScopedLock sl (clientStateLock);
            clientEvents.add(ClientEvent(ClientEvent::PeerChangedState, ""));
        }
        triggerAsyncUpdate();
    }
    else if (pname == SonobusAudioProcessor::paramMainReverbEnabled) {
        {
            const ScopedLock sl (clientStateLock);
            clientEvents.add(ClientEvent(ClientEvent::PeerChangedState, ""));
        }
        triggerAsyncUpdate();
    }
    else if (pname == SonobusAudioProcessor::paramSendChannels) {
        {
            const ScopedLock sl (clientStateLock);
            clientEvents.add(ClientEvent(ClientEvent::PeerChangedState, ""));
        }
        triggerAsyncUpdate();
    }
    else if (pname == SonobusAudioProcessor::paramInMonitorPan1 
             || pname == SonobusAudioProcessor::paramInMonitorPan2
             || pname == SonobusAudioProcessor::paramInMonitorMonoPan
             ) 
    {
        mPanChanged = true;
        triggerAsyncUpdate();
    }
}

void SonobusAudioProcessorEditor::updateServerStatusLabel(const String & mesg, bool mainonly)
{
    const double fadeAfterSec = 5.0;
    //mMainStatusLabel->setText(mesg, dontSendNotification);
    //Desktop::getInstance().getAnimator().fadeIn(mMainStatusLabel.get(), 200);
    //serverStatusFadeTimestamp = Time::getMillisecondCounterHiRes() * 1e-3 + fadeAfterSec;
    
    if (!mainonly) {
        mServerStatusLabel->setText(mesg, dontSendNotification);
        mPublicServerStatusInfoLabel->setText(mesg, dontSendNotification);
        mServerInfoLabel->setVisible(false);
        mServerStatusLabel->setVisible(true);
        mPublicServerStatusInfoLabel->setVisible(true);
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
    if (mPanChanged) {
        float monopan = processor.getValueTreeState().getParameter(SonobusAudioProcessor::paramInMonitorMonoPan)->convertFrom0to1( processor.getValueTreeState().getParameter(SonobusAudioProcessor::paramInMonitorMonoPan)->getValue());
        mInMonPanMonoSlider->setValue(monopan, dontSendNotification);

        float span1 = processor.getValueTreeState().getParameter(SonobusAudioProcessor::paramInMonitorPan1)->convertFrom0to1( processor.getValueTreeState().getParameter(SonobusAudioProcessor::paramInMonitorPan1)->getValue());
        mInMonPanSlider1->setValue(span1, dontSendNotification);

        float span2 = processor.getValueTreeState().getParameter(SonobusAudioProcessor::paramInMonitorPan2)->convertFrom0to1( processor.getValueTreeState().getParameter(SonobusAudioProcessor::paramInMonitorPan2)->getValue());
        mInMonPanSlider1->setValue(span2, dontSendNotification);

        mPanChanged = false;
    }
    
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

                    DBG("Trying again with name: " << currConnectionInfo.userName);
                    connectWithInfo(currConnectionInfo, true);
                    
                    return;
                    
                } else {
                    statstr = TRANS("Connect failed: ") + ev.message;
                }

            }
            
            // now attempt to join group
            if (ev.success) {

                if (currConnectionInfo.groupName.isNotEmpty()) {

                    currConnectionInfo.timestamp = Time::getCurrentTime().toMilliseconds();
                    processor.addRecentServerConnectionInfo(currConnectionInfo);

                    processor.setWatchPublicGroups(false);

                    processor.joinServerGroup(currConnectionInfo.groupName, currConnectionInfo.groupPassword, currConnectionInfo.groupIsPublic);
                }
                else {
                    // we've connected but have not specified group, assume we want to see public groups
                    processor.setWatchPublicGroups(true);
                    updatePublicGroups();
                }

                updateServerFieldsFromConnectionInfo();
            }
            else {
                // switch to group page
                int adjindex = mConnectTab->getCurrentTabIndex() + (mConnectTab->getNumTabs() > 3 ? 0 : 1);
                if (adjindex != 1 && adjindex != 2) {
                    if (currConnectionInfo.groupIsPublic) {
                        mConnectTab->setCurrentTabIndex(mConnectTab->getNumTabs() > 3 ? 2 : 1);
                    } else {
                        mConnectTab->setCurrentTabIndex(mConnectTab->getNumTabs() > 3 ? 1 : 0);
                    }
                }
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
            mPeerContainer->resetPendingUsers();
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

                mServerGroupPasswordEditor->setColour(TextEditor::backgroundColourId, Colour(0xff880000));
                mServerGroupPasswordEditor->repaint();

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
            mPeerContainer->resetPendingUsers();
            updateServerStatusLabel(statstr);
            updatePeerState(true);
            updateState();
        }
        else if (ev.type == ClientEvent::PublicGroupModifiedEvent) {
            updatePublicGroups();
        }
        else if (ev.type == ClientEvent::PublicGroupDeletedEvent) {
            updatePublicGroups();
        }
        else if (ev.type == ClientEvent::PeerJoinEvent) {
            DBG("Peer " << ev.user << "joined doing full update");
            updatePeerState(true);
            updateState();
        }
        else if (ev.type == ClientEvent::PeerLeaveEvent) {
            updatePeerState(true);
            updateState();
        }
        else if (ev.type == ClientEvent::PeerPendingJoinEvent) {
            mPeerContainer->peerPendingJoin(ev.group, ev.user);
        }
        else if (ev.type == ClientEvent::PeerFailedJoinEvent) {
            mPeerContainer->peerFailedJoin(ev.group, ev.user);
        }
    }
    
    if (mReloadFile) {
        loadAudioFromURL(mCurrentAudioFile);
        mReloadFile = false;
    }
}

void SonobusAudioProcessorEditor::parentHierarchyChanged()
{    
    AudioProcessorEditor::parentHierarchyChanged();
}

//==============================================================================
void SonobusAudioProcessorEditor::paint (Graphics& g)
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll (getLookAndFeel().findColour (ResizableWindow::backgroundColourId));

}

void SonobusAudioProcessorEditor::resized()
{
    // This is generally where you'll want to lay out the positions of any
    // subcomponents in your editor..

    Component::resized();
 
#if JUCE_IOS || JUCE_ANDROID
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
    
    
   
    
    DBG("RESIZED to " << getWidth() << " " << getHeight());
    
    auto mainBounds = getLocalBounds();
    const auto menuHeight = getLookAndFeel().getDefaultMenuBarHeight();
    
    if (mMenuBar) {
        auto menuBounds = mainBounds.removeFromTop(menuHeight);
        mMenuBar->setBounds(menuBounds);
    }
    
    mainBox.performLayout(mainBounds);    

    Rectangle<int> peersminbounds = mPeerContainer->getMinimumContentBounds();
    
    mPeerContainer->setBounds(Rectangle<int>(0, 0, std::max(peersminbounds.getWidth(), mPeerViewport->getWidth() - 10), std::max(peersminbounds.getHeight() + 5, mPeerViewport->getHeight())));

#if JUCE_IOS || JUCE_ANDROID
    mSetupAudioButton->setSize(150, 1);
#else
    mSetupAudioButton->setSize(150, 50);	
#endif
	
    mSetupAudioButton->setCentrePosition(mPeerViewport->getX() + 0.5*mPeerViewport->getWidth(), mPeerViewport->getY() + 45);
    
    mMainMessageLabel->setBounds(mPeerViewport->getX() + 10, mSetupAudioButton->getBottom() + 10, mPeerViewport->getRight() - mPeerViewport->getX() - 20, jmin(120, mPeerViewport->getBottom() - (mSetupAudioButton->getBottom() + 10)));
    
    auto metbgbounds = Rectangle<int>(mMetEnableButton->getX(), mMetEnableButton->getY(), mMetConfigButton->getRight() - mMetEnableButton->getX(),  mMetEnableButton->getHeight()).expanded(2, 2);
    mMetButtonBg->setRectangle (metbgbounds.toFloat());


    auto grouptextbounds = Rectangle<int>(mMainPeerLabel->getX(), mMainGroupImage->getY(), mMainUserLabel->getRight() - mMainPeerLabel->getX(),  mMainGroupImage->getHeight()).expanded(2, 2);
    mMainLinkButton->setBounds(grouptextbounds);
    
    mDragDropBg->setRectangle (getLocalBounds().toFloat());


    auto filebgbounds = Rectangle<int>(mPlayButton->getX(), mWaveformThumbnail->getY(), 
                                       mDismissTransportButton->getRight() - mPlayButton->getX(),  
                                       mDismissTransportButton->getBottom() - mWaveformThumbnail->getY()).expanded(4, 6);
    mFileAreaBg->setRectangle (filebgbounds.toFloat());
    
    
    // connect component stuff
    mConnectComponent->setBounds(getLocalBounds());
    mConnectComponentBg->setRectangle (mConnectComponent->getLocalBounds().toFloat());

    if (getWidth() > 700) {
        if (mConnectTab->getNumTabs() > 3) {
            // move recents to main connect component, out of tab
            int adjcurrtab = jmax(0, mConnectTab->getCurrentTabIndex() - 1);

            mConnectTab->removeTab(0);
            mRecentsGroup->addAndMakeVisible(mRecentsContainer.get());
            mConnectComponent->addAndMakeVisible(mRecentsGroup.get());

            mConnectTab->setCurrentTabIndex(adjcurrtab);
            updateLayout();
        }
    } else {
        if (mConnectTab->getNumTabs() < 4) {
            int tabsel = mConnectTab->getCurrentTabIndex();
            mRecentsGroup->removeChildComponent(mRecentsContainer.get());
            mRecentsGroup->setVisible(false);
            
            mConnectTab->addTab(TRANS("RECENTS"), Colour::fromFloatRGBA(0.1, 0.1, 0.1, 1.0), mRecentsContainer.get(), false);
            mConnectTab->moveTab(3, 0);
            mConnectTab->setCurrentTabIndex(tabsel + 1);
            updateLayout();
        }
    }

    connectMainBox.performLayout(mConnectComponent->getLocalBounds());
    
    mServerConnectContainer->setBounds(0,0, 
                                       mServerConnectViewport->getWidth() - (mServerConnectViewport->getHeight() < minServerConnectHeight ? mServerConnectViewport->getScrollBarThickness() : 0 ),
                                       jmax(minServerConnectHeight, mServerConnectViewport->getHeight()));
    
    remoteBox.performLayout(mDirectConnectContainer->getLocalBounds().withSizeKeepingCentre(jmin(400, mDirectConnectContainer->getWidth()), mDirectConnectContainer->getHeight()));
    serverBox.performLayout(mServerConnectContainer->getLocalBounds().withSizeKeepingCentre(jmin(400, mServerConnectContainer->getWidth()), mServerConnectContainer->getHeight()));

    //mPublicServerConnectContainer->setBounds(0,0,
    //                                   mPublicServerConnectViewport->getWidth() - (mPublicServerConnectViewport->getHeight() < minServerConnectHeight ? mPublicServerConnectViewport->getScrollBarThickness() : 0 ),
    //                                   jmax(minServerConnectHeight, mPublicServerConnectViewport->getHeight()));


    publicGroupsBox.performLayout(mPublicServerConnectContainer->getLocalBounds());

    mPublicGroupsListBox->setBounds(mPublicGroupComponent->getLocalBounds().reduced(4).withTrimmedTop(10));


    if (mConnectTab->getNumTabs() < 4) {
        mRecentsContainer->setBounds(mRecentsGroup->getLocalBounds().reduced(4).withTrimmedTop(10));
    }
    recentsBox.performLayout(mRecentsContainer->getLocalBounds());

    mServerInfoLabel->setBounds(mServerStatusLabel->getBounds());

    mConnectionTimeLabel->setBounds(mConnectButton->getBounds().removeFromBottom(16));
    
    if (mRecordingButton) {
        mFileRecordingLabel->setBounds(mRecordingButton->getBounds().removeFromBottom(14).translated(0, 1));
    }

    //mInMonPanMonoSlider->setBounds(mInMonPanSlider1->getBounds().withRight(mInMonPanSlider2->getRight()));    
    mInMonPanMonoSlider->setBounds(mInMonPanStereoSlider->getBounds());
    mInMonPanMonoSlider->setMouseDragSensitivity(jmax(128, mInMonPanMonoSlider->getWidth()));

    mInMonPanStereoSlider->setMouseDragSensitivity(jmax(128, mInMonPanStereoSlider->getWidth()));

    mDrySlider->setMouseDragSensitivity(jmax(128, mDrySlider->getWidth()));
    mOutGainSlider->setMouseDragSensitivity(jmax(128, mOutGainSlider->getWidth()));
    mInGainSlider->setMouseDragSensitivity(jmax(128, mInGainSlider->getWidth()));

    mDryLabel->setBounds(mDrySlider->getBounds().removeFromTop(14).removeFromLeft(mDrySlider->getWidth() - mDrySlider->getTextBoxWidth() + 3).translated(4, 0));
    mInGainLabel->setBounds(mInGainSlider->getBounds().removeFromTop(14).removeFromLeft(mInGainSlider->getWidth() - mInGainSlider->getTextBoxWidth() + 3).translated(4, 0));
    mOutGainLabel->setBounds(mOutGainSlider->getBounds().removeFromTop(14).removeFromLeft(mOutGainSlider->getWidth() - mOutGainSlider->getTextBoxWidth() + 3).translated(4, 0));

    
    if (mConnectComponent->isVisible()) {
        mRecentsListBox->updateContent();
    }
    
    
    
    Component* dw = this; 
    
    if (auto * callout = dynamic_cast<CallOutBox*>(inEffectsCalloutBox.get())) {
        callout->updatePosition(dw->getLocalArea(nullptr, mInEffectsButton->getScreenBounds()), dw->getLocalBounds());
    }

    if (auto * callout = dynamic_cast<CallOutBox*>(effectsCalloutBox.get())) {
        callout->updatePosition(dw->getLocalArea(nullptr, mEffectsButton->getScreenBounds()), dw->getLocalBounds());
    }

    if (auto * callout = dynamic_cast<CallOutBox*>(inPannerCalloutBox.get())) {
        callout->updatePosition(dw->getLocalArea(nullptr, mPanButton->getScreenBounds()), dw->getLocalBounds());
    }

    updateSliderSnap();

}


void SonobusAudioProcessorEditor::updateLayout()
{
    int minKnobWidth = 50;
    int minSliderWidth = 50;
    int minPannerWidth = 40;
    int maxPannerWidth = 100;
    int minitemheight = 36;
    int knobitemheight = 80;
    int minpassheight = 30;
    int setitemheight = 36;
    int minButtonWidth = 90;
    int sliderheight = 44;
    int meterwidth = 22 ;
    int servLabelWidth = 72;
    int iconheight = 24;
    int iconwidth = iconheight;
    int knoblabelheight = 18;
    int panbuttwidth = 26;
    
#if JUCE_IOS || JUCE_ANDROID
    // make the button heights a bit more for touchscreen purposes
    minitemheight = 44;
    knobitemheight = 90;
    minpassheight = 38;
    panbuttwidth = 32;
#endif
    
    
#if 0
    
    inGainBox.items.clear();
    inGainBox.flexDirection = FlexBox::Direction::column;
    inGainBox.items.add(FlexItem(minKnobWidth, knoblabelheight, *mInGainLabel).withMargin(0).withFlex(0));
    inGainBox.items.add(FlexItem(minKnobWidth, minitemheight, *mInGainSlider).withMargin(0).withFlex(1));

    dryBox.items.clear();
    dryBox.flexDirection = FlexBox::Direction::column;
    dryBox.items.add(FlexItem(minKnobWidth, knoblabelheight, *mDryLabel).withMargin(0).withFlex(0));
    dryBox.items.add(FlexItem(minKnobWidth, minitemheight, *mDrySlider).withMargin(0).withFlex(1));

    
    outBox.items.clear();
    outBox.flexDirection = FlexBox::Direction::column;
    outBox.items.add(FlexItem(minKnobWidth, knoblabelheight, *mOutGainLabel).withMargin(0).withFlex(0));
    outBox.items.add(FlexItem(minKnobWidth, minitemheight, *mOutGainSlider).withMargin(0).withFlex(1)); //.withAlignSelf(FlexItem::AlignSelf::center));

    inMeterBox.items.clear();
    inMeterBox.flexDirection = FlexBox::Direction::column;
    inMeterBox.items.add(FlexItem(meterwidth, minitemheight, *inputMeter).withMargin(0).withFlex(1).withMaxWidth(meterwidth).withAlignSelf(FlexItem::AlignSelf::center));

    mainMeterBox.items.clear();
    mainMeterBox.flexDirection = FlexBox::Direction::column;
    mainMeterBox.items.add(FlexItem(panbuttwidth, minitemheight - 4, *mEffectsButton).withMargin(0).withFlex(0)); 
    mainMeterBox.items.add(FlexItem(2, 2).withMargin(0).withFlex(0));
    mainMeterBox.items.add(FlexItem(meterwidth, minitemheight, *outputMeter).withMargin(0).withFlex(1).withMaxWidth(meterwidth).withAlignSelf(FlexItem::AlignSelf::center));

    
    knobButtonBox.items.clear();
    knobButtonBox.flexDirection = FlexBox::Direction::column;
    knobButtonBox.items.add(FlexItem(38, minitemheight, *mPanButton).withMargin(0).withFlex(1));
    knobButtonBox.items.add(FlexItem(2, 6).withMargin(0).withFlex(0));
    knobButtonBox.items.add(FlexItem(38, minitemheight, *mInEffectsButton).withMargin(0).withFlex(1));

    
    knobBox.items.clear();
    knobBox.flexDirection = FlexBox::Direction::row;
    knobBox.items.add(FlexItem(4, 5).withMargin(0).withFlex(isNarrow ? 0.0 : 0.1));
    knobBox.items.add(FlexItem(minKnobWidth, minitemheight, inGainBox).withMargin(0).withFlex(1).withMaxWidth(isNarrow ? 160 : 120));
    knobBox.items.add(FlexItem(2, 5).withMargin(0).withFlex(0));
    knobBox.items.add(FlexItem(38, minitemheight, knobButtonBox).withMargin(0).withFlex(1).withMaxWidth(50)); // //.withAlignSelf(FlexItem::AlignSelf::center));
    knobBox.items.add(FlexItem(6, 5).withMargin(0).withFlex(0.1).withMaxWidth(20));
    knobBox.items.add(FlexItem(meterwidth, minitemheight, inMeterBox).withMargin(0).withFlex(0));
    knobBox.items.add(FlexItem(2, 5).withMargin(0).withFlex(0.1).withMaxWidth(20));
    knobBox.items.add(FlexItem(minKnobWidth, minitemheight, dryBox).withMargin(0).withFlex(1).withMaxWidth(isNarrow ? 160 : 120));
    knobBox.items.add(FlexItem(2, 5).withMargin(0).withFlex(0));
    knobBox.items.add(FlexItem(minKnobWidth, minitemheight, outBox).withMargin(0).withFlex(1).withMaxWidth(isNarrow ? 160 : 120));
    knobBox.items.add(FlexItem(2, 5).withMargin(0).withFlex(0));
    knobBox.items.add(FlexItem(jmax(panbuttwidth,meterwidth), minitemheight, mainMeterBox).withMargin(0).withFlex(1).withMaxWidth(42)); //.withAlignSelf(FlexItem::AlignSelf::center));
    knobBox.items.add(FlexItem(isNarrow ? 14 : 16, 5).withMargin(0).withFlex(0));

    paramsBox.items.clear();
    paramsBox.flexDirection = FlexBox::Direction::column;
    paramsBox.items.add(FlexItem(minKnobWidth*4 + 2*meterwidth, knobitemheight, knobBox).withMargin(2).withFlex(0));
    
#else

    inGainBox.items.clear();
    inGainBox.flexDirection = FlexBox::Direction::row;
    inGainBox.items.add(FlexItem(minKnobWidth, minitemheight, *mInGainSlider).withMargin(0).withFlex(1));

    int choicew = 60;
    int mutew = 56;

    dryBox.items.clear();
    dryBox.flexDirection = FlexBox::Direction::row;
    dryBox.items.add(FlexItem(4, 6).withMargin(0).withFlex(0));
    dryBox.items.add(FlexItem(minKnobWidth, minitemheight, *mDrySlider).withMargin(0).withFlex(1));

    
    outBox.items.clear();
    outBox.flexDirection = FlexBox::Direction::column;
    outBox.items.add(FlexItem(minKnobWidth, minitemheight, *mOutGainSlider).withMargin(0).withFlex(1)); //.withAlignSelf(FlexItem::AlignSelf::center));

    inMeterBox.items.clear();
    inMeterBox.flexDirection = FlexBox::Direction::column;
    inMeterBox.items.add(FlexItem(meterwidth, minitemheight, *inputMeter).withMargin(0).withFlex(1).withMaxWidth(meterwidth).withAlignSelf(FlexItem::AlignSelf::center));

    mainMeterBox.items.clear();
    mainMeterBox.flexDirection = FlexBox::Direction::column;
    mainMeterBox.items.add(FlexItem(meterwidth, minitemheight, *outputMeter).withMargin(0).withFlex(1).withMaxWidth(meterwidth).withAlignSelf(FlexItem::AlignSelf::center));
    
    inputPannerBox.items.clear();
    inputPannerBox.flexDirection = FlexBox::Direction::row;
    inputPannerBox.items.add(FlexItem(4, 6).withMargin(0).withFlex(0.2));
    inputPannerBox.items.add(FlexItem(mutew, minitemheight, *mPanButton).withMargin(0).withFlex(0));
    inputPannerBox.items.add(FlexItem(2, 6).withMargin(0).withFlex(0.1));
    inputPannerBox.items.add(FlexItem(mutew, minitemheight, *mInEffectsButton).withMargin(0).withFlex(0));
    inputPannerBox.items.add(FlexItem(4, 6).withMargin(0).withFlex(0.2));

    inputLeftBox.items.clear();
    inputLeftBox.flexDirection = FlexBox::Direction::column;
    inputLeftBox.items.add(FlexItem(minSliderWidth, minitemheight, inGainBox).withMargin(0).withFlex(1)); //.withMaxWidth(isNarrow ? 160 : 120));
    inputLeftBox.items.add(FlexItem(4, 2).withMargin(0).withFlex(0));
    inputLeftBox.items.add(FlexItem(2*mutew + 10, minitemheight, inputPannerBox).withMargin(0).withFlex(1)); //.withMaxWidth(maxPannerWidth));

    inputButtonBox.items.clear();
    inputButtonBox.flexDirection = FlexBox::Direction::row;
    inputButtonBox.items.add(FlexItem(4, 6).withMargin(0).withFlex(0.2));
    inputButtonBox.items.add(FlexItem(mutew, minpassheight, *mInMuteButton).withMargin(0).withFlex(0) ); //.withMaxWidth(maxPannerWidth));
    inputButtonBox.items.add(FlexItem(2, 6).withMargin(0).withFlex(0.1));
    inputButtonBox.items.add(FlexItem(mutew, minitemheight, *mInSoloButton).withMargin(0).withFlex(0) ); //.withMaxWidth(maxPannerWidth));
    inputButtonBox.items.add(FlexItem(4, 6).withMargin(0).withFlex(0.2));

    inputRightBox.items.clear();
    inputRightBox.flexDirection = FlexBox::Direction::column;
    inputRightBox.items.add(FlexItem(minSliderWidth, minitemheight, dryBox).withMargin(0).withFlex(1)); //.withMaxWidth(isNarrow ? 160 : 120));
    inputRightBox.items.add(FlexItem(4, 2).withMargin(0).withFlex(0));
    inputRightBox.items.add(FlexItem(2*mutew + 10, minitemheight, inputButtonBox).withMargin(0).withFlex(1)); //.withMaxWidth(maxPannerWidth));

    inputMainBox.items.clear();
    inputMainBox.flexDirection = FlexBox::Direction::row;
    inputMainBox.items.add(FlexItem(jmax(minSliderWidth, 2*mutew + 16), minitemheight + minitemheight, inputLeftBox).withMargin(0).withFlex(1)); //.withMaxWidth(isNarrow ? 160 : 120));
    inputMainBox.items.add(FlexItem(4, 6).withMargin(0).withFlex(0));
    inputMainBox.items.add(FlexItem(meterwidth, minitemheight, inMeterBox).withMargin(0).withFlex(0));
    inputMainBox.items.add(FlexItem(4, 6).withMargin(0).withFlex(0));
    inputMainBox.items.add(FlexItem(jmax(minSliderWidth, 2*mutew + 16), minitemheight + minitemheight, inputRightBox).withMargin(0).withFlex(1)) ; //.withMaxWidth(isNarrow ? 160 : 120));

    int minmainw = jmax(minPannerWidth, 2*mutew + 8) + meterwidth + jmax(minPannerWidth, 2*mutew + 8);
    
    outputMainBox.items.clear();
    outputMainBox.flexDirection = FlexBox::Direction::row;
    outputMainBox.items.add(FlexItem(7, 6).withMargin(0).withFlex(0));
    outputMainBox.items.add(FlexItem(44, minitemheight, *mEffectsButton).withMargin(0).withFlex(0)); 
    outputMainBox.items.add(FlexItem(4, 6).withMargin(0).withFlex(0));
    outputMainBox.items.add(FlexItem(minSliderWidth, minitemheight, outBox).withMargin(0).withFlex(1)); //.withMaxWidth(isNarrow ? 160 : 120));
    outputMainBox.items.add(FlexItem(4, 6).withMargin(0).withFlex(0));
    outputMainBox.items.add(FlexItem(meterwidth, minitemheight, mainMeterBox).withMargin(0).withFlex(0)); 
    if (isNarrow) {
        outputMainBox.items.add(FlexItem(21, 6).withMargin(0).withFlex(0));
    } else {
        outputMainBox.items.add(FlexItem(16, 6).withMargin(0).withFlex(0));        
    }

    
    paramsBox.items.clear();
    paramsBox.flexDirection = FlexBox::Direction::column;
    paramsBox.items.add(FlexItem(minmainw, minitemheight + minitemheight, inputMainBox).withMargin(2).withFlex(0));

    
#endif
    
    // options
    optionsNetbufBox.items.clear();
    optionsNetbufBox.flexDirection = FlexBox::Direction::row;
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


    optionsUdpBox.items.clear();
    optionsUdpBox.flexDirection = FlexBox::Direction::row;
    optionsUdpBox.items.add(FlexItem(10, 12));
    optionsUdpBox.items.add(FlexItem(minButtonWidth, minitemheight, *mOptionsUseSpecificUdpPortButton).withMargin(0).withFlex(1));
    optionsUdpBox.items.add(FlexItem(90, minitemheight, *mOptionsUdpPortEditor).withMargin(0).withFlex(0));

    optionsDynResampleBox.items.clear();
    optionsDynResampleBox.flexDirection = FlexBox::Direction::row;
    optionsDynResampleBox.items.add(FlexItem(10, 12).withFlex(0));
    optionsDynResampleBox.items.add(FlexItem(180, minpassheight, *mOptionsDynamicResamplingButton).withMargin(0).withFlex(1));

    optionsAutoReconnectBox.items.clear();
    optionsAutoReconnectBox.flexDirection = FlexBox::Direction::row;
    optionsAutoReconnectBox.items.add(FlexItem(10, 12).withFlex(0));
    optionsAutoReconnectBox.items.add(FlexItem(180, minpassheight, *mOptionsAutoReconnectButton).withMargin(0).withFlex(1));

    optionsOverrideSamplerateBox.items.clear();
    optionsOverrideSamplerateBox.flexDirection = FlexBox::Direction::row;
    optionsOverrideSamplerateBox.items.add(FlexItem(10, 12).withFlex(0));
    optionsOverrideSamplerateBox.items.add(FlexItem(180, minpassheight, *mOptionsOverrideSamplerateButton).withMargin(0).withFlex(1));

    optionsInputLimitBox.items.clear();
    optionsInputLimitBox.flexDirection = FlexBox::Direction::row;
    optionsInputLimitBox.items.add(FlexItem(10, 12).withFlex(0));
    optionsInputLimitBox.items.add(FlexItem(180, minpassheight, *mOptionsInputLimiterButton).withMargin(0).withFlex(1));

    optionsChangeAllQualBox.items.clear();
    optionsChangeAllQualBox.flexDirection = FlexBox::Direction::row;
    optionsChangeAllQualBox.items.add(FlexItem(10, 12).withFlex(1));
    optionsChangeAllQualBox.items.add(FlexItem(180, minpassheight, *mOptionsChangeAllFormatButton).withMargin(0).withFlex(0));

    optionsCheckForUpdateBox.items.clear();
    optionsCheckForUpdateBox.flexDirection = FlexBox::Direction::row;
    optionsCheckForUpdateBox.items.add(FlexItem(10, 12).withFlex(0));
    optionsCheckForUpdateBox.items.add(FlexItem(180, minpassheight, *mOptionsShouldCheckForUpdateButton).withMargin(0).withFlex(1));

    optionsSnapToMouseBox.items.clear();
    optionsSnapToMouseBox.flexDirection = FlexBox::Direction::row;
    optionsSnapToMouseBox.items.add(FlexItem(10, 12).withFlex(0));
    optionsSnapToMouseBox.items.add(FlexItem(180, minpassheight, *mOptionsSliderSnapToMouseButton).withMargin(0).withFlex(1));

    
    optionsBox.items.clear();
    optionsBox.flexDirection = FlexBox::Direction::column;
    optionsBox.items.add(FlexItem(4, 6));
    optionsBox.items.add(FlexItem(100, 15, *mVersionLabel).withMargin(2).withFlex(0));
    optionsBox.items.add(FlexItem(4, 4));
    optionsBox.items.add(FlexItem(100, minitemheight, optionsSendQualBox).withMargin(2).withFlex(0));
    optionsBox.items.add(FlexItem(100, minitemheight - 10, optionsChangeAllQualBox).withMargin(1).withFlex(0));
    optionsBox.items.add(FlexItem(4, 4));
    optionsBox.items.add(FlexItem(100, minitemheight, optionsNetbufBox).withMargin(2).withFlex(0));
    optionsBox.items.add(FlexItem(4, 6));
    optionsBox.items.add(FlexItem(100, minpassheight, optionsInputLimitBox).withMargin(2).withFlex(0));
    optionsBox.items.add(FlexItem(100, minpassheight, optionsHearlatBox).withMargin(2).withFlex(0));
    optionsBox.items.add(FlexItem(100, minpassheight, optionsDynResampleBox).withMargin(2).withFlex(0));
    optionsBox.items.add(FlexItem(100, minpassheight, optionsAutoReconnectBox).withMargin(2).withFlex(0));
    optionsBox.items.add(FlexItem(100, minitemheight, optionsUdpBox).withMargin(2).withFlex(0));
    optionsBox.items.add(FlexItem(100, minpassheight, optionsSnapToMouseBox).withMargin(2).withFlex(0));
    if (JUCEApplicationBase::isStandaloneApp()) {
        optionsBox.items.add(FlexItem(100, minpassheight, optionsOverrideSamplerateBox).withMargin(2).withFlex(0));
        optionsBox.items.add(FlexItem(100, minpassheight, optionsCheckForUpdateBox).withMargin(2).withFlex(0));
    }
    minOptionsHeight = 0;
    for (auto & item : optionsBox.items) {
        minOptionsHeight += item.minHeight + item.margin.top + item.margin.bottom;
    }

    // record options
    
    optionsMetRecordBox.items.clear();
    optionsMetRecordBox.flexDirection = FlexBox::Direction::row;
    optionsMetRecordBox.items.add(FlexItem(10, 12));

    optionsMetRecordBox.items.add(FlexItem(minButtonWidth, minitemheight, *mOptionsMetRecordedButton).withMargin(0).withFlex(1));

    int indentw = 40;

    optionsRecordDirBox.items.clear();
    optionsRecordDirBox.flexDirection = FlexBox::Direction::row;
    optionsRecordDirBox.items.add(FlexItem(115, minitemheight, *mRecLocationStaticLabel).withMargin(0).withFlex(0));
    optionsRecordDirBox.items.add(FlexItem(minButtonWidth, minitemheight, *mRecLocationButton).withMargin(0).withFlex(3));

    optionsRecordFormatBox.items.clear();
    optionsRecordFormatBox.flexDirection = FlexBox::Direction::row;
    optionsRecordFormatBox.items.add(FlexItem(115, minitemheight, *mRecFormatStaticLabel).withMargin(0).withFlex(0));
    optionsRecordFormatBox.items.add(FlexItem(minButtonWidth, minitemheight, *mRecFormatChoice).withMargin(0).withFlex(1));
    optionsRecordFormatBox.items.add(FlexItem(2, 4));
    optionsRecordFormatBox.items.add(FlexItem(80, minitemheight, *mRecBitsChoice).withMargin(0).withFlex(0.25));

    optionsRecMixBox.items.clear();
    optionsRecMixBox.flexDirection = FlexBox::Direction::row;
    optionsRecMixBox.items.add(FlexItem(indentw, 12));
    optionsRecMixBox.items.add(FlexItem(minButtonWidth, minpassheight, *mOptionsRecMixButton).withMargin(0).withFlex(1));

    optionsRecMixMinusBox.items.clear();
    optionsRecMixMinusBox.flexDirection = FlexBox::Direction::row;
    optionsRecMixMinusBox.items.add(FlexItem(indentw, 12));
    optionsRecMixMinusBox.items.add(FlexItem(minButtonWidth, minpassheight, *mOptionsRecMixMinusButton).withMargin(0).withFlex(1));

    optionsRecSelfBox.items.clear();
    optionsRecSelfBox.flexDirection = FlexBox::Direction::row;
    optionsRecSelfBox.items.add(FlexItem(indentw, 12));
    optionsRecSelfBox.items.add(FlexItem(minButtonWidth, minpassheight, *mOptionsRecSelfButton).withMargin(0).withFlex(1));

    optionsRecOthersBox.items.clear();
    optionsRecOthersBox.flexDirection = FlexBox::Direction::row;
    optionsRecOthersBox.items.add(FlexItem(indentw, 12));
    optionsRecOthersBox.items.add(FlexItem(minButtonWidth, minpassheight, *mOptionsRecOthersButton).withMargin(0).withFlex(1));

    
    recOptionsBox.items.clear();
    recOptionsBox.flexDirection = FlexBox::Direction::column;
    recOptionsBox.items.add(FlexItem(4, 6));
#if !(JUCE_IOS || JUCE_ANDROID)
    recOptionsBox.items.add(FlexItem(100, minitemheight, optionsRecordDirBox).withMargin(2).withFlex(0));
#endif
    recOptionsBox.items.add(FlexItem(100, minitemheight, optionsRecordFormatBox).withMargin(2).withFlex(0));
    recOptionsBox.items.add(FlexItem(4, 4));
    recOptionsBox.items.add(FlexItem(100, minpassheight, *mOptionsRecFilesStaticLabel).withMargin(2).withFlex(0));
    recOptionsBox.items.add(FlexItem(100, minpassheight, optionsRecMixBox).withMargin(2).withFlex(0));
    recOptionsBox.items.add(FlexItem(100, minpassheight, optionsRecMixMinusBox).withMargin(2).withFlex(0));
    recOptionsBox.items.add(FlexItem(100, minpassheight, optionsRecSelfBox).withMargin(2).withFlex(0));
    recOptionsBox.items.add(FlexItem(100, minpassheight, optionsRecOthersBox).withMargin(2).withFlex(0));
    recOptionsBox.items.add(FlexItem(4, 4));
    recOptionsBox.items.add(FlexItem(100, minpassheight, optionsMetRecordBox).withMargin(2).withFlex(0));
    minRecOptionsHeight = 0;
    for (auto & item : recOptionsBox.items) {
        minRecOptionsHeight += item.minHeight + item.margin.top + item.margin.bottom;
    }
    
    
    
#if 1

    inPannerMainBox.items.clear();
    inPannerMainBox.flexDirection = FlexBox::Direction::column;
    
    int pannerMaxW = 240;
    int numpan = 1;
    inPannerBox.items.clear();
    inPannerBox.flexDirection = FlexBox::Direction::row;
    inPannerLabelBox.items.clear();
    inPannerLabelBox.flexDirection = FlexBox::Direction::row;

    
    inPannerBox.items.add(FlexItem(minPannerWidth, minitemheight, *mInMonPanStereoSlider).withMargin(0).withFlex(1)); //.withMaxWidth(maxPannerWidth));
    inPannerLabelBox.items.add(FlexItem(minPannerWidth, 16, *mInMonPanLabel1).withMargin(0).withFlex(1)); //q.withMaxWidth(maxPannerWidth));
                
    inPannerBox.items.add(FlexItem(6,5).withMargin(0).withFlex(0)); //.withMaxWidth(maxPannerWidth));
    inPannerLabelBox.items.add(FlexItem(6,5).withMargin(0).withFlex(0)); //q.withMaxWidth(maxPannerWidth));
    
    //int choicew = 100;
    inPannerBox.items.add(FlexItem(choicew, minitemheight, *mSendChannelsChoice).withMargin(0).withFlex(1).withMaxWidth(120) ); //.withMaxWidth(maxPannerWidth));
    inPannerLabelBox.items.add(FlexItem(choicew, 16, *mSendChannelsLabel).withMargin(0).withFlex(1)); //q.withMaxWidth(maxPannerWidth));
    
    
    inPannerMainBox.items.add(FlexItem(6, 16, inPannerLabelBox).withMargin(0).withFlex(0).withMaxWidth(pannerMaxW*numpan + 120));
    inPannerMainBox.items.add(FlexItem(6, minitemheight, inPannerBox).withMargin(0).withFlex(1).withMaxWidth(pannerMaxW*numpan + 120));
    
#else
    inPannerMainBox.items.clear();
    inPannerMainBox.flexDirection = FlexBox::Direction::column;
    
    int pannerMaxW = 120;
    int numpan = 2;
    inPannerBox.items.clear();
    inPannerBox.flexDirection = FlexBox::Direction::row;
    inPannerLabelBox.items.clear();
    inPannerLabelBox.flexDirection = FlexBox::Direction::row;

    
    inPannerBox.items.add(FlexItem(minPannerWidth, minitemheight, *mInMonPanSlider1).withMargin(0).withFlex(1)); //.withMaxWidth(maxPannerWidth));
    inPannerLabelBox.items.add(FlexItem(minPannerWidth, 16, *mInMonPanLabel1).withMargin(0).withFlex(1)); //q.withMaxWidth(maxPannerWidth));
        
    inPannerBox.items.add(FlexItem(4, 16).withMargin(1).withFlex(0));
    inPannerBox.items.add(FlexItem(minPannerWidth, minitemheight, *mInMonPanSlider2).withMargin(0).withFlex(1)); //.withMaxWidth(maxPannerWidth));
    
    inPannerLabelBox.items.add(FlexItem(4, 16).withMargin(1).withFlex(0));
    inPannerLabelBox.items.add(FlexItem(minPannerWidth, 16, *mInMonPanLabel2).withMargin(0).withFlex(1)); //.withMaxWidth(maxPannerWidth));
        
    inPannerBox.items.add(FlexItem(6,5).withMargin(0).withFlex(0)); //.withMaxWidth(maxPannerWidth));
    inPannerLabelBox.items.add(FlexItem(6,5).withMargin(0).withFlex(0)); //q.withMaxWidth(maxPannerWidth));
    
    //int choicew = 100;
    inPannerBox.items.add(FlexItem(choicew, minitemheight, *mSendChannelsChoice).withMargin(0).withFlex(1) ); //.withMaxWidth(maxPannerWidth));
    inPannerLabelBox.items.add(FlexItem(choicew, 16, *mSendChannelsLabel).withMargin(0).withFlex(1)); //q.withMaxWidth(maxPannerWidth));
    
    
    inPannerMainBox.items.add(FlexItem(6, 16, inPannerLabelBox).withMargin(0).withFlex(0).withMaxWidth(pannerMaxW*numpan + choicew));
    inPannerMainBox.items.add(FlexItem(6, minitemheight, inPannerBox).withMargin(0).withFlex(1).withMaxWidth(pannerMaxW*numpan + choicew));
#endif
    
    auto mincompbounds = mInCompressorView->getMinimumContentBounds();
    auto mincompheadbounds = mInCompressorView->getMinimumHeaderBounds();
    auto minexpbounds = mInExpanderView->getMinimumContentBounds();
    auto minexpheadbounds = mInCompressorView->getMinimumHeaderBounds();
    auto mineqbounds = mInEqView->getMinimumContentBounds();
    auto mineqheadbounds = mInEqView->getMinimumHeaderBounds();

    inEffectsBox.items.clear();
    inEffectsBox.flexDirection = FlexBox::Direction::column;
    inEffectsBox.items.add(FlexItem(4, 2));
    inEffectsBox.items.add(FlexItem(minexpbounds.getWidth(), jmax(minexpbounds.getHeight(), mincompbounds.getHeight(), mineqbounds.getHeight()) + 3*minitemheight, *mInEffectsConcertina).withMargin(1).withFlex(1));

    mInEffectsConcertina->setPanelHeaderSize(mInCompressorView.get(), mincompheadbounds.getHeight());
    mInEffectsConcertina->setPanelHeaderSize(mInExpanderView.get(), minexpheadbounds.getHeight());
    mInEffectsConcertina->setPanelHeaderSize(mInEqView.get(), mineqheadbounds.getHeight());

    mInEffectsConcertina->setMaximumPanelSize(mInCompressorView.get(), mincompbounds.getHeight()+5);
    mInEffectsConcertina->setMaximumPanelSize(mInExpanderView.get(), minexpbounds.getHeight()+5);
    mInEffectsConcertina->setMaximumPanelSize(mInEqView.get(), mineqbounds.getHeight());

    
       
    localAddressBox.items.clear();
    localAddressBox.flexDirection = FlexBox::Direction::row;
    localAddressBox.items.add(FlexItem(60, 18, *mLocalAddressStaticLabel).withMargin(2).withFlex(1).withMaxWidth(110));
    localAddressBox.items.add(FlexItem(100, minitemheight, *mLocalAddressLabel).withMargin(2).withFlex(1).withMaxWidth(160));


    addressBox.items.clear();
    addressBox.flexDirection = FlexBox::Direction::row;
    addressBox.items.add(FlexItem(servLabelWidth, 18, *mRemoteAddressStaticLabel).withMargin(2).withFlex(0.25));
    addressBox.items.add(FlexItem(172, minitemheight, *mAddRemoteHostEditor).withMargin(2).withFlex(1));

    
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

    servStatusBox.items.clear();
    servStatusBox.flexDirection = FlexBox::Direction::row;
#if !(JUCE_IOS || JUCE_ANDROID)
    servStatusBox.items.add(FlexItem(minPannerWidth, minitemheight, *mServerPasteButton).withMargin(2).withFlex(0).withMaxHeight(minitemheight));
#endif
    servStatusBox.items.add(FlexItem(servLabelWidth, minpassheight, *mServerStatusLabel).withMargin(2).withFlex(1));
#if !(JUCE_IOS || JUCE_ANDROID)
    servStatusBox.items.add(FlexItem(minPannerWidth, minitemheight, *mServerCopyButton).withMargin(2).withFlex(0).withMaxHeight(minitemheight));
#else
    servStatusBox.items.add(FlexItem(minPannerWidth, minitemheight, *mServerShareButton).withMargin(2).withFlex(0).withMaxHeight(minitemheight));    
#endif
    
    servButtonBox.items.clear();
    servButtonBox.flexDirection = FlexBox::Direction::row;
    servButtonBox.items.add(FlexItem(5, 3).withFlex(0.1));
    servButtonBox.items.add(FlexItem(minButtonWidth, minitemheight, *mServerConnectButton).withMargin(2).withFlex(1).withMaxWidth(300));
    servButtonBox.items.add(FlexItem(5, 3).withFlex(0.1));


    
    int maxservboxwidth = 400;

    serverBox.items.clear();
    serverBox.flexDirection = FlexBox::Direction::column;
    serverBox.items.add(FlexItem(5, 3).withFlex(0));
    serverBox.items.add(FlexItem(80, minpassheight, servStatusBox).withMargin(2).withFlex(1).withMaxWidth(maxservboxwidth).withMaxHeight(60));
    serverBox.items.add(FlexItem(5, 4).withFlex(0));
    serverBox.items.add(FlexItem(180, minitemheight, servGroupBox).withMargin(2).withFlex(0).withMaxWidth(maxservboxwidth));
    serverBox.items.add(FlexItem(180, minpassheight, servGroupPassBox).withMargin(2).withFlex(0).withMaxWidth(maxservboxwidth));
    serverBox.items.add(FlexItem(5, 6).withFlex(0));
    serverBox.items.add(FlexItem(180, minitemheight, servUserBox).withMargin(2).withFlex(0).withMaxWidth(maxservboxwidth));
    serverBox.items.add(FlexItem(5, 6).withFlex(1).withMaxHeight(14));
    serverBox.items.add(FlexItem(minButtonWidth, minitemheight, servButtonBox).withMargin(2).withFlex(0).withMaxWidth(maxservboxwidth));
    serverBox.items.add(FlexItem(5, 10).withFlex(1));
    serverBox.items.add(FlexItem(100, minitemheight, servAddressBox).withMargin(2).withFlex(0).withMaxWidth(maxservboxwidth));
    serverBox.items.add(FlexItem(80, minpassheight, *mServerAudioInfoLabel).withMargin(2).withFlex(1).withMaxWidth(maxservboxwidth).withMaxHeight(60));
    serverBox.items.add(FlexItem(5, 8).withFlex(0));

    minServerConnectHeight = 4*minitemheight + 3*minpassheight + 58;

    // public groups stuff

    publicServAddressBox.items.clear();
    publicServAddressBox.flexDirection = FlexBox::Direction::row;
    publicServAddressBox.items.add(FlexItem(servLabelWidth, minitemheight, *mPublicServerHostStaticLabel).withMargin(2).withFlex(1));
    publicServAddressBox.items.add(FlexItem(150, minpassheight, *mPublicServerHostEditor).withMargin(2).withFlex(1));
    publicServAddressBox.items.add(FlexItem(servLabelWidth, minitemheight, *mPublicServerStatusInfoLabel).withMargin(2).withFlex(0.75));

    publicServUserBox.items.clear();
    publicServUserBox.flexDirection = FlexBox::Direction::row;
    publicServUserBox.items.add(FlexItem(servLabelWidth, minitemheight, *mPublicServerUserStaticLabel).withMargin(2).withFlex(1));
    publicServUserBox.items.add(FlexItem(172, minitemheight, *mPublicServerUsernameEditor).withMargin(2).withFlex(1));

    publicAddGroupBox.items.clear();
    publicAddGroupBox.flexDirection = FlexBox::Direction::row;
    publicAddGroupBox.items.add(FlexItem(100, minitemheight, *mPublicServerInfoStaticLabel).withMargin(2).withFlex(0.5));
    publicAddGroupBox.items.add(FlexItem(100, minitemheight, *mPublicServerAddGroupButton).withMargin(2).withFlex(1).withMaxWidth(180));
    //publicAddGroupBox.items.add(FlexItem(4, 8).withFlex(0.25));
    publicAddGroupBox.items.add(FlexItem(100, minitemheight, *mPublicServerGroupEditor).withMargin(2).withFlex(1));


    publicGroupsBox.items.clear();
    publicGroupsBox.flexDirection = FlexBox::Direction::column;
    publicGroupsBox.items.add(FlexItem(5, 3).withFlex(0));
    publicGroupsBox.items.add(FlexItem(100, minitemheight, publicServAddressBox).withMargin(2).withFlex(0));
    publicGroupsBox.items.add(FlexItem(5, 4).withFlex(0));
    publicGroupsBox.items.add(FlexItem(180, minitemheight, publicServUserBox).withMargin(2).withFlex(0).withMaxWidth(maxservboxwidth));
    publicGroupsBox.items.add(FlexItem(5, 4).withFlex(0));
    publicGroupsBox.items.add(FlexItem(180, minitemheight, publicAddGroupBox).withMargin(2).withFlex(0));
    publicGroupsBox.items.add(FlexItem(5, 7).withFlex(0));
    publicGroupsBox.items.add(FlexItem(100, minitemheight, *mPublicGroupComponent).withMargin(2).withFlex(1));



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
    connectBox.items.add(FlexItem(minButtonWidth, minitemheight, titleBox).withMargin(2).withFlex(0));
    connectBox.items.add(FlexItem(minButtonWidth, minitemheight/2, mainGroupUserBox).withMargin(2).withFlex(0));

    titleBox.items.clear();
    titleBox.flexDirection = FlexBox::Direction::row;
    titleBox.items.add(FlexItem(2, 20).withMargin(1).withFlex(0));
    titleBox.items.add(FlexItem(minitemheight - 8, minitemheight - 12, *mSettingsButton).withMargin(1).withMaxWidth(44).withFlex(0));
    titleBox.items.add(FlexItem(86, 20, *mTitleLabel).withMargin(1).withFlex(0));
    if (iaaConnected) {
        titleBox.items.add(FlexItem(4, 4).withMargin(1).withFlex(0.0));
        titleBox.items.add(FlexItem(minitemheight, minitemheight, *mIAAHostButton).withMargin(0).withFlex(0));
    }
    titleBox.items.add(FlexItem(4, 4).withMargin(1).withFlex(0.0));
    titleBox.items.add(FlexItem(minButtonWidth, minitemheight, *mConnectButton).withMargin(0).withFlex(1));

    

    clearRecentsBox.items.clear();
    clearRecentsBox.flexDirection = FlexBox::Direction::row;
    clearRecentsBox.items.add(FlexItem(10, 5).withMargin(0).withFlex(1));
    clearRecentsBox.items.add(FlexItem(minButtonWidth, minitemheight, *mClearRecentsButton).withMargin(0).withFlex(0));
    clearRecentsBox.items.add(FlexItem(10, 5).withMargin(0).withFlex(1));
    
    recentsBox.items.clear();
    recentsBox.flexDirection = FlexBox::Direction::column;
    recentsBox.items.add(FlexItem(minButtonWidth, minitemheight, *mRecentsListBox).withMargin(6).withFlex(1));
    recentsBox.items.add(FlexItem(minButtonWidth, minitemheight, clearRecentsBox).withMargin(1).withFlex(0));
    recentsBox.items.add(FlexItem(10, 5).withMargin(0).withFlex(0));

    
    middleBox.items.clear();
    int midboxminheight =  minitemheight * 2 + 8 ; //(knobitemheight + 4);
    int midboxminwidth = 296;
    
    if (isNarrow) {
        middleBox.flexDirection = FlexBox::Direction::column;
        middleBox.items.add(FlexItem(150, minitemheight*1.5 + 8, connectBox).withMargin(2).withFlex(0));
        middleBox.items.add(FlexItem(5, 2));
        middleBox.items.add(FlexItem(100, minitemheight + minitemheight + 6, paramsBox).withMargin(2).withFlex(0));
        middleBox.items.add(FlexItem(6, 2));

        midboxminheight = (minitemheight*1.5 + minitemheight * 2 + 20);
        midboxminwidth = 190;
    } else {
        middleBox.flexDirection = FlexBox::Direction::row;        
        middleBox.items.add(FlexItem(280, minitemheight*1.5 , connectBox).withMargin(2).withFlex(1).withMaxWidth(440));
        middleBox.items.add(FlexItem(1, 1).withMaxWidth(3).withFlex(0.5));
        middleBox.items.add(FlexItem(minKnobWidth*4 + meterwidth*2, minitemheight + minitemheight, paramsBox).withMargin(2).withFlex(4));
    }


    toolbarTextBox.items.clear();
    toolbarTextBox.flexDirection = FlexBox::Direction::column;
    toolbarTextBox.items.add(FlexItem(40, minitemheight/2, *mMainStatusLabel).withMargin(0).withFlex(1));


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

    
    // effects

    reverbSizeBox.items.clear();
    reverbSizeBox.flexDirection = FlexBox::Direction::column;
    reverbSizeBox.items.add(FlexItem(minKnobWidth, knoblabelheight, *mReverbSizeLabel).withMargin(0).withFlex(0));
    reverbSizeBox.items.add(FlexItem(minKnobWidth, minitemheight, *mReverbSizeSlider).withMargin(0).withFlex(1));

    reverbLevelBox.items.clear();
    reverbLevelBox.flexDirection = FlexBox::Direction::column;
    reverbLevelBox.items.add(FlexItem(minKnobWidth, knoblabelheight, *mReverbLevelLabel).withMargin(0).withFlex(0));
    reverbLevelBox.items.add(FlexItem(minKnobWidth, minitemheight, *mReverbLevelSlider).withMargin(0).withFlex(1));

    reverbDampBox.items.clear();
    reverbDampBox.flexDirection = FlexBox::Direction::column;
    reverbDampBox.items.add(FlexItem(minKnobWidth, knoblabelheight, *mReverbDampingLabel).withMargin(0).withFlex(0));
    reverbDampBox.items.add(FlexItem(minKnobWidth, minitemheight, *mReverbDampingSlider).withMargin(0).withFlex(1));

    reverbPreDelayBox.items.clear();
    reverbPreDelayBox.flexDirection = FlexBox::Direction::column;
    reverbPreDelayBox.items.add(FlexItem(minKnobWidth, knoblabelheight, *mReverbPreDelayLabel).withMargin(0).withFlex(0));
    reverbPreDelayBox.items.add(FlexItem(minKnobWidth, minitemheight, *mReverbPreDelaySlider).withMargin(0).withFlex(1));

    
    reverbCheckBox.items.clear();
    reverbCheckBox.flexDirection = FlexBox::Direction::row;
    reverbCheckBox.items.add(FlexItem(5, 5).withMargin(0).withFlex(0));
    reverbCheckBox.items.add(FlexItem(minKnobWidth, minitemheight, *mReverbEnabledButton).withMargin(0).withFlex(0));
    reverbCheckBox.items.add(FlexItem(minKnobWidth, minitemheight, *mReverbTitleLabel).withMargin(0).withFlex(2));
    reverbCheckBox.items.add(FlexItem(minKnobWidth, minitemheight, *mReverbModelChoice).withMargin(0).withFlex(1));
    reverbCheckBox.items.add(FlexItem(5, 5).withMargin(0).withFlex(0));


    
    reverbKnobBox.items.clear();
    reverbKnobBox.flexDirection = FlexBox::Direction::row;
    reverbKnobBox.items.add(FlexItem(5, 5).withMargin(0).withFlex(0));
    reverbKnobBox.items.add(FlexItem(minKnobWidth, minitemheight, reverbPreDelayBox).withMargin(0).withFlex(1));
    reverbKnobBox.items.add(FlexItem(minKnobWidth, minitemheight, reverbLevelBox).withMargin(0).withFlex(1));
    reverbKnobBox.items.add(FlexItem(minKnobWidth, minitemheight, reverbSizeBox).withMargin(0).withFlex(1));
    reverbKnobBox.items.add(FlexItem(minKnobWidth, minitemheight, reverbDampBox).withMargin(0).withFlex(1));
    reverbKnobBox.items.add(FlexItem(5, 5).withMargin(0).withFlex(0));

    reverbBox.items.clear();
    reverbBox.flexDirection = FlexBox::Direction::column;
    reverbBox.items.add(FlexItem(6, 5).withMargin(0).withFlex(0));
    reverbBox.items.add(FlexItem(100, minitemheight, reverbCheckBox).withMargin(0).withFlex(0));
    reverbBox.items.add(FlexItem(6, 5).withMargin(0).withFlex(0));
    reverbBox.items.add(FlexItem(100, knoblabelheight + minitemheight, reverbKnobBox).withMargin(0).withFlex(1));

    
    effectsBox.items.clear();
    effectsBox.flexDirection = FlexBox::Direction::column;
    effectsBox.items.add(FlexItem(minKnobWidth, knoblabelheight + minitemheight + 10, reverbBox).withMargin(0).withFlex(1));
    
    
    toolbarBox.items.clear();
    toolbarBox.flexDirection = FlexBox::Direction::row;
    toolbarBox.items.add(FlexItem(7, 6).withMargin(0).withFlex(0));
    toolbarBox.items.add(FlexItem(44, minitemheight, *mMainMuteButton).withMargin(0).withFlex(0));
    toolbarBox.items.add(FlexItem(2, 6).withMargin(0).withFlex(0.1).withMaxWidth(8));
    toolbarBox.items.add(FlexItem(44, minitemheight, *mMainRecvMuteButton).withMargin(0).withFlex(0));
    toolbarBox.items.add(FlexItem(2, 6).withMargin(0).withFlex(0.1).withMaxWidth(8));
    toolbarBox.items.add(FlexItem(44, minitemheight, *mMainPushToTalkButton).withMargin(0).withFlex(0));
    toolbarBox.items.add(FlexItem(2, 6).withMargin(0).withFlex(0.1));
    toolbarBox.items.add(FlexItem(44, minitemheight, *mMetEnableButton).withMargin(0).withFlex(0).withMaxHeight(minitemheight+2).withAlignSelf(FlexItem::AlignSelf::center));
    toolbarBox.items.add(FlexItem(36, minitemheight, *mMetConfigButton).withMargin(0).withFlex(0).withMaxHeight(minitemheight+2).withAlignSelf(FlexItem::AlignSelf::center));
    toolbarBox.items.add(FlexItem(2, 5).withMargin(0).withFlex(0.1));

   
#if JUCE_IOS || JUCE_ANDROID
#else
    if (processor.getCurrentJoinedGroup().isEmpty() && processor.getNumberRemotePeers() > 1) {
        toolbarBox.items.add(FlexItem(6, 6).withMargin(1).withFlex(0.2));
        toolbarBox.items.add(FlexItem(60, minitemheight, *mPatchbayButton).withMargin(2).withFlex(0.5).withMaxWidth(120));
    }
#endif

    if (mRecordingButton) {

        transportVBox.items.clear();
        transportVBox.flexDirection = FlexBox::Direction::column;

        transportBox.items.clear();
        transportBox.flexDirection = FlexBox::Direction::row;

        transportWaveBox.items.clear();
        transportWaveBox.flexDirection = FlexBox::Direction::row;

        transportWaveBox.items.add(FlexItem(isNarrow ? 11 : 5, 6).withMargin(0).withFlex(0));
        transportWaveBox.items.add(FlexItem(100, minitemheight, *mWaveformThumbnail).withMargin(0).withFlex(1));
        transportWaveBox.items.add(FlexItem(isNarrow ? 17 : 3, 6).withMargin(0).withFlex(0));

        transportBox.items.add(FlexItem(9, 6).withMargin(0).withFlex(0));
        transportBox.items.add(FlexItem(44, minitemheight, *mPlayButton).withMargin(0).withFlex(0));
        transportBox.items.add(FlexItem(3, 6).withMargin(0).withFlex(0));
        transportBox.items.add(FlexItem(44, minitemheight, *mLoopButton).withMargin(0).withFlex(0));
        transportBox.items.add(FlexItem(3, 6).withMargin(0).withFlex(0));
        transportBox.items.add(FlexItem(40, minitemheight, *mFileMenuButton).withMargin(0).withFlex(0));
        
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
#if JUCE_IOS || JUCE_ANDROID
        transportBox.items.add(FlexItem(6, 6).withMargin(1).withFlex(0));
#else
        transportBox.items.add(FlexItem(14, 6).withMargin(1).withFlex(0));        
#endif

        toolbarBox.items.add(FlexItem(44, minitemheight, *mRecordingButton).withMargin(0).withFlex(0));
        toolbarBox.items.add(FlexItem(2, 6).withMargin(0).withFlex(0.1).withMaxWidth(6));
        toolbarBox.items.add(FlexItem(44, minitemheight, *mFileBrowseButton).withMargin(0).withFlex(0));
    }

    if (!isNarrow) {
        toolbarBox.items.add(FlexItem(1, 5).withMargin(0).withFlex(0.1));
        toolbarBox.items.add(FlexItem(100, minitemheight, outputMainBox).withMargin(0).withFlex(1).withMaxWidth(350));
        toolbarBox.items.add(FlexItem(6, 6).withMargin(0).withFlex(0));
    }
    else {    
        toolbarBox.items.add(FlexItem(14, 6).withMargin(0).withFlex(0));
    }
    

    
    int minheight = 0;
    
    mainBox.items.clear();
    mainBox.flexDirection = FlexBox::Direction::column;    
    mainBox.items.add(FlexItem(midboxminwidth, midboxminheight, middleBox).withMargin(3).withFlex(0)); minheight += midboxminheight;
    mainBox.items.add(FlexItem(120, 50, *mPeerViewport).withMargin(3).withFlex(3)); minheight += 50 + 6;
    mainBox.items.add(FlexItem(10, 2).withFlex(0)); minheight += 2;

    if (!mCurrentAudioFile.isEmpty()) {
        mainBox.items.add(FlexItem(4, 5).withMargin(0).withFlex(0)); minheight += 5;
        if (isNarrow) {
            mainBox.items.add(FlexItem(100, 2*minitemheight + 3, transportVBox).withMargin(2).withFlex(0)); minheight += 2*minitemheight + 3;
        }
        else {
            mainBox.items.add(FlexItem(100, minitemheight, transportVBox).withMargin(2).withFlex(0)); minheight += minitemheight;
        }
        mainBox.items.add(FlexItem(4, 11).withMargin(0).withFlex(0)); minheight += 11;
    }
    
    if (isNarrow) {
        mainBox.items.add(FlexItem(100, minitemheight + 4, outputMainBox).withMargin(0).withFlex(0)); 
        mainBox.items.add(FlexItem(4, 4).withMargin(0).withFlex(0));
        minheight += minitemheight + 8;
    }
    mainBox.items.add(FlexItem(100, minitemheight + 4, toolbarBox).withMargin(0).withFlex(0)); minheight += minitemheight + 10;
    mainBox.items.add(FlexItem(10, 6).withFlex(0));
        
    
    // Connect component
    
    connectTitleBox.items.clear();
    connectTitleBox.flexDirection = FlexBox::Direction::row;
    connectTitleBox.items.add(FlexItem(50, minitemheight, *mConnectCloseButton).withMargin(2));
    connectTitleBox.items.add(FlexItem(80, minitemheight, *mConnectTitle).withMargin(2).withFlex(1));
    connectTitleBox.items.add(FlexItem(50, minitemheight));

    connectHorizBox.items.clear();
    connectHorizBox.flexDirection = FlexBox::Direction::row;
    connectHorizBox.items.add(FlexItem(100, 100, *mConnectTab).withMargin(3).withFlex(1));
    if (mConnectTab->getNumTabs() < 4) {
        connectHorizBox.items.add(FlexItem(335, 100, *mRecentsGroup).withMargin(3).withFlex(0));
    }

    connectMainBox.items.clear();
    connectMainBox.flexDirection = FlexBox::Direction::column;
    connectMainBox.items.add(FlexItem(100, minitemheight, connectTitleBox).withMargin(3).withFlex(0));
    connectMainBox.items.add(FlexItem(100, 100, connectHorizBox).withMargin(3).withFlex(1));

}

void SonobusAudioProcessorEditor::showPopTip(const String & message, int timeoutMs, Component * target, int maxwidth)
{
    if (!popTip) {
        popTip = std::make_unique<BubbleMessageComponent>();
    }
    
    popTip->setAllowedPlacement(BubbleMessageComponent::BubblePlacement::above | BubbleMessageComponent::BubblePlacement::below);
    
    if (target) {
        if (auto * parent = target->findParentComponentOfClass<AudioProcessorEditor>()) {
            parent->addChildComponent (popTip.get());
            parent->toFront(false);
        }
        else {
            addChildComponent(popTip.get());            
        }
    }
    else 
    {
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

    popTip->toFront(false);
}


void SonobusAudioProcessorEditor::showFilePopupMenu(Component * source)
{
    Array<GenericItemChooserItem> items;
    items.add(GenericItemChooserItem(TRANS("Trim to New")));
#if JUCE_IOS || JUCE_ANDROID
    items.add(GenericItemChooserItem(TRANS("Share File")));    
#else
    items.add(GenericItemChooserItem(TRANS("Reveal File")));
#endif
    
    Component* dw = this; 
    
    
    Rectangle<int> bounds =  dw->getLocalArea(nullptr, source->getScreenBounds());
    
    GenericItemChooser::launchPopupChooser(items, bounds, dw, this, 1000);
}

void SonobusAudioProcessorEditor::compressorParamsChanged(CompressorView *comp, SonobusAudioProcessor::CompressorParams & params) 
{
    processor.setInputCompressorParams(params);
    
    bool infxon = processor.getInputEffectsActive();
    mInEffectsButton->setToggleState(infxon, dontSendNotification);
}

void SonobusAudioProcessorEditor::expanderParamsChanged(ExpanderView *comp, SonobusAudioProcessor::CompressorParams & params) 
{
    processor.setInputExpanderParams(params);
    
    bool infxon = processor.getInputEffectsActive();
    mInEffectsButton->setToggleState(infxon, dontSendNotification);
}

void SonobusAudioProcessorEditor::parametricEqParamsChanged(ParametricEqView *comp, SonobusAudioProcessor::ParametricEqParams & params) 
{
    processor.setInputEqParams(params);
    
    bool infxon = processor.getInputEffectsActive();
    mInEffectsButton->setToggleState(infxon, dontSendNotification);
}


void SonobusAudioProcessorEditor::effectsHeaderClicked(EffectsBaseView *comp, const MouseEvent & ev) 
{
    if (comp == mInCompressorView.get()) {
        bool changed = mInEffectsConcertina->setPanelSize(mInEqView.get(), 0, true);
        changed |= mInEffectsConcertina->expandPanelFully(mInExpanderView.get(), true);
        changed |= mInEffectsConcertina->expandPanelFully(mInCompressorView.get(), true);
        if (!changed) {
            // toggle it
            SonobusAudioProcessor::CompressorParams params;
            processor.getInputCompressorParams(params);
            params.enabled = !params.enabled;
            processor.setInputCompressorParams(params);
            updateState();
        }
    } 
    else if (comp == mInExpanderView.get()) {
        bool changed = mInEffectsConcertina->setPanelSize(mInEqView.get(), 0, true);
        changed |= mInEffectsConcertina->expandPanelFully(mInCompressorView.get(), true);
        changed |= mInEffectsConcertina->expandPanelFully(mInExpanderView.get(), true);
        if (!changed) {
            // toggle it
            SonobusAudioProcessor::CompressorParams params;
            processor.getInputExpanderParams(params);
            params.enabled = !params.enabled;
            processor.setInputExpanderParams(params);
            updateState();
        }
    } 
    else if (comp == mInEqView.get()) {
        bool changed = mInEffectsConcertina->expandPanelFully(mInEqView.get(), true);
        if (!changed) {
            // toggle it
            SonobusAudioProcessor::ParametricEqParams params;
            processor.getInputEqParams(params);
            params.enabled = !params.enabled;
            processor.setInputEqParams(params);            
            updateState();
        }
    } 
}



void SonobusAudioProcessorEditor::genericItemChooserSelected(GenericItemChooser *comp, int index)
{
    int choosertag = comp->getTag();

    if (choosertag == 1000) {
        if (index == 0) {
            // trim to new
            trimCurrentAudioFile(false);
        }
        else if (index == 1) {
#if JUCE_IOS || JUCE_ANDROID
            // share
            Array<URL> urlarray;
            urlarray.add(mCurrentAudioFile);
            ContentSharer::getInstance()->shareFiles(urlarray, [](bool result, const String& msg){ DBG("url share returned " << (int)result << " : " << msg); });
#else
            // reveal
            if (mCurrentAudioFile.getFileName().isNotEmpty()) {
                mCurrentAudioFile.getLocalFile().revealToUser();
                return; // needed in case we are already gone (weird windows issue)
            }
#endif

        }
    }
    
    if (CallOutBox* const cb = comp->findParentComponentOfClass<CallOutBox>()) {
        cb->dismiss();
    }

}


void SonobusAudioProcessorEditor::changeListenerCallback (ChangeBroadcaster* source)
{
    if (source == mWaveformThumbnail.get()) {
        loadAudioFromURL(URL (mWaveformThumbnail->getLastDroppedFile()));
    } else if (source == &(processor.getTransportSource())) {
        updateTransportState();
    }
}

class SonobusAudioProcessorEditor::TrimFileJob : public ThreadPoolJob
{
public:
    TrimFileJob(SonobusAudioProcessorEditor * parent_, const String & file_, double startPos_, double lenSecs_, bool replace_)
    : ThreadPoolJob("TrimFilesJob"), parent(parent_), file(file_), startPos(startPos_), lenSecs(lenSecs_), replaceExisting(replace_) {}
    
    JobStatus runJob ()
    {
        // AppState * app = AppState::getInstance();
        DBG("Starting trim file job");
        
        File sourcefile = File(file);
        File outputfile = sourcefile.getParentDirectory().getNonexistentChildFile(sourcefile.getFileNameWithoutExtension() + "-trim", sourcefile.getFileExtension());
        std::unique_ptr<AudioFormatReader> reader;
        reader.reset(parent->processor.getFormatManager().createReaderFor (sourcefile));
        
        bool success = false;
        
        if (reader != nullptr) {
            
            String pathname = outputfile.getFullPathName();

            std::unique_ptr<AudioFormat> audioFormat;
            int qualindex = 0;

            if (outputfile.getFileExtension().toLowerCase() == ".wav") {
                audioFormat = std::make_unique<WavAudioFormat>();
            }
            else if (outputfile.getFileExtension().toLowerCase() == ".ogg") {
                audioFormat = std::make_unique<OggVorbisAudioFormat>();
                qualindex = 8; // 256k
            }
            else {
                // default to flac
                audioFormat = std::make_unique<FlacAudioFormat>();
                if (outputfile.getFileExtension().toLowerCase() != ".flac") {
                    // force name to end in flac
                    outputfile = outputfile.getParentDirectory().getNonexistentChildFile(outputfile.getFileNameWithoutExtension(), ".flac");
                }
            }

            std::unique_ptr<FileOutputStream> fos (outputfile.createOutputStream());
            
            if (fos != nullptr) {
                // Now create a writer object that writes to our output stream...

                std::unique_ptr<AudioFormatWriter> writer;
                writer.reset(audioFormat->createWriterFor (fos.get(), reader->sampleRate, reader->numChannels, 16, {}, qualindex));
                
                if (writer != nullptr)
                {
                    fos.release(); // (passes responsibility for deleting the stream to the writer object that is now using it)
                    
                    writer->writeFromAudioReader(*reader, startPos * writer->getSampleRate(), lenSecs * writer->getSampleRate());
                
                    writer->flush();
                    success = true;
                }
                
                DBG("Finished triming file JOB to: " << pathname);
            }
        }
        else {
            DBG("Error trimming file JOB to: " << file);
        }
        
        if (success && replaceExisting) {
            outputfile.moveFileTo(sourcefile);
            DBG("Moved " << outputfile.getFullPathName() << " to " << sourcefile.getFullPathName());
            
            // remove any meta file
            File metafname = sourcefile.getParentDirectory().getChildFile("." + sourcefile.getFileName() + ".json");
            metafname.deleteFile();
            
        }

        if (success) {
            parent->trimFinished(replaceExisting ? sourcefile.getFullPathName() : outputfile.getFullPathName());
        }
        
        
        return ThreadPoolJob::jobHasFinished;
    }
    
    SonobusAudioProcessorEditor * parent;
    String file;
    
    double startPos;
    double lenSecs;
    bool replaceExisting;
};

void SonobusAudioProcessorEditor::trimFinished(const String & trimmedFile)
{
    // load it up!
    mCurrentAudioFile = URL(File(trimmedFile));
    mReloadFile  = true;
    //mTrimDone = true;
    triggerAsyncUpdate();

}

void SonobusAudioProcessorEditor::trimCurrentAudioFile(bool replaceExisting)
{
    if (mCurrentAudioFile.getFileName().isNotEmpty()) {
        String selfile = mCurrentAudioFile.getLocalFile().getFullPathName();
        double startpos, looplen;
        mWaveformThumbnail->getLoopRangeSec(startpos, looplen);
        if (looplen < processor.getTransportSource().getLengthInSeconds()) {
            trimAudioFile(selfile, startpos, looplen, replaceExisting);
        }
    }

}

void SonobusAudioProcessorEditor::trimAudioFile(const String & fname, double startPos, double lenSecs, bool replaceExisting)
{

    mWorkPool->addJob(new TrimFileJob(this, fname, startPos, lenSecs, replaceExisting), true);

}


bool SonobusAudioProcessorEditor::setupLocalisation(const String & overrideLang)
{
    String displang = SystemStats::getDisplayLanguage();
    String lang = SystemStats::getDisplayLanguage();

    if (overrideLang.isNotEmpty()) {
        displang = lang = overrideLang;
    }

    LocalisedStrings::setCurrentMappings(nullptr);

    bool retval = false;

    int retbytes = 0;
    int retfbytes = 0;
    String region = SystemStats::getUserRegion();

    String sflang = lang.initialSectionNotContaining("_").toLowerCase().replace("-", "");
    String slang = lang.initialSectionNotContaining("_").initialSectionNotContaining("-").toLowerCase();

    String resname = String::formatted("localized_%s_txt", slang.toRawUTF8());
    String resfname = String::formatted("localized_%s_txt", sflang.toRawUTF8());

    const char * rawdata = BinaryData::getNamedResource(resname.toRawUTF8(), retbytes);
    const char * rawdataf = BinaryData::getNamedResource(resfname.toRawUTF8(), retfbytes);

    if (rawdataf) {
        DBG("Found fulldisp localization for language: " << lang << "  region: " << region << " displang: " <<  displang <<  "  - resname: " << resfname);
        LocalisedStrings * lstrings = new LocalisedStrings(String::createStringFromData(rawdataf, retfbytes), true);

        LocalisedStrings::setCurrentMappings(lstrings);
        retval = true;
    }
    else if (rawdata) {
        DBG("Found localization for language: " << lang << "  region: " << region << " displang: " <<  displang <<  "  - resname: " << resname);
        LocalisedStrings * lstrings = new LocalisedStrings(String::createStringFromData(rawdata, retbytes), true);

        LocalisedStrings::setCurrentMappings(lstrings);
        retval = true;
    }
    else if (lang.startsWith("en")) {
        // special case, since untranslated is english
        retval = true;
    }
    else {
        DBG("Couldn't find mapping for lang: " << lang << "  region: " << region << " displang: " <<  displang <<  "  - resname: " << resname);
        retval = false;
    }

#ifdef JUCE_ANDROID
   // Font::setFallbackFontName("Droid Sans Fallback");
#endif

    if (retval) {
        mActiveLanguageCode = displang.toStdString();
    } else {
        mActiveLanguageCode = "en-us"; // indicates we are using english
    }

    return retval;
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
            info.addDefaultKeypress ('f', ModifierKeys::commandModifier);
            break;
        case SonobusCommands::RevealFile:
            info.setInfo (TRANS("Reveal File"),
                          TRANS("Reveal file"),
                          TRANS("Popup"), 0);
            info.setActive(mCurrentAudioFile.getFileName().isNotEmpty());
            info.addDefaultKeypress ('e', ModifierKeys::commandModifier);
            break;
        case SonobusCommands::Connect:
            info.setInfo (TRANS("Connect"),
                          TRANS("Connect"),
                          TRANS("Popup"), 0);
            info.setActive(!currConnected || currGroup.isEmpty());
            info.addDefaultKeypress ('n', ModifierKeys::commandModifier);
            break;
        case SonobusCommands::Disconnect:
            info.setInfo (TRANS("Disconnect"),
                          TRANS("Disconnect"),
                          TRANS("Popup"), 0);
            info.setActive(currConnected && currGroup.isNotEmpty());
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
        case SonobusCommands::CheckForNewVersion:
            info.setInfo (TRANS("Check For New Version"),
                          TRANS("Check for New Version"),
                          TRANS("Popup"), 0);
            info.setActive(true);
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
    cmds.add(SonobusCommands::RevealFile);
    cmds.add(SonobusCommands::Connect);
    cmds.add(SonobusCommands::Disconnect);
    cmds.add(SonobusCommands::ShowOptions);
    cmds.add(SonobusCommands::OpenFile);
    cmds.add(SonobusCommands::RecordToggle);
    cmds.add(SonobusCommands::CheckForNewVersion);
}

bool SonobusAudioProcessorEditor::perform (const InvocationInfo& info) {
    bool ret = true;
    
    switch (info.commandID) {
        case SonobusCommands::MuteAllInput:
            DBG("got mute toggle!");
            mMainMuteButton->setToggleState(!mMainMuteButton->getToggleState(), sendNotification);
            break;
        case SonobusCommands::MuteAllPeers:
            DBG("got mute peers toggle!");
            mMainRecvMuteButton->setToggleState(!mMainRecvMuteButton->getToggleState(), sendNotification);
            break;
        case SonobusCommands::TogglePlayPause:
            DBG("got play pause!");
            if (mPlayButton->isVisible()) {
                mPlayButton->setToggleState(!mPlayButton->getToggleState(), sendNotification);
            }
            break;
        case SonobusCommands::ToggleLoop:
            DBG("got loop toggle!");
            if (mLoopButton->isVisible()) {
                mLoopButton->setToggleState(!mLoopButton->getToggleState(), sendNotification);
            }
            break;
        case SonobusCommands::TrimSelectionToNewFile:
            DBG("Got trim!");
            trimCurrentAudioFile(false);
            break;
        case SonobusCommands::CloseFile:
            DBG("got close file!");
            if (mDismissTransportButton->isVisible()) {
                buttonClicked(mDismissTransportButton.get());
            }

            break;
        case SonobusCommands::ShareFile:
            DBG("got share file!");

            break;
        case SonobusCommands::RevealFile:
            DBG("got reveal file!");
            if (mCurrentAudioFile.getFileName().isNotEmpty()) {
                mCurrentAudioFile.getLocalFile().revealToUser();
            }
            else {
                if (mCurrOpenDir.getFullPathName().isEmpty()) {
                    mCurrOpenDir = File(processor.getDefaultRecordingDirectory());
                }
                mCurrOpenDir.revealToUser();
            }
            break;
        case SonobusCommands::OpenFile:
            DBG("got open file!");
            openFileBrowser();

            break;
        case SonobusCommands::Connect:
            DBG("got connect!");
            if (!currConnected || currGroup.isEmpty()) {
                buttonClicked(mConnectButton.get());
            }
            break;
        case SonobusCommands::Disconnect:
            DBG("got disconnect!");
            
            if (currConnected && currGroup.isNotEmpty()) {
                buttonClicked(mConnectButton.get());
            }

            break;
        case SonobusCommands::ShowOptions:
            DBG("got show options!");
            buttonClicked(mSettingsButton.get());

            break;
        case SonobusCommands::RecordToggle:
            DBG("got record toggle!");
            buttonClicked(mRecordingButton.get());

            break;
        case SonobusCommands::CheckForNewVersion:   
            LatestVersionCheckerAndUpdater::getInstance()->checkForNewVersion (true); 
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
                       TRANS("Transport"));
    //TRANS("Help"));
}

PopupMenu SonobusAudioProcessorEditor::SonobusMenuBarModel::getMenuForIndex (int topLevelMenuIndex, const String& /*menuName*/)
{
     PopupMenu retval;
        
    switch (topLevelMenuIndex) {
        case MenuFileIndex:
            retval.addCommandItem (&parent.commandManager, SonobusCommands::OpenFile);
            retval.addCommandItem (&parent.commandManager, SonobusCommands::CloseFile);
#if JUCE_IOS || JUCE_ANDROID
            retval.addCommandItem (&parent.commandManager, SonobusCommands::ShareFile);
#else
            retval.addCommandItem (&parent.commandManager, SonobusCommands::RevealFile);            
#endif
            retval.addSeparator();
            retval.addCommandItem (&parent.commandManager, SonobusCommands::TrimSelectionToNewFile);

            retval.addSeparator();
            retval.addCommandItem (&parent.commandManager, SonobusCommands::CheckForNewVersion);

#if (JUCE_WINDOWS || JUCE_LINUX)
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
                // about
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
    removeImage = Drawable::createFromImageData(BinaryData::x_icon_svg, BinaryData::x_icon_svgSize);
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
    int removewidth = jmin(36, height - 6);
    float yratio = 0.6;
    float adjwidth = width - removewidth;
    
    // DebugLogC("Paint %s", text.toRawUTF8());
    float iconsize = height*yratio;
    float groupheight = height*yratio;
    g.drawImageWithin(groupImage, 0, 0, iconsize, iconsize, RectanglePlacement::fillDestination);
    g.drawFittedText (info.groupName, iconsize + 4, 0, adjwidth*xratio - 8 - iconsize, groupheight, Justification::centredLeft, true);

    g.setFont (parent->recentsNameFont);    
    g.setColour (parent->findColour(nameTextColourId).withAlpha(0.8f));
    g.drawImageWithin(personImage, adjwidth*xratio, 0, iconsize, iconsize, RectanglePlacement::fillDestination);
    g.drawFittedText (info.userName, adjwidth*xratio + iconsize, 0, adjwidth*(1.0f - xratio) - 4 - iconsize, groupheight, Justification::centredLeft, true);

    String infostr;

    if (info.groupIsPublic) {
        infostr += TRANS("PUBLIC") + " ";
    }

    if (info.groupPassword.isNotEmpty()) {
        infostr += TRANS("password protected,") + " ";
    }

    infostr += TRANS("on") + " " + Time(info.timestamp).toString(true, true, false) + " " ;
    
    if (info.serverHost != "aoo.sonobus.net") {
        infostr += TRANS("to") + " " +  info.serverHost;
    }

    g.setColour (parent->findColour(nameTextColourId).withAlpha(0.5f));
    g.setFont (parent->recentsInfoFont);    
    
    g.drawFittedText (infostr, 14, height * yratio, adjwidth - 24, height * (1.0f - yratio), Justification::centredTop, true);

    removeImage->drawWithin(g, Rectangle<float>(adjwidth + removewidth*0.25*yratio, height*0.5 - removewidth*0.5*yratio, removewidth*yratio, removewidth*yratio), RectanglePlacement::fillDestination, 0.9);

    removeButtonX = adjwidth;
    cachedWidth = width;
}

void SonobusAudioProcessorEditor::RecentsListModel::listBoxItemClicked (int rowNumber, const MouseEvent& e)
{
    // use this
    DBG("Clicked " << rowNumber << "  x: " << e.getPosition().x << "  width: " << cachedWidth);
    
    if (e.getPosition().x > removeButtonX) {
        parent->processor.removeRecentServerConnectionInfo(rowNumber);
        parent->updateRecents();
    }
    else {
        parent->connectWithInfo(recents.getReference(rowNumber));
    }
}

void SonobusAudioProcessorEditor::RecentsListModel::selectedRowsChanged(int rowNumber)
{

}

#pragma PublicGroupsListModel

SonobusAudioProcessorEditor::PublicGroupsListModel::PublicGroupsListModel(SonobusAudioProcessorEditor * parent_) : parent(parent_)
{
    groupImage = ImageCache::getFromMemory(BinaryData::people_png, BinaryData::people_pngSize);
    personImage = ImageCache::getFromMemory(BinaryData::person_png, BinaryData::person_pngSize);
}


void SonobusAudioProcessorEditor::PublicGroupsListModel::updateState()
{
    groups.clear();
    parent->processor.getPublicGroupInfos(groups);
}

int SonobusAudioProcessorEditor::PublicGroupsListModel::getNumRows()
{
    return groups.size();
}

void SonobusAudioProcessorEditor::PublicGroupsListModel::paintListBoxItem (int rowNumber, Graphics &g, int width, int height, bool rowIsSelected)
{
    if (rowNumber >= groups.size()) return;

    if (rowIsSelected) {
        g.setColour (parent->findColour(selectedColourId));
        g.fillRect(Rectangle<int>(0,0,width,height));
    }

    g.setColour(parent->findColour(separatorColourId));
    g.drawLine(0, height-1, width, height);


    g.setColour (parent->findColour(nameTextColourId));
    g.setFont (parent->recentsGroupFont);

    AooPublicGroupInfo & info = groups.getReference(rowNumber);

    float xratio = 0.7;
    int removewidth = 0 ; // jmin(36, height - 6);
    float yratio = 1.0; // 0.6
    float adjwidth = width - removewidth;

    // DebugLogC("Paint %s", text.toRawUTF8());
    float iconsize = height*yratio;
    float groupheight = height*yratio;
    g.drawImageWithin(groupImage, 0, 0, iconsize, iconsize, RectanglePlacement::fillDestination);
    g.drawFittedText (info.groupName, iconsize + 4, 0, adjwidth*xratio - 8 - iconsize, groupheight, Justification::centredLeft, true);

    g.setFont (parent->recentsNameFont);
    g.setColour (parent->findColour(nameTextColourId).withAlpha(0.8f));
    g.drawImageWithin(personImage, adjwidth*xratio, 0, iconsize, iconsize, RectanglePlacement::fillDestination);
    String usertext;
    usertext << info.activeCount << (info.activeCount > 1 ? TRANS(" active users") : TRANS(" active user"));
    g.drawFittedText (usertext, adjwidth*xratio + iconsize, 0, adjwidth*(1.0f - xratio) - 4 - iconsize, groupheight, Justification::centredLeft, true);


    //g.setColour (parent->findColour(nameTextColourId).withAlpha(0.5f));
    //g.setFont (parent->recentsInfoFont);
    //g.drawFittedText (infostr, 14, height * yratio, adjwidth - 24, height * (1.0f - yratio), Justification::centredTop, true);

    cachedWidth = width;
}

void SonobusAudioProcessorEditor::PublicGroupsListModel::listBoxItemClicked (int rowNumber, const MouseEvent& e)
{
    // use this
    DBG("Clicked " << rowNumber << "  x: " << e.getPosition().x << "  width: " << cachedWidth);

    if (rowNumber >= groups.size() || rowNumber < 0) {
        DBG("Clicked out of bounds row!");
        return;
    }

    auto & ginfo = groups.getReference(rowNumber);

    if (parent->processor.isConnectedToServer()) {
        parent->currConnectionInfo.groupName = ginfo.groupName;
        parent->currConnectionInfo.groupPassword.clear();
        parent->currConnectionInfo.groupIsPublic = true;
        parent->currConnectionInfo.timestamp = Time::getCurrentTime().toMilliseconds();
        parent->processor.addRecentServerConnectionInfo(parent->currConnectionInfo);

        bool isPublic = true;
        parent->processor.joinServerGroup(parent->currConnectionInfo.groupName, parent->currConnectionInfo.groupPassword, isPublic);

        parent->processor.setWatchPublicGroups(false);
    }
    else {

        AooServerConnectionInfo cinfo;
        cinfo.userName = parent->mPublicServerUsernameEditor->getText();
        cinfo.groupName = ginfo.groupName;
        cinfo.groupIsPublic = true;
        cinfo.serverHost = parent->currConnectionInfo.serverHost;
        cinfo.serverPort = parent->currConnectionInfo.serverPort;

        parent->connectWithInfo(cinfo);
    }

}

void SonobusAudioProcessorEditor::PublicGroupsListModel::selectedRowsChanged(int rowNumber)
{

}
