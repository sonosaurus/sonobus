/*
  ==============================================================================

    PeersContainerView.cpp
    Created: 27 Jun 2020 12:42:15pm
    Author:  Jesse Chappell

  ==============================================================================
*/

#include "PeersContainerView.h"


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
        label->setColour(Label::textColourId, Colour(0xc0eeeeee));
        label->setJustificationType(Justification::centredRight);        
    }
    else {
        label->setFont(14);
        //label->setColour(Label::textColourId, Colour(0xaaeeeeee));
        label->setJustificationType(Justification::centredLeft);
    }
}



PeerViewInfo::PeerViewInfo()
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
}


PeersContainerView::PeersContainerView(FluxAoOAudioProcessor& proc)
 : Component("pcv"),  processor(proc)
{
    rebuildPeerViews();
}

void PeersContainerView::resized()
{
    peersBox.performLayout(getLocalBounds().reduced(5, 0));
    
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

    
    pvf->sendActiveButton = std::make_unique<ToggleButton>(TRANS("Send"));
    pvf->sendActiveButton->addListener(this);

    pvf->recvActiveButton = std::make_unique<ToggleButton>(TRANS("Allow Recv"));
    pvf->recvActiveButton->addListener(this);

    pvf->statusLabel = std::make_unique<Label>("status", "");
    configLabel(pvf->statusLabel.get(), false);
    pvf->statusLabel->setJustificationType(Justification::centredLeft);

    
    pvf->levelSlider     = std::make_unique<Slider>(Slider::LinearBar,  Slider::TextBoxBelow);
    pvf->levelSlider->setName("level");
    pvf->levelSlider->addListener(this);
    configLevelSlider(pvf->levelSlider.get());
    
    pvf->levelLabel = std::make_unique<Label>("level", TRANS("Level"));
    configLabel(pvf->levelLabel.get(), false);

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

    pvf->latencyLabel = std::make_unique<Label>("lat");
    configLabel(pvf->latencyLabel.get(), true);
    pvf->pingLabel = std::make_unique<Label>("ping");
    configLabel(pvf->pingLabel.get(), true);

    pvf->sendActualBitrateLabel = std::make_unique<Label>("sbit");
    configLabel(pvf->sendActualBitrateLabel.get(), true);
    pvf->recvActualBitrateLabel = std::make_unique<Label>("rbit");
    configLabel(pvf->recvActualBitrateLabel.get(), true);

    
    // meters
    auto flags = foleys::LevelMeter::Horizontal|foleys::LevelMeter::Minimal;
    
    pvf->recvMeter = std::make_unique<foleys::LevelMeter>(foleys::LevelMeter::Minimal);
    pvf->recvMeter->setLookAndFeel(&(pvf->rmeterLnf));

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

        pvf->addAndMakeVisible(pvf->sendActiveButton.get());
        pvf->addAndMakeVisible(pvf->recvActiveButton.get());
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
        pvf->addAndMakeVisible(pvf->statusLabel.get());
        //pvf->addAndMakeVisible(pvf->sendMeter.get());
        pvf->addAndMakeVisible(pvf->recvMeter.get());
        pvf->addAndMakeVisible(pvf->sendActualBitrateLabel.get());
        pvf->addAndMakeVisible(pvf->recvActualBitrateLabel.get());

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
    const int singleph = mincheckheight + minitemheight*2 + 12;
    
    
    for (int i=0; i < mPeerViews.size(); ++i) {
        PeerViewInfo * pvf = mPeerViews.getUnchecked(i);

        //pvf->updateLayout();
        
        const int ph = singleph;
        const int pw = 200;
        


        pvf->namebox.items.clear();
        pvf->namebox.flexDirection = FlexBox::Direction::row;
        pvf->namebox.items.add(FlexItem(mincheckheight, mincheckheight, *pvf->menuButton).withMargin(0).withFlex(0));
        pvf->namebox.items.add(FlexItem(100, mincheckheight, *pvf->nameLabel).withMargin(2).withFlex(1));

        pvf->sendbox.items.clear();
        pvf->sendbox.flexDirection = FlexBox::Direction::row;
        pvf->sendbox.items.add(FlexItem(60, minitemheight, *pvf->sendActiveButton).withMargin(0).withFlex(1));
        pvf->sendbox.items.add(FlexItem(60, minitemheight, *pvf->formatChoiceButton).withMargin(0).withFlex(1));

        
        pvf->sendmeterbox.items.clear();
        pvf->sendmeterbox.flexDirection = FlexBox::Direction::row;
        pvf->sendmeterbox.items.add(FlexItem(60, minitemheight, *pvf->sendMeter).withMargin(0).withFlex(1));
        pvf->sendmeterbox.items.add(FlexItem(60, minitemheight, *pvf->sendActualBitrateLabel).withMargin(0).withFlex(1));

        pvf->recvbox.items.clear();
        pvf->recvbox.flexDirection = FlexBox::Direction::row;
        pvf->recvbox.items.add(FlexItem(60, mincheckheight, *pvf->recvActiveButton).withMargin(0).withFlex(1));
        pvf->recvbox.items.add(FlexItem(60, mincheckheight, *pvf->recvActualBitrateLabel).withMargin(0).withFlex(0));

        pvf->pingbox.items.clear();
        pvf->pingbox.flexDirection = FlexBox::Direction::row;
        pvf->pingbox.items.add(FlexItem(50, textheight, *pvf->staticPingLabel).withMargin(0).withFlex(0));
        pvf->pingbox.items.add(FlexItem(40, textheight, *pvf->pingLabel).withMargin(0).withFlex(0));

        pvf->latencybox.items.clear();
        pvf->latencybox.flexDirection = FlexBox::Direction::row;
        pvf->latencybox.items.add(FlexItem(50, textheight, *pvf->staticLatencyLabel).withMargin(0).withFlex(0));
        pvf->latencybox.items.add(FlexItem(40, textheight, *pvf->latencyLabel).withMargin(0).withFlex(0));

        
        pvf->netstatbox.items.clear();
        pvf->netstatbox.flexDirection = FlexBox::Direction::column;
        pvf->netstatbox.items.add(FlexItem(90, textheight, pvf->pingbox).withMargin(0).withFlex(0));
        pvf->netstatbox.items.add(FlexItem(90, textheight, pvf->latencybox).withMargin(0).withFlex(0));

        
        pvf->recvnetbox.items.clear();
        pvf->recvnetbox.flexDirection = FlexBox::Direction::row;
        pvf->recvnetbox.items.add(FlexItem(60, minitemheight, *pvf->bufferTimeSlider).withMargin(0).withFlex(1));
        pvf->recvnetbox.items.add(FlexItem(90, minitemheight, pvf->netstatbox).withMargin(0).withFlex(0));

        pvf->recvlevelbox.items.clear();
        pvf->recvlevelbox.flexDirection = FlexBox::Direction::row;
        pvf->recvlevelbox.items.add(FlexItem(100, minitemheight, *pvf->levelSlider).withMargin(0).withFlex(2));

        
        pvf->mainrecvbox.items.clear();
        pvf->mainrecvbox.flexDirection = FlexBox::Direction::column;
        pvf->mainrecvbox.items.add(FlexItem(150, mincheckheight, pvf->recvbox).withMargin(2).withFlex(0));
        pvf->mainrecvbox.items.add(FlexItem(150, minitemheight, pvf->recvnetbox).withMargin(2).withFlex(0));
        pvf->mainrecvbox.items.add(FlexItem(150, minitemheight, pvf->recvlevelbox).withMargin(2).withFlex(0));

        
        pvf->mainsendbox.items.clear();
        pvf->mainsendbox.flexDirection = FlexBox::Direction::column;
        pvf->mainsendbox.items.add(FlexItem(100 + mincheckheight, mincheckheight, pvf->namebox).withMargin(2).withFlex(0));
        pvf->mainsendbox.items.add(FlexItem(120, minitemheight, pvf->sendbox).withMargin(2).withFlex(0));
        pvf->mainsendbox.items.add(FlexItem(120, minitemheight, pvf->sendmeterbox).withMargin(2).withFlex(0));
        
        
        pvf->mainbox.items.clear();
        pvf->mainbox.flexDirection = FlexBox::Direction::row;
        pvf->mainbox.items.add(FlexItem(100 + mincheckheight, singleph, pvf->mainsendbox).withMargin(2).withFlex(1));
        pvf->mainbox.items.add(FlexItem(30, 50, *pvf->recvMeter).withMargin(2).withFlex(0));
        pvf->mainbox.items.add(FlexItem(150, singleph, pvf->mainrecvbox).withMargin(2).withFlex(1));

        peersBox.items.add(FlexItem(8, 5).withMargin(0));
        peersBox.items.add(FlexItem(pw, ph + 6, *pvf).withMargin(0).withFlex(0));

        peersheight += ph + 6 + 5;
    }

    peersMinHeight = std::max(singleph + 11,  peersheight);
    peersMinWidth = 180 + 150 + mincheckheight + 50;
    
}

Rectangle<int> PeersContainerView::getMinimumContentBounds() const
{
    return Rectangle<int>(0,0,peersMinWidth, peersMinHeight);
}


void PeersContainerView::updatePeerViews()
{
    for (int i=0; i < mPeerViews.size(); ++i) {
        PeerViewInfo * pvf = mPeerViews.getUnchecked(i);
        String hostname;
        int port = 0;
        processor.getRemotePeerAddressInfo(i, hostname, port);
        bool connected = processor.getRemotePeerConnected(i);

        pvf->nameLabel->setText(String::formatted("%s : %d", hostname.toRawUTF8(), port), dontSendNotification);

        String sendtext;
        bool sendactive = processor.getRemotePeerSendActive(i);
        bool recvactive = processor.getRemotePeerRecvActive(i);
        if (sendactive) {
            sendtext += String::formatted("%Ld sent", processor.getRemotePeerPacketsSent(i) );
        }
        else {
            sendtext += TRANS("send off");
        }
        
        String recvtext;
        
        if (recvactive) {
            recvtext += String::formatted("%Ld recvd", processor.getRemotePeerPacketsReceived(i));
        } else {
            recvtext += TRANS("recv off");            
        }

        pvf->sendActualBitrateLabel->setText(sendtext, dontSendNotification);
        pvf->recvActualBitrateLabel->setText(recvtext, dontSendNotification);

        pvf->pingLabel->setText(String::formatted("%d ms", (int)processor.getRemotePeerPingMs(i) ), dontSendNotification);
        pvf->latencyLabel->setText(String::formatted("%d ms", (int)processor.getRemotePeerTotalLatencyMs(i) ), dontSendNotification);

        
        //pvf->statusLabel->setText(statustext, dontSendNotification);
        
        pvf->sendActiveButton->setToggleState(processor.getRemotePeerSendAllow(i) && connected, dontSendNotification);
        pvf->recvActiveButton->setToggleState(processor.getRemotePeerRecvAllow(i) && connected, dontSendNotification);

        if (!pvf->levelSlider->isMouseOverOrDragging()) {
            pvf->levelSlider->setValue(processor.getRemotePeerLevelGain(i), dontSendNotification);
        }
        if (!pvf->bufferTimeSlider->isMouseOverOrDragging()) {
            pvf->bufferTimeSlider->setValue(processor.getRemotePeerBufferTime(i), dontSendNotification);
        }

        
        int formatindex = processor.getRemotePeerAudioCodecFormat(i);
        pvf->formatChoiceButton->setSelectedItemIndex(formatindex >= 0 ? formatindex : processor.getDefaultAudioCodecFormat());
        
        pvf->recvMeter->setMeterSource (processor.getRemotePeerRecvMeterSource(i));        
        //pvf->sendMeter->setMeterSource (processor.getRemotePeerSendMeterSource(i));

        
        //pvf->packetsizeSlider->setValue(findHighestSetBit(processor.getRemotePeerSendPacketsize(i)), dontSendNotification);
        
        //pvf->removeButton->setEnabled(connected);
        pvf->levelSlider->setEnabled(connected);
        pvf->bufferTimeSlider->setEnabled(connected);

        pvf->formatChoiceButton->setAlpha(connected ? 1.0 : 0.8);
        pvf->nameLabel->setAlpha(connected ? 1.0 : 0.8);
        pvf->sendActiveButton->setAlpha(connected ? 1.0 : 0.8);
        pvf->recvActiveButton->setAlpha(connected ? 1.0 : 0.8);
        pvf->levelLabel->setAlpha(connected ? 1.0 : 0.6);
        pvf->statusLabel->setAlpha(connected ? 1.0 : 0.6);
        pvf->levelSlider->setAlpha(connected ? 1.0 : 0.6);
        pvf->bufferTimeLabel->setAlpha(connected ? 1.0 : 0.6);
        pvf->bufferTimeSlider->setAlpha(connected ? 1.0 : 0.6);
    }
}

void PeersContainerView::choiceButtonSelected(SonoChoiceButton *comp, int index, int ident)
{
    for (int i=0; i < mPeerViews.size(); ++i) {
        PeerViewInfo * pvf = mPeerViews.getUnchecked(i);
        if (pvf->formatChoiceButton.get() == comp) {
            processor.setRemotePeerAudioCodecFormat(i, index);
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
        
        if (pvf->sendActiveButton.get() == buttonThatWasClicked) {
            if (!connected) {
                String hostname;
                int port = 0;
                processor.getRemotePeerAddressInfo(i, hostname, port);
                processor.connectRemotePeer(hostname, port);
            } else {
                // turns on allow and starts sending
                if (pvf->sendActiveButton->getToggleState()) {
                    processor.setRemotePeerSendActive(i, true);
                } else {
                    // turns off sending and allow
                    processor.setRemotePeerSendAllow(i, false);
                }
            }
            break;
        }
        else if (pvf->recvActiveButton.get() == buttonThatWasClicked) {
            if (!connected) {
                String hostname;
                int port = 0;
                processor.getRemotePeerAddressInfo(i, hostname, port);
                processor.connectRemotePeer(hostname, port);
            } else {
                if (pvf->recvActiveButton->getToggleState()) {
                    // allows receiving and invites
                    processor.setRemotePeerRecvActive(i, true);             
                } else {
                    // turns off receiving and allow
                    processor.setRemotePeerRecvAllow(i, false);             
                }
            }
            break;
        }
        
        else if (pvf->menuButton.get() == buttonThatWasClicked) {
            if (!connected) {
                // remove it
                processor.removeRemotePeer(i);
            }
            else {
                processor.disconnectRemotePeer(i);                    
            }
            break;
        }
         
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

   }

}
