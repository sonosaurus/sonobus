// SPDX-License-Identifier: GPLv3-or-later
// Copyright (C) 2020 Jesse Chappell

#include "ChannelGroupsView.h"


using namespace SonoAudio;


ChannelGroupEffectsView::ChannelGroupEffectsView(SonobusAudioProcessor& proc, bool peermode)
: Component(), peerMode(peermode), processor(proc)
{
    effectsConcertina =  std::make_unique<ConcertinaPanel>();

    compressorView =  std::make_unique<CompressorView>();
    compressorView->addListener(this);
    compressorView->addHeaderListener(this);

    compressorBg = std::make_unique<DrawableRectangle>();
    compressorBg->setCornerSize(Point<float>(6,6));
    compressorBg->setFill (Colour::fromFloatRGBA(0.07, 0.07, 0.07, 1.0));
    compressorBg->setStrokeFill (Colour::fromFloatRGBA(0.5, 0.5, 0.5, 0.25));
    compressorBg->setStrokeThickness(0.5);

    expanderView =  std::make_unique<ExpanderView>();
    expanderView->addListener(this);
    expanderView->addHeaderListener(this);

    expanderBg = std::make_unique<DrawableRectangle>();
    expanderBg->setCornerSize(Point<float>(6,6));
    expanderBg->setFill (Colour::fromFloatRGBA(0.07, 0.07, 0.07, 1.0));
    expanderBg->setStrokeFill (Colour::fromFloatRGBA(0.5, 0.5, 0.5, 0.25));
    expanderBg->setStrokeThickness(0.5);

    eqView =  std::make_unique<ParametricEqView>();
    eqView->addListener(this);
    eqView->addHeaderListener(this);

    effectsConcertina->addPanel(-1, expanderView.get(), false);
    effectsConcertina->addPanel(-1, compressorView.get(), false);
    effectsConcertina->addPanel(-1, eqView.get(), false);

    effectsConcertina->setCustomPanelHeader(compressorView.get(), compressorView->getHeaderComponent(), false);
    effectsConcertina->setCustomPanelHeader(expanderView.get(), expanderView->getHeaderComponent(), false);
    effectsConcertina->setCustomPanelHeader(eqView.get(), eqView->getHeaderComponent(), false);

    addAndMakeVisible (effectsConcertina.get());

    updateLayout();
}

ChannelGroupEffectsView::~ChannelGroupEffectsView() {}

juce::Rectangle<int> ChannelGroupEffectsView::getMinimumContentBounds() const {
    auto minbounds = compressorView->getMinimumContentBounds();
    auto minexpbounds = expanderView->getMinimumContentBounds();
    auto mineqbounds = eqView->getMinimumContentBounds();
    auto headbounds = compressorView->getMinimumHeaderBounds();

    int defWidth = jmax(minbounds.getWidth(), minexpbounds.getWidth(), mineqbounds.getWidth()) + 12;
    int defHeight = jmax(minbounds.getHeight(), minexpbounds.getHeight(), mineqbounds.getHeight()) + 3*headbounds.getHeight() + 8;

    return Rectangle<int>(0,0,defWidth,defHeight);
}


void ChannelGroupEffectsView::updateState()
{
    if (peerMode) {
        updateStateForRemotePeer();
    } else {
        updateStateForInput();
    }

}

void ChannelGroupEffectsView::updateStateForRemotePeer()
{
    CompressorParams compParams;
    if (processor.getRemotePeerCompressorParams(peerIndex, groupIndex, compParams)) {
        compressorView->updateParams(compParams);
    }

    CompressorParams expParams;
    if (processor.getRemotePeerExpanderParams(peerIndex, groupIndex, expParams)) {
        expanderView->updateParams(expParams);
    }

    ParametricEqParams eqparams;
    if (processor.getRemotePeerEqParams(peerIndex, groupIndex, eqparams)) {
        eqView->updateParams(eqparams);
    }

    if (firstShow) {
        if (eqparams.enabled && !(compParams.enabled || expParams.enabled)) {
            effectsConcertina->expandPanelFully(eqView.get(), false);
        }
        else {
            effectsConcertina->setPanelSize(eqView.get(), 0, false);
            effectsConcertina->expandPanelFully(expanderView.get(), false);
            effectsConcertina->expandPanelFully(compressorView.get(), false);
        }

        firstShow = false;
    }

}

void ChannelGroupEffectsView::updateStateForInput()
{

    CompressorParams compParams;
    if (processor.getInputCompressorParams(groupIndex, compParams)) {
        compressorView->updateParams(compParams);
    }

    CompressorParams expParams;
    if (processor.getInputExpanderParams(groupIndex, expParams)) {
        expanderView->updateParams(expParams);
    }

    ParametricEqParams eqparams;
    if (processor.getInputEqParams(groupIndex, eqparams)) {
        eqView->updateParams(eqparams);
    }

    if (firstShow) {
        if (eqparams.enabled && !(compParams.enabled || expParams.enabled)) {
            effectsConcertina->expandPanelFully(eqView.get(), false);
        }
        else {
            effectsConcertina->setPanelSize(eqView.get(), 0, false);
            effectsConcertina->expandPanelFully(expanderView.get(), false);
            effectsConcertina->expandPanelFully(compressorView.get(), false);
        }

        firstShow = false;
    }
}

void ChannelGroupEffectsView::updateLayout()
{
    int minitemheight = 32;
#if JUCE_IOS || JUCE_ANDROID
    // make the button heights a bit more for touchscreen purposes
    minitemheight = 40;
#endif

    auto mincompbounds = compressorView->getMinimumContentBounds();
    auto mincompheadbounds = compressorView->getMinimumHeaderBounds();
    auto minexpbounds = expanderView->getMinimumContentBounds();
    auto minexpheadbounds = compressorView->getMinimumHeaderBounds();
    auto mineqbounds = eqView->getMinimumContentBounds();
    auto mineqheadbounds = eqView->getMinimumHeaderBounds();



    effectsBox.items.clear();
    effectsBox.flexDirection = FlexBox::Direction::column;
    effectsBox.items.add(FlexItem(4, 2));
    effectsBox.items.add(FlexItem(minexpbounds.getWidth(), jmax(minexpbounds.getHeight(), mincompbounds.getHeight(), mineqbounds.getHeight()) + 3*minitemheight, *effectsConcertina).withMargin(1).withFlex(1));

    effectsConcertina->setPanelHeaderSize(compressorView.get(), mincompheadbounds.getHeight());
    effectsConcertina->setPanelHeaderSize(expanderView.get(), minexpheadbounds.getHeight());
    effectsConcertina->setPanelHeaderSize(eqView.get(), mineqheadbounds.getHeight());

    effectsConcertina->setMaximumPanelSize(compressorView.get(), mincompbounds.getHeight()+5);
    effectsConcertina->setMaximumPanelSize(expanderView.get(), minexpbounds.getHeight()+5);
    effectsConcertina->setMaximumPanelSize(eqView.get(), mineqbounds.getHeight());

}

void ChannelGroupEffectsView::resized()  {

    effectsBox.performLayout(getLocalBounds().reduced(2, 2));

}

void ChannelGroupEffectsView::compressorParamsChanged(CompressorView *comp, SonoAudio::CompressorParams & params)
{
    if (peerMode) {
        bool wason = processor.getRemotePeerEffectsActive(peerIndex, groupIndex);

        processor.setRemotePeerCompressorParams(peerIndex, groupIndex, params);

        bool ison = processor.getRemotePeerEffectsActive(peerIndex, groupIndex);
        if (wason != ison) {
            listeners.call (&ChannelGroupEffectsView::Listener::effectsEnableChanged, this);
        }
    }
    else {
        bool wason = processor.getInputEffectsActive(groupIndex);

        processor.setInputCompressorParams(groupIndex, params);

        bool ison = processor.getInputEffectsActive(groupIndex);
        if (wason != ison) {
            listeners.call (&ChannelGroupEffectsView::Listener::effectsEnableChanged, this);
        }
    }

}

void ChannelGroupEffectsView::expanderParamsChanged(ExpanderView *comp, SonoAudio::CompressorParams & params)
{
    if (peerMode) {
        bool wason = processor.getRemotePeerEffectsActive(peerIndex, groupIndex);

        processor.setRemotePeerExpanderParams(peerIndex, groupIndex, params);

        bool ison = processor.getRemotePeerEffectsActive(peerIndex, groupIndex);
        if (wason != ison) {
            listeners.call (&ChannelGroupEffectsView::Listener::effectsEnableChanged, this);
        }
    }
    else {
        bool wason = processor.getInputEffectsActive(groupIndex);

        processor.setInputExpanderParams(groupIndex, params);

        bool ison = processor.getInputEffectsActive(groupIndex);
        if (wason != ison) {
            listeners.call (&ChannelGroupEffectsView::Listener::effectsEnableChanged, this);
        }
    }
}

void ChannelGroupEffectsView::parametricEqParamsChanged(ParametricEqView *comp, SonoAudio::ParametricEqParams & params)
{
    if (peerMode) {
        bool wason = processor.getRemotePeerEffectsActive(peerIndex, groupIndex);

        processor.setRemotePeerEqParams(peerIndex, groupIndex, params);

        bool ison = processor.getRemotePeerEffectsActive(peerIndex, groupIndex);
        if (wason != ison) {
            listeners.call (&ChannelGroupEffectsView::Listener::effectsEnableChanged, this);
        }
    } else {
        bool wason = processor.getInputEffectsActive(groupIndex);

        processor.setInputEqParams(groupIndex, params);

        bool ison = processor.getInputEffectsActive(groupIndex);
        if (wason != ison) {
            listeners.call (&ChannelGroupEffectsView::Listener::effectsEnableChanged, this);
        }
    }
}

void ChannelGroupEffectsView::effectsHeaderClicked(EffectsBaseView *comp, const MouseEvent & ev)
{
    if (comp == compressorView.get()) {
        bool changed = effectsConcertina->setPanelSize(eqView.get(), 0, true);
        changed |= effectsConcertina->expandPanelFully(expanderView.get(), true);
        changed |= effectsConcertina->expandPanelFully(compressorView.get(), true);
        if (!changed) {
            // toggle it
            CompressorParams params;

            if (peerMode) {
                processor.getRemotePeerCompressorParams(peerIndex, 0, params);
                params.enabled = !params.enabled;
                processor.setInputCompressorParams(0, params);
            } else {
                processor.getInputCompressorParams(groupIndex, params);
                params.enabled = !params.enabled;
                processor.setInputCompressorParams(groupIndex, params);
            }
            updateState();

            listeners.call (&ChannelGroupEffectsView::Listener::effectsEnableChanged, this);
        }
    }
    else if (comp == expanderView.get()) {
        bool changed = effectsConcertina->setPanelSize(eqView.get(), 0, true);
        changed |= effectsConcertina->expandPanelFully(compressorView.get(), true);
        changed |= effectsConcertina->expandPanelFully(expanderView.get(), true);
        if (!changed) {
            // toggle it
            CompressorParams params;
            if (peerMode) {
                processor.getRemotePeerExpanderParams(peerIndex, groupIndex, params);
                params.enabled = !params.enabled;
                processor.setRemotePeerExpanderParams(peerIndex, groupIndex, params);
            } else {
                processor.getInputExpanderParams(groupIndex, params);
                params.enabled = !params.enabled;
                processor.setInputExpanderParams(groupIndex, params);
            }
            updateState();

            listeners.call (&ChannelGroupEffectsView::Listener::effectsEnableChanged, this);
        }
    }
    else if (comp == eqView.get()) {
        bool changed = effectsConcertina->expandPanelFully(eqView.get(), true);
        if (!changed) {
            // toggle it
            ParametricEqParams params;
            if (peerMode) {
                processor.getRemotePeerEqParams(peerIndex, groupIndex, params);
                params.enabled = !params.enabled;
                processor.setRemotePeerEqParams(peerIndex, groupIndex, params);
            } else {
                processor.getInputEqParams(groupIndex, params);
                params.enabled = !params.enabled;
                processor.setInputEqParams(groupIndex, params);
            }
            updateState();

            listeners.call (&ChannelGroupEffectsView::Listener::effectsEnableChanged, this);
        }
    }
}



#pragma ChannelGroupView


ChannelGroupView::ChannelGroupView() : smallLnf(12), medLnf(14), sonoSliderLNF(12), panSliderLNF(12)
{
    bgColor = Colour::fromFloatRGBA(0.112f, 0.112f, 0.112f, 1.0f);
    borderColor = Colour::fromFloatRGBA(0.5f, 0.5f, 0.5f, 0.3f);

    sonoSliderLNF.textJustification = Justification::centredLeft;
    panSliderLNF.textJustification = Justification::centredLeft;
    
    //Random rcol;
    //itemColor = Colour::fromHSV(rcol.nextFloat(), 0.5f, 0.2f, 1.0f);
}

ChannelGroupView::~ChannelGroupView()
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

void ChannelGroupView::paint(Graphics& g)
{
    //g.fillAll (Colour(0xff111111));
    //g.fillAll (Colour(0xff202020));
    
    //g.setColour(bgColor);
    //g.fillRoundedRectangle(getLocalBounds().toFloat(), 6.0f);

    //g.drawRoundedRectangle(getLocalBounds().toFloat(), 6.0f, 0.5f);

    if (!linked) {
        g.setColour(borderColor);
        g.drawLine(0, 0, getWidth(), 0, 2);
    }

}

void ChannelGroupView::resized()
{
    
    mainbox.performLayout(getLocalBounds());
    
    //if (levelLabel) {
    //    levelLabel->setBounds(levelSlider->getBounds().removeFromLeft(levelSlider->getWidth()*0.5));
    //}
    if (panLabel) {
        panLabel->setBounds(panSlider->getBounds().removeFromTop(12).translated(0, 0));
    }
    

    int triwidth = 10;
    int triheight = 6;
    
    if (levelSlider) {
        levelSlider->setMouseDragSensitivity(jmax(128, levelSlider->getWidth()));
    }

    if (monitorSlider) {
        monitorSlider->setMouseDragSensitivity(jmax(128, monitorSlider->getWidth()));
    }

    //Rectangle<int> optbounds(staticSendQualLabel->getX(), staticSendQualLabel->getY(), sendQualityLabel->getRight() - staticSendQualLabel->getX(), bufferLabel->getBottom() - sendQualityLabel->getY());
    //optionsButton->setBounds(optbounds);
    
}

//
// ==================================================
//

ChannelGroupsView::ChannelGroupsView(SonobusAudioProcessor& proc, bool peerMode, int peerIndex)
 : Component("pcv"),  processor(proc), mPeerMode(peerMode), mPeerIndex(peerIndex)
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
    bgColor = Colour::fromFloatRGBA(0.045f, 0.045f, 0.05f, 1.0f);

    mInGainSlider     = std::make_unique<Slider>(Slider::LinearHorizontal,  Slider::TextBoxAbove);
    mInGainSlider->setName("ingain");
    mInGainSlider->setSliderSnapsToMousePosition(processor.getSlidersSnapToMousePosition());
    mInGainSlider->setTextBoxIsEditable(true);
    mInGainSlider->setScrollWheelEnabled(false);

    rebuildChannelViews();
}

ChannelGroupsView::~ChannelGroupsView()
{
    if (mEffectsView) {
        mEffectsView->removeListener(this);
    }
}

void ChannelGroupsView::configLevelSlider(Slider * slider, bool monmode)
{
    //slider->setTextValueSuffix(" dB");
    slider->setColour(Slider::textBoxBackgroundColourId, Colours::transparentBlack);
    slider->setColour(Slider::textBoxOutlineColourId, Colours::transparentBlack);
    slider->setColour(Slider::textBoxTextColourId, Colour(0x90eeeeee));
    slider->setColour(TooltipWindow::textColourId, Colour(0xf0eeeeee));

    
    slider->setTextBoxStyle(Slider::TextBoxAbove, true, 80, 12);

    slider->setRange(0.0, 2.0, 0.0);
    slider->setSkewFactor(0.5);
    slider->setDoubleClickReturnValue(true, 1.0);
    slider->setTextBoxIsEditable(true);
    slider->setSliderSnapsToMousePosition(false);
    slider->setScrollWheelEnabled(false);
    slider->valueFromTextFunction = [](const String& s) -> float { return Decibels::decibelsToGain(s.getFloatValue()); };

    if (mPeerMode) {
        if (monmode) {
            slider->textFromValueFunction = [](float v) -> String { return String(TRANS("Level: ")) + Decibels::toString(Decibels::gainToDecibels(v), 1); };
        }
        else {
            slider->textFromValueFunction = [](float v) -> String { return String(TRANS("Level: ")) + Decibels::toString(Decibels::gainToDecibels(v), 1); };
        }
    } else {
        if (monmode) {
            slider->textFromValueFunction = [](float v) -> String { return String(TRANS("Monitor: ")) + Decibels::toString(Decibels::gainToDecibels(v), 1); };
        }
        else {
            slider->textFromValueFunction = [](float v) -> String { return String(TRANS("Gain: ")) + Decibels::toString(Decibels::gainToDecibels(v), 1); };
        }
    }


#if JUCE_IOS
    //slider->setPopupDisplayEnabled(true, false, this);
#endif
}

void ChannelGroupsView::configKnobSlider(Slider * slider)
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

void ChannelGroupsView::configLabel(Label *label, int ltype)
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





void ChannelGroupsView::resized()
{
    Rectangle<int> bounds = getLocalBounds().reduced(5, 0);
    bounds.removeFromLeft(3);
    channelsBox.performLayout(bounds);
    
    for (int i=0; i < mChannelViews.size(); ++i) {
        ChannelGroupView * pvf = mChannelViews.getUnchecked(i);
        pvf->resized();
    }
    
    Component* dw = nullptr; // this->findParentComponentOfClass<DocumentWindow>();    
    if (!dw)
        dw = this->findParentComponentOfClass<AudioProcessorEditor>();
    if (!dw)
        dw = this->findParentComponentOfClass<Component>();
    if (!dw)
        dw = this;

    if (auto * callout = dynamic_cast<CallOutBox*>(effectsCalloutBox.get())) {
        callout->dismiss();
        effectsCalloutBox = nullptr;
    }


}

void ChannelGroupsView::showPopTip(const String & message, int timeoutMs, Component * target, int maxwidth)
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

void ChannelGroupsView::paint(Graphics & g)
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

ChannelGroupView * ChannelGroupsView::createChannelGroupView(bool first)
{
    ChannelGroupView * pvf = new ChannelGroupView();

    pvf->nameLabel = std::make_unique<Label>("name", "");
    pvf->nameLabel->setJustificationType(Justification::centredLeft);
    pvf->nameLabel->setFont(16);
    pvf->nameLabel->setEditable(true);
    pvf->nameLabel->addListener(this);
    pvf->nameLabel->setColour(Label::backgroundColourId, Colours::black);

    pvf->muteButton = std::make_unique<TextButton>(TRANS("MUTE"));
    pvf->muteButton->addListener(this);
    pvf->muteButton->setLookAndFeel(&pvf->medLnf);
    pvf->muteButton->setClickingTogglesState(true);
    pvf->muteButton->setColour(TextButton::buttonOnColourId, mutedColor);
    pvf->muteButton->setTooltip(TRANS("Toggles muting of this channel"));

    pvf->soloButton = std::make_unique<TextButton>(TRANS("SOLO"));
    pvf->soloButton->addListener(this);
    pvf->soloButton->setLookAndFeel(&pvf->medLnf);
    pvf->soloButton->setClickingTogglesState(true);
    pvf->soloButton->setColour(TextButton::buttonOnColourId, soloColor.withAlpha(0.7f));
    pvf->soloButton->setColour(TextButton::textColourOnId, Colours::darkblue);
    pvf->soloButton->setTooltip(TRANS("Listen to only this channel"));

    
    pvf->chanLabel = std::make_unique<Label>("status", "");
    configLabel(pvf->chanLabel.get(), LabelTypeRegular);
    pvf->chanLabel->setJustificationType(Justification::centredLeft);

    
    pvf->levelSlider     = std::make_unique<Slider>(Slider::LinearHorizontal,  Slider::TextBoxRight);
    pvf->levelSlider->setName("level");
    pvf->levelSlider->addListener(this);

    configLevelSlider(pvf->levelSlider.get(), false);
    pvf->levelSlider->setLookAndFeel(&pvf->sonoSliderLNF);

    pvf->monitorSlider     = std::make_unique<Slider>(Slider::LinearHorizontal,  Slider::TextBoxRight);
    pvf->monitorSlider->setName("level");
    pvf->monitorSlider->addListener(this);

    configLevelSlider(pvf->monitorSlider.get(), true);
    pvf->monitorSlider->setLookAndFeel(&pvf->sonoSliderLNF);


    pvf->panLabel = std::make_unique<Label>("pan", TRANS("Pan"));
    configLabel(pvf->panLabel.get(), LabelTypeSmall);
    pvf->panLabel->setJustificationType(Justification::centredTop);
    
    pvf->panSlider     = std::make_unique<Slider>(Slider::LinearHorizontal,  Slider::NoTextBox);
    pvf->panSlider->setName("pan1");
    pvf->panSlider->addListener(this);
    pvf->panSlider->getProperties().set ("fromCentre", true);
    pvf->panSlider->getProperties().set ("noFill", true);
    pvf->panSlider->setRange(-1, 1, 0.0f);
    pvf->panSlider->setDoubleClickReturnValue(true, -1.0);
    pvf->panSlider->setTextBoxIsEditable(false);
    pvf->panSlider->setSliderSnapsToMousePosition(false);
    pvf->panSlider->setScrollWheelEnabled(false);
    pvf->panSlider->setMouseDragSensitivity(100);

    pvf->panSlider->textFromValueFunction =  [](double v) -> String { if (fabs(v) < 0.01) return String(TRANS("Pan: Center")); return String(TRANS("Pan: ")) +  String((int)rint(abs(v*100.0f))) + ((v > 0 ? "% R" : "% L")) ; };
    pvf->panSlider->valueFromTextFunction =  [](const String& s) -> double { return s.getDoubleValue()*1e-2f; };
    pvf->panSlider->setValue(0.1, dontSendNotification);
    pvf->panSlider->setValue(0.0, dontSendNotification);
    pvf->panSlider->setLookAndFeel(&pvf->panSliderLNF);
    pvf->panSlider->setPopupDisplayEnabled(true, true, this);


    pvf->linkButton = std::make_unique<SonoDrawableButton>("", DrawableButton::ButtonStyle::ImageFitted);
    if (first) {
        std::unique_ptr<Drawable> linkimg(Drawable::createFromImageData(BinaryData::link_all_svg, BinaryData::link_all_svgSize));
        pvf->linkButton->setImages(linkimg.get());
    } else {
        std::unique_ptr<Drawable> linkimg(Drawable::createFromImageData(BinaryData::link_up_svg, BinaryData::link_up_svgSize));
        pvf->linkButton->setImages(linkimg.get());
    }

    pvf->linkButton->setColour(SonoTextButton::outlineColourId, Colours::transparentBlack);
    pvf->linkButton->setColour(DrawableButton::backgroundOnColourId, Colour::fromFloatRGBA(0.6, 0.4, 0.6, 0.7));
    pvf->linkButton->setColour(DrawableButton::backgroundColourId, Colours::transparentBlack);
    pvf->linkButton->setClickingTogglesState(true);
    pvf->linkButton->setTriggeredOnMouseDown(true);
    pvf->linkButton->setLookAndFeel(&pvf->smallLnf);
    pvf->linkButton->addListener(this);
    pvf->linkButton->addMouseListener(this, false);
    pvf->linkButton->setAlpha(0.6f);
    if (first) {
        pvf->linkButton->setTooltip(TRANS("Link/Unlink ALL channels"));
    } else {
        pvf->linkButton->setTooltip(TRANS("Link with previous channel"));
    }

    

#if JUCE_IOS
    //pvf->panSlider1->setPopupDisplayEnabled(true, false, pvf->pannersContainer.get());
    //pvf->panSlider2->setPopupDisplayEnabled(true, false, pvf->pannersContainer.get());
#endif

    pvf->fxButton = std::make_unique<TextButton>(TRANS("FX"));
    pvf->fxButton->setColour(TextButton::buttonOnColourId, Colour::fromFloatRGBA(0.2, 0.5, 0.7, 0.5));
    pvf->fxButton->addListener(this);
    pvf->fxButton->setLookAndFeel(&pvf->medLnf);
  
    

    // meters
    auto flags = foleys::LevelMeter::Minimal /* | foleys::LevelMeter::SingleChannel */;
    
    pvf->meter = std::make_unique<foleys::LevelMeter>(flags);
    pvf->meter->setLookAndFeel(&(pvf->rmeterLnf));
    pvf->meter->setRefreshRateHz(metersActive ? 8 : 0);
    pvf->meter->addMouseListener(this, false);

    return pvf;
}

void ChannelGroupsView::visibilityChanged()
{
    DBG("ChannelGroupsView visibility changed: " << (int) isVisible());
    setMetersActive(isVisible());
}


void ChannelGroupsView::setMetersActive(bool flag)
{
    float rate = flag ? 8.0f : 0.0f;
    metersActive = flag;

    for (int i=0; i < mChannelViews.size(); ++i) {
        ChannelGroupView * pvf = mChannelViews.getUnchecked(i);
        if (pvf->meter) {
            pvf->meter->setRefreshRateHz(rate);
        }
    }
}



void ChannelGroupsView::rebuildChannelViews()
{
    int numchans = mPeerMode ? processor.getRemotePeerRecvChannelCount(mPeerIndex) : processor.getMainBusNumInputChannels();
    
    while (mChannelViews.size() < numchans) {
        mChannelViews.add(createChannelGroupView(mChannelViews.size() == 0));
    }
    while (mChannelViews.size() > numchans) {
        mChannelViews.removeLast();
    }
    
    for (int i=0; i < mChannelViews.size(); ++i) {
        ChannelGroupView * pvf = mChannelViews.getUnchecked(i);

        String username = processor.getRemotePeerUserName(i);

        pvf->addAndMakeVisible(pvf->linkButton.get());
        pvf->addAndMakeVisible(pvf->muteButton.get());
        pvf->addAndMakeVisible(pvf->soloButton.get());
        pvf->addAndMakeVisible(pvf->levelSlider.get());
        //pvf->addAndMakeVisible(pvf->monitorSlider.get());
        //pvf->addAndMakeVisible(pvf->levelLabel.get());
        pvf->addAndMakeVisible(pvf->panLabel.get());
        pvf->addAndMakeVisible(pvf->nameLabel.get());
        pvf->addAndMakeVisible(pvf->chanLabel.get());

        pvf->addAndMakeVisible(pvf->meter.get());
        pvf->addAndMakeVisible(pvf->fxButton.get());

        pvf->addAndMakeVisible(pvf->panSlider.get());

        addAndMakeVisible(pvf);
    }
    
    updateChannelViews();
    updateLayout();
    resized();
}

void ChannelGroupsView::updateLayout()
{
    int minitemheight = 30;
    int mincheckheight = 32;
    int minPannerWidth = 58;
    int minButtonWidth = 60;
    int maxPannerWidth = 130;
    int minSliderWidth = 100;
    int meterwidth = 10;
    int mutebuttwidth = 52;
    
#if JUCE_IOS || JUCE_ANDROID
    // make the button heights a bit more for touchscreen purposes
    minitemheight = 40;
    mincheckheight = 40;
    minPannerWidth = 60;
#endif

    const int textheight = minitemheight / 2;

    bool servconnected = processor.isConnectedToServer();

    channelsBox.items.clear();
    channelsBox.flexDirection = FlexBox::Direction::column;
    channelsBox.justifyContent = FlexBox::JustifyContent::flexStart;
    int peersheight = 0;
    //const int singleph =  minitemheight*3 + 12;
    const int singleph =  minitemheight;
    
    //channelsBox.items.add(FlexItem(8, 2).withMargin(0));
    //peersheight += 2;

    const int ph = singleph;
    int ipw = 0;//2*minButtonWidth + minSliderWidth + 15 + 32 + 60 + 20;
    int mpw = 0;// 2*minButtonWidth + minPannerWidth + 6 + 44;
    int iph = 0;
    int mph = 0;
    int mbh = 0;

    // Main connected peer views
    
    for (int i=0; i < mChannelViews.size(); ++i) {
        ChannelGroupView * pvf = mChannelViews.getUnchecked(i);

        //pvf->updateLayout();
        


        pvf->namebox.items.clear();
        pvf->namebox.flexDirection = FlexBox::Direction::row;
//.withAlignSelf(FlexItem::AlignSelf::center));
        pvf->namebox.items.add(FlexItem(100, minitemheight, *pvf->nameLabel).withMargin(0).withFlex(1));

        pvf->inbox.items.clear();
        pvf->inbox.flexDirection = FlexBox::Direction::row;
        pvf->inbox.items.add(FlexItem(20, minitemheight, *pvf->chanLabel).withMargin(0).withFlex(0));
        pvf->inbox.items.add(FlexItem(3, 3));
        pvf->inbox.items.add(FlexItem(32, minitemheight, *pvf->linkButton).withMargin(0).withFlex(0));
        pvf->inbox.items.add(FlexItem(3, 3));
        pvf->inbox.items.add(FlexItem(100, minitemheight, pvf->namebox).withMargin(0).withFlex(0));
        if (!isNarrow) {
            pvf->inbox.items.add(FlexItem(6, 3));
            pvf->inbox.items.add(FlexItem(mutebuttwidth, minitemheight, *pvf->muteButton).withMargin(0).withFlex(0));
            pvf->inbox.items.add(FlexItem(3, 3));
            pvf->inbox.items.add(FlexItem(mutebuttwidth, minitemheight, *pvf->soloButton).withMargin(0).withFlex(0));
        }
        pvf->inbox.items.add(FlexItem(3, 3));
        pvf->inbox.items.add(FlexItem(minSliderWidth, minitemheight, *pvf->levelSlider).withMargin(0).withFlex(1));
        pvf->inbox.items.add(FlexItem(1, 3));
        pvf->inbox.items.add(FlexItem(meterwidth, minitemheight, *pvf->meter).withMargin(0).withFlex(0));

        ipw = 0;
        iph = minitemheight;
        for (auto & item : pvf->inbox.items) {
            ipw += item.minWidth;
        }

        pvf->monbox.items.clear();
        pvf->monbox.flexDirection = FlexBox::Direction::row;
        //pvf->monbox.items.add(FlexItem(minSliderWidth, minitemheight, *pvf->monitorSlider).withMargin(0).withFlex(1));
        if (isNarrow) {
            pvf->monbox.items.add(FlexItem(3, 3).withFlex(0.25));
            pvf->monbox.items.add(FlexItem(mutebuttwidth, minitemheight, *pvf->muteButton).withMargin(0).withFlex(0));
            pvf->monbox.items.add(FlexItem(3, 3));
            pvf->monbox.items.add(FlexItem(mutebuttwidth, minitemheight, *pvf->soloButton).withMargin(0).withFlex(0));
            pvf->monbox.items.add(FlexItem(3, 3));
            pvf->monbox.items.add(FlexItem(mutebuttwidth, minitemheight, *pvf->fxButton).withMargin(0).withFlex(0));
            pvf->monbox.items.add(FlexItem(3, 3).withFlex(0.25));
            pvf->monbox.items.add(FlexItem(minPannerWidth, minitemheight, *pvf->panSlider).withMargin(0).withFlex(1).withMaxWidth(maxPannerWidth));
            pvf->monbox.items.add(FlexItem(3, 3).withFlex(0.1).withMaxWidth(meterwidth + 10));
        }
        else {
            pvf->monbox.items.add(FlexItem(minPannerWidth, minitemheight, *pvf->panSlider).withMargin(0).withFlex(0.25).withMaxWidth(maxPannerWidth));
            pvf->monbox.items.add(FlexItem(3, 3));
            pvf->monbox.items.add(FlexItem(mutebuttwidth, minitemheight, *pvf->fxButton).withMargin(0).withFlex(0));
        }

        mpw = 0;
        mph = minitemheight;
        for (auto & item : pvf->monbox.items) {
            mpw += item.minWidth;
        }

        pvf->maincontentbox.items.clear();

        pvf->mainbox.items.clear();
        pvf->mainbox.flexDirection = FlexBox::Direction::column;

        // gap at top
        pvf->mainbox.items.add(FlexItem(3, 6));

        if (isNarrow) {
            pvf->maincontentbox.flexDirection = FlexBox::Direction::column;
            pvf->maincontentbox.items.add(FlexItem(3, 2));
            pvf->maincontentbox.items.add(FlexItem(ipw, iph , pvf->inbox).withMargin(0).withFlex(0));
            pvf->maincontentbox.items.add(FlexItem(2, 2));
            pvf->maincontentbox.items.add(FlexItem(mpw, mph , pvf->monbox).withMargin(0).withFlex(0));

            int mch = 0;
            for (auto & item : pvf->maincontentbox.items) {
                mch += item.minHeight;
            }

            pvf->mainbox.items.add(FlexItem(60, mch, pvf->maincontentbox).withMargin(0).withFlex(0));

        } else {
            pvf->maincontentbox.flexDirection = FlexBox::Direction::row;
            pvf->maincontentbox.items.add(FlexItem(3, 2));
            pvf->maincontentbox.items.add(FlexItem(ipw, iph , pvf->inbox).withMargin(0).withFlex(2));
            pvf->maincontentbox.items.add(FlexItem(6, 2));
            pvf->maincontentbox.items.add(FlexItem(mpw, mph , pvf->monbox).withMargin(0).withFlex(1).withMaxWidth(maxPannerWidth + mutebuttwidth + 10));


            pvf->mainbox.items.add(FlexItem(60, iph, pvf->maincontentbox).withMargin(0).withFlex(0));
        }

        mbh = 0;
        for (auto & item : pvf->mainbox.items) {
            mbh += item.minHeight;
        }


        channelsBox.items.add(FlexItem(8, 3).withMargin(0));
        peersheight += 4;


        if (isNarrow) {
            channelsBox.items.add(FlexItem(ipw, mbh, *pvf).withMargin(0).withFlex(0));
            //peersheight += ph*2 + 6;
            peersheight += mbh + 2;
        } else {
            channelsBox.items.add(FlexItem(ipw+mpw + 6, mbh, *pvf).withMargin(1).withFlex(0));
            //peersheight += ph + 11;
            peersheight += mbh + 2;
        }


    }


    if (isNarrow) {
        channelMinHeight = std::max(mbh,  peersheight);
        channelMinWidth = ipw + 12;
    }
    else {
        channelMinHeight = std::max(mbh + 18,  peersheight);
        channelMinWidth = ipw + mpw + 50;
    }
    
}

Rectangle<int> ChannelGroupsView::getMinimumContentBounds() const
{
    return Rectangle<int>(0,0, channelMinWidth, channelMinHeight);
}

void ChannelGroupsView::applyToAllSliders(std::function<void(Slider *)> & routine)
{
    for (int i=0; i < mChannelViews.size(); ++i) {
        ChannelGroupView * pvf = mChannelViews.getUnchecked(i);
        routine(pvf->levelSlider.get());
        routine(pvf->monitorSlider.get());
        routine(pvf->panSlider.get());
    }
}

void ChannelGroupsView::updateChannelViews(int specific)
{
    if (mPeerMode) {
        updatePeerModeChannelViews(specific);
    } else {
        updateInputModeChannelViews(specific);
    }
}

void ChannelGroupsView::updateInputModeChannelViews(int specific)
{
    uint32 nowstampms = Time::getMillisecondCounter();
    bool needsUpdateLayout = false;

    int changroup = 0;
    int sendcnt = (int) processor.getValueTreeState().getParameter(SonobusAudioProcessor::paramSendChannels)->convertFrom0to1( processor.getValueTreeState().getParameter(SonobusAudioProcessor::paramSendChannels)->getValue());

    if (sendcnt == 0) {
        sendcnt = processor.getMainBusNumInputChannels();
    }

    int totalchans = processor.getMainBusNumInputChannels();
    int changroups = processor.getInputGroupCount();
    int chi = 0;

    int chstart = 0;
    int chcnt = 1;
    processor.getInputGroupChannelStartAndCount(changroup, chstart, chcnt);

    for (int i=0; i < mChannelViews.size(); ++i, ++chi) {
        if (specific >= 0 && specific != i) continue;

        if (i >= chstart+chcnt && changroup < changroups-1) {
            changroup++;
            processor.getInputGroupChannelStartAndCount(changroup, chstart, chcnt);
            chi = 0;
        }

        ChannelGroupView * pvf = mChannelViews.getUnchecked(i);

        String name = processor.getInputGroupName(changroup);

        //DBG("Got username: '" << username << "'");

        pvf->nameLabel->setText(name, dontSendNotification);
        pvf->chanLabel->setText(String::formatted("%d", i+1), dontSendNotification);

        bool aresoloed = processor.getInputGroupSoloed(changroup);
        bool aremuted = processor.getInputGroupMuted(changroup);

        pvf->linked = chi > 0 || i == 0;

        if (i == 0) {
            // only if all are linked
            pvf->linkButton->setToggleState(chcnt == mChannelViews.size(), dontSendNotification);
            //pvf->linkButton->setVisible(false);
        } else {
            pvf->linkButton->setToggleState(pvf->linked, dontSendNotification);
            //pvf->linkButton->setVisible(true);
        }

        /*
        if ((processor.isAnythingSoloed() && !aresoloed && !aremuted)) {
            pvf->muteButton->setColour(TextButton::buttonColourId, mutedBySoloColor);
        } else {
            pvf->muteButton->removeColour(TextButton::buttonColourId);
        }
         */

        pvf->muteButton->setToggleState(aremuted , dontSendNotification);
        pvf->soloButton->setToggleState(aresoloed , dontSendNotification);


        bool infxon = processor.getInputEffectsActive(changroup);
        pvf->fxButton->setToggleState(infxon, dontSendNotification);


        if (!pvf->levelSlider->isMouseOverOrDragging()) {
            pvf->levelSlider->setValue(processor.getInputGroupGain(changroup), dontSendNotification);
        }


        pvf->meter->setMeterSource (&processor.getInputMeterSource());
        pvf->meter->setSelectedChannel(i);
        pvf->meter->setFixedNumChannels(1);


        if (sendcnt != 2 && processor.getMainBusNumOutputChannels() != 2) {
            pvf->panSlider->setVisible(false);
            pvf->panLabel->setVisible(false);
        }
        else if (chcnt == 1) {
            pvf->panLabel->setVisible(true);
            pvf->panSlider->setVisible(true);
            pvf->panSlider->setDoubleClickReturnValue(true, 0.0);

            //if (!pvf->singlePanner) {
            //    pvf->panSlider->setSliderStyle(Slider::LinearHorizontal); // LinearBar
            //    pvf->panSlider->setTextBoxStyle(Slider::NoTextBox, true, 10, 2); // TextBoxAbove
            //
            //    pvf->singlePanner = true;
            //}

        } else {
            pvf->panLabel->setVisible(true);
            pvf->panSlider->setDoubleClickReturnValue(true, (chi & 2) ? 1.0f : -1.0f); // double click defaults to altenating left/right
            pvf->panSlider->setVisible(true);

            //if (pvf->singlePanner) {
            //    pvf->panSlider1->setSliderStyle(Slider::TwoValueHorizontal);
            //    pvf->panSlider1->setTextBoxStyle(Slider::NoTextBox, true, 60, 12);
            //
            //    pvf->singlePanner = false;
            //}
        }

        if (pvf->panSlider->isTwoValue()) {
            pvf->panSlider->setMinAndMaxValues(processor.getInputChannelPan(changroup, 0), processor.getInputChannelPan(changroup, 1), dontSendNotification);
        }
        else {
            pvf->panSlider->setValue(processor.getInputChannelPan(changroup, chi), dontSendNotification);
        }

        // hide things if we are not the first channel
        bool isprimary = chi == 0;
        pvf->levelSlider->setVisible(isprimary);
        pvf->soloButton->setVisible(isprimary);
        pvf->muteButton->setVisible(isprimary);
        pvf->nameLabel->setVisible(isprimary);

        // effects aren't used if channel count is above 2, right now
        pvf->fxButton->setVisible(isprimary && chcnt <= 2);

        pvf->linkButton->setVisible(totalchans > 1);

        pvf->repaint();
    }

    if (needsUpdateLayout) {
        updateLayout();
    }

    lastUpdateTimestampMs = nowstampms;
}

void ChannelGroupsView::updatePeerModeChannelViews(int specific)
{
    uint32 nowstampms = Time::getMillisecondCounter();
    bool needsUpdateLayout = false;

    int changroup = 0;
    int changroups = processor.getRemotePeerChannelGroupCount(mPeerIndex);
    int chi = 0;

    int totalchans = processor.getRemotePeerRecvChannelCount(mPeerIndex);

    int chstart = 0;
    int chcnt = 0;
    processor.getRemotePeerChannelGroupStartAndCount(mPeerIndex, changroup, chstart, chcnt);


    for (int i=0; i < mChannelViews.size(); ++i, ++chi) {
        if (specific >= 0 && specific != i) continue;

        if (i >= chstart+chcnt && changroup < changroups-1) {
            changroup++;
            chstart += chcnt;

            processor.getRemotePeerChannelGroupStartAndCount(mPeerIndex, changroup, chstart, chcnt);
            chi = 0;
        }


        ChannelGroupView * pvf = mChannelViews.getUnchecked(i);

        String name = processor.getRemotePeerChannelGroupName (mPeerIndex, changroup);

        //DBG("Got username: '" << username << "'");
        
        pvf->nameLabel->setText(name, dontSendNotification);
        pvf->chanLabel->setText(String::formatted("%d", i+1), dontSendNotification);

        int chstart=0, chcnt=0;
        processor.getRemotePeerChannelGroupStartAndCount(mPeerIndex, changroup, chstart, chcnt);

        bool aresoloed = processor.getRemotePeerChannelSoloed(mPeerIndex, changroup);
        bool aremuted = processor.getRemotePeerChannelMuted(mPeerIndex, changroup);

        /*
        if (safetymuted || (processor.isAnythingSoloed() && !aresoloed && !aremuted)) {
            pvf->mutedButton->setColour(TextButton::buttonColourId, mutedBySoloColor);
        } else {
            pvf->mutedButton->removeColour(TextButton::buttonColourId);
        }
         */

        pvf->muteButton->setToggleState(aremuted , dontSendNotification);
        pvf->soloButton->setToggleState(aresoloed , dontSendNotification);


        bool infxon = processor.getRemotePeerEffectsActive(mPeerIndex, changroup);
        pvf->fxButton->setToggleState(infxon, dontSendNotification);

        if (!pvf->levelSlider->isMouseOverOrDragging()) {
            pvf->levelSlider->setValue(processor.getRemotePeerLevelGain (mPeerIndex, changroup), dontSendNotification);
        }


        pvf->meter->setMeterSource (processor.getRemotePeerRecvMeterSource(mPeerIndex));
        pvf->meter->setSelectedChannel(i);

        pvf->linkButton->setVisible(totalchans > 1); // XXX

        if (processor.getMainBusNumOutputChannels() != 2 && chcnt <= processor.getMainBusNumOutputChannels()) {
            pvf->panSlider->setVisible(false);
            pvf->panLabel->setVisible(false);
        }
        else if (chcnt == 1) {
            pvf->panLabel->setVisible(true);
            pvf->panSlider->setVisible(true);
            pvf->panSlider->setDoubleClickReturnValue(true, 0.0);

            //if (!pvf->singlePanner) {
            //    pvf->panSlider->setSliderStyle(Slider::LinearHorizontal); // LinearBar
            //    pvf->panSlider->setTextBoxStyle(Slider::NoTextBox, true, 10, 2); // TextBoxAbove
            //
            //    pvf->singlePanner = true;
            //}
                        
        } else {
            pvf->panLabel->setVisible(true);
            pvf->panSlider->setDoubleClickReturnValue(true, (chi & 2) ? 1.0f: -1.0f);
            pvf->panSlider->setVisible(true);

            //if (pvf->singlePanner) {
            //    pvf->panSlider1->setSliderStyle(Slider::TwoValueHorizontal);
            //    pvf->panSlider1->setTextBoxStyle(Slider::NoTextBox, true, 60, 12);
            //
            //    pvf->singlePanner = false;
            //}
        }
        
        if (pvf->panSlider->isTwoValue()) {
            pvf->panSlider->setMinAndMaxValues(processor.getRemotePeerChannelPan(mPeerIndex, changroup, 0), processor.getRemotePeerChannelPan(mPeerIndex, changroup, 1), dontSendNotification);
        }
        else {        
            pvf->panSlider->setValue(processor.getRemotePeerChannelPan(mPeerIndex, changroup, chi), dontSendNotification);
        }

    }
    
    if (needsUpdateLayout) {
        updateLayout();
    }
    
    lastUpdateTimestampMs = nowstampms;
}




void ChannelGroupsView::timerCallback(int timerId)
{
    /*
    if (timerId == FillRatioUpdateTimerId) {
        for (int i=0; i < mChannelViews.size(); ++i) {
            ChannelGroupView * pvf = mChannelViews.getUnchecked(i);

            float ratio, stdev;
            if (processor.getRemotePeerReceiveBufferFillRatio(i, ratio, stdev) > 0) {
                pvf->jitterBufferMeter->setFillRatio(ratio, stdev);
            }
        }
    }
     */
}


void ChannelGroupsView::choiceButtonSelected(SonoChoiceButton *comp, int index, int ident)
{
    for (int i=0; i < mChannelViews.size(); ++i) {
        ChannelGroupView * pvf = mChannelViews.getUnchecked(i);

        /*
        if (pvf->formatChoiceButton.get() == comp) {
            // set them all if this option is selected
            if (processor.getChangingDefaultAudioCodecSetsExisting()) {
                for (int j=0; j < mChannelViews.size(); ++j) {
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
                for (int j=0; j < mChannelViews.size(); ++j) {

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
         */
    }
}

void ChannelGroupsView::buttonClicked (Button* buttonThatWasClicked)
{
    if (mPeerMode) {
        int changroup = 0;
        int changroups = processor.getRemotePeerChannelGroupCount(mPeerIndex);
        int chi = 0;

        int chstart = 0;
        int chcnt = 0;
        processor.getRemotePeerChannelGroupStartAndCount(mPeerIndex, changroup, chstart, chcnt);


        for (int i=0; i < mChannelViews.size(); ++i, ++chi) {

            if (i >= chstart+chcnt && changroup < changroups-1) {
                changroup++;
                chstart += chcnt;
                //chcnt =
                processor.getRemotePeerChannelGroupStartAndCount(mPeerIndex, changroup, chstart, chcnt);
                chi = 0;
            }

            ChannelGroupView * pvf = mChannelViews.getUnchecked(i);

            if (pvf->muteButton.get() == buttonThatWasClicked) {
                processor.setRemotePeerChannelMuted (mPeerIndex, changroup, buttonThatWasClicked->getToggleState());
                updateChannelViews();
                break;
            }
            else if (pvf->linkButton.get() == buttonThatWasClicked) {
                //processor.setRemotePeerChannelMuted (mPeerIndex, changroup, buttonThatWasClicked->getToggleState());
                //updateChannelViews();

                linkButtonPressed(i, buttonThatWasClicked->getToggleState());

                break;
            }
            else if (pvf->soloButton.get() == buttonThatWasClicked) {
                if (ModifierKeys::currentModifiers.isAltDown()) {
                    // exclusive solo this one

                    bool newsolo = buttonThatWasClicked->getToggleState();

                    for (int j=0; j < changroups; ++j) {
                        if (newsolo) {
                            processor.setRemotePeerChannelSoloed(mPeerIndex, j, changroup == j);
                        }
                        else {
                            processor.setRemotePeerChannelSoloed(mPeerIndex, j, false);
                        }
                    }
                    
                    // disable solo for main monitor too
                    processor.getValueTreeState().getParameter(SonobusAudioProcessor::paramMainMonitorSolo)->setValueNotifyingHost(0.0);

                    updateChannelViews();
                } else {
                    bool newsolo = buttonThatWasClicked->getToggleState();

                    processor.setRemotePeerChannelSoloed (mPeerIndex, changroup, newsolo);

                    updateChannelViews();
                }
                break;
            }
            else if (pvf->fxButton.get() == buttonThatWasClicked) {

                if (!effectsCalloutBox) {
                    showEffects(changroup, true, pvf->fxButton.get());
                } else {
                    showEffects(changroup, false);
                }

                break;
            }
        }

    }
    else {
        int changroup = 0;
        int changroups = processor.getInputGroupCount();
        int chi = 0;

        int chstart = 0;
        int chcnt = 1;
        processor.getInputGroupChannelStartAndCount(changroup, chstart, chcnt);

        for (int i=0; i < mChannelViews.size(); ++i,++chi)
        {
            if (i >= chstart+chcnt && changroup < changroups-1) {
                changroup++;
                processor.getInputGroupChannelStartAndCount(changroup, chstart, chcnt);
                chi = 0;
            }

            ChannelGroupView * pvf = mChannelViews.getUnchecked(i);

            if (pvf->muteButton.get() == buttonThatWasClicked) {
                processor.setInputGroupMuted(changroup, buttonThatWasClicked->getToggleState());
                updateChannelViews();
                break;
            }
            else if (pvf->linkButton.get() == buttonThatWasClicked) {
                //processor.setRemotePeerChannelMuted (mPeerIndex, changroup, buttonThatWasClicked->getToggleState());
                //updateChannelViews();

                linkButtonPressed(i, buttonThatWasClicked->getToggleState());

                break;
            }
            else if (pvf->soloButton.get() == buttonThatWasClicked) {

                if (ModifierKeys::currentModifiers.isAltDown()) {
                    // exclusive solo this one

                    bool newsolo = buttonThatWasClicked->getToggleState();

                    for (int j=0; j < changroups; ++j) {
                        if (newsolo) {
                            processor.setInputGroupSoloed(j, changroup == j);
                        }
                        else {
                            processor.setInputGroupSoloed(j, false);
                        }
                    }

                    // change solo for main monitor too
                    processor.getValueTreeState().getParameter(SonobusAudioProcessor::paramMainMonitorSolo)->setValueNotifyingHost(newsolo ? 1.0f : 0.0);

                    updateChannelViews();
                } else {
                    bool newsolo = buttonThatWasClicked->getToggleState();
                    processor.setInputGroupSoloed (changroup, newsolo);

                    if (newsolo) {
                        // only enable main in solo
                        processor.getValueTreeState().getParameter(SonobusAudioProcessor::paramMainMonitorSolo)->setValueNotifyingHost(1.0f);
                    }

                    updateChannelViews();
                }
                break;
            }
            else if (pvf->fxButton.get() == buttonThatWasClicked) {

                if (!effectsCalloutBox) {
                    showEffects(changroup, true, pvf->fxButton.get());
                } else {
                    showEffects(changroup, false);
                }

                break;
            }
        }

    }
}

void ChannelGroupsView::linkButtonPressed(int index, bool newlinkstate)
{
    int changroup = 0;
    int changroups = 0;
    int totalchans = 0;
    int chi = 0;

    if (mPeerMode) {
        changroups = processor.getRemotePeerChannelGroupCount(mPeerIndex);
        totalchans = processor.getRemotePeerRecvChannelCount(mPeerIndex);
    } else {
        changroups = processor.getInputGroupCount();
        totalchans = processor.getMainBusNumInputChannels();
    }


    int chstart = 0;
    int chcnt = 0;
    if (mPeerMode) {
        processor.getRemotePeerChannelGroupStartAndCount(mPeerIndex, changroup, chstart, chcnt);
    } else {
        processor.getInputGroupChannelStartAndCount(changroup, chstart, chcnt);
    }

    for (int i=0; i < mChannelViews.size(); ++i, ++chi) {

        if (i >= chstart+chcnt && changroup < changroups-1) {
            changroup++;
            chstart += chcnt;
            //chcnt = processor.getRemotePeerChannelGroupChannelCount(mPeerIndex, changroup);
            if (mPeerMode) {
                processor.getRemotePeerChannelGroupStartAndCount(mPeerIndex, changroup, chstart, chcnt);
            } else {
                processor.getInputGroupChannelStartAndCount(changroup, chstart, chcnt);
            }
            chi = 0;
        }

        if (i < index) {
            continue;
        }

        // got here we found the current position of this item

        if (newlinkstate) {
            if (i == 0) {
                // first channel is special, means link all
                if (mPeerMode) {
                    processor.setRemotePeerChannelGroupStartAndCount(mPeerIndex, 0, 0, totalchans); // add all from this group
                } else {
                    processor.setInputGroupChannelStartAndCount(0, 0, totalchans); // add all from this group
                    processor.setInputGroupCount(1);
                }
            }
            else if (chi == 0) {
                // we want to make this channel a part of the previous changroup if this is the first chan in changroup
                if (changroup > 0) {
                    int prevst, prevcnt;
                    if (mPeerMode) {
                        processor.getRemotePeerChannelGroupStartAndCount(mPeerIndex, changroup-1, prevst, prevcnt);
                        processor.setRemotePeerChannelGroupStartAndCount(mPeerIndex, changroup-1, prevst, prevcnt+chcnt); // add all from this group
                    } else {
                        processor.getInputGroupChannelStartAndCount(changroup-1, prevst, prevcnt);
                        processor.setInputGroupChannelStartAndCount(changroup-1, prevst, prevcnt+chcnt); // add all from this group
                    }
                }

                // remove this group, move all the next ones up
                if (mPeerMode) {
                    processor.removeRemotePeerChannelGroup(mPeerIndex, changroup);
                } else {
                    processor.removeInputChannelGroup(changroup);
                    --changroups;
                    processor.setInputGroupCount(changroups);
                }
                // TODO
            }
            else {
                // do nothing, it's already linked

            }
        } else {
            // unlinking a channel means putting it on its own group, possibly splitting an existing one

            if (i == 0) {
                // first channel is special, means unlink all
                if (mPeerMode) {
                    for (int chan=0; chan < totalchans; ++chan) {
                        processor.setRemotePeerChannelGroupStartAndCount(mPeerIndex, chan, chan, 1);
                    }
                } else {
                    for (int chan=0; chan < totalchans; ++chan) {
                        processor.setInputGroupChannelStartAndCount(chan, chan, 1); // add all from this group
                    }
                    processor.setInputGroupCount(totalchans);
                }
            }
            else if (chi == 0) {
                // shouldn't happen! the first channel in a group is never linked.. do nothing
            } else if (chi == chcnt-1) {
                // last one

                // reduce the count of this one
                if (mPeerMode) {
                    processor.setRemotePeerChannelGroupStartAndCount(mPeerIndex, changroup, chstart, chcnt-1); // remove one (us!)
                } else {
                    processor.setInputGroupChannelStartAndCount(changroup, chstart, chcnt-1); // remove one (us!)
                }

                // we just insert new one at changroup+1
                if (changroup < MAX_CHANGROUPS - 1) {
                    if (mPeerMode) {
                        processor.insertRemotePeerChannelGroup(mPeerIndex, changroup+1, i, 1);
                        processor.setRemotePeerChannelGroupName(mPeerIndex, changroup+1, processor.getRemotePeerChannelGroupName(mPeerIndex, changroup));
                    } else {
                        processor.insertInputChannelGroup(changroup+1, i, 1);
                        processor.setInputGroupName(changroup+1, processor.getInputGroupName(changroup));
                        changroups++;
                        processor.setInputGroupCount(changroups);
                    }

                }


            } else {
                // middle one

                int newcnt = chcnt - chi;
                // reduce the count of this one by newcnt
                if (mPeerMode) {
                    processor.setRemotePeerChannelGroupStartAndCount(mPeerIndex, changroup, chstart, chcnt-newcnt); // remove one (us!)
                } else {
                    processor.setInputGroupChannelStartAndCount(changroup, chstart, chcnt-newcnt); // remove one (us!)
                }


                // insert new one with newcnt
                // we just insert new one at changroup+1
                if (changroup < MAX_CHANGROUPS - 1) {
                    if (mPeerMode) {
                        processor.insertRemotePeerChannelGroup(mPeerIndex, changroup+1, i, newcnt);
                    } else {
                        processor.insertInputChannelGroup(changroup+1, i, newcnt);
                        changroups++;
                        processor.setInputGroupCount(changroups);
                    }

                }

            }

        }

        updateChannelViews();

        break;
    }
}

void ChannelGroupsView::showEffects(int index, bool flag, Component * fromView)
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
#if JUCE_IOS || JUCE_ANDROID
        int defHeight = 180;
#else
        int defHeight = 156;
#endif

        if (!mEffectsView) {
            mEffectsView = std::make_unique<ChannelGroupEffectsView>(processor, mPeerMode);
            mEffectsView->addListener(this);
        }

        auto minbounds = mEffectsView->getMinimumContentBounds();
        defWidth = minbounds.getWidth();
        defHeight = minbounds.getHeight();
        

        int extrawidth = 0;
        if (defHeight > dw->getHeight() - 24) {
            extrawidth = wrap->getScrollBarThickness() + 1;
        }

        wrap->setSize(jmin(defWidth + extrawidth, dw->getWidth() - 10), jmin(defHeight, dw->getHeight() - 24));

        
        mEffectsView->setBounds(Rectangle<int>(0,0,defWidth,defHeight));

        mEffectsView->peerMode = mPeerMode;
        mEffectsView->peerIndex = mPeerIndex;
        mEffectsView->groupIndex = index;

        mEffectsView->updateState();

        wrap->setViewedComponent(mEffectsView.get(), false);
        mEffectsView->setVisible(true);
        
        Rectangle<int> bounds =  dw->getLocalArea(nullptr, fromView->getScreenBounds());
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

void ChannelGroupsView::mouseUp (const MouseEvent& event)
{
    for (int i=0; i < mChannelViews.size(); ++i) {
        ChannelGroupView * pvf = mChannelViews.getUnchecked(i);

        if (event.eventComponent == pvf->meter.get()) {
            pvf->meter->clearClipIndicator(-1);
            break;
        }
    }
}

void ChannelGroupsView::clearClipIndicators()
{
    for (int i=0; i < mChannelViews.size(); ++i) {
        ChannelGroupView * pvf = mChannelViews.getUnchecked(i);
        pvf->meter->clearClipIndicator(-1);
    }
}


void ChannelGroupsView::showPopupMenu(Component * source, int index)
{
    if (index >= mChannelViews.size()) return;
    
    ChannelGroupView * pvf = mChannelViews.getUnchecked(index);
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

void ChannelGroupsView::labelTextChanged (Label* labelThatHasChanged)
{
    if (mPeerMode) {
        int changroup = 0;
        int changroups = processor.getRemotePeerChannelGroupCount(mPeerIndex);
        int chi = 0;

        int chstart = 0;
        int chcnt = 0;
        processor.getRemotePeerChannelGroupStartAndCount(mPeerIndex, changroup, chstart, chcnt);

        for (int i=0; i < mChannelViews.size(); ++i, ++chi)
        {
            if (i >= chstart+chcnt && changroup < changroups-1) {
                changroup++;
                chstart += chcnt;
                processor.getRemotePeerChannelGroupStartAndCount(mPeerIndex, changroup, chstart, chcnt);
                chi = 0;
            }


            ChannelGroupView * pvf = mChannelViews.getUnchecked(i);
            if (pvf->nameLabel.get() == labelThatHasChanged) {
                processor.setRemotePeerChannelGroupName(mPeerIndex, changroup, labelThatHasChanged->getText());
                break;
            }
        }
    }
    else {

        int changroup = 0;
        int changroups = processor.getInputGroupCount();
        int chi = 0;

        int chstart = 0;
        int chcnt = 1;
        processor.getInputGroupChannelStartAndCount(changroup, chstart, chcnt);

        for (int i=0; i < mChannelViews.size(); ++i, ++chi)
        {
            if (i >= chstart+chcnt && changroup < changroups-1) {
                changroup++;
                processor.getInputGroupChannelStartAndCount(changroup, chstart, chcnt);
                chi = 0;
            }


            ChannelGroupView * pvf = mChannelViews.getUnchecked(i);

            if (pvf->nameLabel.get() == labelThatHasChanged) {
                processor.setInputGroupName(changroup, labelThatHasChanged->getText());
                break;
            }
        }

    }
}

void ChannelGroupsView::genericItemChooserSelected(GenericItemChooser *comp, int index)
{

}

void ChannelGroupsView::effectsEnableChanged(ChannelGroupEffectsView *comp)
{
    updateChannelViews();
}



void ChannelGroupsView::sliderValueChanged (Slider* slider)
{

    if (mPeerMode) {
        int changroup = 0;
        int changroups = processor.getRemotePeerChannelGroupCount(mPeerIndex);
        int chi = 0;

        int chstart = 0;
        int chcnt = 0;
        processor.getRemotePeerChannelGroupStartAndCount(mPeerIndex, changroup, chstart, chcnt);

        for (int i=0; i < mChannelViews.size(); ++i, ++chi)
        {
            if (i >= chstart+chcnt && changroup < changroups-1) {
                changroup++;
                chstart += chcnt;
                processor.getRemotePeerChannelGroupStartAndCount(mPeerIndex, changroup, chstart, chcnt);
                chi = 0;
            }


            ChannelGroupView * pvf = mChannelViews.getUnchecked(i);
            if (pvf->levelSlider.get() == slider) {
                processor.setRemotePeerLevelGain(mPeerIndex, changroup, pvf->levelSlider->getValue());
                break;
            }
            else if (pvf->monitorSlider.get() == slider) {
                //
                break;
            }
            else if (pvf->panSlider.get() == slider) {
                if (pvf->panSlider->isTwoValue()) {
                    float pan1 = pvf->panSlider->getMinValue();
                    float pan2 = pvf->panSlider->getMaxValue();
                    processor.setRemotePeerChannelPan(mPeerIndex, changroup, 0, pan1);
                    processor.setRemotePeerChannelPan(mPeerIndex, changroup, 1, pan2);
                }
                else {
                    processor.setRemotePeerChannelPan(mPeerIndex, changroup, chi, pvf->panSlider->getValue());
                }
                break;
            }

        }
    }
    else {

        int changroup = 0;
        int changroups = processor.getInputGroupCount();
        int chi = 0;

        int chstart = 0;
        int chcnt = 1;
        processor.getInputGroupChannelStartAndCount(changroup, chstart, chcnt);

        for (int i=0; i < mChannelViews.size(); ++i, ++chi)
        {
            if (i >= chstart+chcnt && changroup < changroups-1) {
                changroup++;
                processor.getInputGroupChannelStartAndCount(changroup, chstart, chcnt);
                chi = 0;
            }


            ChannelGroupView * pvf = mChannelViews.getUnchecked(i);

            if (pvf->levelSlider.get() == slider) {
                processor.setInputGroupGain(changroup, pvf->levelSlider->getValue());
                break;
            }
            else if (pvf->monitorSlider.get() == slider) {
                processor.setInputMonitor(changroup, pvf->monitorSlider->getValue());
                break;
            }
            else if (pvf->panSlider.get() == slider) {
                if (pvf->panSlider->isTwoValue()) {
                    float pan1 = pvf->panSlider->getMinValue();
                    float pan2 = pvf->panSlider->getMaxValue();
                    processor.setInputChannelPan(changroup, 0, pan1);
                    processor.setInputChannelPan(changroup, 1, pan2);
                }
                else {
                    processor.setInputChannelPan(changroup, chi, pvf->panSlider->getValue());
                }
                break;
            }

        }

    }

}
