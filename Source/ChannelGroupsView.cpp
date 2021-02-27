// SPDX-License-Identifier: GPLv3-or-later
// Copyright (C) 2020 Jesse Chappell

#include "ChannelGroupsView.h"


using namespace SonoAudio;

struct DestChannelListItemData : public GenericItemChooserItem::UserData
{
public:
    DestChannelListItemData(const DestChannelListItemData & other) : startIndex(other.startIndex), count(other.count) {}
    DestChannelListItemData(int start, int cnt) : startIndex(start), count(cnt) {}

    int startIndex;
    int count;
};


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

    if (showDivider) {
        g.setColour(borderColor);
        g.drawLine(0, 0, getWidth(), 0, 1);
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

    //if (monitorSlider) {
    //    monitorSlider->setMouseDragSensitivity(jmax(128, monitorSlider->getWidth()));
    //}

    //Rectangle<int> optbounds(staticSendQualLabel->getX(), staticSendQualLabel->getY(), sendQualityLabel->getRight() - staticSendQualLabel->getX(), bufferLabel->getBottom() - sendQualityLabel->getY());
    //optionsButton->setBounds(optbounds);
    
}

//
// ==================================================
//

ChannelGroupsView::ChannelGroupsView(SonobusAudioProcessor& proc, bool peerMode, int peerIndex)
 : Component("pcv"),  addLnf(20), processor(proc), mPeerMode(peerMode), mPeerIndex(peerIndex)
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

    mAddButton = std::make_unique<TextButton>("+");
    mAddButton->onClick = [this] {  addGroupPressed(); };
    mAddButton->setLookAndFeel(&addLnf);
    mAddButton->setTooltip(TRANS("Add New Input Group"));
    addChildComponent(mAddButton.get());

    mClearButton = std::make_unique<TextButton>(TRANS("CLEAR"));
    mClearButton->onClick = [this] {  clearGroupsPressed(); };
    //mClearButton->setLookAndFeel(&addLnf);
    mClearButton->setTooltip(TRANS("Remove all input groups"));
    addChildComponent(mClearButton.get());

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

    if (monmode) {
        slider->setRange(0.0, 1.0, 0.0);
        slider->setMouseDragSensitivity(90);
    } else {
        slider->setRange(0.0, 2.0, 0.0);
    }
    slider->setSkewFactor(0.5);
    slider->setDoubleClickReturnValue(true, 1.0);
    slider->setTextBoxIsEditable(true);
    slider->setSliderSnapsToMousePosition(processor.getSlidersSnapToMousePosition());
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


void ChannelGroupsView::setPeerMode(bool peermode, int index)
{
    if (mPeerMode != peermode || index != mPeerIndex) {
        mPeerMode = peermode;
        mPeerIndex = index;
        updateLayout();
    }
}



void ChannelGroupsView::resized()
{
    Rectangle<int> bounds = getLocalBounds();

    if (!mPeerMode) {
        bounds = bounds.reduced(5, 0);
        bounds.removeFromLeft(3);
    }
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

    if (!mPeerMode) {
        bounds.reduce(1, 1);
        bounds.removeFromLeft(3);

        g.setColour(bgColor);
        g.fillRoundedRectangle(bounds.toFloat(), 6.0f);
        g.setColour(outlineColor);
        g.drawRoundedRectangle(bounds.toFloat(), 6.0f, 0.5f);
    }
}

ChannelGroupView * ChannelGroupsView::createChannelGroupView(bool first)
{
    ChannelGroupView * pvf = new ChannelGroupView();

    pvf->nameLabel = std::make_unique<Label>("name", "");
    pvf->nameLabel->setJustificationType(Justification::centredLeft);
    pvf->nameLabel->setFont(15);
    pvf->nameLabel->setEditable(!mPeerMode);
    pvf->nameLabel->addListener(this);
    if (!mPeerMode) {
        pvf->nameLabel->setColour(Label::backgroundColourId, Colours::black);
    }



    pvf->muteButton = std::make_unique<TextButton>(TRANS("MUTE"));
    pvf->muteButton->addListener(this);
    pvf->muteButton->setLookAndFeel(&pvf->medLnf);
    pvf->muteButton->setClickingTogglesState(true);
    pvf->muteButton->setColour(TextButton::buttonOnColourId, mutedColor);
    pvf->muteButton->setTooltip(TRANS("Mute this channel for both sending and monitoring"));

    pvf->soloButton = std::make_unique<TextButton>(TRANS("SOLO"));
    pvf->soloButton->addListener(this);
    pvf->soloButton->setLookAndFeel(&pvf->medLnf);
    pvf->soloButton->setClickingTogglesState(true);
    pvf->soloButton->setColour(TextButton::buttonOnColourId, soloColor.withAlpha(0.7f));
    pvf->soloButton->setColour(TextButton::textColourOnId, Colours::darkblue);
    if (mPeerMode) {
        pvf->soloButton->setTooltip(TRANS("Listen to only this channel for this user"));
    } else {
        pvf->soloButton->setTooltip(TRANS("Listen to only this channel, does not affect sending"));
    }

    
    pvf->chanLabel = std::make_unique<Label>("status", "");
    configLabel(pvf->chanLabel.get(), LabelTypeRegular);
    pvf->chanLabel->setJustificationType(Justification::centredLeft);

    
    pvf->levelSlider     = std::make_unique<Slider>(Slider::LinearHorizontal,  Slider::TextBoxRight);
    pvf->levelSlider->setName("level");
    pvf->levelSlider->addListener(this);

    configLevelSlider(pvf->levelSlider.get(), false);
    pvf->levelSlider->setLookAndFeel(&pvf->sonoSliderLNF);

    pvf->monitorSlider     = std::make_unique<Slider>(Slider::RotaryHorizontalVerticalDrag,  Slider::TextBoxRight);
    pvf->monitorSlider->setName("level");
    pvf->monitorSlider->addListener(this);

    configLevelSlider(pvf->monitorSlider.get(), true);
    pvf->monitorSlider->setLookAndFeel(&pvf->panSliderLNF);
    pvf->monitorSlider->setTextBoxStyle(Slider::NoTextBox, true, 60, 14);
    //pvf->monitorSlider->setTooltip(TRANS("Monitor output level"));


    pvf->panLabel = std::make_unique<Label>("pan", TRANS("Pan"));
    configLabel(pvf->panLabel.get(), LabelTypeSmall);
    pvf->panLabel->setJustificationType(Justification::centredTop);
    
    pvf->panSlider     = std::make_unique<Slider>(Slider::LinearHorizontal,  Slider::NoTextBox);
    pvf->panSlider->setName("pan1");
    pvf->panSlider->addListener(this);
    pvf->panSlider->getProperties().set ("fromCentre", true);
    pvf->panSlider->getProperties().set ("noFill", true);
    pvf->panSlider->setRange(-1, 1, 0.0f);
    pvf->panSlider->setDoubleClickReturnValue(true, 0.0);
    pvf->panSlider->setTextBoxIsEditable(false);
    pvf->panSlider->setSliderSnapsToMousePosition(false);
    pvf->panSlider->setScrollWheelEnabled(false);
    pvf->panSlider->setMouseDragSensitivity(100);

    pvf->panSlider->textFromValueFunction =  [](double v) -> String { if (fabs(v) < 0.01) return String(TRANS("Pan: Center")); return String(TRANS("Pan: ")) +  String((int)rint(abs(v*100.0f))) + ((v > 0 ? "% R" : "% L")) ; };
    pvf->panSlider->valueFromTextFunction =  [](const String& s) -> double { return s.getDoubleValue()*1e-2f; };
    pvf->panSlider->setValue(0.1, dontSendNotification);
    pvf->panSlider->setValue(0.0, dontSendNotification);
    pvf->panSlider->setLookAndFeel(&pvf->panSliderLNF);


    std::unique_ptr<Drawable> destimg(Drawable::createFromImageData(BinaryData::chevron_forward_svg, BinaryData::chevron_forward_svgSize));
    std::unique_ptr<Drawable> linkimg(Drawable::createFromImageData(BinaryData::chevron_forward_svg, BinaryData::chevron_forward_svgSize));

    pvf->linkButton = std::make_unique<SonoDrawableButton>("", DrawableButton::ButtonStyle::ImageRightOfTextLabel);

    if (mPeerMode && !first) {
        pvf->linkButton->setImages(linkimg.get());
    }
    else {
        pvf->linkButton->setImages(destimg.get());
    }


    pvf->linkButton->setForegroundImageRatio(0.5f);
    pvf->linkButton->setColour(SonoTextButton::outlineColourId, Colours::transparentBlack);
    pvf->linkButton->setColour(DrawableButton::backgroundOnColourId, Colour::fromFloatRGBA(0.4, 0.35, 0.4, 0.7));
    pvf->linkButton->setColour(DrawableButton::backgroundColourId, Colour::fromFloatRGBA(0.2, 0.2, 0.2, 0.6));
    pvf->linkButton->setClickingTogglesState(false);
    pvf->linkButton->setTriggeredOnMouseDown(false);
    pvf->linkButton->setLookAndFeel(&pvf->smallLnf);
    pvf->linkButton->addListener(this);
    //pvf->linkButton->addMouseListener(this, false);
    //pvf->linkButton->setAlpha(0.6f);
    pvf->linkButton->setTooltip(TRANS("Select Input channel source"));


    pvf->monoButton = std::make_unique<SonoDrawableButton>("", DrawableButton::ButtonStyle::ImageRightOfTextLabel);
    pvf->monoButton->setForegroundImageRatio(0.1f);
    pvf->monoButton->setColour(SonoTextButton::outlineColourId, Colours::transparentBlack);
    pvf->monoButton->setColour(DrawableButton::backgroundOnColourId, Colour::fromFloatRGBA(0.4, 0.35, 0.4, 0.7));
    pvf->monoButton->setColour(DrawableButton::backgroundColourId, Colour::fromFloatRGBA(0.2, 0.2, 0.2, 0.6));
    pvf->monoButton->setLookAndFeel(&pvf->smallLnf);
    //pvf->linkButton->addListener(this);
    //pvf->linkButton->addMouseListener(this, false);
    pvf->monoButton->setAlpha(0.8f);
    pvf->monoButton->setButtonText(TRANS("mono"));
    pvf->monoButton->setEnabled(false);

    pvf->destButton = std::make_unique<SonoDrawableButton>("", DrawableButton::ButtonStyle::ImageLeftOfTextLabel);

    pvf->destButton->setImages(destimg.get());

    pvf->destButton->setForegroundImageRatio(0.5f);
    //pvf->destButton->setColour(SonoTextButton::outlineColourId, Colour::fromFloatRGBA(0.4, 0.4, 0.4, 0.6));
    //pvf->destButton->setColour(DrawableButton::backgroundOnColourId, Colour::fromFloatRGBA(0.4, 0.35, 0.4, 0.7));
    pvf->destButton->setColour(DrawableButton::backgroundColourId, Colour::fromFloatRGBA(0.2, 0.2, 0.2, 0.6));
    pvf->destButton->setLookAndFeel(&pvf->smallLnf);
    pvf->destButton->addListener(this);
    if (mPeerMode) {
        pvf->destButton->setTooltip(TRANS("Choose destination output channels"));
    } else {
        pvf->destButton->setTooltip(TRANS("Choose destination monitoring channels"));
    }


    

#if JUCE_IOS
    //pvf->panSlider1->setPopupDisplayEnabled(true, false, pvf->pannersContainer.get());
    //pvf->panSlider2->setPopupDisplayEnabled(true, false, pvf->pannersContainer.get());
#endif

    pvf->fxButton = std::make_unique<TextButton>(TRANS("FX"));
    pvf->fxButton->setColour(TextButton::buttonOnColourId, Colour::fromFloatRGBA(0.2, 0.5, 0.7, 0.5));
    pvf->fxButton->addListener(this);
    pvf->fxButton->setLookAndFeel(&pvf->medLnf);
    if (!mPeerMode) {
        pvf->fxButton->setTooltip(TRANS("Edit input effects (applied before sending)"));
    } else {
        pvf->fxButton->setTooltip(TRANS("Edit effects"));
    }

    

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
    float subrate = mPeerMode && !processor.getRemotePeerViewExpanded(mPeerIndex) ? 0.0f : rate;
    metersActive = flag;


    if (mMainChannelView) {
        mMainChannelView->meter->setRefreshRateHz(rate);
    }

    for (int i=0; i < mChannelViews.size(); ++i) {
        ChannelGroupView * pvf = mChannelViews.getUnchecked(i);
        if (pvf->meter) {
            pvf->meter->setRefreshRateHz(subrate);
        }
    }
}



void ChannelGroupsView::rebuildChannelViews(bool notify)
{
    int numchans = 0;

    if (mPeerMode) {
        numchans = jmax(1, processor.getRemotePeerRecvChannelCount(mPeerIndex));

        if (!mMainChannelView) {
            mMainChannelView.reset(createChannelGroupView(true));
            mMainChannelView->linkButton->setClickingTogglesState(true);

            mMainChannelView->linkButton->onClick = [this]() {
                processor.setRemotePeerViewExpanded(mPeerIndex, mMainChannelView->linkButton->getToggleState());
                updateLayout();
                updateChannelViews();
                setMetersActive(metersActive);
                resized();
            };

            mMainChannelView->soloButton->onClick = [this]() {
                if (ModifierKeys::currentModifiers.isAltDown()) {
                    // exclusive solo this one

                    bool newsolo = mMainChannelView->soloButton->getToggleState();

                    for (int j=0; j < processor.getNumberRemotePeers(); ++j) {
                        if (newsolo) {
                            processor.setRemotePeerSoloed(j, mPeerIndex == j);
                        }
                        else {
                            processor.setRemotePeerSoloed(j, false);
                        }
                    }


                    // disable solo for main monitor too
                    processor.getValueTreeState().getParameter(SonobusAudioProcessor::paramMainMonitorSolo)->setValueNotifyingHost(0.0);

                    updateChannelViews();
                } else {
                    bool newsolo = mMainChannelView->soloButton->getToggleState();

                    processor.setRemotePeerSoloed(mPeerIndex, newsolo);

                    updateChannelViews();
                }
            };

            mMainChannelView->muteButton->onClick = [this]() {

                bool newmute = mMainChannelView->muteButton->getToggleState();


                if (!newmute) {
                    // allows receiving and invites
                    processor.setRemotePeerRecvActive(mPeerIndex, true);
                } else {
                    // turns off receiving and allow
                    processor.setRemotePeerRecvAllow(mPeerIndex, false);
                }

                updateChannelViews();
            };


            mMainChannelView->fxButton->onClick = [this]() {
                if (!effectsCalloutBox) {
                    showEffects(0, true, mMainChannelView->fxButton.get());
                } else {
                    showEffects(0, false);
                }
            };
        }
    } else {
        int changroups = processor.getInputGroupCount();
        numchans = 0;
        for (int i=0; i < changroups; ++i) {
            int chst, chcnt=0;
            processor.getInputGroupChannelStartAndCount(i, chst, chcnt);
            numchans += chcnt;
        }
    }

    while (mChannelViews.size() < numchans) {
        mChannelViews.add(createChannelGroupView(!mPeerMode && mChannelViews.size() == 0));
    }
    while (mChannelViews.size() > numchans) {
        mChannelViews.removeLast();
    }

    Component* dw = this->findParentComponentOfClass<AudioProcessorEditor>();
    if (!dw) dw = this->findParentComponentOfClass<Component>();
    if (!dw) dw = this;

    for (int i= (mPeerMode ? -1 : 0); i < mChannelViews.size(); ++i) {
        ChannelGroupView * pvf = i < 0 ? mMainChannelView.get() : mChannelViews.getUnchecked(i);

        pvf->addAndMakeVisible(pvf->linkButton.get());
        pvf->addChildComponent(pvf->monoButton.get());
        pvf->addAndMakeVisible(pvf->destButton.get());
        pvf->addAndMakeVisible(pvf->muteButton.get());
        pvf->addAndMakeVisible(pvf->soloButton.get());
        pvf->addAndMakeVisible(pvf->levelSlider.get());
        pvf->addChildComponent(pvf->monitorSlider.get());
        //pvf->addAndMakeVisible(pvf->levelLabel.get());
        pvf->addAndMakeVisible(pvf->panLabel.get());
        pvf->addAndMakeVisible(pvf->nameLabel.get());
        pvf->addAndMakeVisible(pvf->chanLabel.get());

        pvf->addAndMakeVisible(pvf->meter.get());
        pvf->addAndMakeVisible(pvf->fxButton.get());

        pvf->addAndMakeVisible(pvf->panSlider.get());

        pvf->panSlider->setPopupDisplayEnabled(true, true, dw);

        pvf->monitorSlider->setPopupDisplayEnabled(true, true, dw);

        addAndMakeVisible(pvf);
    }
    
    updateChannelViews();
    updateLayout(notify);
    resized();
}

void ChannelGroupsView::updateLayout(bool notify)
{
    int minitemheight = mPeerMode ? 34 : 30;
    int mincheckheight = 32;
    int minPannerWidth = isNarrow ? 56 : 64;
    int minButtonWidth = 60;
    int maxPannerWidth = 130;
    int compactMaxPannerWidth = 90;
    int minSliderWidth = isNarrow ? 90 : 100;
    int meterwidth = 10;
    int mainmeterwidth = 10;
    int mutebuttwidth = isNarrow ? 42 : 52;
    int linkbuttwidth = 50;
    int destbuttwidth = 44;
    int monsliderwidth = mPeerMode ? 0 : 40;
    int namewidth = isNarrow ? 88 : mPeerMode ? 110 : 100;
    int addrowheight = minitemheight - 2;

#if JUCE_IOS || JUCE_ANDROID
    // make the button heights a bit more for touchscreen purposes
    minitemheight = mPeerMode ? 42 : 38;
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


    int chi = 0;
    int changroup = 0;
    int sendcnt = 0;
    int totalchans = 0;
    int changroups = 0;

    int chstart = 0;
    int chcnt = 1;

    int deststart = 0;
    int destcnt = 1;

    bool pannervisible = true;

    int totaloutchans = 1;

    int estwidth = mEstimatedWidth > 13 ? mEstimatedWidth - 13 : 320;

    if (mPeerMode) {
        changroups = processor.getRemotePeerChannelGroupCount(mPeerIndex);

        totalchans = processor.getRemotePeerRecvChannelCount(mPeerIndex);

        processor.getRemotePeerChannelGroupStartAndCount(mPeerIndex, changroup, chstart, chcnt);
        processor.getRemotePeerChannelGroupDestStartAndCount(mPeerIndex, changroup, deststart, destcnt);
        destcnt = jmin(processor.getMainBusNumOutputChannels(), destcnt);

        totaloutchans = processor.getMainBusNumOutputChannels();

        if ((destcnt != 2 && chcnt <= destcnt) || chcnt == 0) {
            pannervisible = false;
        }

        if (totalchans > 2) {
            mainmeterwidth = 5 * totalchans;
        }
    }
    else {
        sendcnt = (int) processor.getValueTreeState().getParameter(SonobusAudioProcessor::paramSendChannels)->convertFrom0to1( processor.getValueTreeState().getParameter(SonobusAudioProcessor::paramSendChannels)->getValue());

        if (sendcnt == 0) {
            sendcnt = processor.getTotalNumInputChannels(); // processor.getMainBusNumInputChannels();
        }

        totalchans = processor.getTotalNumInputChannels();
        changroups = processor.getInputGroupCount();

        totaloutchans = processor.getMainBusNumOutputChannels();

        processor.getInputGroupChannelStartAndCount(changroup, chstart, chcnt);
        processor.getInputGroupChannelDestStartAndCount(changroup, deststart, destcnt);

        destcnt = jmin(processor.getMainBusNumOutputChannels(), destcnt);
        if ((sendcnt != 2 && destcnt != 2) || chcnt == 0) {
            pannervisible = false;
        }
    }



    // Main connected peer views

    if (!mPeerMode) {
        addrowBox.items.clear();
        addrowBox.flexDirection = FlexBox::Direction::row;
        addrowBox.items.add(FlexItem(4, 2).withMargin(0));
        addrowBox.items.add(FlexItem(linkbuttwidth, addrowheight, *mAddButton).withMargin(0).withFlex(0));
        addrowBox.items.add(FlexItem(6, 2).withMargin(0).withFlex(1));
        addrowBox.items.add(FlexItem(mutebuttwidth, addrowheight, *mClearButton).withMargin(0).withFlex(0));
        addrowBox.items.add(FlexItem(4, 2).withMargin(0));

        int gaph = 6 ; //changroups > 0 ? 2 : 6;
        int bgaph = changroups > 0 ? 4 : 6;
        channelsBox.items.add(FlexItem(8, gaph).withMargin(0));
        channelsBox.items.add(FlexItem(100, addrowheight, addrowBox).withMargin(0).withFlex(0));
        channelsBox.items.add(FlexItem(8, bgaph).withMargin(0));
        peersheight += addrowheight + gaph + bgaph;
    }



    for (int i = (mPeerMode ? -1 : 0); i < mChannelViews.size(); ++i, ++chi) {
        if (i==0) {
            chi = 0; // ensure this
        }

        if (chi >= chcnt && changroup < changroups-1) {
            changroup++;
            if (mPeerMode) {
                processor.getRemotePeerChannelGroupStartAndCount(mPeerIndex, changroup, chstart, chcnt);
                processor.getRemotePeerChannelGroupDestStartAndCount(mPeerIndex, changroup, deststart, destcnt);
                destcnt = jmin(processor.getMainBusNumOutputChannels(), destcnt);

                if ((destcnt != 2 && chcnt <= destcnt) || chcnt == 0) {
                    pannervisible = false;
                }
                else {
                    pannervisible = true;
                }
            } else {
                processor.getInputGroupChannelStartAndCount(changroup, chstart, chcnt);

                processor.getInputGroupChannelDestStartAndCount(changroup, deststart, destcnt);

                destcnt = jmin(processor.getMainBusNumOutputChannels(), destcnt);
                if ((sendcnt != 2 && destcnt != 2) || chcnt == 0) {
                    pannervisible = false;
                } else {
                    pannervisible = true;
                }
            }
            chi = 0;
        }

        ChannelGroupView * pvf = i < 0 ? mMainChannelView.get() : mChannelViews.getUnchecked(i);

        //pvf->updateLayout();
        bool viewexpanded = !mPeerMode || processor.getRemotePeerViewExpanded(mPeerIndex);

        if (!viewexpanded && i >= 0) {
            // skip this one, not expanded
            continue;
        }
        

        bool destbuttvisible = true;

        if (chi == 0 || !mPeerMode || (totalchans > 1) )
        {

            pvf->namebox.items.clear();
            pvf->namebox.flexDirection = FlexBox::Direction::row;
            //.withAlignSelf(FlexItem::AlignSelf::center));
            pvf->namebox.items.add(FlexItem(namewidth, minitemheight, *pvf->nameLabel).withMargin(0).withFlex(1));

            pvf->inbox.items.clear();
            pvf->inbox.flexDirection = FlexBox::Direction::row;
            // pvf->inbox.items.add(FlexItem(20, minitemheight, *pvf->chanLabel).withMargin(0).withFlex(0));
            // pvf->inbox.items.add(FlexItem(3, 3));

            //if (/*!mPeerMode && */ totalchans > 1) {
            if (mPeerMode && totalchans == 1) {
                pvf->inbox.items.add(FlexItem(linkbuttwidth, minitemheight, *pvf->monoButton).withMargin(0).withFlex(0));
            } else {
                pvf->inbox.items.add(FlexItem(linkbuttwidth, minitemheight, *pvf->linkButton).withMargin(0).withFlex(0));
            }
            pvf->inbox.items.add(FlexItem(3, 3));

            pvf->inbox.items.add(FlexItem(namewidth, minitemheight, pvf->namebox).withMargin(0).withFlex(0));
            if (!isNarrow) {
                pvf->inbox.items.add(FlexItem(6, 3));
                pvf->inbox.items.add(FlexItem(mutebuttwidth, minitemheight, *pvf->muteButton).withMargin(0).withFlex(0));
                pvf->inbox.items.add(FlexItem(3, 3));
                pvf->inbox.items.add(FlexItem(mutebuttwidth, minitemheight, *pvf->soloButton).withMargin(0).withFlex(0));

                pvf->inbox.items.add(FlexItem(5, 3));
                pvf->inbox.items.add(FlexItem(mutebuttwidth, minitemheight, *pvf->fxButton).withMargin(0).withFlex(0));

            }
            pvf->inbox.items.add(FlexItem(3, 3));
            pvf->inbox.items.add(FlexItem(minSliderWidth, minitemheight, *pvf->levelSlider).withMargin(0).withFlex(1));
            pvf->inbox.items.add(FlexItem(1, 3));

            /*
            if (mPeerMode && totalchans == 2) {
                pvf->inbox.items.add(FlexItem(2*meterwidth, minitemheight, *pvf->meter).withMargin(0).withFlex(0));
            }
            else {
                if (!isNarrow) {
                    pvf->inbox.items.add(FlexItem(meterwidth, minitemheight, *pvf->meter).withMargin(0).withFlex(0));
                }
            }
*/

            if (isNarrow) {
                pvf->inbox.items.add(FlexItem(2, 3));
            }


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

                if (mPeerMode && i < 0 ) {
                    pvf->monbox.items.add(FlexItem(mainmeterwidth, minitemheight, *pvf->meter).withMargin(0).withFlex(0));
                } else if (mPeerMode && totalchans == 2) {
                    pvf->monbox.items.add(FlexItem(2*meterwidth, minitemheight, *pvf->meter).withMargin(0).withFlex(0));
                } else {
                    pvf->monbox.items.add(FlexItem(meterwidth, minitemheight, *pvf->meter).withMargin(0).withFlex(0));
                }
                pvf->monbox.items.add(FlexItem(4, 3));

                //if (pannervisible)
                {
                    pvf->monbox.items.add(FlexItem(2, 3));
                    pvf->monbox.items.add(FlexItem(minPannerWidth, minitemheight, *pvf->panSlider).withMargin(0).withFlex(1).withMaxWidth(maxPannerWidth));
                    pvf->monbox.items.add(FlexItem(3, 3).withFlex(0.1).withMaxWidth(meterwidth + 10));
                }

                if (!mPeerMode) {
                    pvf->monbox.items.add(FlexItem(monsliderwidth, minitemheight, *pvf->monitorSlider).withMargin(0).withFlex(0));
                    pvf->monbox.items.add(FlexItem(2, 3));
                }


                if (destbuttvisible) {
                    pvf->monbox.items.add(FlexItem(destbuttwidth, minitemheight, *pvf->destButton).withMargin(0).withFlex(0));
                    pvf->monbox.items.add(FlexItem(2, 3));
                }

            }
            else {
                if (mPeerMode && i < 0 ) {
                    pvf->monbox.items.add(FlexItem(mainmeterwidth, minitemheight, *pvf->meter).withMargin(0).withFlex(0));
                }else if (mPeerMode && totalchans == 2) {
                    pvf->monbox.items.add(FlexItem(2*meterwidth, minitemheight, *pvf->meter).withMargin(0).withFlex(0));
                } else {
                    pvf->monbox.items.add(FlexItem(meterwidth, minitemheight, *pvf->meter).withMargin(0).withFlex(0));
                }
                pvf->monbox.items.add(FlexItem(4, 3));

                //if (pannervisible) {
                    pvf->monbox.items.add(FlexItem(minPannerWidth, minitemheight, *pvf->panSlider).withMargin(0).withFlex(0.25).withMaxWidth(maxPannerWidth));
                    pvf->monbox.items.add(FlexItem(3, 3));
                //}
                //pvf->monbox.items.add(FlexItem(mutebuttwidth, minitemheight, *pvf->fxButton).withMargin(0).withFlex(0));
                //pvf->monbox.items.add(FlexItem(2, 3));

                if (!mPeerMode) {
                    pvf->monbox.items.add(FlexItem(monsliderwidth, minitemheight, *pvf->monitorSlider).withMargin(0).withFlex(0));
                    pvf->monbox.items.add(FlexItem(2, 3));
                }

                if (destbuttvisible) {
                    pvf->monbox.items.add(FlexItem(destbuttwidth, minitemheight, *pvf->destButton).withMargin(0).withFlex(0));
                    pvf->monbox.items.add(FlexItem(2, 3));
                }
            }

            mpw = 0;
            mph = minitemheight;
            for (auto & item : pvf->monbox.items) {
                mpw += item.minWidth;
            }

            pvf->maincontentbox.items.clear();

            pvf->mainbox.items.clear();
            pvf->mainbox.flexDirection = FlexBox::Direction::column;

            bool dotopgap = (chi == 0) && (!mPeerMode || i >= 0 );

            if (i == -1 && mPeerMode) {
                pvf->mainbox.items.add(FlexItem(3, 4));
            }


            if (dotopgap) {
                // gap at top
                pvf->mainbox.items.add(FlexItem(3, 6));
            }


            if (isNarrow) {
                pvf->maincontentbox.flexDirection = FlexBox::Direction::column;
                pvf->maincontentbox.items.add(FlexItem(3, 2));
                if (chi == 0) {
                    pvf->maincontentbox.items.add(FlexItem(ipw, iph , pvf->inbox).withMargin(0).withFlex(0));
                    pvf->maincontentbox.items.add(FlexItem(2, 2));
                }
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
                pvf->maincontentbox.items.add(FlexItem(2, 2));
                //if (pannervisible)
                {
                    pvf->maincontentbox.items.add(FlexItem(mpw, mph , pvf->monbox).withMargin(0).withFlex(1).withMaxWidth(maxPannerWidth + mutebuttwidth + monsliderwidth + (destbuttvisible ? destbuttwidth + 2 : 0) + 2));
                }
                //else {
                //    pvf->maincontentbox.items.add(FlexItem(mpw, mph , pvf->monbox).withMargin(0).withFlex(0)); // (1).withMaxWidth(mutebuttwidth + destbuttwidth + 4));
                //}


                pvf->mainbox.items.add(FlexItem(60, iph, pvf->maincontentbox).withMargin(0).withFlex(0));
            }



            mbh = 0;
            for (auto & item : pvf->mainbox.items) {
                mbh += item.minHeight;
            }



            if (isNarrow) {
                channelsBox.items.add(FlexItem(ipw, mbh, *pvf).withMargin(0).withFlex(0));
                //peersheight += ph*2 + 6;
                peersheight += mbh + 2;
            } else {
                channelsBox.items.add(FlexItem(ipw+mpw + 6, mbh, *pvf).withMargin(1).withFlex(0));
                //peersheight += ph + 11;
                peersheight += mbh + 2;
            }

            if (i < mChannelViews.size()-1) {
                channelsBox.items.add(FlexItem(3, 4));
                peersheight += 4;
            }

        }

    }


    if (isNarrow) {
        channelMinHeight = std::max(mbh,  peersheight);
        channelMinWidth = ipw + 12;
    }
    else {
        if (!mPeerMode) {
            channelMinHeight = std::max(mbh + 18,  peersheight);
        } else {
            channelMinHeight = std::max(mbh,  peersheight);
        }
        channelMinWidth = ipw + mpw + 50;
    }

    if (notify) {
        listeners.call (&ChannelGroupsView::Listener::channelLayoutChanged, this);
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

        mAddButton->setVisible(false);
        mClearButton->setVisible(false);
    } else {
        updateInputModeChannelViews(specific);

        mAddButton->setVisible(true);
        mClearButton->setVisible(true);
    }
}


void ChannelGroupsView::updateInputModeChannelViews(int specific)
{
    uint32 nowstampms = Time::getMillisecondCounter();
    bool needsUpdateLayout = false;

    int changroup = 0;
    int sendcnt = (int) processor.getValueTreeState().getParameter(SonobusAudioProcessor::paramSendChannels)->convertFrom0to1( processor.getValueTreeState().getParameter(SonobusAudioProcessor::paramSendChannels)->getValue());

    if (sendcnt == 0) {
        sendcnt = processor.getTotalNumInputChannels(); // processor.getMainBusNumInputChannels();
    }

    int totalchans = processor.getTotalNumInputChannels(); // processor.getMainBusNumInputChannels();

    int totaloutchans = processor.getMainBusNumOutputChannels();
    int changroups = processor.getInputGroupCount();
    int chi = 0;

    int chstart = 0;
    int chcnt = 1;
    processor.getInputGroupChannelStartAndCount(changroup, chstart, chcnt);

    for (int i=0; i < mChannelViews.size(); ++i, ++chi) {
        if (specific >= 0 && specific != i) continue;

        if (chi >= chcnt && changroup < changroups-1) {
            changroup++;
            processor.getInputGroupChannelStartAndCount(changroup, chstart, chcnt);
            chi = 0;
        }

        ChannelGroupView * pvf = mChannelViews.getUnchecked(i);

        String name = processor.getInputGroupName(changroup);

        //DBG("Got username: '" << username << "'");

        pvf->nameLabel->setText(name, dontSendNotification);

        String chantext;
        if (chi == 0 && chcnt > 1) {
            chantext << chstart+1 << "-" << chstart+chcnt;
        } else {
            chantext << chstart + chi + 1;
        }
        //pvf->chanLabel->setText(String::formatted("%d", i+1), dontSendNotification);

        pvf->linkButton->setButtonText(chantext);

        bool aresoloed = processor.getInputGroupSoloed(changroup);
        bool aremuted = processor.getInputGroupMuted(changroup);

        pvf->showDivider = chi == 0 ; // || i == 0;

        //if (i == 0) {
            // only if all are linked
        //    pvf->linkButton->setToggleState(chcnt == mChannelViews.size(), dontSendNotification);
            //pvf->linkButton->setVisible(false);
        //} else
        //{
        //    pvf->linkButton->setToggleState(pvf->linked, dontSendNotification);
            //pvf->linkButton->setVisible(true);
        //}

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


        if (!pvf->monitorSlider->isMouseOverOrDragging()) {
            pvf->monitorSlider->setValue(processor.getInputMonitor(changroup), dontSendNotification);
        }


        pvf->meter->setMeterSource (&processor.getInputMeterSource());
        pvf->meter->setSelectedChannel(chstart + chi);
        pvf->meter->setFixedNumChannels(1);

        int deststart = 0;
        int destcnt = 2;
        processor.getInputGroupChannelDestStartAndCount(changroup, deststart, destcnt);
        destcnt = jmin(processor.getMainBusNumOutputChannels(), destcnt);

        String desttext;
        if (destcnt == 1) {
            desttext << deststart + 1;
        } else {
            desttext << deststart + 1 << "-" << deststart+destcnt;
        }
        pvf->destButton->setButtonText(desttext);

        if (sendcnt != 2 && destcnt != 2) {
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
        bool destbuttvisible = isprimary; // && chcnt < totaloutchans;

        pvf->levelSlider->setVisible(isprimary);
        pvf->soloButton->setVisible(isprimary);
        pvf->muteButton->setVisible(isprimary);
        pvf->nameLabel->setVisible(isprimary);
        pvf->destButton->setVisible(destbuttvisible);
        pvf->monitorSlider->setVisible(isprimary);


        // effects aren't used if channel count is above 2, right now
        pvf->fxButton->setVisible(isprimary && chcnt <= 2);

        pvf->linkButton->setVisible(isprimary);
        pvf->monoButton->setVisible(false);

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

    if (mMainChannelView.get() == nullptr) return;

    int changroup = 0;
    int changroups = processor.getRemotePeerChannelGroupCount(mPeerIndex);
    int chi = 0;

    int totalchans = processor.getRemotePeerRecvChannelCount(mPeerIndex);
    int totaloutchans = processor.getMainBusNumOutputChannels();

    int chstart = 0;
    int chcnt = 0;
    processor.getRemotePeerChannelGroupStartAndCount(mPeerIndex, changroup, chstart, chcnt);

    int deststart = 0;
    int destcnt = 2;
    processor.getRemotePeerChannelGroupDestStartAndCount(mPeerIndex, changroup, deststart, destcnt);
    destcnt = jmin(processor.getMainBusNumOutputChannels(), destcnt);

    bool expanded = processor.getRemotePeerViewExpanded(mPeerIndex);

    // deal with main strip

    bool connected = processor.getRemotePeerConnected(mPeerIndex);
    String username = processor.getRemotePeerUserName(mPeerIndex);

    mMainChannelView->nameLabel->setText(username, dontSendNotification);

    String chantext;
    chantext << totalchans << TRANS("ch");
    mMainChannelView->linkButton->setButtonText(chantext);

    bool safetymuted = processor.getRemotePeerSafetyMuted(mPeerIndex);
    bool recvactive = processor.getRemotePeerRecvActive(mPeerIndex);
    bool recvallow = processor.getRemotePeerRecvAllow(mPeerIndex);

    bool mainsoloed = processor.getRemotePeerSoloed(mPeerIndex);
    bool mainmuted = !processor.getRemotePeerRecvAllow(mPeerIndex);
    if (safetymuted || (processor.isAnythingSoloed() && !mainsoloed && !mainmuted)) {
        mMainChannelView->muteButton->setColour(TextButton::buttonColourId, mutedBySoloColor);
    } else {
        mMainChannelView->muteButton->removeColour(TextButton::buttonColourId);
    }

    //if (chi == 0 && totalchans==1) {
    //    mMainChannelView->monoButton->setVisible(true);
    //    mMainChannelView->linkButton->setVisible(false);
    //    } else
    {
        mMainChannelView->linkButton->setVisible(true);
        mMainChannelView->monoButton->setVisible(false);
    }
    //pvf->linkButton->setAlpha(totalchans > 1 ? 0.6f : 0.4f);

    mMainChannelView->muteButton->setToggleState(mainmuted , dontSendNotification);
    mMainChannelView->soloButton->setToggleState(mainsoloed , dontSendNotification);

    if (!mMainChannelView->levelSlider->isMouseOverOrDragging()) {
        mMainChannelView->levelSlider->setValue(processor.getRemotePeerLevelGain(mPeerIndex), dontSendNotification);
    }


    mMainChannelView->meter->setMeterSource (processor.getRemotePeerRecvMeterSource(mPeerIndex));
    mMainChannelView->meter->setSelectedChannel(0);

    if (expanded || changroups > 1 || (destcnt != 2 && chcnt <= destcnt)) {
        mMainChannelView->panSlider->setVisible(false);
        mMainChannelView->panLabel->setVisible(false);
    }
    else if (chcnt == 1) {
        mMainChannelView->panLabel->setVisible(true);
        mMainChannelView->panSlider->setVisible(true);
        mMainChannelView->panSlider->setDoubleClickReturnValue(true, 0.0);

        if (!mMainChannelView->singlePanner) {
            mMainChannelView->panSlider->setSliderStyle(Slider::LinearHorizontal); // LinearBar
            mMainChannelView->panSlider->setTextBoxStyle(Slider::NoTextBox, true, 10, 2); // TextBoxAbove

            mMainChannelView->singlePanner = true;
        }

    } else if (chcnt == 2) {
        mMainChannelView->panLabel->setVisible(true);
        mMainChannelView->panSlider->setVisible(true);
        mMainChannelView->panSlider->setDoubleClickReturnValue(true, (chi & 2) ? 1.0f: -1.0f);

        if (mMainChannelView->singlePanner && totalchans == 2 && chi == 0) {
            mMainChannelView->panSlider->setSliderStyle(Slider::TwoValueHorizontal);
            mMainChannelView->panSlider->setTextBoxStyle(Slider::NoTextBox, true, 60, 12);

            mMainChannelView->singlePanner = false;
        } else if (!mMainChannelView->singlePanner && totalchans != 2) {
            mMainChannelView->panSlider->setSliderStyle(Slider::LinearHorizontal); // LinearBar
            mMainChannelView->panSlider->setTextBoxStyle(Slider::NoTextBox, true, 10, 2); // TextBoxAbove

            mMainChannelView->singlePanner = true;
        }
    } else {
        if (!mMainChannelView->singlePanner && totalchans != 2) {
            mMainChannelView->panSlider->setSliderStyle(Slider::LinearHorizontal); // LinearBar
            mMainChannelView->panSlider->setTextBoxStyle(Slider::NoTextBox, true, 10, 2); // TextBoxAbove

            mMainChannelView->singlePanner = true;
        }

        mMainChannelView->panSlider->setVisible(false);
        mMainChannelView->panLabel->setVisible(false);
    }

    if (mMainChannelView->panSlider->isTwoValue()) {
        mMainChannelView->panSlider->setMinAndMaxValues(processor.getRemotePeerChannelPan(mPeerIndex, changroup, 0), processor.getRemotePeerChannelPan(mPeerIndex, changroup, 1), dontSendNotification);
    }
    else {
        mMainChannelView->panSlider->setValue(processor.getRemotePeerChannelPan(mPeerIndex, changroup, chi), dontSendNotification);
    }

    mMainChannelView->destButton->setVisible(false);
    mMainChannelView->monitorSlider->setVisible(false);

    float disalpha = 0.4;
    mMainChannelView->nameLabel->setAlpha(connected ? 1.0 : 0.8);
    mMainChannelView->levelSlider->setAlpha((recvactive && !safetymuted) ? 1.0 : disalpha);

    // effects aren't used if channel count is above 2, right now
    mMainChannelView->fxButton->setVisible(!expanded && changroups == 1 && chcnt <= 2);
    bool infxon = processor.getRemotePeerEffectsActive(mPeerIndex, changroup);
    mMainChannelView->fxButton->setToggleState(infxon, dontSendNotification);


    for (int i=0; expanded && i < mChannelViews.size(); ++i, ++chi) {
        if (specific >= 0 && specific != i) continue;


        if (chi >= chcnt && changroup < changroups-1) {
            changroup++;
            chstart += chcnt;

            processor.getRemotePeerChannelGroupStartAndCount(mPeerIndex, changroup, chstart, chcnt);
            chi = 0;
        }


        ChannelGroupView * pvf = mChannelViews.getUnchecked(i);

        String name = processor.getRemotePeerChannelGroupName (mPeerIndex, changroup);

        //DBG("Got username: '" << username << "'");
        String dispname = name ; //(i==0) ? username : "";
        //if (name.isNotEmpty()) {
        //    dispname << " | " << name;
        //}

        pvf->nameLabel->setText(dispname, dontSendNotification);

        //pvf->chanLabel->setText(String::formatted("%d", i+1), dontSendNotification);

        String chantext;
        if (chi == 0 && chcnt > 1) {
            chantext << chstart+1 << "-" << chstart+chcnt;
        } else {
            chantext << chstart + chi + 1;
        }
        pvf->linkButton->setButtonText(chantext);


        int chstart=0, chcnt=0;
        processor.getRemotePeerChannelGroupStartAndCount(mPeerIndex, changroup, chstart, chcnt);

        bool aresoloed = processor.getRemotePeerChannelSoloed(mPeerIndex, changroup) /* || processor.getRemotePeerSoloed(mPeerIndex) */;
        bool aremuted = !processor.getRemotePeerRecvAllow(mPeerIndex) || processor.getRemotePeerChannelMuted(mPeerIndex, changroup);

        if (safetymuted || (processor.isAnythingSoloed() && (!mainsoloed || (!aresoloed && !aremuted)))) {
            pvf->muteButton->setColour(TextButton::buttonColourId, mutedBySoloColor);
        } else {
            pvf->muteButton->removeColour(TextButton::buttonColourId);
        }

        pvf->showDivider = chi == 0; //  && i != 0 ; //&& i < mChannelViews.size()-1; // || i == 0;

        //if (i == 0) {
            // only if all are linked
        //    pvf->linkButton->setToggleState(chcnt == mChannelViews.size(), dontSendNotification);
            //pvf->linkButton->setVisible(false);
        //} else
        //{
        //    pvf->linkButton->setToggleState(pvf->linked, dontSendNotification);
            //pvf->linkButton->setVisible(true);
        //}

        if (chi == 0 && totalchans==1) {
            pvf->monoButton->setVisible(true);
            pvf->linkButton->setVisible(false);
        } else {
            pvf->linkButton->setVisible(true);
            pvf->monoButton->setVisible(false);

            if (i < 0) {
                pvf->linkButton->setToggleState(expanded, dontSendNotification);
            }
            else { // if (chi == 0 && chcnt > 1) {
                pvf->linkButton->setToggleState(false, dontSendNotification);
            }
        }
        //pvf->linkButton->setAlpha(totalchans > 1 ? 0.6f : 0.4f);


        pvf->muteButton->setToggleState(aremuted , dontSendNotification);
        pvf->soloButton->setToggleState(aresoloed , dontSendNotification);


        bool infxon = processor.getRemotePeerEffectsActive(mPeerIndex, changroup);
        pvf->fxButton->setToggleState(infxon, dontSendNotification);

        if (!pvf->levelSlider->isMouseOverOrDragging()) {
            pvf->levelSlider->setValue(processor.getRemotePeerChannelGain (mPeerIndex, changroup), dontSendNotification);
        }


        pvf->meter->setMeterSource (processor.getRemotePeerRecvMeterSource(mPeerIndex));
        pvf->meter->setSelectedChannel(i);
        //if (mPeerMode && totalchans == 2 && chi == 0) {
        //    pvf->meter->setFixedNumChannels(2);
        //} else
        {
            pvf->meter->setFixedNumChannels(1);
        }


        deststart = 0;
        destcnt = 2;
        processor.getRemotePeerChannelGroupDestStartAndCount(mPeerIndex, changroup, deststart, destcnt);
        destcnt = jmin(processor.getMainBusNumOutputChannels(), destcnt);

        String desttext;
        if (destcnt == 1) {
            desttext << deststart + 1;
        } else {
            desttext << deststart + 1 << "-" << deststart+destcnt;
        }
        pvf->destButton->setButtonText(desttext);

        if (destcnt != 2 && chcnt <= destcnt) {
            pvf->panSlider->setVisible(false);
            pvf->panLabel->setVisible(false);
        }
        else {
            //if (chcnt == 1) {
            pvf->panLabel->setVisible(true);
            pvf->panSlider->setVisible(true);
            pvf->panSlider->setDoubleClickReturnValue(true, 0.0);

            if (!pvf->singlePanner) {
                pvf->panSlider->setSliderStyle(Slider::LinearHorizontal); // LinearBar
                pvf->panSlider->setTextBoxStyle(Slider::NoTextBox, true, 10, 2); // TextBoxAbove

                pvf->singlePanner = true;
            }
                        
        }
        /*
        else if (chcnt == 2) {
            pvf->panLabel->setVisible(true);
            pvf->panSlider->setVisible(true);
            pvf->panSlider->setDoubleClickReturnValue(true, (chi & 2) ? 1.0f: -1.0f);

            if (pvf->singlePanner && totalchans == 2 && chi == 0) {
                pvf->panSlider->setSliderStyle(Slider::TwoValueHorizontal);
                pvf->panSlider->setTextBoxStyle(Slider::NoTextBox, true, 60, 12);

                pvf->singlePanner = false;
            } else if (!pvf->singlePanner && totalchans != 2) {
                pvf->panSlider->setSliderStyle(Slider::LinearHorizontal); // LinearBar
                pvf->panSlider->setTextBoxStyle(Slider::NoTextBox, true, 10, 2); // TextBoxAbove

                pvf->singlePanner = true;
            }
        } else {
            if (!pvf->singlePanner && totalchans != 2) {
                pvf->panSlider->setSliderStyle(Slider::LinearHorizontal); // LinearBar
                pvf->panSlider->setTextBoxStyle(Slider::NoTextBox, true, 10, 2); // TextBoxAbove

                pvf->singlePanner = true;
            }
        }
         */

        if (pvf->panSlider->isTwoValue()) {
            pvf->panSlider->setMinAndMaxValues(processor.getRemotePeerChannelPan(mPeerIndex, changroup, 0), processor.getRemotePeerChannelPan(mPeerIndex, changroup, 1), dontSendNotification);
        }
        else {        
            pvf->panSlider->setValue(processor.getRemotePeerChannelPan(mPeerIndex, changroup, chi), dontSendNotification);
        }

        bool isprimary = chi == 0;
        bool destbuttvisible = isprimary && chcnt < totaloutchans;
        pvf->levelSlider->setVisible(isprimary);
        pvf->soloButton->setVisible(isprimary);
        pvf->muteButton->setVisible(isprimary);
        pvf->nameLabel->setVisible(isprimary);
        pvf->destButton->setVisible(destbuttvisible);
        pvf->monitorSlider->setVisible(false);

        const float disalpha = 0.4;
        pvf->nameLabel->setAlpha(connected ? 1.0 : 0.8);
        pvf->levelSlider->setAlpha((recvactive && !safetymuted) ? 1.0 : disalpha);

        // effects aren't used if channel count is above 2, right now
        pvf->fxButton->setVisible(isprimary && chcnt <= 2);

        pvf->repaint();
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

            if (chi >= chcnt && changroup < changroups-1) {
                changroup++;
                processor.getRemotePeerChannelGroupStartAndCount(mPeerIndex, changroup, chstart, chcnt);
                chi = 0;
            }

            ChannelGroupView * pvf = mChannelViews.getUnchecked(i);

            if (pvf->muteButton.get() == buttonThatWasClicked) {

                bool newmute = buttonThatWasClicked->getToggleState();

                processor.setRemotePeerChannelMuted (mPeerIndex, changroup, newmute);

                // see if any are muted
                bool anysubmute = false;
                bool allsubmute = true;
                for (int gi=0; gi < changroups; ++gi) {
                    if (processor.getRemotePeerChannelMuted(mPeerIndex, gi)) {
                        anysubmute = true;
                    } else {
                        allsubmute = false;
                    }
                }

                bool recvactive = processor.getRemotePeerRecvActive(mPeerIndex);

                if (!recvactive && !allsubmute) {
                    // allows receiving and invites
                    processor.setRemotePeerRecvActive(mPeerIndex, true);
                } else if (allsubmute && recvactive) {
                    // turns off receiving and allow
                    processor.setRemotePeerRecvAllow(mPeerIndex, false);
                }

                updateChannelViews();
                break;
            }
            else if (pvf->linkButton.get() == buttonThatWasClicked) {
                //processor.setRemotePeerChannelMuted (mPeerIndex, changroup, buttonThatWasClicked->getToggleState());
                //updateChannelViews();

                peerChanButtonPressed(buttonThatWasClicked, i, buttonThatWasClicked->getToggleState());

                break;
            }
            else if (pvf->destButton.get() == buttonThatWasClicked) {
                //processor.setRemotePeerChannelMuted (mPeerIndex, changroup, buttonThatWasClicked->getToggleState());
                //updateChannelViews();

                showDestSelectionMenu(buttonThatWasClicked, i);

                break;
            }
            else if (pvf->soloButton.get() == buttonThatWasClicked) {
                if (ModifierKeys::currentModifiers.isAltDown()) {
                    // exclusive solo this one

                    bool newsolo = buttonThatWasClicked->getToggleState();

                    // exclusive solo this one XX

                    for (int gi=0; gi < changroups; ++gi) {
                        if (newsolo) {
                            processor.setRemotePeerChannelSoloed(mPeerIndex, gi, changroup == gi);
                        }
                        else {
                            processor.setRemotePeerChannelSoloed(mPeerIndex, gi, false);
                        }
                    }

                    /*
                    for (int j=0; j < processor.getNumberRemotePeers(); ++j) {
                        if (newsolo) {
                            processor.setRemotePeerSoloed(j, mPeerIndex == j);
                        }
                        else {
                            processor.setRemotePeerSoloed(j, false);
                        }
                    }
                     */

                    // disable solo for main monitor too
                    //processor.getValueTreeState().getParameter(SonobusAudioProcessor::paramMainMonitorSolo)->setValueNotifyingHost(0.0);

                    updateChannelViews();
                } else {
                    bool newsolo = buttonThatWasClicked->getToggleState();

                    processor.setRemotePeerChannelSoloed (mPeerIndex, changroup, newsolo);

                    bool anysolo = false;
                    for (int j=0; j < changroups; ++j) {
                        if (processor.getRemotePeerChannelSoloed(mPeerIndex, j)) {
                            anysolo = true;
                        }
                    }

                    //if (newsolo) {
                    //    processor.setRemotePeerSoloed(mPeerIndex, newsolo);
                    //} else if (!anysolo) {
                    //    processor.setRemotePeerSoloed(mPeerIndex, false);
                    //}


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
            if (chi >= chcnt && changroup < changroups-1) {
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

                inputButtonPressed(buttonThatWasClicked, i, buttonThatWasClicked->getToggleState());

                break;
            }
            else if (pvf->destButton.get() == buttonThatWasClicked) {
                //processor.setRemotePeerChannelMuted (mPeerIndex, changroup, buttonThatWasClicked->getToggleState());
                //updateChannelViews();

                showDestSelectionMenu(buttonThatWasClicked, i);

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
                    //processor.getValueTreeState().getParameter(SonobusAudioProcessor::paramMainMonitorSolo)->setValueNotifyingHost(newsolo ? 1.0f : 0.0);

                    updateChannelViews();
                } else {
                    bool newsolo = buttonThatWasClicked->getToggleState();
                    processor.setInputGroupSoloed (changroup, newsolo);

                    //if (newsolo) {
                       // only enable main in solo
                      //  processor.getValueTreeState().getParameter(SonobusAudioProcessor::paramMainMonitorSolo)->setValueNotifyingHost(1.0f);
                    //}

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

void ChannelGroupsView::clearGroupsPressed()
{
    if (mPeerMode) return; // not used for it

    Array<GenericItemChooserItem> items;
    items.add(GenericItemChooserItem(TRANS("Confirm Remove of All Inputs")));

    Component* dw = mClearButton->findParentComponentOfClass<AudioProcessorEditor>();
    if (!dw) dw = mClearButton->findParentComponentOfClass<Component>();
    Rectangle<int> bounds =  dw->getLocalArea(nullptr, mClearButton->getScreenBounds());

    SafePointer<ChannelGroupsView> safeThis(this);

    auto callback = [safeThis](GenericItemChooser* chooser,int index) mutable {
        if (!safeThis) return;

        safeThis->processor.setInputGroupCount(0);
        safeThis->rebuildChannelViews(true);
    };

    GenericItemChooser::launchPopupChooser(items, bounds, dw, callback, -1, dw ? dw->getHeight()-30 : 0);

}

void ChannelGroupsView::addGroupPressed()
{
    if (mPeerMode) return; // not used for it

    Array<GenericItemChooserItem> items;

    int totalouts = processor.getMainBusNumOutputChannels();
    int totalins = processor.getTotalNumInputChannels();
    totalins = jmin(totalins, MAX_CHANNELS);

    items.add(GenericItemChooserItem(TRANS("ADD INPUT GROUP:"), {}, nullptr, false));

    for (int i=0; i < totalins; ++i) {
        String name;
        if (i == 0) {
            name << TRANS("Mono");
        } else if (i == 1) {
            name << TRANS("Stereo");
        } else {
            name << i+1 << " " << TRANS("channel");
        }

        items.add(GenericItemChooserItem(name, {}, nullptr, i==0));
    }


    Component* dw = mAddButton->findParentComponentOfClass<AudioProcessorEditor>();
    if (!dw) dw = mAddButton->findParentComponentOfClass<Component>();
    Rectangle<int> bounds =  dw->getLocalArea(nullptr, mAddButton->getScreenBounds());

    SafePointer<ChannelGroupsView> safeThis(this);

    auto callback = [safeThis,totalins,totalouts](GenericItemChooser* chooser,int index) mutable {
        if (!safeThis) return;
        int insAt = safeThis->processor.getInputGroupCount();
        int chstart = 0;
        if (insAt > 0) {
            int chcnt;
            safeThis->processor.getInputGroupChannelStartAndCount(insAt-1, chstart, chcnt);
            chstart = jmin(totalins - (index), chstart+chcnt);
        }
        // add new group to end, defaulting to after last existing
        if (safeThis->processor.insertInputChannelGroup(insAt, chstart, index)) {
            safeThis->processor.setInputGroupChannelDestStartAndCount(insAt, 0, jmin(totalouts, jmax(2, index)));
            safeThis->processor.setInputGroupCount(insAt+1);
            safeThis->rebuildChannelViews(true);
        }
    };

    GenericItemChooser::launchPopupChooser(items, bounds, dw, callback, -1, dw ? dw->getHeight()-30 : 0);
}

void ChannelGroupsView::showChangeGroupChannels(int changroup, Component * showfrom)
{
    if (mPeerMode) return; // not used for it

    Array<GenericItemChooserItem> items;

    int totalouts = processor.getMainBusNumOutputChannels();
    int totalins = processor.getTotalNumInputChannels();

    totalins = jmin(totalins, MAX_CHANNELS);

    items.add(GenericItemChooserItem(TRANS("CHANGE CHANNEL LAYOUT:")));

    for (int i=0; i < totalins; ++i) {
        String name;
        if (i == 0) {
            name << TRANS("Mono");
        } else if (i == 1) {
            name << TRANS("Stereo");
        } else {
            name << i+1 << " " << TRANS("channel");
        }

        items.add(GenericItemChooserItem(name, {}, nullptr, i==0));
    }


    Component* dw = showfrom->findParentComponentOfClass<AudioProcessorEditor>();
    if (!dw) dw = showfrom->findParentComponentOfClass<Component>();
    Rectangle<int> bounds =  dw->getLocalArea(nullptr, showfrom->getScreenBounds());

    SafePointer<ChannelGroupsView> safeThis(this);

    auto callback = [safeThis,changroup,totalins,totalouts](GenericItemChooser* chooser,int index) mutable {
        if (!safeThis) return;
        int chstart = 0;
        int chcnt;
        safeThis->processor.getInputGroupChannelStartAndCount(changroup, chstart, chcnt);
        // just set the new channel count
        safeThis->processor.setInputGroupChannelStartAndCount(changroup, chstart, index);
        safeThis->rebuildChannelViews(true);
    };

    GenericItemChooser::launchPopupChooser(items, bounds, dw, callback, -1, dw ? dw->getHeight()-30 : 0);
}



int ChannelGroupsView::getChanGroupFromIndex(int index)
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
        totalchans = processor.getTotalNumInputChannels(); // processor.getMainBusNumInputChannels();
    }


    int chstart = 0;
    int chcnt = 0;
    if (mPeerMode) {
        processor.getRemotePeerChannelGroupStartAndCount(mPeerIndex, changroup, chstart, chcnt);
    } else {
        processor.getInputGroupChannelStartAndCount(changroup, chstart, chcnt);
    }

    for (int i=0; i < mChannelViews.size(); ++i, ++chi) {

        if (chi >= chcnt && changroup < changroups-1) {
            changroup++;
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

        // got here we're done
        return changroup;
    }

    return 0; // failed, return something safe
}

void ChannelGroupsView::peerChanButtonPressed(Component * source, int index, bool newlinkstate)
{
    // jlc  show selector to change the target start channel for this group
    Array<GenericItemChooserItem> items;



}

void ChannelGroupsView::inputButtonPressed(Component * source, int index, bool newlinkstate)
{
    // jlc  show selector to change the target start channel for this group
    Array<GenericItemChooserItem> items;


    int chstart=0, chcnt=0;
    int totalins = 0;

    int changroup =  getChanGroupFromIndex(index);

    if (mPeerMode) {
        totalins = processor.getRemotePeerRecvChannelCount(mPeerIndex);
        processor.getRemotePeerChannelGroupStartAndCount(mPeerIndex, changroup, chstart, chcnt);
    }
    else {
        totalins = processor.getTotalNumInputChannels();
        processor.getInputGroupChannelStartAndCount(changroup, chstart, chcnt);
    }

    // for each number of channel counts possible (1-chcnt)
    int selindex = -1;

    items.add(GenericItemChooserItem(chcnt > 1 ? TRANS("SELECT INPUTS:") : TRANS("SELECT INPUT:"), {}, nullptr, false));


    StringArray inputnames;
    if (JUCEApplicationBase::isStandaloneApp() && getAudioDeviceManager()) {
        auto adm = getAudioDeviceManager();
        if (auto cad = adm->getCurrentAudioDevice()) {
            auto activeIns = cad->getActiveInputChannels();
            auto allinputnames = cad->getInputChannelNames();
            int ind = 0;
            for ( int ni=0; ni < allinputnames.size(); ++ni ) {
                if (activeIns[ni]) {
                    String name;
                    //name << ind + 1 << ": " << allinputnames[ni];
                    name << allinputnames[ni];
                    inputnames.add(name);
                    ++ind;
                }
            }
        }
    }
    else {
        for (int i=0; i < totalins; ++i) {
            int busnum = 0;
            auto bch = processor.getOffsetInBusBufferForAbsoluteChannelIndex(true, i, busnum);
            String chname;
            String name;
            if (auto bus = processor.getBus(true, busnum)) {
                chname = bus->getName();
                //name << i+1 << ": " << chname << " " << bch+1;
                name << chname << " " << bch+1;
            } else {
                name << i+1;
            }

            inputnames.add(name);
        }
    }


    int ind = 1;
    for (int cc=chcnt; cc <= jmin( chcnt, totalins); ++cc) {
        for (int i=0; i < totalins - (cc - 1); ++i) {
            String name;
            if (mPeerMode) {
                name << i+1;
            } else {
                if (i < inputnames.size()) {
                    name << inputnames[i];
                }
                else {
                    name << i+1;
                }
            }

            if (cc > 1) {

                name << " - ";

                if (mPeerMode) {
                    name << i+1;
                } else {
                    if (i+cc-1 < inputnames.size()) {
                        name << inputnames[i+cc-1];
                    }
                    else {
                        name << i+cc;
                    }
                }
            }

            auto udata = std::make_shared<DestChannelListItemData>(i, cc);
            items.add(GenericItemChooserItem(name, Image(), udata, ind==1));

            if (i == chstart && cc == chcnt) {
                selindex = ind;
            }
            ++ind;
        }
    }

    items.add(GenericItemChooserItem(TRANS("CHANGE LAYOUT..."), {}, nullptr, true));
    items.add(GenericItemChooserItem(TRANS("REMOVE")));



    Component* dw = source->findParentComponentOfClass<AudioProcessorEditor>();
    if (!dw) dw = source->findParentComponentOfClass<Component>();
    Rectangle<int> bounds =  dw->getLocalArea(nullptr, source->getScreenBounds());

    SafePointer<ChannelGroupsView> safeThis(this);
    auto callback = [safeThis,changroup,inputnames, source](GenericItemChooser* chooser,int index) mutable {
        // first ignore
        auto & items = chooser->getItems();
        if (!safeThis) return;
        if (index == 0) return;
        if (index == items.size()-1) {
            // last is remove
            if (safeThis->mPeerMode) {
                int groupcount = safeThis->processor.getRemotePeerChannelGroupCount(safeThis->mPeerIndex);
                if (safeThis->processor.removeRemotePeerChannelGroup(safeThis->mPeerIndex, changroup)) {
                    safeThis->processor.setRemotePeerChannelGroupCount(safeThis->mPeerIndex, groupcount-1);
                }
            }
            else {
                int groupcount = safeThis->processor.getInputGroupCount();
                if (safeThis->processor.removeInputChannelGroup(changroup)) {
                    safeThis->processor.setInputGroupCount(groupcount-1);
                }
            }

            safeThis->rebuildChannelViews(true);
            return;
        }
        else if (index == items.size()-2) {
            // second to last is change size
            safeThis->showChangeGroupChannels(changroup, source);
            return;
        }

        auto & selitem = items.getReference(index);
        auto dclitem = std::dynamic_pointer_cast<DestChannelListItemData>(selitem.userdata);
        if (!dclitem) {
            DBG("Error getting user data");
            return;
        }

        // change src chan stuff

        if (safeThis->mPeerMode) {
            safeThis->processor.setRemotePeerChannelGroupStartAndCount(safeThis->mPeerIndex, changroup, dclitem->startIndex, dclitem->count);
        }
        else {
            safeThis->processor.setInputGroupChannelStartAndCount(changroup, dclitem->startIndex, dclitem->count);
            //if (safeThis->processor.getInputGroupName(changroup).isEmpty()) {
            //    safeThis->processor.setInputGroupName(changroup, selitem.name);
            //}
        }

        safeThis->updateChannelViews();
        safeThis->updateLayout();
        safeThis->resized();
    };

    GenericItemChooser::launchPopupChooser(items, bounds, dw, callback, selindex, dw ? dw->getHeight()-30 : 0);

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
        pvf->meter->clearMaxLevelDisplay(-1);
    }
}


void ChannelGroupsView::showDestSelectionMenu(Component * source, int index)
{
    if (index >= mChannelViews.size()) return;
    
    ChannelGroupView * pvf = mChannelViews.getUnchecked(index);

    Array<GenericItemChooserItem> items;
    items.add(GenericItemChooserItem(TRANS("SELECT MONITOR OUT:")));

    int chstart=0, chcnt=0;
    int totalouts = 0;
    int destst=0, destcnt=0;

    int changroup =  getChanGroupFromIndex(index);

    if (mPeerMode) {
        totalouts = processor.getTotalNumOutputChannels();
        processor.getRemotePeerChannelGroupStartAndCount(mPeerIndex, changroup, chstart, chcnt);
        processor.getRemotePeerChannelGroupDestStartAndCount(mPeerIndex, changroup, destst, destcnt);
    }
    else {
        totalouts = processor.getTotalNumOutputChannels();
        processor.getInputGroupChannelStartAndCount(changroup, chstart, chcnt);
        processor.getInputGroupChannelDestStartAndCount(changroup, destst, destcnt);
    }


    StringArray outputnames;
    if (JUCEApplicationBase::isStandaloneApp() && getAudioDeviceManager()) {
        auto adm = getAudioDeviceManager();
        if (auto cad = adm->getCurrentAudioDevice()) {
            auto actives = cad->getActiveOutputChannels();
            auto allnames = cad->getOutputChannelNames();
            int ind = 0;
            for ( int ni=0; ni < allnames.size(); ++ni ) {
                if (actives[ni]) {
                    String name;
                    //name << ind + 1 << ": " << allinputnames[ni];
                    name << allnames[ni];
                    outputnames.add(name);
                    ++ind;
                }
            }
        }
    }
    else {
        for (int i=0; i < totalouts; ++i) {
            int busnum = 0;
            auto bch = processor.getOffsetInBusBufferForAbsoluteChannelIndex(false, i, busnum);
            String chname;
            String name;
            if (auto bus = processor.getBus(false, busnum)) {
                chname = bus->getName();
                //name << i+1 << ": " << chname << " " << bch+1;
                name << chname << " " << bch+1;
            } else {
                name << i+1;
            }

            outputnames.add(name);
        }
    }


    // for each number of channel counts possible (1-chcnt)
    int selindex = -1;

    int ind = 1;
    for (int cc=chcnt; cc <= jmin( jmax(2, chcnt), totalouts); ++cc) {
        for (int i=0; i < totalouts - (cc - 1); ++i) {
            String name;
            if (cc == 1) {
                if (i < outputnames.size()) {
                    name << outputnames[i];
                } else {
                    name << i+1;
                }
            } else {
                if (i+cc-1 < outputnames.size()) {
                    name << outputnames[i] << " - " << outputnames[i+cc-1];
                } else {
                    name << i+1 << " - " << i+cc;
                }
            }
            auto udata = std::make_shared<DestChannelListItemData>(i, cc);
            items.add(GenericItemChooserItem(name, Image(), udata));

            if (i == destst && cc == destcnt) {
                selindex = ind;
            }
            ++ind;
        }
    }

    SafePointer<ChannelGroupsView> safeThis(this);

    auto callback = [safeThis,changroup](GenericItemChooser* chooser,int index) mutable {
        auto & items = chooser->getItems();
        auto & selitem = items.getReference(index);
        auto dclitem = std::dynamic_pointer_cast<DestChannelListItemData>(selitem.userdata);
        if (!dclitem) {
            DBG("Error getting user data");
            return;
        }

        // change src chan stuff

        if (safeThis->mPeerMode) {
            safeThis->processor.setRemotePeerChannelGroupDestStartAndCount(safeThis->mPeerIndex, changroup, dclitem->startIndex, dclitem->count);
        }
        else {
            safeThis->processor.setInputGroupChannelDestStartAndCount(changroup, dclitem->startIndex, dclitem->count);
        }

        safeThis->updateChannelViews();
        safeThis->updateLayout();
        safeThis->resized();
    };


    Component* dw = source->findParentComponentOfClass<AudioProcessorEditor>();
    if (!dw) dw = source->findParentComponentOfClass<Component>();

    Rectangle<int> bounds =  dw->getLocalArea(nullptr, source->getScreenBounds());
    
    GenericItemChooser::launchPopupChooser(items, bounds, dw, callback, selindex, dw ? dw->getHeight()-30 : 0);
}

void ChannelGroupsView::genericItemChooserSelected(GenericItemChooser *comp, int index)
{

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
            if (chi >= chcnt && changroup < changroups-1) {
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
            if (chi >= chcnt && changroup < changroups-1) {
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

        if (slider == mMainChannelView->levelSlider.get()) {
            processor.setRemotePeerLevelGain(mPeerIndex, mMainChannelView->levelSlider->getValue());
            return;
        }
        else if (slider == mMainChannelView->panSlider.get()) {
            if (mMainChannelView->panSlider->isTwoValue()) {
                float pan1 = mMainChannelView->panSlider->getMinValue();
                float pan2 = mMainChannelView->panSlider->getMaxValue();
                processor.setRemotePeerChannelPan(mPeerIndex, changroup, 0, pan1);
                processor.setRemotePeerChannelPan(mPeerIndex, changroup, 1, pan2);
            }
            else {
                processor.setRemotePeerChannelPan(mPeerIndex, changroup, chi, mMainChannelView->panSlider->getValue());
            }
            return;
        }

        for (int i=0; i < mChannelViews.size(); ++i, ++chi)
        {
            if (chi >= chcnt && changroup < changroups-1) {
                changroup++;
                chstart += chcnt;
                processor.getRemotePeerChannelGroupStartAndCount(mPeerIndex, changroup, chstart, chcnt);
                chi = 0;
            }


            ChannelGroupView * pvf = mChannelViews.getUnchecked(i);
            if (pvf->levelSlider.get() == slider) {
                processor.setRemotePeerChannelGain(mPeerIndex, changroup, pvf->levelSlider->getValue());
                break;
            }
            else if (pvf->monitorSlider.get() == slider) {
                //processor.setRemotePeerChanel (mPeerIndex, changroup, pvf->levelSlider->getValue());
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
            if (chi >= chcnt && changroup < changroups-1) {
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
