// SPDX-License-Identifier: GPLv3-or-later WITH Appstore-exception
// Copyright (C) 2020 Jesse Chappell

#include "PeersContainerView.h"
#include "JitterBufferMeter.h"
#include <set>

using namespace SonoAudio;

PeerViewInfo::PeerViewInfo() : smallLnf(12), medLnf(14), sonoSliderLNF(12), panSliderLNF(12)
{
    bgColor = Colour::fromFloatRGBA(0.112f, 0.112f, 0.112f, 1.0f);
    borderColor = Colour::fromFloatRGBA(0.5f, 0.5f, 0.5f, 0.3f);

    sonoSliderLNF.textJustification = Justification::centredLeft;
    panSliderLNF.textJustification = Justification::centredLeft;

    setFocusContainerType(FocusContainerType::focusContainer);

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
    auto localbounds = getLocalBounds();

    int chantargwidth = localbounds.getWidth() + 8;

    if (channelGroups->getEstimatedWidth() != chantargwidth && channelGroups->isVisible()) {
        channelGroups->setEstimatedWidth(chantargwidth);
        channelGroups->updateLayout(false);
    }

    mainbox.performLayout(localbounds);



    
    if (latActiveButton) {
        //latActiveButton->setBounds(staticPingLabel->getX(), staticPingLabel->getY(), pingLabel->getRight() - staticPingLabel->getX(), latencyLabel->getBottom() - pingLabel->getY());
        latActiveButton->setBounds(staticLatencyLabel->getX(), staticLatencyLabel->getY(), pingLabel->getRight() - staticLatencyLabel->getX(), latencyUpLabel->getBottom() - staticLatencyLabel->getY());

        auto arrwidth = 10;
        auto arrheight = 14;
        auto urect = Rectangle<int>(latencyUpLabel->getX() - arrwidth, latencyUpLabel->getY() + (latencyUpLabel->getHeight() - arrheight)/2, arrwidth, arrheight);
        latUpArrow->setTransformToFit(urect.toFloat(), RectanglePlacement::stretchToFit);
        auto drect = Rectangle<int>(latencyDownLabel->getX() - arrwidth, latencyDownLabel->getY() + (latencyDownLabel->getHeight() - arrheight)/2, arrwidth, arrheight);
        latDownArrow->setTransformToFit(drect.toFloat(), RectanglePlacement::stretchToFit);
    }

    int triwidth = 10;
    int triheight = 6;

    if (recvOptionsButton && latActiveButton) {
        auto leftedge = (sendActualBitrateLabel->getRight() + (recvActualBitrateLabel->getX() - sendActualBitrateLabel->getRight()) / 2) + 2;
        if (isNarrow) {
            //leftedge = 4;
            leftedge = (staticSendQualLabel->getRight() + (bufferLabel->getX() - staticSendQualLabel->getRight()) / 2) + 2;

            recvOptionsButton->setBounds(leftedge, staticBufferLabel->getY(), bufferLabel->getRight() - leftedge + 2, bufferLabel->getBottom() - staticBufferLabel->getY());
        }
        else {
            recvOptionsButton->setBounds(leftedge, staticBufferLabel->getY(), latActiveButton->getX() - leftedge - 5, recvActualBitrateLabel->getBottom() - staticBufferLabel->getY());
        }


        if (recvOptionsButton->getWidth() > 260) {
            int buttw = recvOptionsButton->getHeight() - 4;
            bufferMinFrontButton->setBounds(recvOptionsButton->getRight() - buttw - 2, recvOptionsButton->getY() + 2, buttw, buttw);
            bufferMinFrontButton->setVisible(fullMode);
        } else {
            bufferMinFrontButton->setVisible(false);            
        }

        if (isNarrow) {
            auto rect = Rectangle<int>(recvOptionsButton->getX() +  3, recvOptionsButton->getBottom() - recvOptionsButton->getHeight()/2 - triheight + 2, triwidth, triheight);
            recvButtonImage->setTransformToFit(rect.toFloat(), RectanglePlacement::stretchToFit);
        }
        else {
            auto rect = Rectangle<int>(recvOptionsButton->getX() +  3, recvOptionsButton->getBottom() - triheight - 1, triwidth, triheight);
            recvButtonImage->setTransformToFit(rect.toFloat(), RectanglePlacement::stretchToFit);
        }
    }

    if (sendOptionsButton) {
        int rightedge;
        if (isNarrow) {
            rightedge = (sendQualityLabel->getRight() + (staticBufferLabel->getX() - sendQualityLabel->getRight()) / 2) - 3;

            sendOptionsButton->setBounds(staticSendQualLabel->getX(), staticSendQualLabel->getY(), rightedge - staticSendQualLabel->getX(), sendQualityLabel->getBottom() - staticSendQualLabel->getY());

            auto rect = Rectangle<int>(sendOptionsButton->getX() + 3, sendOptionsButton->getBottom() - sendOptionsButton->getHeight()/2 - triheight + 2, triwidth, triheight);
            sendButtonImage->setTransformToFit(rect.toFloat(), RectanglePlacement::stretchToFit);
        }
        else {
            rightedge = (sendActualBitrateLabel->getRight() + (recvActualBitrateLabel->getX() - sendActualBitrateLabel->getRight()) / 2) - 3;

            sendOptionsButton->setBounds(staticSendQualLabel->getX(), staticSendQualLabel->getY(), rightedge - staticSendQualLabel->getX(), sendActualBitrateLabel->getBottom() - staticSendQualLabel->getY());

            auto rect = Rectangle<int>(sendOptionsButton->getX() + 3, sendOptionsButton->getBottom() - triheight - 1, triwidth, triheight);
            sendButtonImage->setTransformToFit(rect.toFloat(), RectanglePlacement::stretchToFit);
        }
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

    if (!isNarrow) {
        auto arrwidth = 10;
        auto arrheight = 14;
        auto srect = Rectangle<int>(sendActualBitrateLabel->getX() - arrwidth, sendActualBitrateLabel->getY() + (sendActualBitrateLabel->getHeight() - arrheight)/2, arrwidth, arrheight);
        sendUpArrow->setTransformToFit(srect.toFloat(), RectanglePlacement::stretchToFit);
        auto rrect = Rectangle<int>(recvActualBitrateLabel->getX() - arrwidth, recvActualBitrateLabel->getY() + (recvActualBitrateLabel->getHeight() - arrheight)/2, arrwidth, arrheight);
        recvDownArrow->setTransformToFit(rrect.toFloat(), RectanglePlacement::stretchToFit);
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

    peerModeFull = processor.getPeerDisplayMode() == SonobusAudioProcessor::PeerDisplayModeFull;

    //setFocusContainerType(FocusContainerType::focusContainer);

    mDragDrawable = std::make_unique<DrawableImage>();
    mDragDrawable->setAlpha(0.4f);
    mDragDrawable->setAlwaysOnTop(true);
    addChildComponent(mDragDrawable.get());

    mInsertLine = std::make_unique<DrawableRectangle>();
    //mInsertLine->setCornerSize(Point<float>(6,6));
    //mInsertLine->setFill (Colour::fromFloatRGBA(0.07, 0.07, 0.07, 1.0));
    mInsertLine->setFill (Colours::transparentBlack);
    mInsertLine->setStrokeFill (Colour::fromFloatRGBA(0.5, 0.5, 0.5, 0.75));
    mInsertLine->setStrokeThickness(2);
    addChildComponent(mInsertLine.get());


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
    slider->setSkewFactor(0.4);
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

    // if the width has changed, need to rebuild layout potentially
    if (mLastWidth != bounds.getWidth()) {
        mLastWidth = bounds.getWidth();
        updateLayout();
    }

    peersBox.performLayout(bounds);

    mPeerViewBounds.clearQuick();

    for (int i=0; i < mPeerViews.size(); ++i) {
        PeerViewInfo * pvf = mPeerViews.getUnchecked(i);
        pvf->resized();

        mPeerViewBounds.add(pvf->getBounds());
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
    text.setFont(Font(12 * SonoLookAndFeel::getFontScale()));
    if (target) {
        popTip->showAt(target, text, timeoutMs);
    }
    else {
        Rectangle<int> topbox(getWidth()/2 - maxwidth/2, 0, maxwidth, 2);
        popTip->showAt(topbox, text, timeoutMs);
    }
    popTip->toFront(false);
    // TODO make sure it is read for accessibility
    popTip->setWantsKeyboardFocus(true);
    popTip->setTitle(message);
    popTip->setAccessible(true);
    popTip->grabKeyboardFocus();
    //AccessibilityHandler::postAnnouncement(message, AccessibilityHandler::AnnouncementPriority::medium);
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

    pvf->addrLabel = std::make_unique<TextEditor>("addr");
    pvf->addrLabel->setJustification(Justification::centred);
    pvf->addrLabel->setColour(TextEditor::textColourId, dimTextColor);
    pvf->addrLabel->setColour(TextEditor::backgroundColourId, Colours::transparentBlack);
    pvf->addrLabel->setColour(TextEditor::outlineColourId, Colours::transparentBlack);
    pvf->addrLabel->setReadOnly(true);
    pvf->addrLabel->setCaretVisible(false);
    pvf->addrLabel->setFont(13 * SonoLookAndFeel::getFontScale());
    pvf->addrLabel->addMouseListener(this, false);

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

    
    pvf->latActiveButton = std::make_unique<SonoDrawableButton>("", DrawableButton::ButtonStyle::ImageFitted);
    pvf->latActiveButton->setColour(SonoTextButton::outlineColourId, Colours::transparentBlack);
    pvf->latActiveButton->setColour(DrawableButton::backgroundOnColourId, Colour::fromFloatRGBA(0.4, 0.2, 0.4, 0.7));
    pvf->latActiveButton->setColour(DrawableButton::backgroundColourId, Colours::transparentBlack);
    pvf->latActiveButton->setClickingTogglesState(false);
    pvf->latActiveButton->setTriggeredOnMouseDown(false);
    pvf->latActiveButton->setLookAndFeel(&pvf->smallLnf);
    pvf->latActiveButton->addListener(this);
    pvf->latActiveButton->addMouseListener(this, false);
    pvf->latActiveButton->setTitle(TRANS("Latency"));

    pvf->statusLabel = std::make_unique<Label>("status", "");
    configLabel(pvf->statusLabel.get(), LabelTypeRegular);
    pvf->statusLabel->setJustificationType(Justification::centredLeft);

    
    
    pvf->bufferTimeSlider     = std::make_unique<Slider>(Slider::LinearBar,  Slider::TextBoxBelow);
    pvf->bufferTimeSlider->setName("buffer");
    pvf->bufferTimeSlider->setTitle(TRANS("Jitter Buffer"));
    pvf->bufferTimeSlider->setRange(0, 5000, 1);
    pvf->bufferTimeSlider->setTextValueSuffix(" ms");
    //pvf->bufferTimeSlider->getProperties().set ("noFill", true);
    pvf->bufferTimeSlider->setSkewFactor(0.25);
    pvf->bufferTimeSlider->setDoubleClickReturnValue(true, 20.0);
    pvf->bufferTimeSlider->setTextBoxIsEditable(true);
    pvf->bufferTimeSlider->setSliderSnapsToMousePosition(false);
    pvf->bufferTimeSlider->setChangeNotificationOnlyOnRelease(true);
    pvf->bufferTimeSlider->setScrollWheelEnabled(false);
    pvf->bufferTimeSlider->setPopupDisplayEnabled(true, false, this);
    pvf->bufferTimeSlider->setColour(Slider::trackColourId, Colour::fromFloatRGBA(0.1, 0.4, 0.6, 0.3));

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
    pvf->bufferMinButton->setTooltip(TRANS("Resets jitter buffer to the minimum. Hold Alt key to reset for all (with auto)."));
    pvf->bufferMinButton->setTitle(TRANS("Reset Jitter Buffer"));
    pvf->bufferMinButton->setAlpha(0.8f);

    pvf->bufferMinFrontButton = std::make_unique<SonoDrawableButton>("", DrawableButton::ButtonStyle::ImageFitted);
    pvf->bufferMinFrontButton->setImages(backimg.get());
    pvf->bufferMinFrontButton->addListener(this);
    pvf->bufferMinFrontButton->setTooltip(TRANS("Resets jitter buffer to the minimum. Hold Alt key to reset for all (with auto)."));
    pvf->bufferMinFrontButton->setTitle(TRANS("Reset Jitter Buffer"));
    pvf->bufferMinFrontButton->setAlpha(0.8f);
    
    pvf->recvButtonImage = Drawable::createFromImageData(BinaryData::triangle_disclosure_svg, BinaryData::triangle_disclosure_svgSize);
    pvf->recvButtonImage->setInterceptsMouseClicks(false, false);
    pvf->recvButtonImage->setAlpha(0.7f);
    
    
    pvf->sendButtonImage = Drawable::createFromImageData(BinaryData::triangle_disclosure_svg, BinaryData::triangle_disclosure_svgSize);
    pvf->sendButtonImage->setInterceptsMouseClicks(false, false);
    pvf->sendButtonImage->setAlpha(0.7f);

    
    pvf->bufferTimeLabel = std::make_unique<Label>("buf", TRANS("Jitter Buffer"));
    configLabel(pvf->bufferTimeLabel.get(), LabelTypeRegular);
    pvf->bufferTimeLabel->setAccessible(false);

    pvf->recvOptionsButton = std::make_unique<SonoDrawableButton>("menu", DrawableButton::ImageFitted);
    pvf->recvOptionsButton->addListener(this);
    pvf->recvOptionsButton->setColour(SonoTextButton::outlineColourId, Colours::transparentBlack);
    pvf->recvOptionsButton->setColour(DrawableButton::backgroundOnColourId, Colour::fromFloatRGBA(0.4, 0.2, 0.4, 0.7));
    pvf->recvOptionsButton->setColour(DrawableButton::backgroundColourId, Colours::transparentBlack);
    pvf->recvOptionsButton->setTitle(TRANS("Receive Options"));

    
    pvf->sendOptionsButton = std::make_unique<SonoDrawableButton>("settings",  DrawableButton::ButtonStyle::ImageFitted);
    pvf->sendOptionsButton->addListener(this);
    pvf->sendOptionsButton->setColour(SonoTextButton::outlineColourId, Colours::transparentBlack);
    pvf->sendOptionsButton->setColour(DrawableButton::backgroundOnColourId, Colour::fromFloatRGBA(0.4, 0.2, 0.4, 0.7));
    pvf->sendOptionsButton->setColour(DrawableButton::backgroundColourId, Colours::transparentBlack);
    pvf->sendOptionsButton->setTitle(TRANS("Send Options"));


    pvf->changeAllFormatButton = std::make_unique<ToggleButton>(TRANS("Change for all"));
    pvf->changeAllFormatButton->addListener(this);
    pvf->changeAllFormatButton->setLookAndFeel(&pvf->smallLnf);

    
    pvf->formatChoiceButton = std::make_unique<SonoChoiceButton>();
    pvf->formatChoiceButton->setTitle(TRANS("Send Quality"));
    pvf->formatChoiceButton->addChoiceListener(this);
    int numformats = processor.getNumberAudioCodecFormats();
    for (int i=0; i < numformats; ++i) {
        SonobusAudioProcessor::AudioCodecFormatInfo finfo;
        processor.getAudioCodeFormatInfo(i, finfo);
        auto name = finfo.name;
        if (finfo.codec == SonobusAudioProcessor::AudioCodecFormatCodec::CodecOpus && finfo.bitrate < 96000) {
            name += String(" (*)");
        }
        pvf->formatChoiceButton->addItem(name, i);
    }
    pvf->formatChoiceButton->addItem("(*) " + TRANS("not recommended"), -2, true, true);

    pvf->staticFormatChoiceLabel = std::make_unique<Label>("sendfmtst", TRANS("Send Quality"));
    pvf->staticFormatChoiceLabel->setAccessible(false);
    configLabel(pvf->staticFormatChoiceLabel.get(), LabelTypeRegular);


    pvf->remoteSendFormatChoiceButton = std::make_unique<SonoChoiceButton>();
    pvf->remoteSendFormatChoiceButton->addChoiceListener(this);
    pvf->remoteSendFormatChoiceButton->setTitle(TRANS("Preferred Receive Quality"));
    pvf->remoteSendFormatChoiceButton->addItem(TRANS("No Preference"), -1);
    for (int i=0; i < numformats; ++i) {
        pvf->remoteSendFormatChoiceButton->addItem(processor.getAudioCodeFormatName(i), i);
    }

    pvf->staticRemoteSendFormatChoiceLabel = std::make_unique<Label>("recvfmtst", TRANS("Preferred Recv Quality"));
    configLabel(pvf->staticRemoteSendFormatChoiceLabel.get(), LabelTypeRegular);
    pvf->staticRemoteSendFormatChoiceLabel->setAccessible(false);

    pvf->changeAllRecvFormatButton = std::make_unique<ToggleButton>(TRANS("Change all"));
    pvf->changeAllRecvFormatButton->addListener(this);
    pvf->changeAllRecvFormatButton->setLookAndFeel(&pvf->smallLnf);


    pvf->staticLatencyLabel = std::make_unique<Label>("latst", TRANS("Latency (ms)"));
    configLabel(pvf->staticLatencyLabel.get(), LabelTypeSmallDim);
    pvf->staticLatencyLabel->setJustificationType(Justification::centred);
    pvf->staticLatencyLabel->setAccessible(false);

    pvf->staticPingLabel = std::make_unique<Label>("pingst", TRANS("Ping"));
    configLabel(pvf->staticPingLabel.get(), LabelTypeSmallDim);
    pvf->staticPingLabel->setAccessible(false);

    pvf->latencyUpLabel = std::make_unique<Label>("ulat", TRANS("PRESS"));
    configLabel(pvf->latencyUpLabel.get(), LabelTypeSmall);
    pvf->latencyUpLabel->setJustificationType(Justification::left);
    pvf->latencyUpLabel->setAccessible(false);

    pvf->latencyDownLabel = std::make_unique<Label>("dlat", "");
    configLabel(pvf->latencyDownLabel.get(), LabelTypeSmall);
    pvf->latencyDownLabel->setJustificationType(Justification::left);
    pvf->latencyDownLabel->setAccessible(false);

    pvf->pingLabel = std::make_unique<Label>("ping");
    configLabel(pvf->pingLabel.get(), LabelTypeSmall);
    pvf->pingLabel->setAccessible(false);

    pvf->staticSendQualLabel = std::make_unique<Label>("sendqualst", TRANS("Send Quality:"));
    configLabel(pvf->staticSendQualLabel.get(), LabelTypeSmallDim);
    pvf->staticBufferLabel = std::make_unique<Label>("bufst", TRANS("Recv Jitter Buffer:"));
    configLabel(pvf->staticBufferLabel.get(), LabelTypeSmallDim);
    pvf->staticSendQualLabel->setAccessible(false);
    pvf->staticBufferLabel->setAccessible(false);

    pvf->sendQualityLabel = std::make_unique<Label>("qual", "");
    configLabel(pvf->sendQualityLabel.get(), LabelTypeSmall);
    pvf->sendQualityLabel->setJustificationType(Justification::centredLeft);
    pvf->sendQualityLabel->setAccessible(false);

    pvf->bufferLabel = std::make_unique<Label>("buf");
    configLabel(pvf->bufferLabel.get(), LabelTypeSmall);
    pvf->bufferLabel->setJustificationType(Justification::centredLeft);
    pvf->bufferLabel->setAccessible(false);

    
    pvf->sendActualBitrateLabel = std::make_unique<Label>("sbit");
    configLabel(pvf->sendActualBitrateLabel.get(), LabelTypeSmall);
    pvf->sendActualBitrateLabel->setJustificationType(Justification::centredLeft);
    pvf->sendActualBitrateLabel->setMinimumHorizontalScale(0.75);
    pvf->sendActualBitrateLabel->setAccessible(false);

    pvf->recvActualBitrateLabel = std::make_unique<Label>("rbit");
    configLabel(pvf->recvActualBitrateLabel.get(), LabelTypeSmall);
    pvf->recvActualBitrateLabel->setJustificationType(Justification::centredLeft);
    pvf->recvActualBitrateLabel->setMinimumHorizontalScale(0.75);
    pvf->recvActualBitrateLabel->setAccessible(false);

    pvf->sendUpArrow =   Drawable::createFromImageData(BinaryData::arrowupnarrow_svg, BinaryData::arrowupnarrow_svgSize);
    pvf->sendUpArrow->setInterceptsMouseClicks(false, false);

    pvf->recvDownArrow = Drawable::createFromImageData(BinaryData::arrowdownnarrow_svg, BinaryData::arrowdownnarrow_svgSize);
    pvf->recvDownArrow->setInterceptsMouseClicks(false, false);

    pvf->latUpArrow =   pvf->sendUpArrow->createCopy();
    pvf->latUpArrow->setInterceptsMouseClicks(false, false);
    pvf->latDownArrow = pvf->recvDownArrow->createCopy();
    pvf->latDownArrow->setInterceptsMouseClicks(false, false);


    pvf->channelGroups = std::make_unique<ChannelGroupsView>(processor, true);
    pvf->channelGroups->addListener(this);


    pvf->jitterBufferMeter = std::make_unique<JitterBufferMeter>();
    
    pvf->optionsResetDropButton = std::make_unique<TextButton>(TRANS("Reset Dropped"));
    pvf->optionsResetDropButton->addListener(this);
    pvf->optionsResetDropButton->setLookAndFeel(&pvf->medLnf);

    pvf->optionsRemoveButton = std::make_unique<TextButton>(TRANS("Remove"));
    pvf->optionsRemoveButton->addListener(this);
    pvf->optionsRemoveButton->setLookAndFeel(&pvf->smallLnf);
    pvf->optionsRemoveButton->setTooltip(TRANS("Removes user from your own connections, does not affect the whole group"));

    pvf->optionsBlockButton = std::make_unique<TextButton>(TRANS("BLOCK"));
    pvf->optionsBlockButton->addListener(this);
    pvf->optionsBlockButton->setLookAndFeel(&pvf->smallLnf);
    pvf->optionsBlockButton->setTooltip(TRANS("BLOCK any connections from this users IP address for yourself, does not affect other users"));

    
    // meters
    auto flags = foleys::LevelMeter::Minimal;
    
    pvf->recvMeter = std::make_unique<foleys::LevelMeter>(flags);
    pvf->recvMeter->setLookAndFeel(&(pvf->rmeterLnf));
    pvf->recvMeter->setRefreshRateHz(0); // XXX
    pvf->recvMeter->addMouseListener(this, false);


    pvf->sendOptionsContainer = std::make_unique<Component>();
    pvf->sendOptionsContainer->setFocusContainerType(FocusContainerType::focusContainer);

    pvf->recvOptionsContainer = std::make_unique<Component>();
    pvf->recvOptionsContainer->setFocusContainerType(FocusContainerType::focusContainer);

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

    pvf->fullMode = peerModeFull;

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

    pvf->removeButton = std::make_unique<TextButton>(TRANS("Remove"));
    pvf->removeButton->addListener(this);
    pvf->removeButton->setTooltip(TRANS("Removes pending user from list"));

    pvf->unblockButton = std::make_unique<TextButton>(TRANS("Unblock"));
    pvf->unblockButton->addListener(this);
    pvf->unblockButton->setTooltip(TRANS("Unblocks address from blocked list"));

    return pvf;
}

void PeersContainerView::channelLayoutChanged(ChannelGroupsView *comp)
{
    updateLayout();

    listeners.call (&PeersContainerView::Listener::internalSizesChanged, this);

    resized();
}

void PeersContainerView::nameLabelClicked(ChannelGroupsView *comp)
{
    if (mIgnoreNameClick) return;

    // toggle full mode
    for (int i=0; i < mPeerViews.size(); ++i) {
        PeerViewInfo * pvf = mPeerViews.getUnchecked(i);
        if (pvf->channelGroups.get() == comp) {
            pvf->fullMode = !pvf->fullMode;
            break;
        }
    }

    rebuildPeerViews();

    listeners.call (&PeersContainerView::Listener::internalSizesChanged, this);
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

void PeersContainerView::peerBlockedJoin(String & group, String & user, String & address, int port)
{
    auto found = mPendingUsers.find(user);
    if (found != mPendingUsers.end()) {
        found->second.failed = true;
        found->second.blocked = true;
        found->second.address = address;
        found->second.port = port;
        updatePeerViews();
    }    
}

void PeersContainerView::peerLeftGroup(String & group, String & user)
{
    // check if it was pending and remove it
    auto found = mPendingUsers.find(user);
    if (found != mPendingUsers.end()) {
        auto & peer = found->second;
        mPendingUsers.erase(found);
    }
}


void PeersContainerView::setPeerDisplayMode(SonobusAudioProcessor::PeerDisplayMode mode)
{
    // set full mode for all
    for (int i=0; i < mPeerViews.size(); ++i) {
        PeerViewInfo * pvf = mPeerViews.getUnchecked(i);

        pvf->fullMode = (mode == SonobusAudioProcessor::PeerDisplayModeFull);
    }

    peerModeFull = (mode == SonobusAudioProcessor::PeerDisplayModeFull);

    rebuildPeerViews();
    listeners.call (&PeersContainerView::Listener::internalSizesChanged, this);
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

    bool anyfull = false;

    for (int i=0; i < mPeerViews.size(); ++i) {
        PeerViewInfo * pvf = mPeerViews.getUnchecked(i);
        String username = processor.getRemotePeerUserName(i);
        // remove from pending if necessary
        mPendingUsers.erase(username);

        int prio = processor.getRemotePeerOrderPriority(i);
        if (prio >= 0) {
            mPeerPriorityOrdering[username] = prio;
        }


        pvf->channelGroups->getAudioDeviceManager = getAudioDeviceManager;

        pvf->addAndMakeVisible(pvf->channelGroups.get());
        pvf->channelGroups->setPeerMode(true, i);
        pvf->channelGroups->setNarrowMode(isNarrow);
        pvf->channelGroups->rebuildChannelViews();

        pvf->channelGroups->addMouseListener(this, true);
        
        pvf->addAndMakeVisible(pvf->sendStatsBg.get());
        pvf->addAndMakeVisible(pvf->recvStatsBg.get());
        pvf->addAndMakeVisible(pvf->pingBg.get());
        //pvf->addAndMakeVisible(pvf->sendMutedButton.get());
        pvf->addAndMakeVisible(pvf->recvMutedButton.get());
        pvf->addAndMakeVisible(pvf->recvSoloButton.get());
        pvf->addAndMakeVisible(pvf->latActiveButton.get());


        pvf->addAndMakeVisible(pvf->sendOptionsButton.get());
        pvf->addAndMakeVisible(pvf->recvOptionsButton.get());
        pvf->addAndMakeVisible(pvf->staticLatencyLabel.get());
        pvf->addAndMakeVisible(pvf->staticPingLabel.get());
        pvf->addAndMakeVisible(pvf->latencyUpLabel.get());
        pvf->addAndMakeVisible(pvf->latencyDownLabel.get());
        pvf->addAndMakeVisible(pvf->pingLabel.get());

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
        pvf->recvOptionsContainer->addAndMakeVisible(pvf->bufferTimeLabel.get());
        pvf->recvOptionsContainer->addAndMakeVisible(pvf->bufferTimeSlider.get());
        pvf->recvOptionsContainer->addAndMakeVisible(pvf->bufferMinButton.get());
        pvf->recvOptionsContainer->addAndMakeVisible(pvf->optionsResetDropButton.get());
        pvf->recvOptionsContainer->addAndMakeVisible(pvf->remoteSendFormatChoiceButton.get());
        pvf->recvOptionsContainer->addAndMakeVisible(pvf->staticRemoteSendFormatChoiceLabel.get());
        pvf->recvOptionsContainer->addAndMakeVisible(pvf->changeAllRecvFormatButton.get());

        pvf->sendOptionsContainer->addAndMakeVisible(pvf->formatChoiceButton.get());
        pvf->sendOptionsContainer->addAndMakeVisible(pvf->changeAllFormatButton.get());
        pvf->sendOptionsContainer->addAndMakeVisible(pvf->staticFormatChoiceLabel.get());
        pvf->sendOptionsContainer->addAndMakeVisible(pvf->sendMutedButton.get());
        pvf->sendOptionsContainer->addAndMakeVisible(pvf->optionsRemoveButton.get());
        pvf->sendOptionsContainer->addAndMakeVisible(pvf->optionsBlockButton.get());

        //pvf->addAndMakeVisible(pvf->recvMeter.get());
        pvf->addAndMakeVisible(pvf->sendActualBitrateLabel.get());
        pvf->addAndMakeVisible(pvf->recvActualBitrateLabel.get());

        pvf->addAndMakeVisible(pvf->sendUpArrow.get());
        pvf->addAndMakeVisible(pvf->recvDownArrow.get());
        pvf->addAndMakeVisible(pvf->latUpArrow.get());
        pvf->addAndMakeVisible(pvf->latDownArrow.get());

        auto fullmode = pvf->fullMode;

        // visibility based on peerdisplay mode
        pvf->sendQualityLabel->setVisible(fullmode);
        pvf->bufferLabel->setVisible(fullmode);
        pvf->bufferMinFrontButton->setVisible(fullmode);
        pvf->latencyUpLabel->setVisible(fullmode);
        pvf->latencyDownLabel->setVisible(fullmode);
        pvf->latActiveButton->setVisible(fullmode);
        pvf->sendActualBitrateLabel->setVisible(fullmode);
        pvf->recvActualBitrateLabel->setVisible(fullmode);
        pvf->sendUpArrow->setVisible(fullmode);
        pvf->recvDownArrow->setVisible(fullmode);
        pvf->latUpArrow->setVisible(fullmode);
        pvf->latDownArrow->setVisible(fullmode);
        pvf->recvStatsBg->setVisible(fullmode);
        pvf->sendStatsBg->setVisible(fullmode);
        pvf->staticSendQualLabel->setVisible(fullmode);
        pvf->staticLatencyLabel->setVisible(fullmode);
        pvf->staticPingLabel->setVisible(fullmode);
        pvf->staticBufferLabel->setVisible(fullmode);
        pvf->pingBg->setVisible(fullmode);
        pvf->recvButtonImage->setVisible(fullmode);
        pvf->sendButtonImage->setVisible(fullmode);
        pvf->jitterBufferMeter->setVisible(fullmode);
        pvf->pingLabel->setVisible(fullmode);
        pvf->sendOptionsButton->setVisible(fullmode);
        pvf->recvOptionsButton->setVisible(fullmode);
        pvf->latActiveButton->setVisible(fullmode);

        if (fullmode) {
            anyfull = true;
        }

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
        ppvf->addChildComponent(ppvf->removeButton.get());
        ppvf->addChildComponent(ppvf->unblockButton.get());
        addAndMakeVisible(ppvf);
    }
    
    if (mPeerViews.size() > 0 && anyfull) {
        startTimer(FillRatioUpdateTimerId, 100);
    } else {
        stopTimer(FillRatioUpdateTimerId);
    }

    updatePeerOrdering();

    updatePeerViews();
    updateLayout();
    resized();
}

void PeersContainerView::updatePeerOrdering()
{
    mPeerUpdateOrdering.clear();

    std::set<int> addedindexes;
    std::map<int,int> priorityIndexes; // key is priority, value is peer index

    for (int i=0; i < processor.getNumberRemotePeers(); ++i) {
        String username = processor.getRemotePeerUserName(i);

        auto found = mPeerPriorityOrdering.find(username);
        if (found != mPeerPriorityOrdering.end()) {
            priorityIndexes[found->second] = i;
        }
    }

    // add the priority ordered ones first to the update order
    for (auto iter : priorityIndexes) {
        mPeerUpdateOrdering.push_back(iter.second);
        addedindexes.insert(iter.second);
    }

    // add the rest
    for (int i=0; i < processor.getNumberRemotePeers(); ++i) {
        if (addedindexes.find(i) == addedindexes.end()) {
            mPeerUpdateOrdering.push_back(i);
        }
    }

}

void PeersContainerView::updateLayout()
{
    int minitemheight = 36;
    int mincheckheight = 32;
    int minPannerWidth = 58;
    int minButtonWidth = 90;
    int maxPannerWidth = 110;
    int minSmallButtonWidth = 55;

    int mutebuttwidth = 52;
    
#if JUCE_IOS || JUCE_ANDROID
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


    // Main connected peer views

    for (int i=0; i < mPeerViews.size(); ++i) {
        PeerViewInfo * pvf = mPeerViews.getUnchecked(i);

        //pvf->updateLayout();
        
        const int ph = singleph;
        const int pw = 200;
        
        auto fullmode = pvf->fullMode;

        
       
        
        pvf->sendmeterbox.items.clear();
        pvf->sendmeterbox.flexDirection = FlexBox::Direction::row;

        pvf->pingbox.items.clear();
        pvf->pingbox.flexDirection = FlexBox::Direction::column;
        pvf->pingbox.items.add(FlexItem(20, textheight, *pvf->staticPingLabel).withMargin(0).withFlex(0.5));
        pvf->pingbox.items.add(FlexItem(20, textheight, *pvf->pingLabel).withMargin(0).withFlex(0.5));

        pvf->latencylabbox.items.clear();
        pvf->latencylabbox.flexDirection = FlexBox::Direction::row;
        pvf->latencylabbox.items.add(FlexItem(11, textheight).withFlex(0.2).withMargin(0));
        pvf->latencylabbox.items.add(FlexItem(25, textheight, *pvf->latencyUpLabel).withMargin(0).withFlex(1));
        pvf->latencylabbox.items.add(FlexItem(14, textheight).withFlex(0.0).withMargin(0));
        pvf->latencylabbox.items.add(FlexItem(25, textheight, *pvf->latencyDownLabel).withMargin(0).withFlex(1));

        pvf->latencybox.items.clear();
        pvf->latencybox.flexDirection = FlexBox::Direction::column;
        pvf->latencybox.items.add(FlexItem(60, textheight, *pvf->staticLatencyLabel).withMargin(0).withFlex(0.5));
        pvf->latencybox.items.add(FlexItem(60, textheight, pvf->latencylabbox).withMargin(0).withFlex(0.5));

        
        pvf->netstatbox.items.clear();
        pvf->netstatbox.flexDirection = FlexBox::Direction::row;
        pvf->netstatbox.items.add(FlexItem(60, textheight, pvf->latencybox).withMargin(0).withFlex(3));
        pvf->netstatbox.items.add(FlexItem(20, textheight, pvf->pingbox).withMargin(0).withFlex(1));


        if (isNarrow) {
            pvf->squalbox.items.clear();
            pvf->squalbox.flexDirection = FlexBox::Direction::column;
            pvf->squalbox.items.add(FlexItem(60, textheight, *pvf->staticSendQualLabel).withMargin(0).withFlex(0));
            pvf->squalbox.items.add(FlexItem(60, textheight, *pvf->sendQualityLabel).withMargin(0).withFlex(0));
            pvf->staticSendQualLabel->setJustificationType(Justification::centred);
            pvf->sendQualityLabel->setJustificationType(Justification::centred);


            pvf->netbufbox.items.clear();
            pvf->netbufbox.flexDirection = FlexBox::Direction::column;
            //pvf->netbufbox.items.add(FlexItem(0, 10).withFlex(1));
            pvf->netbufbox.items.add(FlexItem(60, textheight, *pvf->staticBufferLabel).withMargin(0).withFlex(0)/*.withMaxWidth(105)*/);
            pvf->netbufbox.items.add(FlexItem(60, textheight, *pvf->bufferLabel).withMargin(0).withFlex(0)/*.withMaxWidth(116)*/);
            //pvf->netbufbox.items.add(FlexItem(0, 10).withFlex(1.5));
            pvf->staticBufferLabel->setJustificationType(Justification::centred);


            pvf->optionsstatbox.items.clear();
            pvf->optionsstatbox.flexDirection = FlexBox::Direction::column;
            pvf->optionsstatbox.items.add(FlexItem(70, 2*textheight, pvf->squalbox).withMargin(0).withFlex(0));
            //pvf->optionsstatbox.items.add(FlexItem(76, textheight, *pvf->sendActualBitrateLabel).withMargin(0).withFlex(0));
            pvf->sendActualBitrateLabel->setVisible(false);
            pvf->sendUpArrow->setVisible(false);


            pvf->recvstatbox.items.clear();
            pvf->recvstatbox.flexDirection = FlexBox::Direction::row;
            pvf->recvstatbox.items.add(FlexItem(2, 10).withFlex(0));
            pvf->recvstatbox.items.add(FlexItem(60, 2*textheight, pvf->netbufbox).withMargin(0).withFlex(1));
            pvf->recvstatbox.items.add(FlexItem(2, 10).withFlex(0));
            //pvf->recvstatbox.items.add(FlexItem(100, textheight, *pvf->recvActualBitrateLabel).withMargin(0).withFlex(0));
            pvf->recvActualBitrateLabel->setVisible(false);
            pvf->recvDownArrow->setVisible(false);

        }
        else {
            pvf->squalbox.items.clear();
            pvf->squalbox.flexDirection = FlexBox::Direction::row;
            pvf->squalbox.items.add(FlexItem(60, textheight, *pvf->staticSendQualLabel).withMargin(0).withFlex(1));
            pvf->squalbox.items.add(FlexItem(40, textheight, *pvf->sendQualityLabel).withMargin(0).withFlex(2));
            pvf->staticSendQualLabel->setJustificationType(Justification::centredRight);
            pvf->sendQualityLabel->setJustificationType(Justification::centredLeft);

            pvf->netbufbox.items.clear();
            pvf->netbufbox.flexDirection = FlexBox::Direction::row;
            pvf->netbufbox.items.add(FlexItem(0, 10).withFlex(1));
            pvf->netbufbox.items.add(FlexItem(88, textheight, *pvf->staticBufferLabel).withMargin(0).withFlex(1).withMaxWidth(105));
            pvf->netbufbox.items.add(FlexItem(40, textheight, *pvf->bufferLabel).withMargin(0).withFlex(3).withMaxWidth(116));
            pvf->netbufbox.items.add(FlexItem(0, 10).withFlex(1.5));
            pvf->staticBufferLabel->setJustificationType(Justification::centredRight);


            pvf->sendlabbox.items.clear();
            pvf->sendlabbox.flexDirection = FlexBox::Direction::row;
            pvf->sendlabbox.items.add(FlexItem(15, 10).withFlex(1));
            pvf->sendlabbox.items.add(FlexItem(76, textheight, *pvf->sendActualBitrateLabel).withMargin(0).withFlex(1));
            pvf->sendlabbox.items.add(FlexItem(0, 10).withFlex(1));

            pvf->recvlabbox.items.clear();
            pvf->recvlabbox.flexDirection = FlexBox::Direction::row;
            pvf->recvlabbox.items.add(FlexItem(15, 10).withFlex(1));
            pvf->recvlabbox.items.add(FlexItem(100, textheight, *pvf->recvActualBitrateLabel).withMargin(0).withFlex(1));
            pvf->recvlabbox.items.add(FlexItem(0, 10).withFlex(1));


            pvf->optionsstatbox.items.clear();
            pvf->optionsstatbox.flexDirection = FlexBox::Direction::column;
            pvf->optionsstatbox.items.add(FlexItem(100, textheight, pvf->squalbox).withMargin(0).withFlex(0));
            //pvf->optionsstatbox.items.add(FlexItem(76, textheight, *pvf->sendActualBitrateLabel).withMargin(0).withFlex(0));
            pvf->optionsstatbox.items.add(FlexItem(90, textheight, pvf->sendlabbox).withMargin(0).withFlex(0));
            pvf->sendActualBitrateLabel->setVisible(true);
            pvf->sendUpArrow->setVisible(true);


            pvf->recvstatbox.items.clear();
            pvf->recvstatbox.flexDirection = FlexBox::Direction::column;
            pvf->recvstatbox.items.add(FlexItem(100, textheight, pvf->netbufbox).withMargin(0).withFlex(0));
            pvf->recvstatbox.items.add(FlexItem(115, textheight, pvf->recvlabbox).withMargin(0).withFlex(0));
            // pvf->recvstatbox.items.add(FlexItem(100, textheight, *pvf->recvActualBitrateLabel).withMargin(0).withFlex(0));
            pvf->recvActualBitrateLabel->setVisible(true);
            pvf->recvDownArrow->setVisible(true);

        }

        
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
        pvf->optionsRemoteQualityBox.items.add(FlexItem(80, minitemheight, *pvf->staticRemoteSendFormatChoiceLabel).withMargin(0).withFlex(0));
        pvf->optionsRemoteQualityBox.items.add(FlexItem(minButtonWidth, minitemheight, *pvf->remoteSendFormatChoiceButton).withMargin(0).withFlex(2));
        pvf->optionsRemoteQualityBox.items.add(FlexItem(68, minitemheight, *pvf->changeAllRecvFormatButton).withMargin(0).withFlex(0));

        
        pvf->optionsSendMutedBox.items.clear();
        pvf->optionsSendMutedBox.flexDirection = FlexBox::Direction::row;
        pvf->optionsSendMutedBox.items.add(FlexItem(5, 12));
        pvf->optionsSendMutedBox.items.add(FlexItem(100, minitemheight, *pvf->sendMutedButton).withMargin(0).withFlex(1));
        pvf->optionsSendMutedBox.items.add(FlexItem(3, 12));
        pvf->optionsSendMutedBox.items.add(FlexItem(minSmallButtonWidth, minitemheight, *pvf->optionsRemoveButton).withMargin(0).withFlex(0));
        pvf->optionsSendMutedBox.items.add(FlexItem(3, 12));
        pvf->optionsSendMutedBox.items.add(FlexItem(minSmallButtonWidth, minitemheight, *pvf->optionsBlockButton).withMargin(0).withFlex(0));

        
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


        pvf->recvbox.items.clear();
        pvf->recvbox.flexDirection = FlexBox::Direction::row;

        pvf->sendbox.items.clear();
        pvf->sendbox.flexDirection = FlexBox::Direction::row;

        if (isNarrow) {

            /*
            pvf->recvbox.items.add(FlexItem(1, 5));
            pvf->recvbox.items.add(FlexItem(60, minitemheight, pvf->optionsstatbox).withMargin(0).withFlex(1));
            pvf->recvbox.items.add(FlexItem(3, 5));
            pvf->recvbox.items.add(FlexItem(100, minitemheight, pvf->netstatbox).withMargin(0).withFlex(1).withMaxWidth(130));
            pvf->recvbox.items.add(FlexItem(3, 5));

            pvf->sendbox.items.add(FlexItem(5, 2).withFlex(0));
            pvf->sendbox.items.add(FlexItem(100, minitemheight, pvf->recvstatbox).withMargin(0).withFlex(1));
            pvf->sendbox.items.add(FlexItem(3, 5));
             */

            pvf->recvbox.items.add(FlexItem(1, 5));
            pvf->recvbox.items.add(FlexItem(40, minitemheight, pvf->optionsstatbox).withMargin(0).withFlex(1).withMaxWidth(300));
            pvf->recvbox.items.add(FlexItem(3, 5));
            pvf->recvbox.items.add(FlexItem(80, minitemheight, pvf->recvstatbox).withMargin(0).withFlex(1));
            pvf->recvbox.items.add(FlexItem(5, 5));
            pvf->recvbox.items.add(FlexItem(60, minitemheight, pvf->netstatbox).withMargin(0).withFlex(1).withMaxWidth(130));
            pvf->recvbox.items.add(FlexItem(3, 2).withFlex(0));

        }
        else {
            pvf->recvbox.items.add(FlexItem(1, 5));
            pvf->recvbox.items.add(FlexItem(60, minitemheight, pvf->optionsstatbox).withMargin(0).withFlex(1).withMaxWidth(300));
            pvf->recvbox.items.add(FlexItem(3, 5));
            pvf->recvbox.items.add(FlexItem(100, minitemheight, pvf->recvstatbox).withMargin(0).withFlex(1));
            pvf->recvbox.items.add(FlexItem(5, 5));
            pvf->recvbox.items.add(FlexItem(100, minitemheight, pvf->netstatbox).withMargin(0).withFlex(1).withMaxWidth(130));
            pvf->recvbox.items.add(FlexItem(3, 2).withFlex(0));
        }



        int meterwidth = 22;

        int chcnt = processor.getRemotePeerRecvChannelCount(i);
        if (chcnt > 2) {
            meterwidth = 5 * chcnt;
        }

        pvf->mainbox.items.clear();
        pvf->mainnarrowbox.items.clear();

        pvf->isNarrow = isNarrow;

        pvf->channelGroups->setNarrowMode(isNarrow, false);
        pvf->channelGroups->updateLayout(false);

        auto changroupminbounds = pvf->channelGroups->getMinimumContentBounds();


        if (isNarrow) {
            pvf->mainnarrowbox.flexDirection = FlexBox::Direction::column;
            pvf->mainnarrowbox.items.add(FlexItem(2, 3));
            //pvf->mainnarrowbox.items.add(FlexItem(100, minitemheight, pvf->mainsendbox).withMargin(0).withFlex(0));
            pvf->mainnarrowbox.items.add(FlexItem(changroupminbounds.getWidth(), changroupminbounds.getHeight(), *pvf->channelGroups).withMargin(0).withFlex(0));

            if (fullmode) {
                pvf->mainnarrowbox.items.add(FlexItem(2, 2));
                pvf->mainnarrowbox.items.add(FlexItem(260, minitemheight, pvf->recvbox).withMargin(0).withFlex(0));
                //pvf->mainnarrowbox.items.add(FlexItem(2, 4));
                //pvf->mainnarrowbox.items.add(FlexItem(260, minitemheight, pvf->sendbox).withMargin(0).withFlex(0));
            }

            int nbh = 0;
            for (auto & item : pvf->mainnarrowbox.items) {
                nbh += item.minHeight;
            }

            pvf->mainbox.flexDirection = FlexBox::Direction::row;
            pvf->mainbox.items.add(FlexItem(3, 2));
            pvf->mainbox.items.add(FlexItem(150, nbh, pvf->mainnarrowbox).withMargin(0).withFlex(1));
            //pvf->mainbox.items.add(FlexItem(2, 2));
            //pvf->mainbox.items.add(FlexItem(meterwidth, 50, *pvf->recvMeter).withMargin(3).withFlex(0));
        } else {
            pvf->mainnarrowbox.flexDirection = FlexBox::Direction::column;
            //pvf->mainnarrowbox.items.add(FlexItem(2, 3));

            //pvf->mainnarrowbox.items.add(FlexItem(100, minitemheight, pvf->mainsendbox).withMargin(0).withFlex(0));
            pvf->mainnarrowbox.items.add(FlexItem(changroupminbounds.getWidth(), changroupminbounds.getHeight(), *pvf->channelGroups).withMargin(0).withFlex(0));

            if (fullmode) {
                pvf->mainnarrowbox.items.add(FlexItem(2, 4));
                pvf->mainnarrowbox.items.add(FlexItem(150, minitemheight, pvf->recvbox).withMargin(0).withFlex(0));
            }

            int nbh = 0;
            for (auto & item : pvf->mainnarrowbox.items) {
                nbh += item.minHeight;
            }

            pvf->mainbox.flexDirection = FlexBox::Direction::row;
            pvf->mainbox.items.add(FlexItem(3, 2));
            pvf->mainbox.items.add(FlexItem(150, nbh, pvf->mainnarrowbox).withMargin(0).withFlex(1));
            //pvf->mainbox.items.add(FlexItem(2, 2));
            //pvf->mainbox.items.add(FlexItem(meterwidth, 50, *pvf->recvMeter).withMargin(3).withFlex(0));
        }

        peersBox.items.add(FlexItem(8, 4).withMargin(0));
        peersheight += 4;

        int mch = 0;
        for (auto & item : pvf->mainbox.items) {
            mch = jmax(mch, (int)item.minHeight);
        }

        if (isNarrow) {
            peersBox.items.add(FlexItem(pw, mch + 3, *pvf).withMargin(0).withFlex(0));
            peersheight += mch + 3;
        } else {
            peersBox.items.add(FlexItem(pw, mch + 5, *pvf).withMargin(1).withFlex(0));
            peersheight += mch + 5;
        }

    }


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
        ppvf->mainbox.items.add(FlexItem(minButtonWidth-14, minitemheight, *ppvf->removeButton).withMargin(3).withFlex(0));
        ppvf->mainbox.items.add(FlexItem(110, minitemheight, *ppvf->messageLabel).withMargin(0).withFlex(1));
        ppvf->mainbox.items.add(FlexItem(minButtonWidth-14, minitemheight, *ppvf->unblockButton).withMargin(3).withFlex(0));

        peersBox.items.add(FlexItem(ppw, ppheight + 5, *ppvf).withMargin(1).withFlex(0));
        peersheight += ppheight + 5;
    }


    // get total min height on peersbox
    int totPeersHeight = 0;
    for (auto & item : peersBox.items) {
        totPeersHeight += item.minHeight + item.margin.top + item.margin.bottom;
    }

    if (isNarrow) {
        peersMinHeight = totPeersHeight;
        peersMinWidth = 300;
    }
    else {
        peersMinHeight = totPeersHeight;
        peersMinWidth = 180 + 150 + mincheckheight + 50;
    }
    
}


int PeersContainerView::getPeerFromIndex(int index)
{
    if (index >= 0 && index < mPeerUpdateOrdering.size()) {

        return mPeerUpdateOrdering[index];
    }

    return 0;
}

int PeersContainerView::getPeerForPoint(Point<int> pos, bool inbetween)
{
    int i=0;
    for (; i < mPeerViewBounds.size(); ++i) {
        auto bounds = mPeerViewBounds.getUnchecked(i);

        if (inbetween) {
            // round it from midpoints
            auto tophalf = bounds.withTrimmedBottom(bounds.getHeight()/2);
            auto bottomhalf = bounds.withTrimmedTop(bounds.getHeight()/2);
            if (tophalf.contains(pos) || pos.getY() < bounds.getY()) {
                return i;
            }
            else if (bottomhalf.contains(pos)) {
                return i+1;
            }
        }
        else {
            if (bounds.contains(pos)) {
                return i;
            }
            if (pos.getY() < bounds.getY()) {
                // return one less
                return i-1;
            }
        }
    }

    return i;
}

juce::Rectangle<int> PeersContainerView::getBoundsForPeer(int chgroup)
{
    if (chgroup >= 0 && chgroup < mPeerViewBounds.size()) {
        return mPeerViewBounds.getUnchecked(chgroup);
    }
    // otherwise return a line after the last of them
    if (!mPeerViewBounds.isEmpty()) {
        auto lastone = mPeerViewBounds.getLast();
        return Rectangle<int>(lastone.getX(), lastone.getBottom(), lastone.getWidth(), 0);
    }
    return {};
}



void PeersContainerView::mouseDown (const MouseEvent& event)
{
    mIgnoreNameClick = false;

    for (int i=0; i < mPeerViews.size(); ++i) {
        auto * pvf = mPeerViews.getUnchecked(i);

        if ( pvf->channelGroups->isDraggable(event.eventComponent)) {
            DBG("Mouse down on peer " << i);
            //int peerindex = mPeerUpdateOrdering[i];
            mDraggingSourcePeer = i; // peerindex;
            break;
        }
    }
}

void PeersContainerView::mouseDrag (const MouseEvent& event)
{
    for (int i=0; i < mPeerViews.size(); ++i) {
        auto * pvf = mPeerViews.getUnchecked(i);

        if ( (pvf->channelGroups->isDraggable(event.eventComponent)
              )) {
            auto adjpos =  getLocalPoint(event.eventComponent, event.getPosition());
            //int peerindex = mPeerUpdateOrdering[i];

            DBG("Dragging peer view: " << adjpos.toString());
            if (abs(event.getDistanceFromDragStartY()) > 4 && !mDraggingActive) {
                // start drag behavior
                mDraggingSourcePeer = i; // peerindex;
                mDraggingActive = true;
                mIgnoreNameClick = true;
                mDraggingGroupPos = getPeerForPoint(adjpos, true);
                auto groupbounds = getBoundsForPeer(mDraggingSourcePeer);
                mDragImage = createComponentSnapshot(groupbounds);
                mDragDrawable->setImage(mDragImage);
                mDragDrawable->setVisible(true);
                mDragDrawable->setBounds(groupbounds.getX(), adjpos.getY() - groupbounds.getHeight()/2, groupbounds.getWidth(), groupbounds.getHeight());
            }
            else if (mDraggingActive) {
                // adjust drag indicator
                int pindex = getPeerForPoint(adjpos, true);
                DBG("In peer: " << pindex);

                mDragDrawable->setBounds(mDragDrawable->getX(), adjpos.getY() - mDragDrawable->getHeight()/2, mDragDrawable->getWidth(), mDragDrawable->getHeight());

                if (auto viewport = findParentComponentOfClass<Viewport>()) {
                    auto vppos = viewport->getLocalPoint(this, adjpos);
                    if (viewport->autoScroll(vppos.getX(), vppos.getY(), 8, 8)) {
                        if (!mAutoscrolling) {
                            event.eventComponent->beginDragAutoRepeat(40);
                            mAutoscrolling = true;
                        }
                    } else if (mAutoscrolling){
                        event.eventComponent->beginDragAutoRepeat(0);
                        mAutoscrolling = false;
                    }
                }

                if (pindex != mDraggingGroupPos) {
                    // insert point changed, update it
                    mDraggingGroupPos = pindex;

                    auto groupbounds = getBoundsForPeer(mDraggingGroupPos);
                    groupbounds.setHeight(0);
                    groupbounds.setWidth(getWidth() - 16);
                    groupbounds.setX(7);
                    mInsertLine->setRectangle (groupbounds.toFloat());

                    int delta = mDraggingGroupPos - mDraggingSourcePeer;
                    bool canmove = delta > 1 || delta < 0;
                    mInsertLine->setVisible(canmove);
                }
            }
            break;
        }
    }
}

void PeersContainerView::mouseUp (const MouseEvent& event)
{
    for (int i=0; i < mPeerViews.size(); ++i) {
        auto * pvf = mPeerViews.getUnchecked(i);

        if (event.eventComponent == pvf->recvMeter.get()) {
            pvf->recvMeter->clearClipIndicator(-1);
            pvf->channelGroups->clearClipIndicators();
            break;
        }
        else if (event.eventComponent == pvf->addrLabel.get()) {
            pvf->addrClicked = true;
            updatePeerViews(i);
            break;
        }
        else if (pvf->channelGroups->isDraggable(event.eventComponent)) {
            if (mDraggingActive) {
                DBG("Mouse up after drag: " << event.getPosition().toString() << " srcpeer: " << mDraggingSourcePeer << " destpos: " << mDraggingGroupPos);
                // commit it
                int delta = mDraggingGroupPos - mDraggingSourcePeer;
                bool canmove = delta > 1 || delta < 0;


                int srcpeer = mPeerUpdateOrdering[mDraggingSourcePeer];

                // need to set priority ordering on all them
                int offs = 0;

                for (int j=0; j < mPeerViews.size(); ++j) {
                    if (j == mDraggingSourcePeer) continue;

                    int peerind = mPeerUpdateOrdering[j];
                    if (j == mDraggingGroupPos) {
                        offs = 1;
                    }

                    String pname = processor.getRemotePeerUserName(peerind);
                    if (pname.isNotEmpty()) {
                        mPeerPriorityOrdering[pname] = j + offs;
                        processor.setRemotePeerOrderPriority(peerind, j+offs);
                        DBG("Setting drag prior for " << pname << " to: " << j + offs);
                    }
                }

                String pname = processor.getRemotePeerUserName(srcpeer);
                if (pname.isNotEmpty()) {
                    mPeerPriorityOrdering[pname] = mDraggingGroupPos;
                    processor.setRemotePeerOrderPriority(srcpeer, mDraggingGroupPos);
                    DBG("Setting drag src for " << pname << " to: " << mDraggingGroupPos);
                }


                updatePeerOrdering();
                updatePeerViews();

                //if (canmove && processor.moveInputChannelGroupTo(mDraggingSourceGroup, mDraggingGroupPos)) {
                //    // moved it
                //    processor.updateRemotePeerUserFormat();
                //    rebuildChannelViews();
                //}

                mInsertLine->setVisible(false);
                mDragDrawable->setVisible(false);
                mDraggingActive = false;
                mAutoscrolling = false;
            }

            break;
        }
    }
}




Rectangle<int> PeersContainerView::getMinimumContentBounds() const
{
    return Rectangle<int>(0,0,peersMinWidth, peersMinHeight);
}

void PeersContainerView::applyToAllSliders(std::function<void(Slider *)> & routine)
{
    for (int i=0; i < mPeerViews.size(); ++i) {
        PeerViewInfo * pvf = mPeerViews.getUnchecked(i);
        pvf->channelGroups->applyToAllSliders(routine);
    }
}

void PeersContainerView::updatePeerViews(int specific)
{
    uint32 nowstampms = Time::getMillisecondCounter();
    bool needsUpdateLayout = false;


    //    for (int i=0; i < mPeerViews.size(); ++i) {
    for (int di=0; di < mPeerUpdateOrdering.size(); ++di) {
        int i = mPeerUpdateOrdering[di];

        if (specific >= 0 && specific != i) continue;
        if (di >= mPeerViews.size()) {
            DBG("Shouldnt happen update ordering greater than peerview size!");
            break;
        }
        PeerViewInfo * pvf = mPeerViews.getUnchecked(di);

        pvf->channelGroups->setPeerMode(true, i);

        bool connected = processor.getRemotePeerConnected(i);
        auto fullmode = pvf->fullMode;

        String hostname;
        int port = 0;
        processor.getRemotePeerAddressInfo(i, hostname, port);

        String addrname;
        addrname << hostname << " : " << port;

        if (pvf->addrClicked) {
            pvf->addrLabel->setText(addrname, dontSendNotification);
        } else {
            pvf->addrLabel->setText(TRANS("<Press to show>"), dontSendNotification);
        }

        String sendtext;
        bool sendactive = processor.getRemotePeerSendActive(i);
        bool sendallow = processor.getRemotePeerSendAllow(i);

        bool recvactive = processor.getRemotePeerRecvActive(i);
        bool recvallow = processor.getRemotePeerRecvAllow(i);
        bool latactive = processor.isRemotePeerLatencyTestActive(i);
        bool safetymuted = processor.getRemotePeerSafetyMuted(i);
        bool blocked = processor.getRemotePeerBlockedUs(i);

        const int chcnt = processor.getRemotePeerRecvChannelCount(i);

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

        if (blocked) {
            sendtext += TRANS("Other end BLOCKED us");
            pvf->sendActualBitrateLabel->setColour(Label::textColourId, mutedTextColor);
        }
        else if (sendactive) {
            //sendtext += String::formatted("%Ld sent", processor.getRemotePeerPacketsSent(i) );
            //sendtext << String(juce::CharPointer_UTF8 ("\xe2\x86\x91")); // up arrow
            sendtext << String::formatted(" %.d kb/s", lrintf(sendrate * 8 * 1e-3) );
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
            //recvtext << String(juce::CharPointer_UTF8 ("\xe2\x86\x93 ")) // down arrow
            recvtext << chcnt << "ch "
            << recvfinfo.name
            << String::formatted(" | %d kb/s", lrintf(recvrate * 8 * 1e-3));

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

        pvf->changeAllRecvFormatButton->setToggleState(processor.getChangingDefaultRecvAudioCodecSetsExisting(), dontSendNotification);

        pvf->sendActualBitrateLabel->setText(sendtext, dontSendNotification);
        pvf->recvActualBitrateLabel->setText(recvtext, dontSendNotification);

        SonobusAudioProcessor::LatencyInfo latinfo;
        processor.getRemotePeerLatencyInfo(i, latinfo);
        
        //pvf->pingLabel->setText(String::formatted("%d ms", (int)latinfo.pingMs ), dontSendNotification);
        pvf->pingLabel->setText(String::formatted("%d", (int)lrintf(latinfo.pingMs) ), dontSendNotification);
        
        if (latinfo.legacy && !latinfo.isreal) {
            if (pvf->stopLatencyTestTimestampMs > 0) {
                pvf->latencyUpLabel->setText("***", dontSendNotification);
            } else {
                pvf->latencyUpLabel->setText(TRANS("PRESS"), dontSendNotification);
            }
        } else {
            String latlab; // = juce::CharPointer_UTF8 ("\xe2\x86\x91"); // up arrow
            latlab << (int)lrintf(latinfo.outgoingMs);
            //latlab  << "   |   ";
            //latlab << String(juce::CharPointer_UTF8 ("\xe2\x86\x93")); // down arrow
            String downlatlab;
            downlatlab << (int)lrintf(latinfo.incomingMs) ;

            pvf->latencyUpLabel->setText(latlab, dontSendNotification);
            pvf->latencyDownLabel->setText(downlatlab, dontSendNotification);
        }

        pvf->latActiveButton->setToggleState(latactive, dontSendNotification);


        bool initCompleted = false;
        int autobufmode = (int)processor.getRemotePeerAutoresizeBufferMode(i, initCompleted);
        float buftimeMs = processor.getRemotePeerBufferTime(i);
        
        pvf->autosizeButton->setSelectedId(autobufmode, dontSendNotification);
        String buflab = (autobufmode == SonobusAudioProcessor::AutoNetBufferModeOff ? "" :
                         autobufmode == SonobusAudioProcessor::AutoNetBufferModeAutoIncreaseOnly ? " (Auto+)" :
                         autobufmode == SonobusAudioProcessor::AutoNetBufferModeInitAuto ? ( initCompleted ? " (IA-Man)" : " (IA-Auto)"  ) :
                         " (Auto)");
        pvf->bufferLabel->setText(String::formatted("%d ms", (int) lrintf(buftimeMs)) + buflab, dontSendNotification);

        pvf->bufferMinButton->setEnabled(autobufmode != SonobusAudioProcessor::AutoNetBufferModeOff);
        pvf->bufferMinFrontButton->setEnabled(autobufmode != SonobusAudioProcessor::AutoNetBufferModeOff);


        if (!pvf->bufferTimeSlider->isMouseButtonDown()) {
            pvf->bufferTimeSlider->setValue(buftimeMs, dontSendNotification);
        }

        pvf->remoteSendFormatChoiceButton->setSelectedId(processor.getRequestRemotePeerSendAudioCodecFormat(i), dontSendNotification);
        
        
        int formatindex = processor.getRemotePeerAudioCodecFormat(i);
        pvf->formatChoiceButton->setSelectedItemIndex(formatindex >= 0 ? formatindex : processor.getDefaultAudioCodecFormat(), dontSendNotification);
        String sendqual;
        sendqual << processor.getRemotePeerActualSendChannelCount(i) << "ch " << processor.getAudioCodeFormatName(formatindex);
        pvf->sendQualityLabel->setText(sendqual, dontSendNotification);
        
        // pvf->recvMeter->setMeterSource (processor.getRemotePeerRecvMeterSource(i));

        if (chcnt != pvf->channelGroups->getGroupViewsCount()) {
            pvf->channelGroups->rebuildChannelViews();
            needsUpdateLayout = true;
        } else {
            pvf->channelGroups->updateChannelViews();
        }


        const float disalpha = 0.4;
        pvf->addrLabel->setAlpha(connected ? 1.0 : 0.8);
        pvf->statusLabel->setAlpha(connected ? 1.0 : disalpha);


        if (latinfo.legacy) {
            if (pvf->stopLatencyTestTimestampMs > 0.0 && nowstampms > pvf->stopLatencyTestTimestampMs
                && !pvf->latActiveButton->isMouseButtonDown()) {

                // only stop if it has actually gotten a real latency
                if (latinfo.isreal) {
                    stopLatencyTest(i);

                    String messagestr = generateLatencyMessage(latinfo);
                    showPopTip(messagestr, 5000, pvf->latActiveButton.get(), 300);

                }
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
            if (pinfo.second.blocked) {
                ppvf->messageLabel->setText(TRANS("User BLOCKED from address: ") + pinfo.second.address, dontSendNotification);
                ppvf->removeButton->setVisible(true);
                ppvf->unblockButton->setVisible(true);
            }
            else {

                ppvf->messageLabel->setText(TRANS("Could not connect with user, one or both of you may need to configure your internal firewall or network router to allow SonoBus to work between you. See the help documentation to enable port forwarding on your router."), dontSendNotification);
                ppvf->removeButton->setVisible(true);
            }
        }
        else {
            ppvf->messageLabel->setText(TRANS("Connecting..."), dontSendNotification);
            ppvf->removeButton->setVisible(false);
            ppvf->unblockButton->setVisible(false);
        }
        ++i;
    }
    
    if (needsUpdateLayout) {
        updateLayout();
        listeners.call (&PeersContainerView::Listener::internalSizesChanged, this);
    }
    
    lastUpdateTimestampMs = nowstampms;
}

void PeersContainerView::startLatencyTest(int di)
{
    if (di >= mPeerViews.size()) return;
    
    PeerViewInfo * pvf = mPeerViews.getUnchecked(di);

    int i = mPeerUpdateOrdering[di];

    pvf->stopLatencyTestTimestampMs = Time::getMillisecondCounter(); // make it stop after the first one  //+ 1500;
    pvf->wasRecvActiveAtLatencyTest = processor.getRemotePeerRecvActive(i);
    pvf->wasSendActiveAtLatencyTest = processor.getRemotePeerSendActive(i);
    
    pvf->latencyUpLabel->setText("***", dontSendNotification);
    pvf->latencyDownLabel->setText("***", dontSendNotification);

    processor.startRemotePeerLatencyTest(i);             
}

void PeersContainerView::stopLatencyTest(int di)
{
    if (di >= mPeerViews.size()) return;
    PeerViewInfo * pvf = mPeerViews.getUnchecked(di);

    int i = mPeerUpdateOrdering[di];

    processor.stopRemotePeerLatencyTest(i);
    
    pvf->stopLatencyTestTimestampMs = 0;
    
    SonobusAudioProcessor::LatencyInfo latinfo;
    processor.getRemotePeerLatencyInfo(i, latinfo);
        
    if (latinfo.legacy && !latinfo.isreal) {
        pvf->latencyUpLabel->setText(TRANS("PRESS"), dontSendNotification);
        pvf->latencyDownLabel->setText("", dontSendNotification);
    } else {
        //pvf->latencyLabel->setText(String::formatted("%d ms", (int)lrintf(latinfo.totalRoundtripMs)) + (latinfo.estimated ? "*" : "") , dontSendNotification);
        updatePeerViews(i);
    }
}

String PeersContainerView::generateLatencyMessage(const SonobusAudioProcessor::LatencyInfo &latinfo)
{
    String messagestr = TRANS("Estimated Round-trip Latency:") + String::formatted(" %d ms", (int) lrintf(latinfo.totalRoundtripMs));
    messagestr += "\n" + TRANS("Round-trip Network Ping:") + String::formatted(" %.1f ms", (latinfo.pingMs));
    messagestr += "\n" + TRANS("Est. Outgoing:") + String::formatted(" %.1f ms", (latinfo.outgoingMs));
    messagestr += "\n" + TRANS("Est. Incoming:") + String::formatted(" %.1f ms", (latinfo.incomingMs));
    //messagestr += "\n" + TRANS("Est. Jitter:") + String::formatted(" %.1f ms", (latinfo.jitterMs));
    if (latinfo.legacy) {
        messagestr += "\n-------------";
        messagestr += "\n" + TRANS("Legacy-mode, ask them \n to install latest version");
    }
    return messagestr;
}


void PeersContainerView::timerCallback(int timerId)
{
    if (timerId == FillRatioUpdateTimerId) {
        for (int di=0; di < mPeerViews.size(); ++di) {
            PeerViewInfo * pvf = mPeerViews.getUnchecked(di);
            int i = mPeerUpdateOrdering[di];

            float ratio, stdev;
            if (processor.getRemotePeerReceiveBufferFillRatio(i, ratio, stdev)) {
                pvf->jitterBufferMeter->setFillRatio(ratio, stdev);
            }
        }
    }
}


void PeersContainerView::choiceButtonSelected(SonoChoiceButton *comp, int index, int ident)
{
    for (int di=0; di < mPeerViews.size(); ++di) {
        PeerViewInfo * pvf = mPeerViews.getUnchecked(di);
        int i = mPeerUpdateOrdering[di];
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

            if (processor.getChangingDefaultRecvAudioCodecSetsExisting()) {
                for (int j=0; j < mPeerViews.size(); ++j) {
                    processor.setRequestRemotePeerSendAudioCodecFormat(j, ident);
                }
            }
            else
            {
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
    for (int di=0; di < mPeerViews.size(); ++di) {
        PeerViewInfo * pvf = mPeerViews.getUnchecked(di);

        int i = mPeerUpdateOrdering[di];

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
            return;
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
                
                // Send OSC feedback for Peer Mute (only for first 16 peers)
                if (processor.getOSCEnabled() && i < 16) {
                    processor.getOSCManager().sendMessage("/Peer" + String(i + 1) + "Mute", pvf->recvMutedButton->getToggleState() ? 1.0 : 0.0);
                }
            }
            return;
        }
        else if (pvf->recvSoloButton.get() == buttonThatWasClicked) {
            if (ModifierKeys::currentModifiers.isAltDown()) {
                // exclusive solo this one
                for (int dj=0; dj < mPeerViews.size(); ++dj) {
                    int j = mPeerUpdateOrdering[dj];

                    if (buttonThatWasClicked->getToggleState()) {
                        processor.setRemotePeerSoloed(j, i == j);                         
                    }
                    else {
                        processor.setRemotePeerSoloed(j, false); 
                    }
                    
                    // Send OSC feedback for each peer Solo state (only for first 16 peers)
                    if (processor.getOSCEnabled() && j < 16) {
                        bool soloState = buttonThatWasClicked->getToggleState() && (i == j);
                        processor.getOSCManager().sendMessage("/Peer" + String(j + 1) + "Solo", soloState ? 1.0 : 0.0);
                    }
                }
                    
                // disable solo for main monitor too
                processor.getValueTreeState().getParameter(SonobusAudioProcessor::paramMainMonitorSolo)->setValueNotifyingHost(0.0);
                
                updatePeerViews();
            } else {
                processor.setRemotePeerSoloed(i, buttonThatWasClicked->getToggleState()); 
                
                // Send OSC feedback for Peer Solo (only for first 16 peers)
                if (processor.getOSCEnabled() && i < 16) {
                    processor.getOSCManager().sendMessage("/Peer" + String(i + 1) + "Solo", buttonThatWasClicked->getToggleState() ? 1.0 : 0.0);
                }
                
                updatePeerViews();
            }
            return;
        }

        else if (pvf->latActiveButton.get() == buttonThatWasClicked) {
            SonobusAudioProcessor::LatencyInfo latinfo;
            processor.getRemotePeerLatencyInfo(i, latinfo);

            if (latinfo.legacy) {
                pvf->latActiveButton->setToggleState(!pvf->latActiveButton->getToggleState(), dontSendNotification);
                if (pvf->latActiveButton->getToggleState()) {
                    startLatencyTest(di);
                    //showPopTip(TRANS("Measuring actual round-trip latency"), 4000, pvf->latActiveButton.get(), 140);
                } else {
                    stopLatencyTest(di);
                }
            }
            else {
                String messagestr = generateLatencyMessage(latinfo);

                showPopTip(messagestr, 8000, pvf->latActiveButton.get(), 300);
                pvf->latActiveButton->setToggleState(false, dontSendNotification);



            }
            return;
        }

        
        else if (pvf->recvOptionsButton.get() == buttonThatWasClicked) {

            if (!recvOptionsCalloutBox) {
                showRecvOptions(di, true, pvf->recvOptionsButton.get());
            } else {
                showRecvOptions(di, false);
            }

            return;
        }
        else if (pvf->sendOptionsButton.get() == buttonThatWasClicked) {
            if (!sendOptionsCalloutBox) {
                showSendOptions(di, true, pvf->sendOptionsButton.get());
            } else {
                showSendOptions(di, false);
            }
            return;
        }
        else if (pvf->optionsResetDropButton.get() == buttonThatWasClicked) {
            processor.resetRemotePeerPacketStats(i);
            return;
        }
        else if (pvf->changeAllFormatButton.get() == buttonThatWasClicked) {
            processor.setChangingDefaultAudioCodecSetsExisting(buttonThatWasClicked->getToggleState());
            return;
        }
        else if (pvf->changeAllRecvFormatButton.get() == buttonThatWasClicked) {
            processor.setChangingDefaultRecvAudioCodecSetsExisting(buttonThatWasClicked->getToggleState());
            return;
        }
        else if (pvf->optionsRemoveButton.get() == buttonThatWasClicked) {
            processor.removeRemotePeer(i);
            showSendOptions(di, false);
            return;
        }
        else if (pvf->optionsBlockButton.get() == buttonThatWasClicked) {
            // TODO: confirm blocking?
            String remhost;
            int remport;
            processor.getRemotePeerAddressInfo(i, remhost, remport);
            processor.addBlockedAddress(remhost);

            processor.removeRemotePeer(i, true);
            showSendOptions(di, false);
            return;
        }
        else if (pvf->bufferMinButton.get() == buttonThatWasClicked || pvf->bufferMinFrontButton.get() == buttonThatWasClicked) {
            // force to minimum
            if (ModifierKeys::currentModifiers.isAltDown()) {
                // do it for everyone (who's on auto, maybe?)
                bool initCompleted = false;
                for (int dj=0; dj < mPeerViews.size(); ++dj) {
                    int j = mPeerUpdateOrdering[dj];

                    if (processor.getRemotePeerAutoresizeBufferMode(j, initCompleted) != SonobusAudioProcessor::AutoNetBufferModeOff) {
                        float buftime = 0.0;
                        processor.setRemotePeerBufferTime(j, buftime);
                        if (i==j) {
                            pvf->bufferTimeSlider->setValue(buftime, dontSendNotification);
                        }
                        
                        // Send OSC feedback for BufferMin (only for first 16 peers)
                        if (processor.getOSCEnabled() && j < 16) {
                            processor.getOSCManager().sendMessage("/Peer" + String(j + 1) + "BufferMin", 1);
                        }
                    }
                }
            } else {
                float buftime = 0.0;
                processor.setRemotePeerBufferTime(i, buftime);
                pvf->bufferTimeSlider->setValue(buftime, dontSendNotification);
                
                // Send OSC feedback for BufferMin (only for first 16 peers)
                if (processor.getOSCEnabled() && i < 16) {
                    processor.getOSCManager().sendMessage("/Peer" + String(i + 1) + "BufferMin", 1);
                }
            }
            
            updatePeerViews();
            return;
        }
    }

    int i = 0;
    for (auto & pinfo : mPendingUsers) {
        if (i>= mPendingPeerViews.size()) {
            break; // shouldn't happen
        }

        PendingPeerViewInfo * ppvf = mPendingPeerViews.getUnchecked(i);
        if (ppvf->removeButton.get() == buttonThatWasClicked) {
            mPendingUsers.erase(pinfo.first);
            rebuildPeerViews();
            return;
        }
        else if (ppvf->unblockButton.get() == buttonThatWasClicked) {
            processor.removeBlockedAddress(pinfo.second.address);

            processor.connectRemotePeer(pinfo.second.address, pinfo.second.port, pinfo.second.user, pinfo.second.group);

            mPendingUsers.erase(pinfo.first);
            rebuildPeerViews();
            return;
        }

        ++i;
    }

}



void PeersContainerView::showRecvOptions(int dindex, bool flag, Component * fromView)
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
        
        const int defWidth = 300;
#if JUCE_IOS || JUCE_ANDROID
        const int defHeight = 180;
#else
        const int defHeight = 152;
#endif

        wrap->setSize(jmin(defWidth, dw->getWidth() - 20), jmin(defHeight, dw->getHeight() - 24));
        
        auto * pvf = mPeerViews.getUnchecked(dindex);

        pvf->addrClicked = false;
        
        pvf->recvOptionsContainer->setBounds(Rectangle<int>(0,0,defWidth,defHeight));
        
        wrap->setViewedComponent(pvf->recvOptionsContainer.get(), false);
        pvf->recvOptionsContainer->setVisible(true);
        
        pvf->recvOptionsBox.performLayout(pvf->recvOptionsContainer->getLocalBounds());
        pvf->bufferTimeLabel->setBounds(pvf->bufferTimeSlider->getBounds().removeFromLeft(pvf->bufferTimeSlider->getWidth()*0.5));

        updatePeerViews(dindex);
        
        Rectangle<int> bounds =  dw->getLocalArea(nullptr, fromView ? fromView->getScreenBounds() : pvf->recvOptionsButton->getScreenBounds());
        DBG("callout bounds: " << bounds.toString());
        recvOptionsCalloutBox = & CallOutBox::launchAsynchronously (std::move(wrap), bounds , dw, false);
        if (CallOutBox * box = dynamic_cast<CallOutBox*>(recvOptionsCalloutBox.get())) {
            box->setDismissalMouseClicksAreAlwaysConsumed(true);
            box->grabKeyboardFocus();
        }
        pvf->recvOptionsContainer->grabKeyboardFocus();
    }
    else {
        // dismiss it
        if (CallOutBox * box = dynamic_cast<CallOutBox*>(recvOptionsCalloutBox.get())) {
            box->dismiss();
            recvOptionsCalloutBox = nullptr;
        }
    }
}

void PeersContainerView::showSendOptions(int dindex, bool flag, Component * fromView)
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
#if JUCE_IOS || JUCE_ANDROID
        const int defHeight = 144;
#else
        const int defHeight = 116;
#endif
        
        
        wrap->setSize(jmin(defWidth, dw->getWidth() - 20), jmin(defHeight, dw->getHeight() - 24));

        auto * pvf = mPeerViews.getUnchecked(dindex);
        
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
        pvf->sendOptionsContainer->grabKeyboardFocus();
    }
    else {
        // dismiss it
        if (CallOutBox * box = dynamic_cast<CallOutBox*>(sendOptionsCalloutBox.get())) {
            box->dismiss();
            sendOptionsCalloutBox = nullptr;
        }
    }
}





void PeersContainerView::clearClipIndicators()
{
    for (int di=0;  di < mPeerViews.size(); ++di) {
        int i = mPeerUpdateOrdering[di];

        PeerViewInfo * pvf = mPeerViews.getUnchecked(i);
        pvf->recvMeter->clearClipIndicator(-1);
        pvf->recvMeter->clearMaxLevelDisplay(-1);
        pvf->channelGroups->clearClipIndicators();

    }
}


void PeersContainerView::showPopupMenu(Component * source, int dindex)
{
    if (dindex >= mPeerViews.size()) return;

    int index = mPeerUpdateOrdering[dindex];


    PeerViewInfo * pvf = mPeerViews.getUnchecked(dindex);
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
    int dvindex = comp->getTag() - 1;
    if (dvindex >= mPeerViews.size()) return;
    //PeerViewInfo * pvf = mPeerViews.getUnchecked(vindex);

    int vindex = mPeerUpdateOrdering[dvindex];


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
   for (int di=0; di < mPeerViews.size(); ++di) {
       PeerViewInfo * pvf = mPeerViews.getUnchecked(di);
       int i = mPeerUpdateOrdering[di];

       if (pvf->bufferTimeSlider.get() == slider) {
           float buftime = pvf->bufferTimeSlider->getValue();
           processor.setRemotePeerBufferTime(i, buftime);
           pvf->bufferTimeSlider->setValue(buftime, dontSendNotification);
           break;
       }
   }

}
