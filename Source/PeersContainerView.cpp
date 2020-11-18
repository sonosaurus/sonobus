// SPDX-License-Identifier: GPLv3-or-later
// Copyright (C) 2020 Jesse Chappell

#include "PeersContainerView.h"
#include "JitterBufferMeter.h"

PeerViewInfo::PeerViewInfo() : smallLnf(12), medLnf(14), sonoSliderLNF(12), panSliderLNF(12)
{
    bgColor = Colour::fromFloatRGBA(0.112f, 0.112f, 0.112f, 1.0f);
    borderColor = Colour::fromFloatRGBA(0.5f, 0.5f, 0.5f, 0.3f);

    sonoSliderLNF.textJustification = Justification::centredLeft;
    panSliderLNF.textJustification = Justification::centredLeft;
    
    //Random rcol;
    //itemColor = Colour::fromHSV(rcol.nextFloat(), 0.5f, 0.2f, 1.0f);
}

PeerViewInfo::~PeerViewInfo()
{
    
}

enum {
    LabelTypeRegular = 0,
    LabelTypeSmallDim ,
    LabelTypeSmall
};

enum {
    FillRatioUpdateTimerId = 0
};

void PeerViewInfo::paint(Graphics& g) 
{
    //g.fillAll (Colour(0xff111111));
    //g.fillAll (Colour(0xff202020));
    
    g.setColour(bgColor);
    g.fillRoundedRectangle(getLocalBounds().toFloat(), 6.0f);

    g.setColour(borderColor);
    g.drawRoundedRectangle(getLocalBounds().toFloat(), 6.0f, 0.5f);

}

void PeerViewInfo::resized()
{
    
    mainbox.performLayout(getLocalBounds());
    
    //if (levelLabel) {
    //    levelLabel->setBounds(levelSlider->getBounds().removeFromLeft(levelSlider->getWidth()*0.5));
    //}
    if (panLabel) {
        panLabel->setBounds(panSlider1->getBounds().removeFromTop(12).translated(0, 0));
    }
    
    if (latActiveButton) {
        //latActiveButton->setBounds(staticPingLabel->getX(), staticPingLabel->getY(), pingLabel->getRight() - staticPingLabel->getX(), latencyLabel->getBottom() - pingLabel->getY());
        latActiveButton->setBounds(staticLatencyLabel->getX(), staticLatencyLabel->getY(), pingLabel->getRight() - staticLatencyLabel->getX(), latencyLabel->getBottom() - staticLatencyLabel->getY());
    }

    int triwidth = 10;
    int triheight = 6;
    
    if (recvOptionsButton) {
        auto leftedge = (sendActualBitrateLabel->getRight() + (recvActualBitrateLabel->getX() - sendActualBitrateLabel->getRight()) / 2) + 2;
        recvOptionsButton->setBounds(leftedge, staticBufferLabel->getY(), recvActualBitrateLabel->getRight() - leftedge, recvActualBitrateLabel->getBottom() - staticBufferLabel->getY());

        if (recvOptionsButton->getWidth() > 280) {
            int buttw = recvOptionsButton->getHeight() - 4;
            bufferMinFrontButton->setBounds(recvOptionsButton->getRight() - buttw - 2, recvOptionsButton->getY() + 2, buttw, buttw);
            bufferMinFrontButton->setVisible(true);
        } else {
            bufferMinFrontButton->setVisible(false);            
        }
        
        auto rect = Rectangle<int>(recvOptionsButton->getX() +  3, recvOptionsButton->getBottom() - triheight - 1, triwidth, triheight);
        recvButtonImage->setTransformToFit(rect.toFloat(), RectanglePlacement::stretchToFit);
    }

    if (sendOptionsButton) {
        auto rightedge = (sendActualBitrateLabel->getRight() + (recvActualBitrateLabel->getX() - sendActualBitrateLabel->getRight()) / 2) - 3;
        sendOptionsButton->setBounds(staticSendQualLabel->getX(), staticSendQualLabel->getY(), rightedge - staticSendQualLabel->getX(), sendActualBitrateLabel->getBottom() - staticSendQualLabel->getY());

        auto rect = Rectangle<int>(sendOptionsButton->getX() + 3, sendOptionsButton->getBottom() - triheight - 1, triwidth, triheight);        
        sendButtonImage->setTransformToFit(rect.toFloat(), RectanglePlacement::stretchToFit);
    }

    
    if (levelSlider) {
        levelSlider->setMouseDragSensitivity(jmax(128, levelSlider->getWidth()));
    }
    
    //Rectangle<int> optbounds(staticSendQualLabel->getX(), staticSendQualLabel->getY(), sendQualityLabel->getRight() - staticSendQualLabel->getX(), bufferLabel->getBottom() - sendQualityLabel->getY());
    //optionsButton->setBounds(optbounds);
    
    if (jitterBufferMeter) {
        jitterBufferMeter->setBounds(bufferLabel->getBounds().reduced(0, 1));
    }
              
    if (sendStatsBg) {
        auto sendbounds = sendOptionsButton->getBounds();
        sendStatsBg->setRectangle (sendbounds.toFloat().expanded(1.0f));

        auto recvbounds = recvOptionsButton->getBounds();
        recvStatsBg->setRectangle (recvbounds.toFloat().expanded(1.0f));

        auto pingbounds = latActiveButton->getBounds();
        //if (pingbounds.getBottom() < recvbounds.getY()) {
        //    pingbounds.setBottom(recvbounds.getY() + 10);
        //}
        pingBg->setRectangle (pingbounds.toFloat().expanded(1.0f));
    }
    
}

/// --------------------------------------------
#pragma PendingPeerViewInfo


PendingPeerViewInfo::PendingPeerViewInfo()
{
    bgColor = Colour::fromFloatRGBA(0.112f, 0.112f, 0.112f, 1.0f);
    borderColor = Colour::fromFloatRGBA(0.5f, 0.5f, 0.5f, 0.3f);

    //Random rcol;
    //itemColor = Colour::fromHSV(rcol.nextFloat(), 0.5f, 0.2f, 1.0f);
}

PendingPeerViewInfo::~PendingPeerViewInfo()
{
    
}

void PendingPeerViewInfo::paint(Graphics& g) 
{
    //g.fillAll (Colour(0xff111111));
    //g.fillAll (Colour(0xff202020));
    
    g.setColour(bgColor);
    g.fillRoundedRectangle(getLocalBounds().toFloat(), 6.0f);

    g.setColour(borderColor);
    g.drawRoundedRectangle(getLocalBounds().toFloat(), 6.0f, 0.5f);

}

void PendingPeerViewInfo::resized()
{
    
    mainbox.performLayout(getLocalBounds());
}


PeersContainerView::PeersContainerView(SonobusAudioProcessor& proc)
 : Component("pcv"),  processor(proc)
{
    mutedTextColor = Colour::fromFloatRGBA(0.8, 0.5, 0.2, 1.0);
    regularTextColor = Colour(0xa0eeeeee);; //Colour(0xc0eeeeee);
    dimTextColor = Colour(0xa0aaaaaa); //Colour(0xc0aaaaaa);
    //soloColor = Colour::fromFloatRGBA(0.2, 0.5, 0.8, 1.0);
    mutedColor = Colour::fromFloatRGBA(0.6, 0.3, 0.1, 1.0);
    soloColor = Colour::fromFloatRGBA(1.0, 1.0, 0.6, 1.0);
    mutedBySoloColor = Colour::fromFloatRGBA(0.25, 0.125, 0.0, 1.0);
    
    droppedTextColor = Colour(0xc0ee8888);

    outlineColor = Colour::fromFloatRGBA(0.25, 0.25, 0.25, 1.0);
    bgColor = Colours::black;
    
    rebuildPeerViews();
}

void PeersContainerView::configLevelSlider(Slider * slider)
{
    //slider->setTextValueSuffix(" dB");
    slider->setColour(Slider::textBoxBackgroundColourId, Colours::transparentBlack);
    slider->setColour(Slider::textBoxOutlineColourId, Colours::transparentBlack);
    slider->setColour(Slider::textBoxTextColourId, Colour(0x90eeeeee));
    slider->setColour(TooltipWindow::textColourId, Colour(0xf0eeeeee));

    
    slider->setTextBoxStyle(Slider::TextBoxAbove, true, 80, 12);

    slider->setRange(0.0, 2.0, 0.0);
    slider->setSkewFactor(0.25);
    slider->setDoubleClickReturnValue(true, 1.0);
    slider->setTextBoxIsEditable(true);
    slider->setSliderSnapsToMousePosition(false);
    slider->setScrollWheelEnabled(false);
    slider->valueFromTextFunction = [](const String& s) -> float { return Decibels::decibelsToGain(s.getFloatValue()); };
    slider->textFromValueFunction = [](float v) -> String { return String(TRANS("Level: ")) + Decibels::toString(Decibels::gainToDecibels(v), 1); };
#if JUCE_IOS
    //slider->setPopupDisplayEnabled(true, false, this);
#endif
}

void PeersContainerView::configKnobSlider(Slider * slider)
{
    slider->setSliderStyle(Slider::SliderStyle::RotaryHorizontalVerticalDrag);
    slider->setTextBoxStyle(Slider::TextBoxAbove, true, 60, 14);
    slider->setMouseDragSensitivity(128);
    slider->setScrollWheelEnabled(false);
    slider->setTextBoxIsEditable(true);
    slider->setSliderSnapsToMousePosition(false);
    //slider->setPopupDisplayEnabled(true, false, this);
    slider->setColour(Slider::textBoxBackgroundColourId, Colours::transparentBlack);
    slider->setColour(Slider::textBoxOutlineColourId, Colours::transparentBlack);
    slider->setColour(Slider::textBoxTextColourId, Colour(0x90eeeeee));
    slider->setColour(TooltipWindow::textColourId, Colour(0xf0eeeeee));

    //slider->setLookAndFeel(&sonoSliderLNF);
    
    //slider->valueFromTextFunction = [](const String& s) -> float { return Decibels::decibelsToGain(s.getFloatValue()); };
    //slider->textFromValueFunction = [](float v) -> String { return Decibels::toString(Decibels::gainToDecibels(v), 1); };
#if JUCE_IOS
    //slider->setPopupDisplayEnabled(true, false, this);
#endif
}

void PeersContainerView::configLabel(Label *label, int ltype)
{
    if (ltype == LabelTypeSmallDim) {
        label->setFont(12);
        label->setColour(Label::textColourId, dimTextColor);
        label->setJustificationType(Justification::centredRight);        
        label->setMinimumHorizontalScale(0.3);
    }
    else if (ltype == LabelTypeSmall) {
        label->setFont(12);
        label->setColour(Label::textColourId, regularTextColor);
        label->setJustificationType(Justification::centredRight);
        label->setMinimumHorizontalScale(0.3);
    }
    else {
        label->setFont(14);
        //label->setColour(Label::textColourId, Colour(0xaaeeeeee));
        label->setJustificationType(Justification::centredLeft);
    }
}





void PeersContainerView::resized()
{
    Rectangle<int> bounds = getLocalBounds().reduced(5, 0);
    bounds.removeFromLeft(3);
    peersBox.performLayout(bounds);
    
    for (int i=0; i < mPeerViews.size(); ++i) {
        PeerViewInfo * pvf = mPeerViews.getUnchecked(i);
        pvf->resized();
    }
    
    Component* dw = nullptr; // this->findParentComponentOfClass<DocumentWindow>();    
    if (!dw)
        dw = this->findParentComponentOfClass<AudioProcessorEditor>();
    if (!dw)
        dw = this->findParentComponentOfClass<Component>();
    if (!dw)
        dw = this;

    if (auto * callout = dynamic_cast<CallOutBox*>(pannerCalloutBox.get())) {
        callout->dismiss();
        pannerCalloutBox = nullptr;
    }
    if (auto * callout = dynamic_cast<CallOutBox*>(sendOptionsCalloutBox.get())) {
        callout->dismiss();
        sendOptionsCalloutBox = nullptr;
    }
    if (auto * callout = dynamic_cast<CallOutBox*>(recvOptionsCalloutBox.get())) {
        callout->dismiss();
        recvOptionsCalloutBox = nullptr;
    }
    if (auto * callout = dynamic_cast<CallOutBox*>(effectsCalloutBox.get())) {
        callout->dismiss();
        effectsCalloutBox = nullptr;
    }


}

void PeersContainerView::showPopTip(const String & message, int timeoutMs, Component * target, int maxwidth)
{
    popTip.reset(new BubbleMessageComponent());
    popTip->setAllowedPlacement(BubbleComponent::above);
    
    if (target) {
        if (auto * parent = target->findParentComponentOfClass<AudioProcessorEditor>()) {
            parent->addChildComponent (popTip.get());
        } else {
            addChildComponent(popTip.get());            
        }
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
    popTip->toFront(false);
}

void PeersContainerView::paint(Graphics & g)
{    
    //g.fillAll (Colours::black);
    Rectangle<int> bounds = getLocalBounds();

    bounds.reduce(1, 1);
    bounds.removeFromLeft(3);
    
    g.setColour(bgColor);
    g.fillRoundedRectangle(bounds.toFloat(), 6.0f);
    g.setColour(outlineColor);
    g.drawRoundedRectangle(bounds.toFloat(), 6.0f, 0.5f);

}

PeerViewInfo * PeersContainerView::createPeerViewInfo()
{
    PeerViewInfo * pvf = new PeerViewInfo();

    pvf->nameLabel = std::make_unique<Label>("name", "");
    pvf->nameLabel->setJustificationType(Justification::centredLeft);
    pvf->nameLabel->setFont(16);

    pvf->addrLabel = std::make_unique<Label>("name", "");
    pvf->addrLabel->setJustificationType(Justification::centredLeft);
    configLabel(pvf->addrLabel.get(), LabelTypeSmallDim);
    pvf->addrLabel->setFont(13);

    pvf->staticAddrLabel = std::make_unique<Label>("addr", TRANS("Remote address:"));
    pvf->staticAddrLabel->setJustificationType(Justification::centredRight);
    configLabel(pvf->staticAddrLabel.get(), LabelTypeSmallDim);
    pvf->staticAddrLabel->setFont(13);
    
    
    pvf->sendMutedButton = std::make_unique<ToggleButton>(TRANS("Disable Sending"));
    pvf->sendMutedButton->addListener(this);

    pvf->recvMutedButton = std::make_unique<TextButton>(TRANS("MUTE"));
    pvf->recvMutedButton->addListener(this);
    pvf->recvMutedButton->setLookAndFeel(&pvf->medLnf);
    pvf->recvMutedButton->setClickingTogglesState(true);
    pvf->recvMutedButton->setColour(TextButton::buttonOnColourId, mutedColor);
    pvf->recvMutedButton->setTooltip(TRANS("Toggles receive muting, preventing audio from being heard for this user"));

    pvf->recvSoloButton = std::make_unique<TextButton>(TRANS("SOLO"));
    pvf->recvSoloButton->addListener(this);
    pvf->recvSoloButton->setLookAndFeel(&pvf->medLnf);
    pvf->recvSoloButton->setClickingTogglesState(true);
    pvf->recvSoloButton->setColour(TextButton::buttonOnColourId, soloColor.withAlpha(0.7f));
    pvf->recvSoloButton->setColour(TextButton::textColourOnId, Colours::darkblue);
    pvf->recvSoloButton->setTooltip(TRANS("Listen to only this user, and other soloed users. Alt-click to exclusively solo this user."));

    
    pvf->latActiveButton = std::make_unique<SonoDrawableButton>("", DrawableButton::ButtonStyle::ImageFitted); // (TRANS("Latency\nTest"));
    pvf->latActiveButton->setColour(SonoTextButton::outlineColourId, Colours::transparentBlack);
    pvf->latActiveButton->setColour(DrawableButton::backgroundOnColourId, Colour::fromFloatRGBA(0.4, 0.2, 0.4, 0.7));
    pvf->latActiveButton->setColour(DrawableButton::backgroundColourId, Colours::transparentBlack);
    pvf->latActiveButton->setClickingTogglesState(true);
    pvf->latActiveButton->setTriggeredOnMouseDown(true);
    pvf->latActiveButton->setLookAndFeel(&pvf->smallLnf);
    pvf->latActiveButton->addListener(this);
    pvf->latActiveButton->addMouseListener(this, false);

    pvf->statusLabel = std::make_unique<Label>("status", "");
    configLabel(pvf->statusLabel.get(), LabelTypeRegular);
    pvf->statusLabel->setJustificationType(Justification::centredLeft);

    
    pvf->levelSlider     = std::make_unique<Slider>(Slider::LinearHorizontal,  Slider::TextBoxRight);
    pvf->levelSlider->setName("level");
    pvf->levelSlider->addListener(this);

    configLevelSlider(pvf->levelSlider.get());
    pvf->levelSlider->setLookAndFeel(&pvf->sonoSliderLNF);


    pvf->panLabel = std::make_unique<Label>("pan", TRANS("Pan"));
    configLabel(pvf->panLabel.get(), LabelTypeSmall);
    pvf->panLabel->setJustificationType(Justification::centredTop);
    
    pvf->pannersContainer = std::make_unique<Component>();
    
    pvf->panSlider1     = std::make_unique<Slider>(Slider::LinearHorizontal,  Slider::NoTextBox);
    pvf->panSlider1->setName("pan1");
    pvf->panSlider1->addListener(this);
    pvf->panSlider1->getProperties().set ("fromCentre", true);
    pvf->panSlider1->getProperties().set ("noFill", true);
    pvf->panSlider1->setRange(-1, 1, 0.0f);
    pvf->panSlider1->setDoubleClickReturnValue(true, -1.0);
    pvf->panSlider1->setTextBoxIsEditable(false);
    pvf->panSlider1->setSliderSnapsToMousePosition(false);
    pvf->panSlider1->setScrollWheelEnabled(false);
    pvf->panSlider1->setMouseDragSensitivity(100);

    pvf->panSlider1->textFromValueFunction =  [](double v) -> String { if (fabs(v) < 0.01) return String(TRANS("Pan: Center")); return String(TRANS("Pan: ")) +  String((int)rint(abs(v*100.0f))) + ((v > 0 ? "% R" : "% L")) ; };
    pvf->panSlider1->valueFromTextFunction =  [](const String& s) -> double { return s.getDoubleValue()*1e-2f; };
    pvf->panSlider1->setValue(0.1, dontSendNotification);
    pvf->panSlider1->setValue(0.0, dontSendNotification);
    pvf->panSlider1->setLookAndFeel(&pvf->panSliderLNF);
    pvf->panSlider1->setPopupDisplayEnabled(true, true, this);

    
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
    pvf->panSlider2->setMouseDragSensitivity(100);

#if JUCE_IOS
    //pvf->panSlider1->setPopupDisplayEnabled(true, false, pvf->pannersContainer.get());
    //pvf->panSlider2->setPopupDisplayEnabled(true, false, pvf->pannersContainer.get());
#endif
    
    pvf->panSlider2->textFromValueFunction =  [](double v) -> String { if ( fabs(v) < 0.01) return TRANS("C"); return String((int)rint(abs(v*100.0f))) + ((v > 0 ? "% R" : "% L")) ; };
    pvf->panSlider2->valueFromTextFunction =  [](const String& s) -> double { return s.getDoubleValue()*1e-2f; };

    pvf->panButton = std::make_unique<TextButton>(TRANS("Pan"));
    pvf->panButton->addListener(this);
    pvf->panButton->setLookAndFeel(&pvf->medLnf);

    pvf->fxButton = std::make_unique<TextButton>(TRANS("FX"));
    pvf->fxButton->setColour(TextButton::buttonOnColourId, Colour::fromFloatRGBA(0.2, 0.5, 0.7, 0.5));
    pvf->fxButton->addListener(this);
    pvf->fxButton->setLookAndFeel(&pvf->medLnf);
  
    
    pvf->bufferTimeSlider     = std::make_unique<Slider>(Slider::LinearBar,  Slider::TextBoxBelow);
    pvf->bufferTimeSlider->setName("buffer");
    pvf->bufferTimeSlider->setRange(0, 1000, 1);
    pvf->bufferTimeSlider->setTextValueSuffix(" ms");
    pvf->bufferTimeSlider->getProperties().set ("noFill", true);
    pvf->bufferTimeSlider->setSkewFactor(0.4);
    pvf->bufferTimeSlider->setDoubleClickReturnValue(true, 20.0);
    pvf->bufferTimeSlider->setTextBoxIsEditable(false);
    pvf->bufferTimeSlider->setSliderSnapsToMousePosition(false);
    pvf->bufferTimeSlider->setChangeNotificationOnlyOnRelease(true);
    pvf->bufferTimeSlider->setScrollWheelEnabled(false);
    pvf->bufferTimeSlider->setPopupDisplayEnabled(true, false, this);

    pvf->bufferTimeSlider->addListener(this);

    pvf->autosizeButton = std::make_unique<SonoChoiceButton>();
    pvf->autosizeButton->addChoiceListener(this);
    pvf->autosizeButton->addItem(TRANS("Manual"), SonobusAudioProcessor::AutoNetBufferModeOff);
    pvf->autosizeButton->addItem(TRANS("Auto Up"), SonobusAudioProcessor::AutoNetBufferModeAutoIncreaseOnly);
    pvf->autosizeButton->addItem(TRANS("Auto"), SonobusAudioProcessor::AutoNetBufferModeAutoFull);
    pvf->autosizeButton->addItem(TRANS("Initial Auto"), SonobusAudioProcessor::AutoNetBufferModeInitAuto);    
    pvf->autosizeButton->addListener(this);
    
    pvf->bufferMinButton = std::make_unique<SonoDrawableButton>("", DrawableButton::ButtonStyle::ImageFitted);
    std::unique_ptr<Drawable> backimg(Drawable::createFromImageData(BinaryData::reset_buffer_icon_svg, BinaryData::reset_buffer_icon_svgSize));
    pvf->bufferMinButton->setImages(backimg.get());
    pvf->bufferMinButton->addListener(this);
    pvf->bufferMinButton->setTooltip(TRANS("Resets safety buffer to the minimum. Hold Alt key to reset for all (with auto)."));
    pvf->bufferMinButton->setAlpha(0.8f);

    pvf->bufferMinFrontButton = std::make_unique<SonoDrawableButton>("", DrawableButton::ButtonStyle::ImageFitted);
    pvf->bufferMinFrontButton->setImages(backimg.get());
    pvf->bufferMinFrontButton->addListener(this);
    pvf->bufferMinFrontButton->setTooltip(TRANS("Resets safety buffer to the minimum. Hold Alt key to reset for all (with auto)."));
    pvf->bufferMinFrontButton->setAlpha(0.8f);
    
    pvf->recvButtonImage = Drawable::createFromImageData(BinaryData::triangle_disclosure_svg, BinaryData::triangle_disclosure_svgSize);
    pvf->recvButtonImage->setInterceptsMouseClicks(false, false);
    pvf->recvButtonImage->setAlpha(0.7f);
    
    
    pvf->sendButtonImage = Drawable::createFromImageData(BinaryData::triangle_disclosure_svg, BinaryData::triangle_disclosure_svgSize);
    pvf->sendButtonImage->setInterceptsMouseClicks(false, false);
    pvf->sendButtonImage->setAlpha(0.7f);

    
    pvf->bufferTimeLabel = std::make_unique<Label>("level", TRANS("Safety Buffer"));
    configLabel(pvf->bufferTimeLabel.get(), LabelTypeRegular);

    pvf->recvOptionsButton = std::make_unique<SonoDrawableButton>("menu", DrawableButton::ImageFitted);
    pvf->recvOptionsButton->addListener(this);
    pvf->recvOptionsButton->setColour(SonoTextButton::outlineColourId, Colours::transparentBlack);
    pvf->recvOptionsButton->setColour(DrawableButton::backgroundOnColourId, Colour::fromFloatRGBA(0.4, 0.2, 0.4, 0.7));
    pvf->recvOptionsButton->setColour(DrawableButton::backgroundColourId, Colours::transparentBlack);

    
    pvf->sendOptionsButton = std::make_unique<SonoDrawableButton>("settings",  DrawableButton::ButtonStyle::ImageFitted);
    pvf->sendOptionsButton->addListener(this);
    pvf->sendOptionsButton->setColour(SonoTextButton::outlineColourId, Colours::transparentBlack);
    pvf->sendOptionsButton->setColour(DrawableButton::backgroundOnColourId, Colour::fromFloatRGBA(0.4, 0.2, 0.4, 0.7));
    pvf->sendOptionsButton->setColour(DrawableButton::backgroundColourId, Colours::transparentBlack);


    pvf->changeAllFormatButton = std::make_unique<ToggleButton>(TRANS("Change for all"));
    pvf->changeAllFormatButton->addListener(this);
    pvf->changeAllFormatButton->setLookAndFeel(&pvf->smallLnf);

    
    pvf->formatChoiceButton = std::make_unique<SonoChoiceButton>();
    pvf->formatChoiceButton->addChoiceListener(this);
    int numformats = processor.getNumberAudioCodecFormats();
    for (int i=0; i < numformats; ++i) {
        pvf->formatChoiceButton->addItem(processor.getAudioCodeFormatName(i), i);
    }

    pvf->staticFormatChoiceLabel = std::make_unique<Label>("fmt", TRANS("Send Quality"));
    configLabel(pvf->staticFormatChoiceLabel.get(), LabelTypeRegular);


    pvf->remoteSendFormatChoiceButton = std::make_unique<SonoChoiceButton>();
    pvf->remoteSendFormatChoiceButton->addChoiceListener(this);
    pvf->remoteSendFormatChoiceButton->addItem(TRANS("No Preference"), -1);
    for (int i=0; i < numformats; ++i) {
        pvf->remoteSendFormatChoiceButton->addItem(processor.getAudioCodeFormatName(i), i);
    }

    pvf->staticRemoteSendFormatChoiceLabel = std::make_unique<Label>("fmt", TRANS("Preferred Recv Quality"));
    configLabel(pvf->staticRemoteSendFormatChoiceLabel.get(), LabelTypeRegular);
    
    pvf->staticLatencyLabel = std::make_unique<Label>("lat", TRANS("Latency (ms)"));
    configLabel(pvf->staticLatencyLabel.get(), LabelTypeSmallDim);
    pvf->staticLatencyLabel->setJustificationType(Justification::centred);
    
    pvf->staticPingLabel = std::make_unique<Label>("ping", TRANS("Ping"));
    configLabel(pvf->staticPingLabel.get(), LabelTypeSmallDim);

    pvf->latencyLabel = std::make_unique<Label>("lat", TRANS("PRESS"));
    configLabel(pvf->latencyLabel.get(), LabelTypeSmall);
    pvf->latencyLabel->setJustificationType(Justification::centred);

    pvf->pingLabel = std::make_unique<Label>("ping");
    configLabel(pvf->pingLabel.get(), LabelTypeSmall);

    pvf->staticSendQualLabel = std::make_unique<Label>("lat", TRANS("Send Quality:"));
    configLabel(pvf->staticSendQualLabel.get(), LabelTypeSmallDim);
    pvf->staticBufferLabel = std::make_unique<Label>("ping", TRANS("Recv Safety Buffer:"));
    configLabel(pvf->staticBufferLabel.get(), LabelTypeSmallDim);
    
    pvf->sendQualityLabel = std::make_unique<Label>("qual", "");
    configLabel(pvf->sendQualityLabel.get(), LabelTypeSmall);
    pvf->sendQualityLabel->setJustificationType(Justification::centredLeft);

    pvf->bufferLabel = std::make_unique<Label>("buf");
    configLabel(pvf->bufferLabel.get(), LabelTypeSmall);
    pvf->bufferLabel->setJustificationType(Justification::centredLeft);

    
    pvf->sendActualBitrateLabel = std::make_unique<Label>("sbit");
    configLabel(pvf->sendActualBitrateLabel.get(), LabelTypeSmall);
    pvf->sendActualBitrateLabel->setJustificationType(Justification::centred);
    pvf->sendActualBitrateLabel->setMinimumHorizontalScale(0.75);

    pvf->recvActualBitrateLabel = std::make_unique<Label>("rbit");
    configLabel(pvf->recvActualBitrateLabel.get(), LabelTypeSmall);
    pvf->recvActualBitrateLabel->setJustificationType(Justification::centred);
    pvf->recvActualBitrateLabel->setMinimumHorizontalScale(0.75);
    
    
    pvf->jitterBufferMeter = std::make_unique<JitterBufferMeter>();
    
    pvf->optionsResetDropButton = std::make_unique<TextButton>(TRANS("Reset Dropped"));
    pvf->optionsResetDropButton->addListener(this);
    pvf->optionsResetDropButton->setLookAndFeel(&pvf->medLnf);

    pvf->optionsRemoveButton = std::make_unique<TextButton>(TRANS("Remove"));
    pvf->optionsRemoveButton->addListener(this);
    pvf->optionsRemoveButton->setLookAndFeel(&pvf->smallLnf);
    pvf->optionsRemoveButton->setTooltip(TRANS("Removes user from your own connections, does not affect the whole group"));

    
    // meters
    auto flags = foleys::LevelMeter::Minimal;
    
    pvf->recvMeter = std::make_unique<foleys::LevelMeter>(flags);
    pvf->recvMeter->setLookAndFeel(&(pvf->rmeterLnf));
    pvf->recvMeter->setRefreshRateHz(10);
    pvf->recvMeter->addMouseListener(this, false);

    
    // effects
    pvf->effectsContainer = std::make_unique<Component>();

    // compressor stuff
   
    pvf->compressorView = std::make_unique<CompressorView>();
    pvf->compressorView->setDragButtonVisible(false);
    pvf->compressorView->addListener(this);
    pvf->compressorView->addHeaderListener(this);

    pvf->sendOptionsContainer = std::make_unique<Component>();
    pvf->recvOptionsContainer = std::make_unique<Component>();

    Colour statsbgcol = Colour::fromFloatRGBA(0.07, 0.07, 0.07, 1.0); // 0.08
    Colour statsbordcol = Colour::fromFloatRGBA(0.5, 0.5, 0.5, 0.0); // 0.5 alpha 0.25
    pvf->sendStatsBg = std::make_unique<DrawableRectangle>();
    pvf->sendStatsBg->setCornerSize(Point<float>(6,6));
    pvf->sendStatsBg->setFill (statsbgcol);
    pvf->sendStatsBg->setStrokeFill (statsbordcol);
    pvf->sendStatsBg->setStrokeThickness(0.5);

    pvf->recvStatsBg = std::make_unique<DrawableRectangle>();
    pvf->recvStatsBg->setCornerSize(Point<float>(6,6));
    pvf->recvStatsBg->setFill (statsbgcol);
    pvf->recvStatsBg->setStrokeFill (statsbordcol);
    pvf->recvStatsBg->setStrokeThickness(0.5);

    
    pvf->pingBg = std::make_unique<DrawableRectangle>();
    pvf->pingBg->setCornerSize(Point<float>(6,6));
    pvf->pingBg->setFill (statsbgcol);
    pvf->pingBg->setStrokeFill (statsbordcol);
    pvf->pingBg->setStrokeThickness(0.5);
    
    return pvf;
}
    
PendingPeerViewInfo * PeersContainerView::createPendingPeerViewInfo()
{
    auto * pvf = new PendingPeerViewInfo();

    pvf->nameLabel = std::make_unique<Label>("name", "");
    pvf->nameLabel->setJustificationType(Justification::centredLeft);
    pvf->nameLabel->setFont(16);

    pvf->messageLabel = std::make_unique<Label>("msg", "");
    pvf->messageLabel->setJustificationType(Justification::centredLeft);
    pvf->messageLabel->setFont(13);
    pvf->messageLabel->setMinimumHorizontalScale(0.8);

    return pvf;
}

void PeersContainerView::resetPendingUsers()
{
    mPendingUsers.clear();
}

void PeersContainerView::peerPendingJoin(String & group, String & user)
{
    mPendingUsers[user] = PendingUserInfo(group, user);
    rebuildPeerViews();    
}

void PeersContainerView::peerFailedJoin(String & group, String & user)
{
    auto found = mPendingUsers.find(user);
    if (found != mPendingUsers.end()) {
        found->second.failed = true;
        updatePeerViews();
    }    
}


void PeersContainerView::rebuildPeerViews()
{
    int numpeers = processor.getNumberRemotePeers();
    
    showSendOptions(0, false);
    showRecvOptions(0, false);
    
    while (mPeerViews.size() < numpeers) {
        mPeerViews.add(createPeerViewInfo());        
    }
    while (mPeerViews.size() > numpeers) {
        mPeerViews.removeLast();
    }
    
    for (int i=0; i < mPeerViews.size(); ++i) {
        PeerViewInfo * pvf = mPeerViews.getUnchecked(i);
        String username = processor.getRemotePeerUserName(i);
        // remove from pending if necessary
        mPendingUsers.erase(username);
        
        pvf->addAndMakeVisible(pvf->sendStatsBg.get());
        pvf->addAndMakeVisible(pvf->recvStatsBg.get());
        pvf->addAndMakeVisible(pvf->pingBg.get());
        //pvf->addAndMakeVisible(pvf->sendMutedButton.get());
        pvf->addAndMakeVisible(pvf->recvMutedButton.get());
        pvf->addAndMakeVisible(pvf->recvSoloButton.get());
        pvf->addAndMakeVisible(pvf->latActiveButton.get());
        pvf->addAndMakeVisible(pvf->levelSlider.get());
        //pvf->addAndMakeVisible(pvf->levelLabel.get());
        pvf->addAndMakeVisible(pvf->panLabel.get());
        pvf->addAndMakeVisible(pvf->sendOptionsButton.get());
        pvf->addAndMakeVisible(pvf->recvOptionsButton.get());
        pvf->addAndMakeVisible(pvf->staticLatencyLabel.get());
        pvf->addAndMakeVisible(pvf->staticPingLabel.get());
        pvf->addAndMakeVisible(pvf->latencyLabel.get());
        pvf->addAndMakeVisible(pvf->pingLabel.get());
        pvf->addAndMakeVisible(pvf->nameLabel.get());
        pvf->addAndMakeVisible(pvf->statusLabel.get());
        pvf->addAndMakeVisible(pvf->jitterBufferMeter.get());
        pvf->addAndMakeVisible(pvf->staticSendQualLabel.get());
        pvf->addAndMakeVisible(pvf->staticBufferLabel.get());
        pvf->addAndMakeVisible(pvf->sendQualityLabel.get());
        pvf->addAndMakeVisible(pvf->bufferLabel.get());
        pvf->addAndMakeVisible(pvf->bufferMinFrontButton.get());
        pvf->addAndMakeVisible(pvf->recvButtonImage.get());
        pvf->addAndMakeVisible(pvf->sendButtonImage.get());

        pvf->recvOptionsContainer->addAndMakeVisible(pvf->addrLabel.get());
        pvf->recvOptionsContainer->addAndMakeVisible(pvf->staticAddrLabel.get());
        pvf->recvOptionsContainer->addAndMakeVisible(pvf->autosizeButton.get());
        pvf->recvOptionsContainer->addAndMakeVisible(pvf->bufferTimeSlider.get());
        pvf->recvOptionsContainer->addAndMakeVisible(pvf->bufferMinButton.get());
        pvf->recvOptionsContainer->addAndMakeVisible(pvf->bufferTimeLabel.get());
        pvf->recvOptionsContainer->addAndMakeVisible(pvf->optionsResetDropButton.get());
        pvf->recvOptionsContainer->addAndMakeVisible(pvf->remoteSendFormatChoiceButton.get());
        pvf->recvOptionsContainer->addAndMakeVisible(pvf->staticRemoteSendFormatChoiceLabel.get());

        pvf->sendOptionsContainer->addAndMakeVisible(pvf->formatChoiceButton.get());
        pvf->sendOptionsContainer->addAndMakeVisible(pvf->changeAllFormatButton.get());
        pvf->sendOptionsContainer->addAndMakeVisible(pvf->staticFormatChoiceLabel.get());
        pvf->sendOptionsContainer->addAndMakeVisible(pvf->sendMutedButton.get());
        pvf->sendOptionsContainer->addAndMakeVisible(pvf->optionsRemoveButton.get());
        
        pvf->addAndMakeVisible(pvf->recvMeter.get());
        pvf->addAndMakeVisible(pvf->sendActualBitrateLabel.get());
        pvf->addAndMakeVisible(pvf->recvActualBitrateLabel.get());
        pvf->addAndMakeVisible(pvf->panButton.get());
        pvf->addAndMakeVisible(pvf->fxButton.get());

        pvf->addAndMakeVisible(pvf->panSlider1.get());

        pvf->effectsContainer->addAndMakeVisible(pvf->compressorView->getHeaderComponent());
        pvf->effectsContainer->addAndMakeVisible(pvf->compressorView.get());
        
        addAndMakeVisible(pvf);
    }
    
    while (mPendingPeerViews.size() < mPendingUsers.size()) {
        mPendingPeerViews.add(createPendingPeerViewInfo());
    }
    while (mPendingPeerViews.size() > mPendingUsers.size()) {
        mPendingPeerViews.removeLast();
    }

    for (int i=0; i < mPendingPeerViews.size(); ++i) {
        PendingPeerViewInfo * ppvf = mPendingPeerViews.getUnchecked(i);
        ppvf->addAndMakeVisible(ppvf->nameLabel.get());
        ppvf->addAndMakeVisible(ppvf->messageLabel.get());
        addAndMakeVisible(ppvf);
    }
    
    if (mPeerViews.size() > 0) {
        startTimer(FillRatioUpdateTimerId, 100);
    } else {
        stopTimer(FillRatioUpdateTimerId);
    }
    
    updatePeerViews();
    updateLayout();
    resized();
}

void PeersContainerView::updateLayout()
{
    int minitemheight = 36;
    int mincheckheight = 32;
    int minPannerWidth = 58;
    int minButtonWidth = 90;
    int maxPannerWidth = 110;
    
    int mutebuttwidth = 52;
    
#if JUCE_IOS
    // make the button heights a bit more for touchscreen purposes
    minitemheight = 44;
    mincheckheight = 40;
    minPannerWidth = 60;
#endif

    const int textheight = minitemheight / 2;

    bool servconnected = processor.isConnectedToServer();

    peersBox.items.clear();
    peersBox.flexDirection = FlexBox::Direction::column;
    peersBox.justifyContent = FlexBox::JustifyContent::flexStart;
    int peersheight = 0;
    //const int singleph =  minitemheight*3 + 12;
    const int singleph =  minitemheight*2 + 6;
    
    peersBox.items.add(FlexItem(8, 2).withMargin(0));
    peersheight += 2;


    // Pending connection peerviews
    
    int ppheight = minitemheight;
    int ppw = 120;
    
    for (int i=0; i < mPendingPeerViews.size(); ++i) {

        peersBox.items.add(FlexItem(8, 4).withMargin(0));
        peersheight += 4;
        
        PendingPeerViewInfo * ppvf = mPendingPeerViews.getUnchecked(i);
        ppvf->mainbox.items.clear();        
        ppvf->mainbox.flexDirection = FlexBox::Direction::row;
        ppvf->mainbox.items.add(FlexItem(110, minitemheight, *ppvf->nameLabel).withMargin(0).withFlex(0));
        ppvf->mainbox.items.add(FlexItem(110, minitemheight, *ppvf->messageLabel).withMargin(0).withFlex(1));
        
        peersBox.items.add(FlexItem(ppw, ppheight + 5, *ppvf).withMargin(1).withFlex(0));
        peersheight += ppheight + 5;
    }
    
    // Main connected peer views
    
    for (int i=0; i < mPeerViews.size(); ++i) {
        PeerViewInfo * pvf = mPeerViews.getUnchecked(i);

        //pvf->updateLayout();
        
        const int ph = singleph;
        const int pw = 200;
        


        pvf->nameaddrbox.items.clear();
        pvf->nameaddrbox.flexDirection = FlexBox::Direction::column;
        pvf->nameaddrbox.items.add(FlexItem(100, minitemheight, *pvf->nameLabel).withMargin(0).withFlex(1));

        pvf->namebox.items.clear();
        pvf->namebox.flexDirection = FlexBox::Direction::row;
//.withAlignSelf(FlexItem::AlignSelf::center));
        pvf->namebox.items.add(FlexItem(100, minitemheight, pvf->nameaddrbox).withMargin(2).withFlex(1));

        
       
        
        pvf->sendmeterbox.items.clear();
        pvf->sendmeterbox.flexDirection = FlexBox::Direction::row;

      
#if 0        
        pvf->pingbox.items.clear();
        pvf->pingbox.flexDirection = FlexBox::Direction::row;
        pvf->pingbox.items.add(FlexItem(40, textheight, *pvf->staticPingLabel).withMargin(0).withFlex(0.5));
        pvf->pingbox.items.add(FlexItem(20, textheight, *pvf->pingLabel).withMargin(0).withFlex(0.5));

        pvf->latencybox.items.clear();
        pvf->latencybox.flexDirection = FlexBox::Direction::row;
        pvf->latencybox.items.add(FlexItem(40, textheight, *pvf->staticLatencyLabel).withMargin(0).withFlex(0.5));
        pvf->latencybox.items.add(FlexItem(20, textheight, *pvf->latencyLabel).withMargin(0).withFlex(0.5));

        
        pvf->netstatbox.items.clear();
        pvf->netstatbox.flexDirection = FlexBox::Direction::column;
        pvf->netstatbox.items.add(FlexItem(80, textheight, pvf->pingbox).withMargin(0).withFlex(0));
        pvf->netstatbox.items.add(FlexItem(80, textheight, pvf->latencybox).withMargin(0).withFlex(0));
#else
        pvf->pingbox.items.clear();
        pvf->pingbox.flexDirection = FlexBox::Direction::column;
        pvf->pingbox.items.add(FlexItem(20, textheight, *pvf->staticPingLabel).withMargin(0).withFlex(0.5));
        pvf->pingbox.items.add(FlexItem(20, textheight, *pvf->pingLabel).withMargin(0).withFlex(0.5));

        pvf->latencybox.items.clear();
        pvf->latencybox.flexDirection = FlexBox::Direction::column;
        pvf->latencybox.items.add(FlexItem(60, textheight, *pvf->staticLatencyLabel).withMargin(0).withFlex(0.5));
        pvf->latencybox.items.add(FlexItem(60, textheight, *pvf->latencyLabel).withMargin(0).withFlex(0.5));

        
        pvf->netstatbox.items.clear();
        pvf->netstatbox.flexDirection = FlexBox::Direction::row;
        pvf->netstatbox.items.add(FlexItem(60, textheight, pvf->latencybox).withMargin(0).withFlex(3));
        pvf->netstatbox.items.add(FlexItem(20, textheight, pvf->pingbox).withMargin(0).withFlex(1));
#endif
        
        
        pvf->squalbox.items.clear();
        pvf->squalbox.flexDirection = FlexBox::Direction::row;
        pvf->squalbox.items.add(FlexItem(60, textheight, *pvf->staticSendQualLabel).withMargin(0).withFlex(1));
        pvf->squalbox.items.add(FlexItem(40, textheight, *pvf->sendQualityLabel).withMargin(0).withFlex(2));

        pvf->netbufbox.items.clear();
        pvf->netbufbox.flexDirection = FlexBox::Direction::row;
        pvf->netbufbox.items.add(FlexItem(0, 10).withFlex(1));
        pvf->netbufbox.items.add(FlexItem(88, textheight, *pvf->staticBufferLabel).withMargin(0).withFlex(1).withMaxWidth(105));
        pvf->netbufbox.items.add(FlexItem(40, textheight, *pvf->bufferLabel).withMargin(0).withFlex(3).withMaxWidth(120));
        pvf->netbufbox.items.add(FlexItem(0, 10).withFlex(1));

        
        pvf->optionsstatbox.items.clear();
        pvf->optionsstatbox.flexDirection = FlexBox::Direction::column;
        pvf->optionsstatbox.items.add(FlexItem(100, textheight, pvf->squalbox).withMargin(0).withFlex(0));
        pvf->optionsstatbox.items.add(FlexItem(76, textheight, *pvf->sendActualBitrateLabel).withMargin(0).withFlex(0));

        
        pvf->recvstatbox.items.clear();
        pvf->recvstatbox.flexDirection = FlexBox::Direction::column;
        pvf->recvstatbox.items.add(FlexItem(100, textheight, pvf->netbufbox).withMargin(0).withFlex(0));
        pvf->recvstatbox.items.add(FlexItem(100, textheight, *pvf->recvActualBitrateLabel).withMargin(0).withFlex(0));

        
        // options
        pvf->optionsNetbufBox.items.clear();
        pvf->optionsNetbufBox.flexDirection = FlexBox::Direction::row;
        pvf->optionsNetbufBox.items.add(FlexItem(36, minitemheight, *pvf->bufferMinButton).withMargin(0).withFlex(0));
        pvf->optionsNetbufBox.items.add(FlexItem(3, 12));
        pvf->optionsNetbufBox.items.add(FlexItem(minButtonWidth, minitemheight, *pvf->bufferTimeSlider).withMargin(0).withFlex(1));
        pvf->optionsNetbufBox.items.add(FlexItem(4, 12));
        pvf->optionsNetbufBox.items.add(FlexItem(80, minitemheight, *pvf->autosizeButton).withMargin(0).withFlex(0));
        
        pvf->optionsSendQualBox.items.clear();
        pvf->optionsSendQualBox.flexDirection = FlexBox::Direction::row;
        pvf->optionsSendQualBox.items.add(FlexItem(100, minitemheight, *pvf->staticFormatChoiceLabel).withMargin(0).withFlex(0));
        pvf->optionsSendQualBox.items.add(FlexItem(minButtonWidth, minitemheight, *pvf->formatChoiceButton).withMargin(0).withFlex(2));

        pvf->optionsRemoteQualityBox.items.clear();
        pvf->optionsRemoteQualityBox.flexDirection = FlexBox::Direction::row;
        pvf->optionsRemoteQualityBox.items.add(FlexItem(100, minitemheight, *pvf->staticRemoteSendFormatChoiceLabel).withMargin(0).withFlex(0));
        pvf->optionsRemoteQualityBox.items.add(FlexItem(minButtonWidth, minitemheight, *pvf->remoteSendFormatChoiceButton).withMargin(0).withFlex(2));

        
        pvf->optionsSendMutedBox.items.clear();
        pvf->optionsSendMutedBox.flexDirection = FlexBox::Direction::row;
        pvf->optionsSendMutedBox.items.add(FlexItem(5, 12));
        pvf->optionsSendMutedBox.items.add(FlexItem(100, minitemheight, *pvf->sendMutedButton).withMargin(0).withFlex(1));
        pvf->optionsSendMutedBox.items.add(FlexItem(minButtonWidth -20, minitemheight, *pvf->optionsRemoveButton).withMargin(0).withFlex(0));
        
        
        pvf->optionsChangeAllQualBox.items.clear();
        pvf->optionsChangeAllQualBox.flexDirection = FlexBox::Direction::row;
        pvf->optionsChangeAllQualBox.items.add(FlexItem(10, minitemheight-10).withMargin(0).withFlex(1));
        pvf->optionsChangeAllQualBox.items.add(FlexItem(150, minitemheight-10, *pvf->changeAllFormatButton).withMargin(0).withFlex(0));

        
        pvf->optionsbuttbox.items.clear();
        pvf->optionsbuttbox.flexDirection = FlexBox::Direction::row;
        pvf->optionsbuttbox.items.add(FlexItem(12, 12).withFlex(1));
        pvf->optionsbuttbox.items.add(FlexItem(100, minitemheight, *pvf->optionsResetDropButton).withMargin(0).withFlex(0));
        pvf->optionsbuttbox.items.add(FlexItem(12, 12).withFlex(1));
        
        
        pvf->optionsaddrbox.items.clear();
        pvf->optionsaddrbox.flexDirection = FlexBox::Direction::row;
        pvf->optionsaddrbox.items.add(FlexItem(120, 14, *pvf->staticAddrLabel).withMargin(0).withFlex(1));
        pvf->optionsaddrbox.items.add(FlexItem(100, 14, *pvf->addrLabel).withMargin(0).withFlex(2));

        
        pvf->recvOptionsBox.items.clear();
        pvf->recvOptionsBox.flexDirection = FlexBox::Direction::column;
        pvf->recvOptionsBox.items.add(FlexItem(4, 4));
        pvf->recvOptionsBox.items.add(FlexItem(100, 14,  pvf->optionsaddrbox).withMargin(2).withFlex(0));
        pvf->recvOptionsBox.items.add(FlexItem(4, 2));
        pvf->recvOptionsBox.items.add(FlexItem(100, minitemheight,  pvf->optionsNetbufBox).withMargin(2).withFlex(0));
        pvf->recvOptionsBox.items.add(FlexItem(4, 2));
        pvf->recvOptionsBox.items.add(FlexItem(100, minitemheight,  pvf->optionsRemoteQualityBox).withMargin(2).withFlex(0));
        pvf->recvOptionsBox.items.add(FlexItem(4, 8));
        pvf->recvOptionsBox.items.add(FlexItem(100, minitemheight,  pvf->optionsbuttbox).withMargin(2).withFlex(0));


        pvf->sendOptionsBox.items.clear();
        pvf->sendOptionsBox.flexDirection = FlexBox::Direction::column;
        pvf->sendOptionsBox.items.add(FlexItem(4, 4));
        pvf->sendOptionsBox.items.add(FlexItem(100, minitemheight,  pvf->optionsSendQualBox).withMargin(2).withFlex(0));
        pvf->sendOptionsBox.items.add(FlexItem(100, minitemheight-10,  pvf->optionsChangeAllQualBox).withMargin(2).withFlex(0));
        pvf->sendOptionsBox.items.add(FlexItem(4, 4));
        pvf->sendOptionsBox.items.add(FlexItem(100, minitemheight, pvf->optionsSendMutedBox).withMargin(0).withFlex(0));

        pvf->pannerbox.items.clear();
        pvf->pannerbox.flexDirection = FlexBox::Direction::row;
        
        pvf->effectsBox.items.clear();
        pvf->effectsBox.flexDirection = FlexBox::Direction::column;
        pvf->effectsBox.items.add(FlexItem(60, pvf->compressorView->getMinimumHeaderBounds().getHeight(), *pvf->compressorView->getHeaderComponent()).withMargin(0).withFlex(0));
        pvf->effectsBox.items.add(FlexItem(60, pvf->compressorView->getMinimumContentBounds().getHeight(), *pvf->compressorView.get()).withMargin(0).withFlex(1));

        
        pvf->sendbox.items.clear();
        pvf->sendbox.flexDirection = FlexBox::Direction::row;
        pvf->sendbox.items.add(FlexItem(5, 2).withFlex(0));
        if (!isNarrow) {

        } else {
            pvf->sendbox.items.add(FlexItem(3, 2).withFlex(0.5));
            pvf->sendbox.items.add(FlexItem(mutebuttwidth, mincheckheight, *pvf->recvMutedButton).withMargin(0).withFlex(0));
            pvf->sendbox.items.add(FlexItem(5, 2));
            pvf->sendbox.items.add(FlexItem(mutebuttwidth, minitemheight, *pvf->recvSoloButton).withMargin(0).withFlex(0));
            pvf->sendbox.items.add(FlexItem(3, 5).withFlex(0.5));
            pvf->sendbox.items.add(FlexItem(42, minitemheight, *pvf->fxButton).withMargin(0).withFlex(0).withMaxWidth(50));
            pvf->sendbox.items.add(FlexItem(6, 2));            
            pvf->sendbox.items.add(FlexItem(90, minitemheight, pvf->netstatbox).withMargin(0).withFlex(1).withMaxWidth(130));
            pvf->sendbox.items.add(FlexItem(3, 5));
        }
        
        pvf->recvbox.items.clear();
        pvf->recvbox.flexDirection = FlexBox::Direction::row;
        pvf->recvbox.items.add(FlexItem(1, 5));
        pvf->recvbox.items.add(FlexItem(60, minitemheight, pvf->optionsstatbox).withMargin(0).withFlex(1).withMaxWidth(300));
        pvf->recvbox.items.add(FlexItem(3, 5));
        pvf->recvbox.items.add(FlexItem(100, minitemheight, pvf->recvstatbox).withMargin(0).withFlex(1));
        pvf->recvbox.items.add(FlexItem(3, 5));
        if ( ! isNarrow) {            
            pvf->recvbox.items.add(FlexItem(2, 2).withFlex(0));
            pvf->recvbox.items.add(FlexItem(100, minitemheight, pvf->netstatbox).withMargin(0).withFlex(1).withMaxWidth(130));
            pvf->recvbox.items.add(FlexItem(3, 2).withFlex(0));
        }
        
        pvf->recvlevelbox.items.clear();
        pvf->recvlevelbox.flexDirection = FlexBox::Direction::row;

        if (!isNarrow) {
            pvf->recvlevelbox.items.add(FlexItem(mutebuttwidth, mincheckheight, *pvf->recvMutedButton).withMargin(0).withFlex(0));
            pvf->recvlevelbox.items.add(FlexItem(5, 2));
            pvf->recvlevelbox.items.add(FlexItem(mutebuttwidth, minitemheight, *pvf->recvSoloButton).withMargin(0).withFlex(0));
            pvf->recvlevelbox.items.add(FlexItem(3, 5));

            pvf->levelSlider->setTextBoxStyle(Slider::TextBoxAbove, true, 90, 12);
            pvf->levelSlider->setPopupDisplayEnabled(false, false, findParentComponentOfClass<AudioProcessorEditor>());
        }
        else {
            pvf->levelSlider->setTextBoxStyle(Slider::TextBoxAbove, true, 90, 12);
            pvf->levelSlider->setPopupDisplayEnabled(true, true, findParentComponentOfClass<AudioProcessorEditor>());
        }
        pvf->recvlevelbox.items.add(FlexItem(80, minitemheight, *pvf->levelSlider).withMargin(0).withFlex(4));


        if (processor.getTotalNumOutputChannels() > 1) {

            pvf->recvlevelbox.items.add(FlexItem(3, 5));

            pvf->recvlevelbox.items.add(FlexItem(minPannerWidth, minitemheight, *pvf->panSlider1).withMargin(0).withFlex(1).withMaxWidth(maxPannerWidth)); //.withMaxWidth(maxPannerWidth));

#if 0
            if (processor.getRemotePeerChannelCount(i) > 1) {
                // two panners 
                if (pvf->panSlider1->getParentComponent() != pvf->pannersContainer.get()) {
                    pvf->pannersContainer->addAndMakeVisible( pvf->panSlider1.get());
                }
                pvf->recvlevelbox.items.add(FlexItem(40, minitemheight, *pvf->panButton).withMargin(0).withFlex(1).withMaxWidth(50));

                pvf->pannerbox.items.add(FlexItem(minPannerWidth, minitemheight, *pvf->panSlider1).withMargin(0).withFlex(1)); //.withMaxWidth(maxPannerWidth));

                pvf->pannerbox.items.add(FlexItem(2, 5));
                pvf->pannerbox.items.add(FlexItem(minPannerWidth, minitemheight, *pvf->panSlider2).withMargin(0).withFlex(1)); //.withMaxWidth(maxPannerWidth));
                pvf->singlePanner = false;
                
                pvf->panSlider1->setSliderStyle(Slider::TwoValueHorizontal);
                
            }
            else {
                // single panner show at toplevel
                if (pvf->panSlider1->getParentComponent() != pvf) {
                    pvf->addAndMakeVisible(pvf->panSlider1.get());
                }

                pvf->panSlider1->setSliderStyle(Slider::LinearBar);

                
                pvf->recvlevelbox.items.add(FlexItem(minPannerWidth, minitemheight, *pvf->panSlider1).withMargin(0).withFlex(1).withMaxWidth(minPannerWidth+10)); //.withMaxWidth(maxPannerWidth));
                pvf->singlePanner = true;
            }
#endif
        }

        if (!isNarrow) {
            pvf->recvlevelbox.items.add(FlexItem(3, 5));
            pvf->recvlevelbox.items.add(FlexItem(42, minitemheight, *pvf->fxButton).withMargin(0).withFlex(0).withMaxWidth(50));
        }

        
        pvf->recvlevelbox.items.add(FlexItem(3, 5));


        if (isNarrow) {
            pvf->mainsendbox.items.clear();
            pvf->mainsendbox.flexDirection = FlexBox::Direction::row;
            pvf->mainsendbox.items.add(FlexItem(110, minitemheight, pvf->namebox).withMargin(0).withFlex(0));
            pvf->mainsendbox.items.add(FlexItem(3, 3));
            pvf->mainsendbox.items.add(FlexItem(100 + mincheckheight, minitemheight, pvf->recvlevelbox).withMargin(0).withFlex(1));
        } else {
            pvf->mainsendbox.items.clear();
            pvf->mainsendbox.flexDirection = FlexBox::Direction::row;
            pvf->mainsendbox.items.add(FlexItem(110, minitemheight, pvf->namebox).withMargin(0).withFlex(1).withMaxWidth(130));
            pvf->mainsendbox.items.add(FlexItem(3, 3));
            pvf->mainsendbox.items.add(FlexItem(100 + mincheckheight, minitemheight, pvf->recvlevelbox).withMargin(0).withFlex(1));
        }
        
        
        pvf->mainbox.items.clear();
        pvf->mainnarrowbox.items.clear();

        if (isNarrow) {
            pvf->mainnarrowbox.flexDirection = FlexBox::Direction::column;
            pvf->mainnarrowbox.items.add(FlexItem(2, 3));
            pvf->mainnarrowbox.items.add(FlexItem(100, minitemheight, pvf->mainsendbox).withMargin(0).withFlex(0));
            pvf->mainnarrowbox.items.add(FlexItem(2, 4));
            pvf->mainnarrowbox.items.add(FlexItem(120, minitemheight, pvf->sendbox).withMargin(0).withFlex(0));
            pvf->mainnarrowbox.items.add(FlexItem(2, 4));
            pvf->mainnarrowbox.items.add(FlexItem(150, minitemheight, pvf->recvbox).withMargin(0).withFlex(0));

            pvf->mainbox.flexDirection = FlexBox::Direction::row;
            pvf->mainbox.items.add(FlexItem(3, 2));
            pvf->mainbox.items.add(FlexItem(150, singleph*2 - minitemheight , pvf->mainnarrowbox).withMargin(0).withFlex(1));
            pvf->mainbox.items.add(FlexItem(2, 2));
            pvf->mainbox.items.add(FlexItem(22, 50, *pvf->recvMeter).withMargin(3).withFlex(0));
        } else {
            pvf->mainnarrowbox.flexDirection = FlexBox::Direction::column;
            pvf->mainnarrowbox.items.add(FlexItem(2, 3));
            pvf->mainnarrowbox.items.add(FlexItem(100, minitemheight, pvf->mainsendbox).withMargin(0).withFlex(0));
            pvf->mainnarrowbox.items.add(FlexItem(2, 4));
            pvf->mainnarrowbox.items.add(FlexItem(150, minitemheight, pvf->recvbox).withMargin(0).withFlex(0));

            pvf->mainbox.flexDirection = FlexBox::Direction::row;
            pvf->mainbox.items.add(FlexItem(3, 2));
            pvf->mainbox.items.add(FlexItem(150, singleph, pvf->mainnarrowbox).withMargin(0).withFlex(1));
            pvf->mainbox.items.add(FlexItem(2, 2));
            pvf->mainbox.items.add(FlexItem(22, 50, *pvf->recvMeter).withMargin(3).withFlex(0));
        }

        peersBox.items.add(FlexItem(8, 4).withMargin(0));
        peersheight += 4;
        
        if (isNarrow) {
            peersBox.items.add(FlexItem(pw, ph*2 - minitemheight + 2, *pvf).withMargin(0).withFlex(0));
            peersheight += ph*2-minitemheight + 2;
        } else {
            peersBox.items.add(FlexItem(pw, ph + 5, *pvf).withMargin(1).withFlex(0));
            peersheight += ph + 5;
        }

    }

    
    
    if (isNarrow) {
        peersMinHeight = std::max(singleph*2 + 14,  peersheight);
        peersMinWidth = 300;
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


void PeersContainerView::updatePeerViews(int specific)
{
    uint32 nowstampms = Time::getMillisecondCounter();
    bool needsUpdateLayout = false;

    for (int i=0; i < mPeerViews.size(); ++i) {
        if (specific >= 0 && specific != i) continue;
        
        PeerViewInfo * pvf = mPeerViews.getUnchecked(i);
        String hostname;
        int port = 0;
        processor.getRemotePeerAddressInfo(i, hostname, port);
        bool connected = processor.getRemotePeerConnected(i);

        String username = processor.getRemotePeerUserName(i);
        String addrname;
        addrname << hostname << " : " << port;
        
        //DBG("Got username: '" << username << "'");
        
        if (username.isNotEmpty()) {
            pvf->nameLabel->setText(username, dontSendNotification);
            pvf->addrLabel->setText(addrname, dontSendNotification);
            //pvf->addrLabel->setVisible(true);
        } else {
            //pvf->addrLabel->setVisible(false);
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
            sendtext += TRANS("Other end muted us");
            pvf->sendActualBitrateLabel->setColour(Label::textColourId, mutedTextColor);
        }
        else {
            sendtext += TRANS("SEND DISABLED");
            pvf->sendActualBitrateLabel->setColour(Label::textColourId, mutedTextColor);
        }
        
        String recvtext;
        SonobusAudioProcessor::AudioCodecFormatInfo recvfinfo;
        processor.getRemotePeerReceiveAudioCodecFormat(i, recvfinfo);

        if (recvactive) {
            recvtext += recvfinfo.name + String::formatted(" | %d kb/s", lrintf(recvrate * 8 * 1e-3));

            int64_t dropped = processor.getRemotePeerPacketsDropped(i);
            if (dropped > 0) {
                recvtext += String::formatted(" | %d drop", dropped);
            }

            if (dropped > pvf->lastDropped) {
                
                pvf->lastDroppedChangedTimestampMs = nowstampms;
            }

            int64_t resent = processor.getRemotePeerPacketsResent(i);
            if (resent > 0) {
                recvtext += String::formatted(" | %d resent", resent);
            }

            if (nowstampms < pvf->lastDroppedChangedTimestampMs + 1500) {
                pvf->recvActualBitrateLabel->setColour(Label::textColourId, droppedTextColor);
            }
            else {
                pvf->recvActualBitrateLabel->setColour(Label::textColourId, regularTextColor);
            }

            pvf->lastDropped = dropped;
        }
        else if (recvallow) {
            recvtext += TRANS("Other side is muted");    
            pvf->recvActualBitrateLabel->setColour(Label::textColourId, mutedTextColor);
        }
        else {
            recvtext += TRANS("You muted them");    
            pvf->recvActualBitrateLabel->setColour(Label::textColourId, mutedTextColor);
        }


        pvf->changeAllFormatButton->setToggleState(processor.getChangingDefaultAudioCodecSetsExisting(), dontSendNotification);

        pvf->sendActualBitrateLabel->setText(sendtext, dontSendNotification);
        pvf->recvActualBitrateLabel->setText(recvtext, dontSendNotification);

        SonobusAudioProcessor::LatencyInfo latinfo;
        processor.getRemotePeerLatencyInfo(i, latinfo);
        
        //pvf->pingLabel->setText(String::formatted("%d ms", (int)latinfo.pingMs ), dontSendNotification);
        pvf->pingLabel->setText(String::formatted("%d", (int)lrintf(latinfo.pingMs) ), dontSendNotification);
        
        if (!latinfo.isreal) {
            if (pvf->stopLatencyTestTimestampMs > 0) {
                pvf->latencyLabel->setText(TRANS("****"), dontSendNotification);
            } else {
                pvf->latencyLabel->setText(TRANS("PRESS"), dontSendNotification);
            }
        } else {
            //pvf->latencyLabel->setText(String::formatted("%d ms", (int)lrintf(latinfo.totalRoundtripMs)) + (latinfo.estimated ? "*" : ""), dontSendNotification);
            String latlab = juce::CharPointer_UTF8 ("\xe2\x86\x91");
            latlab << (int)lrintf(latinfo.outgoingMs) << "   "; 
            //latlab << String::formatted(TRANS("%.1f"), latinfo.outgoingMs) << "   ";
            //latlab << String(juce::CharPointer_UTF8 ("\xe2\x86\x93")) << (int)lrintf(latinfo.incomingMs);
            latlab << String(juce::CharPointer_UTF8 ("\xe2\x86\x93"));
            latlab << (int)lrintf(latinfo.incomingMs) ;
            //latlab << String::formatted("%.1f", latinfo.incomingMs) ;
            ////<< " = " << String(juce::CharPointer_UTF8 ("\xe2\x86\x91\xe2\x86\x93")) << (int)lrintf(latinfo.totalRoundtripMs)             
            latlab << (latinfo.estimated ? " *" : "");
            
            pvf->latencyLabel->setText(latlab, dontSendNotification);
        }

        
        bool aresoloed = processor.getRemotePeerSoloed(i);
        bool aremuted = !processor.getRemotePeerRecvAllow(i);
        
        if (processor.isAnythingSoloed() && !aresoloed && !aremuted) {
            pvf->recvMutedButton->setColour(TextButton::buttonColourId, mutedBySoloColor);
        } else {
            pvf->recvMutedButton->removeColour(TextButton::buttonColourId);
        }

        pvf->sendMutedButton->setToggleState(!processor.getRemotePeerSendAllow(i) , dontSendNotification);
        pvf->recvMutedButton->setToggleState(aremuted , dontSendNotification);
        pvf->recvSoloButton->setToggleState(aresoloed , dontSendNotification);

        
        pvf->latActiveButton->setToggleState(latactive, dontSendNotification);

        
        int autobufmode = (int)processor.getRemotePeerAutoresizeBufferMode(i);
        float buftimeMs = processor.getRemotePeerBufferTime(i);
        
        pvf->autosizeButton->setSelectedId(autobufmode, dontSendNotification);
        pvf->bufferLabel->setText(String::formatted("%d ms", (int) lrintf(buftimeMs)) + (autobufmode == SonobusAudioProcessor::AutoNetBufferModeOff ? "" : autobufmode == SonobusAudioProcessor::AutoNetBufferModeAutoIncreaseOnly ? " (Auto+)" :  autobufmode == SonobusAudioProcessor::AutoNetBufferModeInitAuto ?  " (I-Auto)"  : " (Auto)"), dontSendNotification);

        pvf->bufferMinButton->setEnabled(autobufmode != SonobusAudioProcessor::AutoNetBufferModeOff);
        pvf->bufferMinFrontButton->setEnabled(autobufmode != SonobusAudioProcessor::AutoNetBufferModeOff);
        
        if (!pvf->levelSlider->isMouseOverOrDragging()) {
            pvf->levelSlider->setValue(processor.getRemotePeerLevelGain(i), dontSendNotification);
        }
        if (!pvf->bufferTimeSlider->isMouseButtonDown()) {
            pvf->bufferTimeSlider->setValue(buftimeMs, dontSendNotification);
        }

        pvf->remoteSendFormatChoiceButton->setSelectedId(processor.getRequestRemotePeerSendAudioCodecFormat(i), dontSendNotification);
        
        
        int formatindex = processor.getRemotePeerAudioCodecFormat(i);
        pvf->formatChoiceButton->setSelectedItemIndex(formatindex >= 0 ? formatindex : processor.getDefaultAudioCodecFormat(), dontSendNotification);
        pvf->sendQualityLabel->setText(processor.getAudioCodeFormatName(formatindex), dontSendNotification);
        
        pvf->recvMeter->setMeterSource (processor.getRemotePeerRecvMeterSource(i));        

        
        if (processor.getTotalNumOutputChannels() == 1) {
            pvf->panSlider1->setVisible(false);
            pvf->panSlider2->setVisible(false);
            pvf->panButton->setVisible(false);
        }
        else if (processor.getRemotePeerChannelCount(i) == 1) {
            pvf->panSlider1->setVisible(true);
            pvf->panSlider1->setDoubleClickReturnValue(true, 0.0);
            pvf->panSlider2->setVisible(false);
            pvf->panButton->setVisible(false);

            if (!pvf->singlePanner) {
                pvf->panSlider1->setSliderStyle(Slider::LinearHorizontal); // LinearBar
                pvf->panSlider1->setTextBoxStyle(Slider::NoTextBox, true, 10, 2); // TextBoxAbove
                
                pvf->singlePanner = true;
            }
                        
        } else {
            pvf->panSlider1->setDoubleClickReturnValue(false, -1.0);
            pvf->panSlider1->setVisible(true);
            pvf->panSlider2->setVisible(false);
            pvf->panButton->setVisible(false);

            if (pvf->singlePanner) {
                pvf->panSlider1->setSliderStyle(Slider::TwoValueHorizontal);
                pvf->panSlider1->setTextBoxStyle(Slider::NoTextBox, true, 60, 12);

                pvf->singlePanner = false;
            }
        }
        
        if (pvf->panSlider1->isTwoValue()) {
            pvf->panSlider1->setMinAndMaxValues(processor.getRemotePeerChannelPan(i, 0), processor.getRemotePeerChannelPan(i, 1), dontSendNotification);            
        }
        else {        
            pvf->panSlider1->setValue(processor.getRemotePeerChannelPan(i, 0), dontSendNotification);
        }
        pvf->panSlider2->setValue(processor.getRemotePeerChannelPan(i, 1), dontSendNotification);
        

        
        const float disalpha = 0.4;
        pvf->nameLabel->setAlpha(connected ? 1.0 : 0.8);
        pvf->addrLabel->setAlpha(connected ? 1.0 : 0.8);
        pvf->statusLabel->setAlpha(connected ? 1.0 : disalpha);
        pvf->levelSlider->setAlpha(recvactive ? 1.0 : disalpha);
        
        SonobusAudioProcessor::CompressorParams compParams;
        if (processor.getRemotePeerCompressorParams(i, compParams)) {
            pvf->compressorView->updateParams(compParams);
            pvf->fxButton->setToggleState(compParams.enabled, dontSendNotification);
        }
        
        if (pvf->stopLatencyTestTimestampMs > 0.0 && nowstampms > pvf->stopLatencyTestTimestampMs
            && !pvf->latActiveButton->isMouseButtonDown()) {

            // only stop if it has actually gotten a real latency
            if (latinfo.isreal) {
                stopLatencyTest(i);
                
                String messagestr = generateLatencyMessage(latinfo);
                showPopTip(messagestr, 5000, pvf->latActiveButton.get(), 140);

            }
        }
    }
    
    int i=0;
    for (auto & pinfo : mPendingUsers) {
        if (i>= mPendingPeerViews.size()) {
            break; // shouldn't happen
        }
        
        PendingPeerViewInfo * ppvf = mPendingPeerViews.getUnchecked(i);
        ppvf->nameLabel->setText(pinfo.second.user, dontSendNotification);
        
        if (pinfo.second.failed) {
            ppvf->messageLabel->setText(TRANS("Could not connect with user, one or both of you may need to configure your internal firewall or network router to allow SonoBus to work between you. See the help documentation to enable port forwarding on your router."), dontSendNotification);
        }
        else {
            ppvf->messageLabel->setText(TRANS("Connecting..."), dontSendNotification);
        }
        ++i;
    }
    
    if (needsUpdateLayout) {
        updateLayout();
    }
    
    lastUpdateTimestampMs = nowstampms;
}

void PeersContainerView::startLatencyTest(int i)
{
    if (i >= mPeerViews.size()) return;
    
    PeerViewInfo * pvf = mPeerViews.getUnchecked(i);

    pvf->stopLatencyTestTimestampMs = Time::getMillisecondCounter(); // make it stop after the first one  //+ 1500;
    pvf->wasRecvActiveAtLatencyTest = processor.getRemotePeerRecvActive(i);
    pvf->wasSendActiveAtLatencyTest = processor.getRemotePeerSendActive(i);
    
    pvf->latencyLabel->setText(TRANS("****"), dontSendNotification);

    processor.startRemotePeerLatencyTest(i);             
}

void PeersContainerView::stopLatencyTest(int i)
{
    if (i >= mPeerViews.size()) return;
    PeerViewInfo * pvf = mPeerViews.getUnchecked(i);

    processor.stopRemotePeerLatencyTest(i);
    
    pvf->stopLatencyTestTimestampMs = 0;
    
    SonobusAudioProcessor::LatencyInfo latinfo;
    processor.getRemotePeerLatencyInfo(i, latinfo);
        
    if (!latinfo.isreal) {
        pvf->latencyLabel->setText(TRANS("PRESS"), dontSendNotification);
    } else {
        //pvf->latencyLabel->setText(String::formatted("%d ms", (int)lrintf(latinfo.totalRoundtripMs)) + (latinfo.estimated ? "*" : "") , dontSendNotification);
        updatePeerViews(i);
    }
}

String PeersContainerView::generateLatencyMessage(const SonobusAudioProcessor::LatencyInfo &latinfo)
{
    String messagestr = String::formatted(TRANS("Measured actual round-trip latency: %d ms"), (int) lrintf(latinfo.totalRoundtripMs));
    messagestr += String::formatted(TRANS("\nEst. Outgoing: %.1f ms"), (latinfo.outgoingMs));
    messagestr += String::formatted(TRANS("\nEst. Incoming: %.1f ms"), (latinfo.incomingMs));
    //messagestr += String::formatted(TRANS("\nJitter: %.1f ms"), (latinfo.jitterMs));
    return messagestr;
}


void PeersContainerView::timerCallback(int timerId)
{
    if (timerId == FillRatioUpdateTimerId) {
        for (int i=0; i < mPeerViews.size(); ++i) {
            PeerViewInfo * pvf = mPeerViews.getUnchecked(i);

            float ratio, stdev;
            if (processor.getRemotePeerReceiveBufferFillRatio(i, ratio, stdev) > 0) {
                pvf->jitterBufferMeter->setFillRatio(ratio, stdev);
            }
        }
    }
}


void PeersContainerView::choiceButtonSelected(SonoChoiceButton *comp, int index, int ident)
{
    for (int i=0; i < mPeerViews.size(); ++i) {
        PeerViewInfo * pvf = mPeerViews.getUnchecked(i);
        if (pvf->formatChoiceButton.get() == comp) {
            // set them all if this option is selected
            if (processor.getChangingDefaultAudioCodecSetsExisting()) {
                for (int j=0; j < mPeerViews.size(); ++j) {
                    processor.setRemotePeerAudioCodecFormat(j, ident);                    
                }
            }
            else {
                processor.setRemotePeerAudioCodecFormat(i, ident);
            }
            break;
        }        
        else if (pvf->remoteSendFormatChoiceButton.get() == comp) {
            // set them all if this option is selected

            if (processor.getChangingDefaultAudioCodecSetsExisting()) {
                for (int j=0; j < mPeerViews.size(); ++j) {

                    processor.setRequestRemotePeerSendAudioCodecFormat(j, ident); 
                }
            }
            else {
                processor.setRequestRemotePeerSendAudioCodecFormat(i, ident); 
            }
            break;
        }        
        else if (pvf->autosizeButton.get() == comp) {
            processor.setRemotePeerAutoresizeBufferMode(i, (SonobusAudioProcessor::AutoNetBufferMode) ident);
            break;
        }        
    }
}

void PeersContainerView::buttonClicked (Button* buttonThatWasClicked)
{
    for (int i=0; i < mPeerViews.size(); ++i) {
        PeerViewInfo * pvf = mPeerViews.getUnchecked(i);
        bool connected = processor.getRemotePeerConnected(i);
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
        else if (pvf->recvSoloButton.get() == buttonThatWasClicked) {
            if (ModifierKeys::currentModifiers.isAltDown()) {
                // exclusive solo this one
                for (int j=0; j < mPeerViews.size(); ++j) {
                    if (buttonThatWasClicked->getToggleState()) {
                        processor.setRemotePeerSoloed(j, i == j);                         
                    }
                    else {
                        processor.setRemotePeerSoloed(j, false); 
                    }
                }
                    
                // disable solo for main monitor too
                processor.getValueTreeState().getParameter(SonobusAudioProcessor::paramMainMonitorSolo)->setValueNotifyingHost(0.0);
                
                updatePeerViews();
            } else {
                processor.setRemotePeerSoloed(i, buttonThatWasClicked->getToggleState()); 
                updatePeerViews();
            }
            break;
        }
        else if (pvf->latActiveButton.get() == buttonThatWasClicked) {
            if (pvf->latActiveButton->getToggleState()) {
                startLatencyTest(i);
                //showPopTip(TRANS("Measuring actual round-trip latency"), 4000, pvf->latActiveButton.get(), 140);
            } else {
                stopLatencyTest(i);
            }
            break;
        }

        
        else if (pvf->recvOptionsButton.get() == buttonThatWasClicked) {

            if (!recvOptionsCalloutBox) {
                showRecvOptions(i, true, pvf->recvOptionsButton.get());
            } else {
                showRecvOptions(i, false);
            }

            break;
        }
        else if (pvf->fxButton.get() == buttonThatWasClicked) {

            if (!effectsCalloutBox) {
                showEffects(i, true, pvf->fxButton.get());
            } else {
                showEffects(i, false);
            }

            break;
        }
        else if (pvf->panButton.get() == buttonThatWasClicked) {
            if (!pannerCalloutBox) {
                showPanners(i, true);
            } else {
                showPanners(i, false);
            }
            break;
        }
        else if (pvf->sendOptionsButton.get() == buttonThatWasClicked) {
            if (!sendOptionsCalloutBox) {
                showSendOptions(i, true, pvf->sendOptionsButton.get());
            } else {
                showSendOptions(i, false);
            }
            break;
        }
        else if (pvf->optionsResetDropButton.get() == buttonThatWasClicked) {
            processor.resetRemotePeerPacketStats(i);
            break;
        }
        else if (pvf->changeAllFormatButton.get() == buttonThatWasClicked) {
            processor.setChangingDefaultAudioCodecSetsExisting(buttonThatWasClicked->getToggleState());
            break;
        }
        else if (pvf->optionsRemoveButton.get() == buttonThatWasClicked) {
            processor.removeRemotePeer(i);
            showSendOptions(i, false);
            break;
        }
        else if (pvf->bufferMinButton.get() == buttonThatWasClicked || pvf->bufferMinFrontButton.get() == buttonThatWasClicked) {
            // force to minimum
            if (ModifierKeys::currentModifiers.isAltDown()) {
                // do it for everyone (who's on auto, maybe?)
                for (int j=0; j < mPeerViews.size(); ++j) {
                    if (processor.getRemotePeerAutoresizeBufferMode(j) != SonobusAudioProcessor::AutoNetBufferModeOff) {
                        float buftime = 0.0;
                        processor.setRemotePeerBufferTime(j, buftime);
                        if (i==j) {
                            pvf->bufferTimeSlider->setValue(buftime, dontSendNotification);
                        }                            
                    }
                }
            } else {
                float buftime = 0.0;
                processor.setRemotePeerBufferTime(i, buftime);
                pvf->bufferTimeSlider->setValue(buftime, dontSendNotification);
            }
            
            updatePeerViews();
            break;
        }
    }    
}

void PeersContainerView::showPanners(int index, bool flag)
{
    
    if (flag && pannerCalloutBox == nullptr) {
        
        auto wrap = std::make_unique<Viewport>();

        Component* dw = nullptr; // this->findParentComponentOfClass<DocumentWindow>();
        
        if (!dw) {
            dw = this->findParentComponentOfClass<AudioProcessorEditor>();
        }
        if (!dw) {
            dw = this->findParentComponentOfClass<Component>();
        }
        if (!dw) {
            dw = this;
        }
        
        const int defWidth = 140;
#if JUCE_IOS
        const int defHeight = 50;
#else
        const int defHeight = 42;
#endif
        
        wrap->setSize(jmin(defWidth, dw->getWidth() - 20), jmin(defHeight, dw->getHeight() - 24));
        
        
        auto * pvf = mPeerViews.getUnchecked(index);
        
        pvf->pannersContainer->setBounds(Rectangle<int>(0,0,defWidth,defHeight));
        
        wrap->setViewedComponent(pvf->pannersContainer.get(), false);
        pvf->pannersContainer->setVisible(true);
        
        pvf->pannerbox.performLayout(pvf->pannersContainer->getLocalBounds());
        
        
        Rectangle<int> bounds =  dw->getLocalArea(nullptr, pvf->panButton->getScreenBounds());
        DBG("callout bounds: " << bounds.toString());
        pannerCalloutBox = & CallOutBox::launchAsynchronously (std::move(wrap), bounds , dw, false);
        if (CallOutBox * box = dynamic_cast<CallOutBox*>(pannerCalloutBox.get())) {
            box->setDismissalMouseClicksAreAlwaysConsumed(true);
        }
    }
    else {
        // dismiss it
        if (CallOutBox * box = dynamic_cast<CallOutBox*>(pannerCalloutBox.get())) {
            box->dismiss();
            pannerCalloutBox = nullptr;
        }
    }
}

void PeersContainerView::showRecvOptions(int index, bool flag, Component * fromView)
{
    
    if (flag && recvOptionsCalloutBox == nullptr) {
        
        auto wrap = std::make_unique<Viewport>();

        Component* dw = nullptr; // this->findParentComponentOfClass<DocumentWindow>();
        
        if (!dw) {
            dw = this->findParentComponentOfClass<AudioProcessorEditor>();
        }
        if (!dw) {
            dw = this->findParentComponentOfClass<Component>();
        }
        if (!dw) {
            dw = this;
        }
        
        const int defWidth = 280;
#if JUCE_IOS
        const int defHeight = 180;
#else
        const int defHeight = 152;
#endif
        
        
        wrap->setSize(jmin(defWidth, dw->getWidth() - 20), jmin(defHeight, dw->getHeight() - 24));
        
        auto * pvf = mPeerViews.getUnchecked(index);
        
        pvf->recvOptionsContainer->setBounds(Rectangle<int>(0,0,defWidth,defHeight));
        
        wrap->setViewedComponent(pvf->recvOptionsContainer.get(), false);
        pvf->recvOptionsContainer->setVisible(true);
        
        pvf->recvOptionsBox.performLayout(pvf->recvOptionsContainer->getLocalBounds());
        pvf->bufferTimeLabel->setBounds(pvf->bufferTimeSlider->getBounds().removeFromLeft(pvf->bufferTimeSlider->getWidth()*0.5));

        
        Rectangle<int> bounds =  dw->getLocalArea(nullptr, fromView ? fromView->getScreenBounds() : pvf->recvOptionsButton->getScreenBounds());
        DBG("callout bounds: " << bounds.toString());
        recvOptionsCalloutBox = & CallOutBox::launchAsynchronously (std::move(wrap), bounds , dw, false);
        if (CallOutBox * box = dynamic_cast<CallOutBox*>(recvOptionsCalloutBox.get())) {
            box->setDismissalMouseClicksAreAlwaysConsumed(true);
        }
    }
    else {
        // dismiss it
        if (CallOutBox * box = dynamic_cast<CallOutBox*>(recvOptionsCalloutBox.get())) {
            box->dismiss();
            recvOptionsCalloutBox = nullptr;
        }
    }
}

void PeersContainerView::showSendOptions(int index, bool flag, Component * fromView)
{
    
    if (flag && sendOptionsCalloutBox == nullptr) {
        
        auto wrap = std::make_unique<Viewport>();

        
        Component* dw = nullptr; // this->findParentComponentOfClass<DocumentWindow>();
        
        if (!dw) {
            dw = this->findParentComponentOfClass<AudioProcessorEditor>();
        }
        if (!dw) {
            dw = this->findParentComponentOfClass<Component>();
        }
        if (!dw) {
            dw = this;
        }
        
        const int defWidth = 245;
#if JUCE_IOS
        const int defHeight = 144;
#else
        const int defHeight = 116;
#endif
        
        
        wrap->setSize(jmin(defWidth, dw->getWidth() - 20), jmin(defHeight, dw->getHeight() - 24));
        
        auto * pvf = mPeerViews.getUnchecked(index);
        
        pvf->sendOptionsContainer->setBounds(Rectangle<int>(0,0,defWidth,defHeight));
        
        wrap->setViewedComponent(pvf->sendOptionsContainer.get(), false);
        pvf->sendOptionsContainer->setVisible(true);
        
        pvf->sendOptionsBox.performLayout(pvf->sendOptionsContainer->getLocalBounds());

        
        Rectangle<int> bounds =  dw->getLocalArea(nullptr, fromView ? fromView->getScreenBounds() : pvf->sendOptionsButton->getScreenBounds());
        DBG("callout bounds: " << bounds.toString());
        sendOptionsCalloutBox = & CallOutBox::launchAsynchronously (std::move(wrap), bounds , dw, false);
        if (CallOutBox * box = dynamic_cast<CallOutBox*>(sendOptionsCalloutBox.get())) {
            box->setDismissalMouseClicksAreAlwaysConsumed(true);
        }
    }
    else {
        // dismiss it
        if (CallOutBox * box = dynamic_cast<CallOutBox*>(sendOptionsCalloutBox.get())) {
            box->dismiss();
            sendOptionsCalloutBox = nullptr;
        }
    }
}

void PeersContainerView::showEffects(int index, bool flag, Component * fromView)
{
    
    if (flag && effectsCalloutBox == nullptr) {
        
        auto wrap = std::make_unique<Viewport>();

        Component* dw = nullptr; // this->findParentComponentOfClass<DocumentWindow>();
        
        if (!dw) {
            dw = this->findParentComponentOfClass<AudioProcessorEditor>();
        }
        if (!dw) {
            dw = this->findParentComponentOfClass<Component>();
        }
        if (!dw) {
            dw = this;
        }
        
        int defWidth = 260;
#if JUCE_IOS
        int defHeight = 180;
#else
        int defHeight = 156;
#endif
        
        
        auto * pvf = mPeerViews.getUnchecked(index);

        auto minbounds = pvf->compressorView->getMinimumContentBounds();
        auto minheadbounds = pvf->compressorView->getMinimumHeaderBounds();
        defWidth = minbounds.getWidth();
        defHeight = minbounds.getHeight() + minheadbounds.getHeight() + 6;
        
        wrap->setSize(jmin(defWidth, dw->getWidth() - 20), jmin(defHeight, dw->getHeight() - 24));

        
        pvf->effectsContainer->setBounds(Rectangle<int>(0,0,defWidth,defHeight));        
        pvf->effectsBox.performLayout(pvf->effectsContainer->getLocalBounds());
        
        SonobusAudioProcessor::CompressorParams compParams;
        if (processor.getRemotePeerCompressorParams(index, compParams)) {
            pvf->compressorView->updateParams(compParams);
        }

        
        wrap->setViewedComponent(pvf->effectsContainer.get(), false);
        pvf->effectsContainer->setVisible(true);
        
        Rectangle<int> bounds =  dw->getLocalArea(nullptr, fromView ? fromView->getScreenBounds() : pvf->fxButton->getScreenBounds());
        DBG("effect callout bounds: " << bounds.toString());
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

void PeersContainerView::mouseUp (const MouseEvent& event) 
{
    for (int i=0; i < mPeerViews.size(); ++i) {
        PeerViewInfo * pvf = mPeerViews.getUnchecked(i);

        if (event.eventComponent == pvf->latActiveButton.get()) {
            uint32 nowtimems = Time::getMillisecondCounter();
            SonobusAudioProcessor::LatencyInfo latinfo;
            processor.getRemotePeerLatencyInfo(i, latinfo);

            // only stop if it has actually gotten a real latency

            if (nowtimems >= pvf->stopLatencyTestTimestampMs && latinfo.isreal) {
                stopLatencyTest(i);
                pvf->latActiveButton->setToggleState(false, dontSendNotification);
                
                String messagestr = generateLatencyMessage(latinfo);

                showPopTip(messagestr, 5000, pvf->latActiveButton.get(), 140);

            }
            break;
        }
        else if (event.eventComponent == pvf->recvMeter.get()) {
            pvf->recvMeter->clearClipIndicator(-1);
            break;
        }

    }
}

void PeersContainerView::clearClipIndicators()
{
    for (int i=0; i < mPeerViews.size(); ++i) {
        PeerViewInfo * pvf = mPeerViews.getUnchecked(i);
        pvf->recvMeter->clearClipIndicator(-1);
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
    
    Component* dw = nullptr; // source->findParentComponentOfClass<DocumentWindow>();

    if (!dw) {
        dw = source->findParentComponentOfClass<AudioProcessorEditor>();        
    }
    if (!dw) {
        dw = source->findParentComponentOfClass<Component>();        
    }
    
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

void PeersContainerView::compressorParamsChanged(CompressorView *comp, SonobusAudioProcessor::CompressorParams & params) 
{
    for (int i=0; i < mPeerViews.size(); ++i) {
        PeerViewInfo * pvf = mPeerViews.getUnchecked(i);
        if (pvf->compressorView.get() == comp) {
            processor.setRemotePeerCompressorParams(i, params);
            pvf->fxButton->setToggleState(params.enabled, dontSendNotification);
            break;
        }
    }
}

void PeersContainerView::effectsHeaderClicked(EffectsBaseView *comp, const MouseEvent & event)
{
    for (int i=0; i < mPeerViews.size(); ++i) {
        PeerViewInfo * pvf = mPeerViews.getUnchecked(i);
        if (pvf->compressorView.get() == comp) {
            SonobusAudioProcessor::CompressorParams params;
            processor.getRemotePeerCompressorParams(i, params);
            params.enabled = !params.enabled;
            processor.setRemotePeerCompressorParams(i, params);
            pvf->compressorView->updateParams(params);
            pvf->fxButton->setToggleState(params.enabled, dontSendNotification);
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
           break;
       }
       else if (pvf->bufferTimeSlider.get() == slider) {
           float buftime = pvf->bufferTimeSlider->getValue();
           processor.setRemotePeerBufferTime(i, buftime);
           pvf->bufferTimeSlider->setValue(buftime, dontSendNotification);
           break;
       }
       else if (pvf->panSlider1.get() == slider) {
           if (pvf->panSlider1->isTwoValue()) {
               float pan1 = pvf->panSlider1->getMinValue();
               float pan2 = pvf->panSlider1->getMaxValue();
               processor.setRemotePeerChannelPan(i, 0, pan1);   
               processor.setRemotePeerChannelPan(i, 1, pan2);
           }
           else {
               processor.setRemotePeerChannelPan(i, 0, pvf->panSlider1->getValue());   
           }
           break;
       }
       else if (pvf->panSlider2.get() == slider) {
           processor.setRemotePeerChannelPan(i, 1, pvf->panSlider2->getValue());   
           break;
       }

   }

}
