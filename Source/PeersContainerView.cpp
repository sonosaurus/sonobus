/*
  ==============================================================================

    PeersContainerView.cpp
    Created: 27 Jun 2020 12:42:15pm
    Author:  Jesse Chappell

  ==============================================================================
*/

#include "PeersContainerView.h"


PeerViewInfo::PeerViewInfo() : smallLnf(12)
{
    
}


void PeerViewInfo::paint(Graphics& g) 
{
    g.fillAll (Colour(0xff111111));
}

void PeerViewInfo::resized()
{
    
    mainbox.performLayout(getLocalBounds());
    
    if (levelLabel) {
        levelLabel->setBounds(levelSlider->getBounds().removeFromLeft(levelSlider->getWidth()*0.5));
        bufferTimeLabel->setBounds(bufferTimeSlider->getBounds().removeFromLeft(bufferTimeSlider->getWidth()*0.5));
    }
    
    latActiveButton->setBounds(staticPingLabel->getX(), staticPingLabel->getY(), pingLabel->getRight() - staticPingLabel->getX(), latencyLabel->getBottom() - pingLabel->getY());
}


PeersContainerView::PeersContainerView(SonobusAudioProcessor& proc)
 : Component("pcv"),  processor(proc)
{
    mutedTextColor = Colour::fromFloatRGBA(0.8, 0.5, 0.2, 1.0);
    regularTextColor = Colour(0xc0eeeeee);
    
    rebuildPeerViews();
}

void PeersContainerView::configLevelSlider(Slider * slider)
{
    //slider->setTextValueSuffix(" dB");
    slider->setRange(0.0, 2.0, 0.0);
    slider->setSkewFactor(0.25);
    slider->setDoubleClickReturnValue(true, 1.0);
    slider->setTextBoxIsEditable(false);
    slider->setSliderSnapsToMousePosition(false);
    slider->setScrollWheelEnabled(false);
    slider->valueFromTextFunction = [](const String& s) -> float { return Decibels::decibelsToGain(s.getFloatValue()); };
    slider->textFromValueFunction = [](float v) -> String { return Decibels::toString(Decibels::gainToDecibels(v), 1); };
}


void PeersContainerView::configLabel(Label *label, bool val)
{
    if (val) {
        label->setFont(12);
        label->setColour(Label::textColourId, regularTextColor);
        label->setJustificationType(Justification::centredRight);        
    }
    else {
        label->setFont(14);
        //label->setColour(Label::textColourId, Colour(0xaaeeeeee));
        label->setJustificationType(Justification::centredLeft);
    }
}





void PeersContainerView::resized()
{
    
    peersBox.performLayout(getLocalBounds().reduced(5, 0));
    
    for (int i=0; i < mPeerViews.size(); ++i) {
        PeerViewInfo * pvf = mPeerViews.getUnchecked(i);
        pvf->resized();
    }
}

void PeersContainerView::showPopTip(const String & message, int timeoutMs, Component * target, int maxwidth)
{
    popTip.reset(new BubbleMessageComponent());
    popTip->setAllowedPlacement(BubbleComponent::above);
    
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

void PeersContainerView::paint(Graphics & g)
{    
    g.fillAll (Colours::black);
}

PeerViewInfo * PeersContainerView::createPeerViewInfo()
{
    /*
    struct PeerViewInfo {
        std::unique_ptr<TextButton> activeButton;
        std::unique_ptr<Label>  levelLabel;
        std::unique_ptr<Slider> levelSlider;
        std::unique_ptr<Label>  bufferTimeLabel;
        std::unique_ptr<Slider> bufferTimeSlider;        
    };
*/
    
    PeerViewInfo * pvf = new PeerViewInfo();

    pvf->nameLabel = std::make_unique<Label>("name", "");
    pvf->nameLabel->setJustificationType(Justification::centredLeft);

    pvf->addrLabel = std::make_unique<Label>("name", "");
    pvf->addrLabel->setJustificationType(Justification::centredLeft);
    pvf->addrLabel->setFont(12);
    
    pvf->sendMutedButton = std::make_unique<SonoDrawableButton>("sendmute", DrawableButton::ButtonStyle::ImageOnButtonBackground);
    std::unique_ptr<Drawable> sendallowimg(Drawable::createFromImageData(BinaryData::outgoing_allowed_svg, BinaryData::outgoing_allowed_svgSize));
    std::unique_ptr<Drawable> senddisallowimg(Drawable::createFromImageData(BinaryData::outgoing_disallowed_svg, BinaryData::outgoing_disallowed_svgSize));
    pvf->sendMutedButton->setImages(sendallowimg.get(), nullptr, nullptr, nullptr, senddisallowimg.get());
    pvf->sendMutedButton->addListener(this);
    pvf->sendMutedButton->setClickingTogglesState(true);
    //pvf->sendMutedButton->setColour(TextButton::buttonOnColourId, Colour::fromFloatRGBA(0.15, 0.15, 0.15, 0.7));
    pvf->sendMutedButton->setColour(TextButton::buttonOnColourId, Colour::fromFloatRGBA(0.2, 0.2, 0.2, 0.7));
    pvf->sendMutedButton->setColour(TextButton::buttonColourId, Colours::transparentBlack);
    
    pvf->recvMutedButton = std::make_unique<SonoDrawableButton>("recvmute", DrawableButton::ButtonStyle::ImageOnButtonBackground);
    std::unique_ptr<Drawable> recvallowimg(Drawable::createFromImageData(BinaryData::incoming_allowed_svg, BinaryData::incoming_allowed_svgSize));
    std::unique_ptr<Drawable> recvdisallowimg(Drawable::createFromImageData(BinaryData::incoming_disallowed_svg, BinaryData::incoming_disallowed_svgSize));
    pvf->recvMutedButton->setImages(recvallowimg.get(), nullptr, nullptr, nullptr, recvdisallowimg.get());
    pvf->recvMutedButton->addListener(this);
    pvf->recvMutedButton->setClickingTogglesState(true);
    pvf->recvMutedButton->setColour(TextButton::buttonOnColourId, Colour::fromFloatRGBA(0.2, 0.2, 0.2, 0.7));
    pvf->recvMutedButton->setColour(TextButton::buttonColourId, Colours::transparentBlack);

    pvf->latActiveButton = std::make_unique<TextButton>(""); // (TRANS("Latency\nTest"));
    pvf->latActiveButton->setClickingTogglesState(true);
    pvf->latActiveButton->setTriggeredOnMouseDown(true);
    pvf->latActiveButton->setLookAndFeel(&pvf->smallLnf);
    pvf->latActiveButton->addListener(this);
    pvf->latActiveButton->addMouseListener(this, false);

    pvf->statusLabel = std::make_unique<Label>("status", "");
    configLabel(pvf->statusLabel.get(), false);
    pvf->statusLabel->setJustificationType(Justification::centredLeft);

    
    pvf->levelSlider     = std::make_unique<Slider>(Slider::LinearBar,  Slider::TextBoxBelow);
    pvf->levelSlider->setName("level");
    pvf->levelSlider->addListener(this);
    configLevelSlider(pvf->levelSlider.get());
    
    pvf->levelLabel = std::make_unique<Label>("level", TRANS("Level"));
    configLabel(pvf->levelLabel.get(), false);

    pvf->panSlider1     = std::make_unique<Slider>(Slider::LinearBar,  Slider::TextBoxBelow);
    pvf->panSlider1->setName("pan1");
    pvf->panSlider1->addListener(this);
    pvf->panSlider1->getProperties().set ("fromCentre", true);
    pvf->panSlider1->getProperties().set ("noFill", true);
    pvf->panSlider1->setRange(-1, 1, 0.0f);
    pvf->panSlider1->setDoubleClickReturnValue(true, -1.0);
    pvf->panSlider1->setTextBoxIsEditable(false);
    pvf->panSlider1->setSliderSnapsToMousePosition(false);
    pvf->panSlider1->setScrollWheelEnabled(false);
    pvf->panSlider1->textFromValueFunction =  [](double v) -> String { if (fabs(v) < 0.01) return TRANS("C"); return String((int)rint(abs(v*100.0f))) + ((v > 0 ? "% R" : "% L")) ; };
    pvf->panSlider1->valueFromTextFunction =  [](const String& s) -> double { return s.getDoubleValue()*1e-2f; };
    
    pvf->panSlider2     = std::make_unique<Slider>(Slider::LinearBar,  Slider::TextBoxBelow);
    pvf->panSlider2->setName("pan2");
    pvf->panSlider2->addListener(this);
    pvf->panSlider2->getProperties().set ("fromCentre", true);
    pvf->panSlider2->getProperties().set ("noFill", true);
    pvf->panSlider2->setRange(-1, 1, 0.01f);
    pvf->panSlider2->setDoubleClickReturnValue(true, -1.0);
    pvf->panSlider2->setTextBoxIsEditable(false);
    pvf->panSlider2->setSliderSnapsToMousePosition(false);
    pvf->panSlider2->setScrollWheelEnabled(false);
    pvf->panSlider2->textFromValueFunction =  [](double v) -> String { if ( fabs(v) < 0.01) return TRANS("C"); return String((int)rint(abs(v*100.0f))) + ((v > 0 ? "% R" : "% L")) ; };
    pvf->panSlider2->valueFromTextFunction =  [](const String& s) -> double { return s.getDoubleValue()*1e-2f; };

    
    
    pvf->bufferTimeSlider     = std::make_unique<Slider>(Slider::LinearBar,  Slider::TextBoxBelow);
    pvf->bufferTimeSlider->setName("buffer");
    pvf->bufferTimeSlider->setRange(0, 1000, 1);
    pvf->bufferTimeSlider->setTextValueSuffix(" ms");
    pvf->bufferTimeSlider->setSkewFactor(0.4);
    pvf->bufferTimeSlider->setDoubleClickReturnValue(true, 20.0);
    pvf->bufferTimeSlider->setTextBoxIsEditable(false);
    pvf->bufferTimeSlider->setSliderSnapsToMousePosition(false);
    pvf->bufferTimeSlider->setChangeNotificationOnlyOnRelease(true);
    pvf->bufferTimeSlider->setScrollWheelEnabled(false);

    pvf->bufferTimeSlider->addListener(this);
    //configKnobSlider(pvf->bufferTimeSlider.get());

    pvf->autosizeButton = std::make_unique<SonoChoiceButton>();
    pvf->autosizeButton->addChoiceListener(this);
    pvf->autosizeButton->addItem(TRANS("Manual"), 1);
    pvf->autosizeButton->addItem(TRANS("Auto Up"), 1);
    pvf->autosizeButton->addItem(TRANS("Auto"), 1);
    pvf->autosizeButton->addListener(this);


    
    pvf->bufferTimeLabel = std::make_unique<Label>("level", TRANS("Net Buffer"));
    configLabel(pvf->bufferTimeLabel.get(), false);

    pvf->menuButton = std::make_unique<SonoDrawableButton>("menu", DrawableButton::ImageOnButtonBackground);
    DrawableImage dotimg;
    dotimg.setImage(ImageCache::getFromMemory (BinaryData::dots_icon_png, BinaryData::dots_icon_pngSize));
    pvf->menuButton->setImages(&dotimg);
    pvf->menuButton->addListener(this);

    pvf->formatChoiceButton = std::make_unique<SonoChoiceButton>();
    pvf->formatChoiceButton->addChoiceListener(this);
    int numformats = processor.getNumberAudioCodecFormats();
    for (int i=0; i < numformats; ++i) {
        pvf->formatChoiceButton->addItem(processor.getAudioCodeFormatName(i), i+1);
    }

    pvf->staticLatencyLabel = std::make_unique<Label>("lat", TRANS("Latency:"));
    configLabel(pvf->staticLatencyLabel.get(), true);
    pvf->staticPingLabel = std::make_unique<Label>("ping", TRANS("Ping:"));
    configLabel(pvf->staticPingLabel.get(), true);

    pvf->latencyLabel = std::make_unique<Label>("lat", TRANS("PRESS"));
    configLabel(pvf->latencyLabel.get(), true);
    pvf->pingLabel = std::make_unique<Label>("ping");
    configLabel(pvf->pingLabel.get(), true);

    pvf->sendActualBitrateLabel = std::make_unique<Label>("sbit");
    configLabel(pvf->sendActualBitrateLabel.get(), true);

    pvf->recvActualBitrateLabel = std::make_unique<Label>("rbit");
    configLabel(pvf->recvActualBitrateLabel.get(), true);
    pvf->recvActualBitrateLabel->addMouseListener(this, false);
    pvf->recvActualBitrateLabel->setInterceptsMouseClicks(true, false);
    
    
    // meters
    auto flags = foleys::LevelMeter::Horizontal|foleys::LevelMeter::Minimal;
    
    pvf->recvMeter = std::make_unique<foleys::LevelMeter>(foleys::LevelMeter::Minimal);
    pvf->recvMeter->setLookAndFeel(&(pvf->rmeterLnf));
    pvf->recvMeter->setRefreshRateHz(10);
    pvf->recvMeter->addMouseListener(this, false);

    //pvf->sendMeter = std::make_unique<foleys::LevelMeter>(flags);
    //pvf->sendMeter->setLookAndFeel(&(pvf->smeterLnf));
    
    return pvf;
}
    


void PeersContainerView::rebuildPeerViews()
{
    int numpeers = processor.getNumberRemotePeers();
    
    while (mPeerViews.size() < numpeers) {
        mPeerViews.add(createPeerViewInfo());        
    }
    while (mPeerViews.size() > numpeers) {
        mPeerViews.removeLast();
    }
    
    for (int i=0; i < mPeerViews.size(); ++i) {
        PeerViewInfo * pvf = mPeerViews.getUnchecked(i);

        pvf->addAndMakeVisible(pvf->sendMutedButton.get());
        pvf->addAndMakeVisible(pvf->recvMutedButton.get());
        pvf->addAndMakeVisible(pvf->latActiveButton.get());
        pvf->addAndMakeVisible(pvf->bufferTimeSlider.get());
        pvf->addAndMakeVisible(pvf->bufferTimeLabel.get());
        pvf->addAndMakeVisible(pvf->levelSlider.get());
        pvf->addAndMakeVisible(pvf->levelLabel.get());
        pvf->addAndMakeVisible(pvf->menuButton.get());
        pvf->addAndMakeVisible(pvf->formatChoiceButton.get());
        pvf->addAndMakeVisible(pvf->staticLatencyLabel.get());
        pvf->addAndMakeVisible(pvf->staticPingLabel.get());
        pvf->addAndMakeVisible(pvf->latencyLabel.get());
        pvf->addAndMakeVisible(pvf->pingLabel.get());
        pvf->addAndMakeVisible(pvf->nameLabel.get());
        pvf->addAndMakeVisible(pvf->addrLabel.get());
        pvf->addAndMakeVisible(pvf->statusLabel.get());
        pvf->addAndMakeVisible(pvf->autosizeButton.get());
        //pvf->addAndMakeVisible(pvf->sendMeter.get());
        pvf->addAndMakeVisible(pvf->recvMeter.get());
        pvf->addAndMakeVisible(pvf->sendActualBitrateLabel.get());
        pvf->addAndMakeVisible(pvf->recvActualBitrateLabel.get());
        pvf->addAndMakeVisible(pvf->panSlider1.get());
        pvf->addAndMakeVisible(pvf->panSlider2.get());

        addAndMakeVisible(pvf);
    }
    
    
    updatePeerViews();
    updateLayout();
    resized();
}

void PeersContainerView::updateLayout()
{
    int minitemheight = 36;
    int mincheckheight = 32;
    const int textheight = 18;

    
    peersBox.items.clear();
    peersBox.flexDirection = FlexBox::Direction::column;
    peersBox.justifyContent = FlexBox::JustifyContent::flexStart;
    int peersheight = 0;
    const int singleph =  minitemheight*3 + 12;
    
    
    for (int i=0; i < mPeerViews.size(); ++i) {
        PeerViewInfo * pvf = mPeerViews.getUnchecked(i);

        //pvf->updateLayout();
        
        const int ph = singleph;
        const int pw = 200;
        


        pvf->nameaddrbox.items.clear();
        pvf->nameaddrbox.flexDirection = FlexBox::Direction::column;
        pvf->nameaddrbox.items.add(FlexItem(100, 18, *pvf->nameLabel).withMargin(0).withFlex(1));
        pvf->nameaddrbox.items.add(FlexItem(100, 14, *pvf->addrLabel).withMargin(0).withFlex(0));
        pvf->nameaddrbox.items.add(FlexItem(3, 2));

        pvf->namebox.items.clear();
        pvf->namebox.flexDirection = FlexBox::Direction::row;
        pvf->namebox.items.add(FlexItem(mincheckheight, mincheckheight, *pvf->menuButton).withMargin(0).withFlex(0).withMaxHeight(mincheckheight).withAlignSelf(FlexItem::AlignSelf::center));
        pvf->namebox.items.add(FlexItem(100, minitemheight, pvf->nameaddrbox).withMargin(2).withFlex(1));

        
        pvf->sendbox.items.clear();
        pvf->sendbox.flexDirection = FlexBox::Direction::row;
        pvf->sendbox.items.add(FlexItem(80, minitemheight, *pvf->sendMutedButton).withMargin(0).withFlex(0));
        if (isNarrow) {
            pvf->sendbox.items.add(FlexItem(3, 2));
            pvf->sendbox.items.add(FlexItem(60, minitemheight, *pvf->formatChoiceButton).withMargin(0).withFlex(1));
            pvf->sendbox.items.add(FlexItem(60, minitemheight, *pvf->sendActualBitrateLabel).withMargin(0).withFlex(1));
        }
        
        pvf->sendmeterbox.items.clear();
        pvf->sendmeterbox.flexDirection = FlexBox::Direction::row;
        //pvf->sendmeterbox.items.add(FlexItem(60, minitemheight, *pvf->sendMeter).withMargin(0).withFlex(1));
        if (!isNarrow) {
            pvf->sendmeterbox.items.add(FlexItem(60, minitemheight, *pvf->formatChoiceButton).withMargin(0).withFlex(2));
            pvf->sendmeterbox.items.add(FlexItem(60, minitemheight, *pvf->sendActualBitrateLabel).withMargin(0).withFlex(1));
        }

        pvf->recvbox.items.clear();
        pvf->recvbox.flexDirection = FlexBox::Direction::row;
        pvf->recvbox.items.add(FlexItem(80, mincheckheight, *pvf->recvMutedButton).withMargin(0).withFlex(0));
        pvf->recvbox.items.add(FlexItem(60, mincheckheight, *pvf->recvActualBitrateLabel).withMargin(0).withFlex(1));
        //pvf->recvbox.items.add(FlexItem(40, mincheckheight, *pvf->latActiveButton).withMargin(0).withFlex(1).withMaxWidth(60));

        pvf->pingbox.items.clear();
        pvf->pingbox.flexDirection = FlexBox::Direction::row;
        pvf->pingbox.items.add(FlexItem(60, textheight, *pvf->staticPingLabel).withMargin(0).withFlex(0));
        pvf->pingbox.items.add(FlexItem(40, textheight, *pvf->pingLabel).withMargin(0).withFlex(0));

        pvf->latencybox.items.clear();
        pvf->latencybox.flexDirection = FlexBox::Direction::row;
        pvf->latencybox.items.add(FlexItem(60, textheight, *pvf->staticLatencyLabel).withMargin(0).withFlex(0));
        pvf->latencybox.items.add(FlexItem(40, textheight, *pvf->latencyLabel).withMargin(0).withFlex(0));

        
        pvf->netstatbox.items.clear();
        pvf->netstatbox.flexDirection = FlexBox::Direction::column;
        pvf->netstatbox.items.add(FlexItem(100, textheight, pvf->pingbox).withMargin(0).withFlex(0));
        pvf->netstatbox.items.add(FlexItem(100, textheight, pvf->latencybox).withMargin(0).withFlex(0));

        
        pvf->recvnetbox.items.clear();
        pvf->recvnetbox.flexDirection = FlexBox::Direction::row;
        pvf->recvnetbox.items.add(FlexItem(72, minitemheight, *pvf->autosizeButton).withMargin(0).withFlex(0));
        pvf->recvnetbox.items.add(FlexItem(2, 5));
        pvf->recvnetbox.items.add(FlexItem(60, minitemheight, *pvf->bufferTimeSlider).withMargin(0).withFlex(1));
        pvf->recvnetbox.items.add(FlexItem(2, 5));
        pvf->recvnetbox.items.add(FlexItem(100, minitemheight, pvf->netstatbox).withMargin(0).withFlex(0));

        pvf->recvlevelbox.items.clear();
        pvf->recvlevelbox.flexDirection = FlexBox::Direction::row;
        pvf->recvlevelbox.items.add(FlexItem(80, minitemheight, *pvf->levelSlider).withMargin(0).withFlex(2));
        if (processor.getTotalNumOutputChannels() > 1) {            
            pvf->recvlevelbox.items.add(FlexItem(3, 5));
            pvf->recvlevelbox.items.add(FlexItem(40, minitemheight, *pvf->panSlider1).withMargin(0).withFlex(0));
            if (processor.getRemotePeerChannelCount(i) > 1) {
                pvf->recvlevelbox.items.add(FlexItem(2, 5));
                pvf->recvlevelbox.items.add(FlexItem(40, minitemheight, *pvf->panSlider2).withMargin(0).withFlex(0));
            }
        }

        
        pvf->mainrecvbox.items.clear();
        pvf->mainrecvbox.flexDirection = FlexBox::Direction::column;
        pvf->mainrecvbox.items.add(FlexItem(150, minitemheight, pvf->recvbox).withMargin(2).withFlex(0));
        pvf->mainrecvbox.items.add(FlexItem(150, minitemheight, pvf->recvnetbox).withMargin(2).withFlex(0));
        pvf->mainrecvbox.items.add(FlexItem(150, minitemheight, pvf->recvlevelbox).withMargin(2).withFlex(0));

        
        pvf->mainsendbox.items.clear();
        pvf->mainsendbox.flexDirection = FlexBox::Direction::column;
        pvf->mainsendbox.items.add(FlexItem(100 + mincheckheight, minitemheight +2, pvf->namebox).withMargin(2).withFlex(0));
        pvf->mainsendbox.items.add(FlexItem(120, minitemheight, pvf->sendbox).withMargin(2).withFlex(0));

        if (isNarrow) {
            //pvf->mainsendbox.items.add(FlexItem(120, minitemheight*0.5, pvf->sendmeterbox).withMargin(2).withFlex(0));
        } else {
            pvf->mainsendbox.items.add(FlexItem(120, minitemheight, pvf->sendmeterbox).withMargin(2).withFlex(0));            
        }
        
        
        pvf->mainbox.items.clear();
        pvf->mainnarrowbox.items.clear();

        if (isNarrow) {
            pvf->mainnarrowbox.flexDirection = FlexBox::Direction::column;
            pvf->mainnarrowbox.items.add(FlexItem(100 + mincheckheight, singleph - minitemheight - 4, pvf->mainsendbox).withMargin(2).withFlex(0));
            pvf->mainnarrowbox.items.add(FlexItem(150, singleph, pvf->mainrecvbox).withMargin(2).withFlex(0));

            pvf->mainbox.flexDirection = FlexBox::Direction::row;
            pvf->mainbox.items.add(FlexItem(150, singleph*2 - minitemheight , pvf->mainnarrowbox).withMargin(2).withFlex(1));
            pvf->mainbox.items.add(FlexItem(30, 50, *pvf->recvMeter).withMargin(2).withFlex(0));
        } else {            
            pvf->mainbox.flexDirection = FlexBox::Direction::row;
            pvf->mainbox.items.add(FlexItem(100 + mincheckheight, singleph, pvf->mainsendbox).withMargin(2).withFlex(1));
            pvf->mainbox.items.add(FlexItem(150, singleph, pvf->mainrecvbox).withMargin(2).withFlex(3));
            pvf->mainbox.items.add(FlexItem(30, 50, *pvf->recvMeter).withMargin(2).withFlex(0));
        }

        peersBox.items.add(FlexItem(8, 5).withMargin(0));
        
        if (isNarrow) {
            peersBox.items.add(FlexItem(pw, ph*2 - minitemheight + 10, *pvf).withMargin(0).withFlex(0));
            peersheight += ph*2 + 6 + 5;            
        } else {
            peersBox.items.add(FlexItem(pw, ph + 6, *pvf).withMargin(0).withFlex(0));
            peersheight += ph + 6 + 5;
        }

    }


    if (isNarrow) {
        peersMinHeight = std::max(singleph*2 + 14,  peersheight);
        peersMinWidth = 320;  
    }
    else {
        peersMinHeight = std::max(singleph + 11,  peersheight);
        peersMinWidth = 180 + 150 + mincheckheight + 50;
    }
    
}

Rectangle<int> PeersContainerView::getMinimumContentBounds() const
{
    return Rectangle<int>(0,0,peersMinWidth, peersMinHeight);
}


void PeersContainerView::updatePeerViews()
{
    double nowstampms = Time::getMillisecondCounterHiRes();
    
    for (int i=0; i < mPeerViews.size(); ++i) {
        PeerViewInfo * pvf = mPeerViews.getUnchecked(i);
        String hostname;
        int port = 0;
        processor.getRemotePeerAddressInfo(i, hostname, port);
        bool connected = processor.getRemotePeerConnected(i);

        String username = processor.getRemotePeerUserName(i);
        String addrname = String::formatted("%s : %d", hostname.toRawUTF8(), port);
        
        if (username.isNotEmpty()) {
            pvf->nameLabel->setText(username, dontSendNotification);
            pvf->addrLabel->setText(addrname, dontSendNotification);
            pvf->addrLabel->setVisible(true);
        } else {
            pvf->addrLabel->setVisible(false);
            pvf->nameLabel->setText(addrname, dontSendNotification);
        }

        String sendtext;
        bool sendactive = processor.getRemotePeerSendActive(i);
        bool sendallow = processor.getRemotePeerSendAllow(i);

        bool recvactive = processor.getRemotePeerRecvActive(i);
        bool recvallow = processor.getRemotePeerRecvAllow(i);
        bool latactive = processor.isRemotePeerLatencyTestActive(i);

        double sendrate = 0.0;
        double recvrate = 0.0;
        
        if (lastUpdateTimestampMs > 0) {
            double timedelta = (nowstampms - lastUpdateTimestampMs) * 1e-3;
            int64_t nbs = processor.getRemotePeerBytesSent(i);
            int64_t nbr = processor.getRemotePeerBytesReceived(i);
            
            sendrate = (nbs - pvf->lastBytesSent) / timedelta;
            recvrate = (nbr - pvf->lastBytesRecv) / timedelta;
            
            pvf->lastBytesRecv = nbr;
            pvf->lastBytesSent = nbs;
        }
                
        if (sendactive) {
            //sendtext += String::formatted("%Ld sent", processor.getRemotePeerPacketsSent(i) );
            sendtext += String::formatted("%.d kb/s", lrintf(sendrate * 8 * 1e-3) );
            pvf->sendActualBitrateLabel->setColour(Label::textColourId, regularTextColor);
        }
        else if (sendallow) {
            sendtext += TRANS("Other end muted us, not sending audio");
            pvf->sendActualBitrateLabel->setColour(Label::textColourId, mutedTextColor);
        }
        else {
            sendtext += TRANS("Self-muted, not sending audio");
            pvf->sendActualBitrateLabel->setColour(Label::textColourId, mutedTextColor);
        }
        
        String recvtext;
        
        if (recvactive) {
            //recvtext += String::formatted("%Ld recvd", processor.getRemotePeerPacketsReceived(i));
            recvtext += String::formatted("%d kb/s", lrintf(recvrate * 8 * 1e-3));

            int64_t dropped = processor.getRemotePeerPacketsDropped(i);
            if (dropped > 0) {
                recvtext += String::formatted(" | %d drop", dropped);
            }

            int64_t resent = processor.getRemotePeerPacketsResent(i);
            if (resent > 0) {
                recvtext += String::formatted(" | %d resent", resent);
            }

            pvf->recvActualBitrateLabel->setColour(Label::textColourId, regularTextColor);
        } 
        else if (recvallow) {
            recvtext += TRANS("Other side is muted");    
            pvf->recvActualBitrateLabel->setColour(Label::textColourId, mutedTextColor);
        }
        else {
            recvtext += TRANS("You muted them");    
            pvf->recvActualBitrateLabel->setColour(Label::textColourId, mutedTextColor);
        }


        
        pvf->sendActualBitrateLabel->setText(sendtext, dontSendNotification);
        pvf->recvActualBitrateLabel->setText(recvtext, dontSendNotification);

        bool estlat = true;
        bool isreal = false;
        float totallat = processor.getRemotePeerTotalLatencyMs(i, isreal, estlat);
        
        pvf->pingLabel->setText(String::formatted("%d ms", (int)processor.getRemotePeerPingMs(i) ), dontSendNotification);

        if (!isreal) {
            pvf->latencyLabel->setText(TRANS("PRESS"), dontSendNotification);
        } else {
            pvf->latencyLabel->setText(String::formatted("%d ms%s", (int)lrintf(totallat), estlat ? "*" : "" ), dontSendNotification);
            //pvf->latencyLabel->setText(String::formatted("%d ms", (int)totallat), dontSendNotification);
        }

        
        //pvf->statusLabel->setText(statustext, dontSendNotification);
        
        pvf->sendMutedButton->setToggleState(!processor.getRemotePeerSendAllow(i) , dontSendNotification);
        pvf->recvMutedButton->setToggleState(!processor.getRemotePeerRecvAllow(i) , dontSendNotification);

        pvf->latActiveButton->setToggleState(latactive, dontSendNotification);

        
        pvf->autosizeButton->setSelectedItemIndex((int)processor.getRemotePeerAutoresizeBufferMode(i), dontSendNotification);

        
        if (!pvf->levelSlider->isMouseOverOrDragging()) {
            pvf->levelSlider->setValue(processor.getRemotePeerLevelGain(i), dontSendNotification);
        }
        if (!pvf->bufferTimeSlider->isMouseButtonDown()) {
            pvf->bufferTimeSlider->setValue(processor.getRemotePeerBufferTime(i), dontSendNotification);
        }

        
        int formatindex = processor.getRemotePeerAudioCodecFormat(i);
        pvf->formatChoiceButton->setSelectedItemIndex(formatindex >= 0 ? formatindex : processor.getDefaultAudioCodecFormat(), dontSendNotification);
        
        pvf->recvMeter->setMeterSource (processor.getRemotePeerRecvMeterSource(i));        
        //pvf->sendMeter->setMeterSource (processor.getRemotePeerSendMeterSource(i));

        pvf->panSlider1->setValue(processor.getRemotePeerChannelPan(i, 0), dontSendNotification);
        pvf->panSlider2->setValue(processor.getRemotePeerChannelPan(i, 1), dontSendNotification);
        
        
        if (processor.getTotalNumOutputChannels() == 1) {
            pvf->panSlider1->setVisible(false);
            pvf->panSlider2->setVisible(false);
        }
        else if (processor.getRemotePeerChannelCount(i) == 1) {
            pvf->panSlider1->setDoubleClickReturnValue(true, 0.0);
            pvf->panSlider2->setVisible(false);
        } else {
            pvf->panSlider1->setDoubleClickReturnValue(true, -1.0);
            pvf->panSlider2->setDoubleClickReturnValue(true, 1.0);
            pvf->panSlider1->setVisible(true);
            pvf->panSlider2->setVisible(true); 
        } 
        
        //pvf->packetsizeSlider->setValue(findHighestSetBit(processor.getRemotePeerSendPacketsize(i)), dontSendNotification);
        
        //pvf->removeButton->setEnabled(connected);
        pvf->levelSlider->setEnabled(recvactive);
        pvf->bufferTimeSlider->setEnabled(recvactive);
        pvf->autosizeButton->setEnabled(recvactive);
        pvf->panSlider1->setEnabled(recvactive);
        pvf->panSlider2->setEnabled(recvactive);
        pvf->formatChoiceButton->setEnabled(sendactive);

        const float disalpha = 0.4;
        pvf->formatChoiceButton->setAlpha(sendactive ? 1.0 : disalpha);
        pvf->nameLabel->setAlpha(connected ? 1.0 : 0.8);
        pvf->addrLabel->setAlpha(connected ? 1.0 : 0.8);
        //pvf->sendMutedButton->setAlpha(connected ? 1.0 : 0.8);
        //pvf->recvMutedButton->setAlpha(connected ? 1.0 : 0.8);
        //pvf->latActiveButton->setAlpha(connected ? 1.0 : 0.8);
        pvf->levelLabel->setAlpha(recvactive ? 1.0 : disalpha);
        pvf->statusLabel->setAlpha(connected ? 1.0 : disalpha);
        pvf->levelSlider->setAlpha(recvactive ? 1.0 : disalpha);
        pvf->bufferTimeLabel->setAlpha(recvactive ? 1.0 : disalpha);
        pvf->bufferTimeSlider->setAlpha(recvactive ? 1.0 : disalpha);
        pvf->panSlider1->setAlpha(recvactive ? 1.0 : disalpha);
        pvf->panSlider2->setAlpha(recvactive ? 1.0 : disalpha);
        //pvf->autosizeButton->setAlpha(recvactive ? 1.0 : 0.6);
        
        if (pvf->stopLatencyTestTimestampMs > 0.0 && nowstampms > pvf->stopLatencyTestTimestampMs
            && !pvf->latActiveButton->isMouseButtonDown()) {

            bool isreal=false, est = false;
            processor.getRemotePeerTotalLatencyMs(i, isreal, est);

            // only stop if it has actually gotten a real latency
            if (isreal) {
                stopLatencyTest(i);
                
                if (popTip && popTip->isVisible()) {
                    Desktop::getInstance().getAnimator().fadeOut(popTip.get(), 200);
                    popTip.reset();
                }
            }
        }
    }
    
    lastUpdateTimestampMs = nowstampms;
}

void PeersContainerView::startLatencyTest(int i)
{
    if (i >= mPeerViews.size()) return;
    
    PeerViewInfo * pvf = mPeerViews.getUnchecked(i);

    pvf->stopLatencyTestTimestampMs = Time::getMillisecondCounterHiRes() + 1500.0;
    pvf->wasRecvActiveAtLatencyTest = processor.getRemotePeerRecvActive(i);
    pvf->wasSendActiveAtLatencyTest = processor.getRemotePeerSendActive(i);
    
    //processor.setRemotePeerSendActive(i, false);
    //processor.setRemotePeerRecvActive(i, false);
    
    processor.startRemotePeerLatencyTest(i);             
}

void PeersContainerView::stopLatencyTest(int i)
{
    if (i >= mPeerViews.size()) return;
    PeerViewInfo * pvf = mPeerViews.getUnchecked(i);

    processor.stopRemotePeerLatencyTest(i);
    
    /*
    if (pvf->wasRecvActiveAtLatencyTest) {
        processor.setRemotePeerRecvActive(i, true);
    }
    if (pvf->wasSendActiveAtLatencyTest) {
        processor.setRemotePeerSendActive(i, true);
    }
    */
     
    pvf->stopLatencyTestTimestampMs = 0.0;
    
}


void PeersContainerView::choiceButtonSelected(SonoChoiceButton *comp, int index, int ident)
{
    for (int i=0; i < mPeerViews.size(); ++i) {
        PeerViewInfo * pvf = mPeerViews.getUnchecked(i);
        if (pvf->formatChoiceButton.get() == comp) {
            processor.setRemotePeerAudioCodecFormat(i, index);
            break;
        }        
        else if (pvf->autosizeButton.get() == comp) {
            processor.setRemotePeerAutoresizeBufferMode(i, (SonobusAudioProcessor::AutoNetBufferMode) index);
            break;
        }        
    }
}

void PeersContainerView::buttonClicked (Button* buttonThatWasClicked)
{
    for (int i=0; i < mPeerViews.size(); ++i) {
        PeerViewInfo * pvf = mPeerViews.getUnchecked(i);
        bool connected = processor.getRemotePeerConnected(i);
        //bool sendallow = processor.getRemotePeerSendAllow(i);
        //bool recvallow = processor.getRemotePeerRecvAllow(i);
        //bool sendactive = processor.getRemotePeerSendActive(i);
        //bool recvactive = processor.getRemotePeerRecvActive(i);
        bool isGroupPeer = processor.getRemotePeerUserName(i).isNotEmpty();
        
        if (pvf->sendMutedButton.get() == buttonThatWasClicked) {
            if (!connected && !isGroupPeer) {
                String hostname;
                int port = 0;
                processor.getRemotePeerAddressInfo(i, hostname, port);
                processor.connectRemotePeer(hostname, port);
            } else {
                // turns on allow and starts sending
                if (!pvf->sendMutedButton->getToggleState()) {
                    processor.setRemotePeerSendActive(i, true);
                } else {
                    // turns off sending and allow
                    processor.setRemotePeerSendAllow(i, false);
                }
            }
            break;
        }
        else if (pvf->recvMutedButton.get() == buttonThatWasClicked) {
            if (!connected && !isGroupPeer) {
                String hostname;
                int port = 0;
                processor.getRemotePeerAddressInfo(i, hostname, port);
                processor.connectRemotePeer(hostname, port);
            } else {
                if (!pvf->recvMutedButton->getToggleState()) {
                    // allows receiving and invites
                    processor.setRemotePeerRecvActive(i, true);             
                } else {
                    // turns off receiving and allow
                    processor.setRemotePeerRecvAllow(i, false);             
                }
            }
            break;
        }
        else if (pvf->latActiveButton.get() == buttonThatWasClicked) {
            if (pvf->latActiveButton->getToggleState()) {
                startLatencyTest(i);
                showPopTip(TRANS("Measuring actual round-trip latency..."), 4000, pvf->latActiveButton.get(), 140);
            } else {
                stopLatencyTest(i);
                if (popTip && popTip->isVisible()) {
                    Desktop::getInstance().getAnimator().fadeOut(popTip.get(), 200);
                    popTip.reset();
                }
            }
            break;
        }

        //else if (pvf->autosizeButton.get() == buttonThatWasClicked) {
            //processor.setRemotePeerAutoresizeBufferMode(i, (SonobusAudioProcessor::AutoNetBufferMode) pvf->autosizeButton->getSelectedItemIndex());
        //}
        else if (pvf->menuButton.get() == buttonThatWasClicked) {
            
            showPopupMenu(pvf->menuButton.get(), i);

            break;
        }
         
    }    
}

void PeersContainerView::mouseUp (const MouseEvent& event) 
{
    for (int i=0; i < mPeerViews.size(); ++i) {
        PeerViewInfo * pvf = mPeerViews.getUnchecked(i);

        if (event.eventComponent == pvf->latActiveButton.get()) {
            double nowtimems = Time::getMillisecondCounterHiRes();
            if (nowtimems >= pvf->stopLatencyTestTimestampMs) {
                stopLatencyTest(i);
                pvf->latActiveButton->setToggleState(false, dontSendNotification);
                if (popTip && popTip->isVisible()) {
                    Desktop::getInstance().getAnimator().fadeOut(popTip.get(), 200);
                    popTip.reset();
                }
            }
            break;
        }
        else if (event.eventComponent == pvf->recvActualBitrateLabel.get()) {
            // reset stats
            processor.resetRemotePeerPacketStats(i);
        }
        else if (event.eventComponent == pvf->recvMeter.get()) {
            pvf->recvMeter->clearClipIndicator(-1);
        }

    }
}

void PeersContainerView::showPopupMenu(Component * source, int index)
{
    if (index >= mPeerViews.size()) return;
    
    PeerViewInfo * pvf = mPeerViews.getUnchecked(index);
    bool isGroupPeer = processor.getRemotePeerUserName(index).isNotEmpty();

    Array<GenericItemChooserItem> items;
    if (isGroupPeer) {
        if (processor.getRemotePeerRecvAllow(index) && processor.getRemotePeerSendAllow(index)) {
            items.add(GenericItemChooserItem(TRANS("Mute All")));
        } else {
            items.add(GenericItemChooserItem(TRANS("Unmute All")));
        }
    } else {
        if (processor.getRemotePeerConnected(index)) {
            items.add(GenericItemChooserItem(TRANS("Mute All")));
        } else {
            items.add(GenericItemChooserItem(TRANS("Unmute All")));        
        }        
    }
    items.add(GenericItemChooserItem(TRANS("Remove")));
    
    Component* dw = source->findParentComponentOfClass<DocumentWindow>();

    if (!dw) {
        dw = source->findParentComponentOfClass<AudioProcessorEditor>();        
    }
    if (!dw) {
        dw = source->findParentComponentOfClass<Component>();        
    }
    
    //chooser->setSize(jmin(chooser->getWidth(), dw->getWidth() - 16), jmin(chooser->getHeight(), dw->getHeight() - 20));

    
    Rectangle<int> bounds =  dw->getLocalArea(nullptr, source->getScreenBounds());
    
    GenericItemChooser::launchPopupChooser(items, bounds, dw, this, index+1);
}

void PeersContainerView::genericItemChooserSelected(GenericItemChooser *comp, int index)
{
    // popup menu
    int vindex = comp->getTag() - 1;
    if (vindex >= mPeerViews.size()) return;        
    //PeerViewInfo * pvf = mPeerViews.getUnchecked(vindex);
    
    bool isGroupPeer = processor.getRemotePeerUserName(vindex).isNotEmpty();
    
    if (index == 0) {

        if (isGroupPeer) {
            if (!processor.getRemotePeerRecvAllow(vindex) || !processor.getRemotePeerSendAllow(vindex)) {
                // allow everything
                processor.setRemotePeerSendActive(vindex, true); 
                processor.setRemotePeerRecvActive(vindex, true);                 
            }
            else {
                // disallow everything
                processor.setRemotePeerSendAllow(vindex, false); 
                processor.setRemotePeerRecvAllow(vindex, false); 
            }
        }
        else {
            if (processor.getRemotePeerConnected(vindex)) {
                // disconnect
                processor.disconnectRemotePeer(vindex);  
            } else {
                
                String hostname;
                int port = 0;
                processor.getRemotePeerAddressInfo(vindex, hostname, port);
                processor.connectRemotePeer(hostname, port);            
            }
        }
    }
    else if (index == 1) {
        processor.removeRemotePeer(vindex);
    }
    
    
    if (CallOutBox* const cb = comp->findParentComponentOfClass<CallOutBox>()) {
        cb->dismiss();
    }

}


void PeersContainerView::sliderValueChanged (Slider* slider)
{
   for (int i=0; i < mPeerViews.size(); ++i) {
       PeerViewInfo * pvf = mPeerViews.getUnchecked(i);
       if (pvf->levelSlider.get() == slider) {
           processor.setRemotePeerLevelGain(i, pvf->levelSlider->getValue());   
       }
       else if (pvf->bufferTimeSlider.get() == slider) {
           processor.setRemotePeerBufferTime(i, pvf->bufferTimeSlider->getValue());   
       }
       else if (pvf->panSlider1.get() == slider) {
           processor.setRemotePeerChannelPan(i, 0, pvf->panSlider1->getValue());   
       }
       else if (pvf->panSlider2.get() == slider) {
           processor.setRemotePeerChannelPan(i, 1, pvf->panSlider2->getValue());   
       }

   }

}
