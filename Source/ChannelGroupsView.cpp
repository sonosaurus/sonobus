// SPDX-License-Identifier: GPLv3-or-later WITH Appstore-exception
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

struct CmdListItemData : public GenericItemChooserItem::UserData
{
public:
    CmdListItemData(const CmdListItemData & other) : command(other.command) {}
    CmdListItemData(int cmd) : command(cmd) {}

    int command;
};

ChannelGroupEffectsView::ChannelGroupEffectsView(SonobusAudioProcessor& proc, bool peermode)
: Component(), peerMode(peermode), processor(proc)
{
    effectsConcertina =  std::make_unique<ConcertinaPanel>();

    compressorView =  std::make_unique<CompressorView>();
    compressorView->addListener(this);
    compressorView->addHeaderListener(this);

    expanderView =  std::make_unique<ExpanderView>();
    expanderView->addListener(this);
    expanderView->addHeaderListener(this);

    eqView =  std::make_unique<ParametricEqView>();
    eqView->addListener(this);
    eqView->addHeaderListener(this);

    reverbSendView =  std::make_unique<ReverbSendView>(processor, false, true);
    reverbSendView->addListener(this);
    reverbSendView->addHeaderListener(this);

    polarityInvertView =  std::make_unique<PolarityInvertView>(processor, false, true);
    polarityInvertView->addListener(this);
    polarityInvertView->addHeaderListener(this);


    effectsConcertina->addPanel(-1, expanderView.get(), false);
    effectsConcertina->addPanel(-1, compressorView.get(), false);
    effectsConcertina->addPanel(-1, eqView.get(), false);
    effectsConcertina->addPanel(-1, reverbSendView.get(), false);
    effectsConcertina->addPanel(-1, polarityInvertView.get(), false);

    effectsConcertina->setCustomPanelHeader(compressorView.get(), compressorView->getHeaderComponent(), false);
    effectsConcertina->setCustomPanelHeader(expanderView.get(), expanderView->getHeaderComponent(), false);
    effectsConcertina->setCustomPanelHeader(eqView.get(), eqView->getHeaderComponent(), false);

    effectsConcertina->setCustomPanelHeader(reverbSendView.get(), reverbSendView->getHeaderComponent(), false);
    effectsConcertina->setCustomPanelHeader(polarityInvertView.get(), polarityInvertView->getHeaderComponent(), false);

    addAndMakeVisible (effectsConcertina.get());

    setFocusContainerType(FocusContainerType::focusContainer);

    updateLayout();
}

ChannelGroupEffectsView::~ChannelGroupEffectsView() {

    compressorView->removeListener(this);
    expanderView->removeListener(this);
    expanderView->removeHeaderListener(this);
    reverbSendView->removeHeaderListener(this);
    reverbSendView->removeListener(this);
    polarityInvertView->removeHeaderListener(this);
    polarityInvertView->removeListener(this);

    effectsConcertina.reset();
}

juce::Rectangle<int> ChannelGroupEffectsView::getMinimumContentBounds() const {
    auto minbounds = compressorView->getMinimumContentBounds();
    auto minexpbounds = expanderView->getMinimumContentBounds();
    auto mineqbounds = eqView->getMinimumContentBounds();
    auto headbounds = compressorView->getMinimumHeaderBounds();
    auto minrevbounds = reverbSendView->getMinimumContentBounds();
    auto minprvbounds = polarityInvertView->getMinimumContentBounds();

    int defWidth = jmax(minbounds.getWidth(), minexpbounds.getWidth(), mineqbounds.getWidth(), jmax(minrevbounds.getWidth(),  minprvbounds.getWidth())) + 12;
    int defHeight = 0;

    defHeight = jmax(minbounds.getHeight(), minexpbounds.getHeight(), mineqbounds.getHeight(), jmax(minrevbounds.getHeight(), minprvbounds.getHeight())) + 5*headbounds.getHeight() + 8;

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

    if (!reverbSendView->isVisible()) {
        reverbSendView->setVisible(true);
        reverbSendView->getHeaderComponent()->setVisible(true);
    }

    reverbSendView->updateParams(processor.getRemotePeerChannelReverbSend(peerIndex, groupIndex));

    if (!polarityInvertView->isVisible()) {
        polarityInvertView->setVisible(true);
        polarityInvertView->getHeaderComponent()->setVisible(true);
    }

    polarityInvertView->updateParams(processor.getRemotePeerPolarityInvert(peerIndex, groupIndex));


    if (firstShow) {
        if (eqparams.enabled && !(compParams.enabled || expParams.enabled)) {
            effectsConcertina->expandPanelFully(eqView.get(), false);
        }
        else {
            effectsConcertina->setPanelSize(polarityInvertView.get(), 0, false);
            effectsConcertina->setPanelSize(reverbSendView.get(), 0, false);
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

    reverbSendView->updateParams(processor.getInputReverbSend(groupIndex, true));
    polarityInvertView->updateParams(processor.getInputPolarityInvert(groupIndex));

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
    auto minrevbounds = reverbSendView->getMinimumContentBounds();
    auto minrevheadbounds = reverbSendView->getMinimumHeaderBounds();
    auto minprvheadbounds = polarityInvertView->getMinimumHeaderBounds();
    auto minprvbounds = polarityInvertView->getMinimumContentBounds();

    int gcount = 5 ;

    effectsBox.items.clear();
    effectsBox.flexDirection = FlexBox::Direction::column;
    effectsBox.items.add(FlexItem(4, 2));
    effectsBox.items.add(FlexItem(minexpbounds.getWidth(), jmax(minexpbounds.getHeight(), mincompbounds.getHeight(), mineqbounds.getHeight()) + gcount*minitemheight, *effectsConcertina).withMargin(1).withFlex(1));

    effectsConcertina->setPanelHeaderSize(compressorView.get(), mincompheadbounds.getHeight());
    effectsConcertina->setPanelHeaderSize(expanderView.get(), minexpheadbounds.getHeight());
    effectsConcertina->setPanelHeaderSize(eqView.get(), mineqheadbounds.getHeight());

    effectsConcertina->setPanelHeaderSize(reverbSendView.get(), minrevheadbounds.getHeight());
    effectsConcertina->setPanelHeaderSize(polarityInvertView.get(), minprvheadbounds.getHeight());

    effectsConcertina->setMaximumPanelSize(compressorView.get(), mincompbounds.getHeight()+5);
    effectsConcertina->setMaximumPanelSize(expanderView.get(), minexpbounds.getHeight()+5);
    effectsConcertina->setMaximumPanelSize(eqView.get(), mineqbounds.getHeight());

    effectsConcertina->setMaximumPanelSize(reverbSendView.get(), minrevbounds.getHeight());
    effectsConcertina->setMaximumPanelSize(polarityInvertView.get(), minprvbounds.getHeight());

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
        
        // Send OSC feedback for peer compressor parameters (only for first 16 peers, channel group 0)
        if (processor.getOSCEnabled() && peerIndex < 16 && groupIndex == 0) {
            processor.getOSCManager().sendMessage("/Peer" + String(peerIndex + 1) + "CompressorEnable", params.enabled ? 1 : 0);
            processor.getOSCManager().sendMessage("/Peer" + String(peerIndex + 1) + "CompressorThreshold", params.thresholdDb);
            processor.getOSCManager().sendMessage("/Peer" + String(peerIndex + 1) + "CompressorRatio", params.ratio);
            processor.getOSCManager().sendMessage("/Peer" + String(peerIndex + 1) + "CompressorAttack", params.attackMs);
            processor.getOSCManager().sendMessage("/Peer" + String(peerIndex + 1) + "CompressorRelease", params.releaseMs);
            processor.getOSCManager().sendMessage("/Peer" + String(peerIndex + 1) + "CompressorMakeupGain", params.makeupGainDb);
            processor.getOSCManager().sendMessage("/Peer" + String(peerIndex + 1) + "CompressorAuto", params.automakeupGain ? 1 : 0);
        }
    }
    else {
        bool wason = processor.getInputEffectsActive(groupIndex);

        processor.setInputCompressorParams(groupIndex, params);

        bool ison = processor.getInputEffectsActive(groupIndex);
        if (wason != ison) {
            listeners.call (&ChannelGroupEffectsView::Listener::effectsEnableChanged, this);
        }
        
        // Send OSC updates for compressor parameters
        processor.getOSCManager().sendMessage("/InputGroup" + String(groupIndex + 1) + "CompressorEnable", params.enabled ? 1.0f : 0.0f);
        processor.getOSCManager().sendMessage("/InputGroup" + String(groupIndex + 1) + "CompressorThreshold", params.thresholdDb);
        processor.getOSCManager().sendMessage("/InputGroup" + String(groupIndex + 1) + "CompressorRatio", params.ratio);
        processor.getOSCManager().sendMessage("/InputGroup" + String(groupIndex + 1) + "CompressorAttack", params.attackMs);
        processor.getOSCManager().sendMessage("/InputGroup" + String(groupIndex + 1) + "CompressorRelease", params.releaseMs);
        processor.getOSCManager().sendMessage("/InputGroup" + String(groupIndex + 1) + "CompressorMakeupGain", params.makeupGainDb);
        processor.getOSCManager().sendMessage("/InputGroup" + String(groupIndex + 1) + "CompressorAuto", params.automakeupGain ? 1.0f : 0.0f);
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
        
        // Send OSC feedback for peer expander parameters (only for first 16 peers, channel group 0)
        if (processor.getOSCEnabled() && peerIndex < 16 && groupIndex == 0) {
            processor.getOSCManager().sendMessage("/Peer" + String(peerIndex + 1) + "ExpanderEnable", params.enabled ? 1 : 0);
            processor.getOSCManager().sendMessage("/Peer" + String(peerIndex + 1) + "ExpanderNoiseFloor", params.thresholdDb);
            processor.getOSCManager().sendMessage("/Peer" + String(peerIndex + 1) + "ExpanderRatio", params.ratio);
            processor.getOSCManager().sendMessage("/Peer" + String(peerIndex + 1) + "ExpanderAttack", params.attackMs);
            processor.getOSCManager().sendMessage("/Peer" + String(peerIndex + 1) + "ExpanderRelease", params.releaseMs);
        }
    }
    else {
        bool wason = processor.getInputEffectsActive(groupIndex);

        processor.setInputExpanderParams(groupIndex, params);

        bool ison = processor.getInputEffectsActive(groupIndex);
        if (wason != ison) {
            listeners.call (&ChannelGroupEffectsView::Listener::effectsEnableChanged, this);
        }
        
        // Send OSC updates for expander (noise gate) parameters
        processor.getOSCManager().sendMessage("/InputGroup" + String(groupIndex + 1) + "ExpanderEnable", params.enabled ? 1.0f : 0.0f);
        processor.getOSCManager().sendMessage("/InputGroup" + String(groupIndex + 1) + "ExpanderNoiseFloor", params.thresholdDb);
        processor.getOSCManager().sendMessage("/InputGroup" + String(groupIndex + 1) + "ExpanderRatio", params.ratio);
        processor.getOSCManager().sendMessage("/InputGroup" + String(groupIndex + 1) + "ExpanderAttack", params.attackMs);
        processor.getOSCManager().sendMessage("/InputGroup" + String(groupIndex + 1) + "ExpanderRelease", params.releaseMs);
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
        
        // Send OSC feedback for peer EQ parameters (only for first 16 peers, channel group 0)
        if (processor.getOSCEnabled() && peerIndex < 16 && groupIndex == 0) {
            processor.getOSCManager().sendMessage("/Peer" + String(peerIndex + 1) + "EqEnable", params.enabled ? 1 : 0);
            processor.getOSCManager().sendMessage("/Peer" + String(peerIndex + 1) + "EqLowShelfFreq", params.lowShelfFreq);
            processor.getOSCManager().sendMessage("/Peer" + String(peerIndex + 1) + "EqLowShelfGain", params.lowShelfGain);
            processor.getOSCManager().sendMessage("/Peer" + String(peerIndex + 1) + "EqPara1Freq", params.para1Freq);
            processor.getOSCManager().sendMessage("/Peer" + String(peerIndex + 1) + "EqPara1Gain", params.para1Gain);
            processor.getOSCManager().sendMessage("/Peer" + String(peerIndex + 1) + "EqPara1Q", params.para1Q);
            processor.getOSCManager().sendMessage("/Peer" + String(peerIndex + 1) + "EqHighShelfFreq", params.highShelfFreq);
            processor.getOSCManager().sendMessage("/Peer" + String(peerIndex + 1) + "EqHighShelfGain", params.highShelfGain);
            processor.getOSCManager().sendMessage("/Peer" + String(peerIndex + 1) + "EqPara2Freq", params.para2Freq);
            processor.getOSCManager().sendMessage("/Peer" + String(peerIndex + 1) + "EqPara2Gain", params.para2Gain);
            processor.getOSCManager().sendMessage("/Peer" + String(peerIndex + 1) + "EqPara2Q", params.para2Q);
        }
    } else {
        bool wason = processor.getInputEffectsActive(groupIndex);

        processor.setInputEqParams(groupIndex, params);

        bool ison = processor.getInputEffectsActive(groupIndex);
        if (wason != ison) {
            listeners.call (&ChannelGroupEffectsView::Listener::effectsEnableChanged, this);
        }
        
        // Send OSC updates for EQ parameters
        processor.getOSCManager().sendMessage("/InputGroup" + String(groupIndex + 1) + "EqEnable", params.enabled ? 1.0f : 0.0f);
        processor.getOSCManager().sendMessage("/InputGroup" + String(groupIndex + 1) + "EqLowShelfFreq", params.lowShelfFreq);
        processor.getOSCManager().sendMessage("/InputGroup" + String(groupIndex + 1) + "EqLowShelfGain", params.lowShelfGain);
        processor.getOSCManager().sendMessage("/InputGroup" + String(groupIndex + 1) + "EqPara1Freq", params.para1Freq);
        processor.getOSCManager().sendMessage("/InputGroup" + String(groupIndex + 1) + "EqPara1Gain", params.para1Gain);
        processor.getOSCManager().sendMessage("/InputGroup" + String(groupIndex + 1) + "EqPara1Q", params.para1Q);
        processor.getOSCManager().sendMessage("/InputGroup" + String(groupIndex + 1) + "EqHighShelfFreq", params.highShelfFreq);
        processor.getOSCManager().sendMessage("/InputGroup" + String(groupIndex + 1) + "EqHighShelfGain", params.highShelfGain);
        processor.getOSCManager().sendMessage("/InputGroup" + String(groupIndex + 1) + "EqPara2Freq", params.para2Freq);
        processor.getOSCManager().sendMessage("/InputGroup" + String(groupIndex + 1) + "EqPara2Gain", params.para2Gain);
        processor.getOSCManager().sendMessage("/InputGroup" + String(groupIndex + 1) + "EqPara2Q", params.para2Q);
    }
}

void ChannelGroupEffectsView::reverbSendLevelChanged(ReverbSendView *comp, float revlevel)
{
    if (peerMode) {
        bool wason = processor.getRemotePeerEffectsActive(peerIndex, groupIndex);

        processor.setRemotePeerChannelReverbSend(peerIndex, groupIndex, revlevel);

        bool ison = processor.getRemotePeerEffectsActive(peerIndex, groupIndex);
        if (wason != ison) {
            listeners.call (&ChannelGroupEffectsView::Listener::effectsEnableChanged, this);
        }
        
        // Send OSC feedback for peer input reverb send (only for first 16 peers, channel group 0)
        if (processor.getOSCEnabled() && peerIndex < 16 && groupIndex == 0) {
            processor.getOSCManager().sendMessage("/Peer" + String(peerIndex + 1) + "InputReverbSend", revlevel);
        }
    }
    else {
        bool wason = processor.getInputEffectsActive(groupIndex);

        // input mode
        processor.setInputReverbSend(groupIndex, revlevel, true);

        bool ison = processor.getInputEffectsActive(groupIndex);
        if (wason != ison) {
            listeners.call (&ChannelGroupEffectsView::Listener::effectsEnableChanged, this);
        }
        
        // Send OSC update for input reverb send
        processor.getOSCManager().sendMessage("/InputGroup" + String(groupIndex + 1) + "InputReverbSend", revlevel);
    }
}

void ChannelGroupEffectsView::polarityInvertChanged(PolarityInvertView *comp, bool polinv)
{
    if (peerMode) {
        processor.setRemotePeerPolarityInvert(peerIndex, groupIndex, polinv);
        
        // Send OSC feedback for peer polarity invert (only for first 16 peers, channel group 0)
        if (processor.getOSCEnabled() && peerIndex < 16 && groupIndex == 0) {
            processor.getOSCManager().sendMessage("/Peer" + String(peerIndex + 1) + "PolarityInvert", polinv ? 1 : 0);
        }
    }
    else {
        // input mode
        processor.setInputPolarityInvert(groupIndex, polinv);
        
        // Send OSC update for polarity invert
        processor.getOSCManager().sendMessage("/InputGroup" + String(groupIndex + 1) + "PolarityInvert", polinv ? 1.0f : 0.0f);
    }
    listeners.call (&ChannelGroupEffectsView::Listener::effectsEnableChanged, this);
}


void ChannelGroupEffectsView::effectsHeaderClicked(EffectsBaseView *comp)
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
    else if (comp == reverbSendView.get()) {
        bool changed = effectsConcertina->expandPanelFully(reverbSendView.get(), true);
    }
}


#pragma ChannelGroupMonitorEffectsView

ChannelGroupMonitorEffectsView::ChannelGroupMonitorEffectsView(SonobusAudioProcessor& proc, bool peermode)
: Component(), peerMode(peermode), processor(proc)
{
    effectsConcertina =  std::make_unique<ConcertinaPanel>();

    delayView =  std::make_unique<MonitorDelayView>(processor);
    delayView->addListener(this);
    delayView->addHeaderListener(this);

    reverbSendView =  std::make_unique<ReverbSendView>(processor, false);
    reverbSendView->addListener(this);
    //reverbSendView->addHeaderListener(this);


    effectsConcertina->addPanel(-1, delayView.get(), false);
    effectsConcertina->setCustomPanelHeader(delayView.get(), delayView->getHeaderComponent(), false);

    effectsConcertina->addPanel(-1, reverbSendView.get(), false);
    effectsConcertina->setCustomPanelHeader(reverbSendView.get(), reverbSendView->getHeaderComponent(), false);


    addAndMakeVisible (effectsConcertina.get());

    setFocusContainerType(FocusContainerType::focusContainer);

    updateLayout();
}

ChannelGroupMonitorEffectsView::~ChannelGroupMonitorEffectsView()
{
    reverbSendView->removeListener(this);
    delayView->removeListener(this);
    delayView->removeHeaderListener(this);

    effectsConcertina.reset();
}

juce::Rectangle<int> ChannelGroupMonitorEffectsView::getMinimumContentBounds() const {
    auto minbounds = delayView->getMinimumContentBounds();
    auto headbounds = delayView->getMinimumHeaderBounds();

    auto minrevbounds = reverbSendView->getMinimumContentBounds();
    auto headrevbounds = reverbSendView->getMinimumHeaderBounds();


    int defWidth = jmax(minbounds.getWidth(), 0) + 12;
    int defHeight = 0;

    if (groupIndex < 0) {
        // don't include reverb
        defHeight = jmax(minbounds.getHeight() , 0) + headbounds.getHeight() + 8;
    } else {
        defHeight = jmax(minbounds.getHeight() + minrevbounds.getHeight(), 0) + headbounds.getHeight() + headrevbounds.getHeight() + 8;
    }

    return Rectangle<int>(0,0,defWidth,defHeight);
}


void ChannelGroupMonitorEffectsView::updateState()
{
    if (peerMode) {
        updateStateForRemotePeer();
    } else {
        updateStateForInput();
    }

}

void ChannelGroupMonitorEffectsView::updateStateForRemotePeer()
{

}

void ChannelGroupMonitorEffectsView::updateStateForInput()
{
    DelayParams monDelayParams;

    // Set the group index for OSC addressing
    delayView->setGroupIndex(groupIndex);

    if (groupIndex == -1) {
        // met
        if (processor.getMetronomeMonitorDelayParams(monDelayParams)) {
            delayView->updateParams(monDelayParams);
        }

        if (reverbSendView->isVisible()) {
            reverbSendView->setVisible(false);
            reverbSendView->getHeaderComponent()->setVisible(false);
        }
    }
    else if (groupIndex == -2) {
        // file playback
        if (processor.getFilePlaybackMonitorDelayParams(monDelayParams)) {
            delayView->updateParams(monDelayParams);
        }

        if (reverbSendView->isVisible()) {
            reverbSendView->setVisible(false);
            reverbSendView->getHeaderComponent()->setVisible(false);
        }
    }
    else if (groupIndex == -3) {
        // soundboard
        monDelayParams = processor.getSoundboardProcessor()->getMonitorDelayParams();
        delayView->updateParams(monDelayParams);

        if (reverbSendView->isVisible()) {
            reverbSendView->setVisible(false);
            reverbSendView->getHeaderComponent()->setVisible(false);
        }
    }
    else {
        if (processor.getInputMonitorDelayParams(groupIndex, monDelayParams)) {
            delayView->updateParams(monDelayParams);
        }
        reverbSendView->updateParams(processor.getInputReverbSend(groupIndex, false));

        if (!reverbSendView->isVisible()) {
            reverbSendView->setVisible(true);
            reverbSendView->getHeaderComponent()->setVisible(true);
        }
    }

    if (firstShow) {
        effectsConcertina->setPanelSize(delayView.get(), 0, false);
        effectsConcertina->setPanelSize(reverbSendView.get(), 0, false);
        //effectsConcertina->expandPanelFully(expanderView.get(), false);
        firstShow = false;
    }
}

void ChannelGroupMonitorEffectsView::updateLayout()
{
    int minitemheight = 32;
#if JUCE_IOS || JUCE_ANDROID
    // make the button heights a bit more for touchscreen purposes
    minitemheight = 40;
#endif

    auto mindelaybounds = delayView->getMinimumContentBounds();
    auto mindelayheadbounds = delayView->getMinimumHeaderBounds();
    auto minrevbounds = reverbSendView->getMinimumContentBounds();
    auto minrevheadbounds = reverbSendView->getMinimumHeaderBounds();
    int gcount = 2;


    effectsBox.items.clear();
    effectsBox.flexDirection = FlexBox::Direction::column;
    effectsBox.items.add(FlexItem(4, 2));
    effectsBox.items.add(FlexItem(mindelaybounds.getWidth(), jmax(mindelaybounds.getHeight() + minrevbounds.getHeight(), 0) + gcount*minitemheight, *effectsConcertina).withMargin(1).withFlex(1));

    effectsConcertina->setPanelHeaderSize(delayView.get(), mindelayheadbounds.getHeight());
    effectsConcertina->setPanelHeaderSize(reverbSendView.get(), minrevheadbounds.getHeight());

    effectsConcertina->setMaximumPanelSize(delayView.get(), mindelaybounds.getHeight());
    effectsConcertina->setMaximumPanelSize(reverbSendView.get(), minrevbounds.getHeight());

}

void ChannelGroupMonitorEffectsView::resized()  {

    effectsBox.performLayout(getLocalBounds().reduced(2, 2));

}

void ChannelGroupMonitorEffectsView::reverbSendLevelChanged(ReverbSendView *comp, float revlevel)
{
    if (peerMode) {

        bool wason = processor.getRemotePeerEffectsActive(peerIndex, groupIndex);

        processor.setRemotePeerChannelReverbSend(peerIndex, groupIndex, revlevel);

        bool ison = processor.getRemotePeerEffectsActive(peerIndex, groupIndex);
        if (wason != ison) {
            listeners.call (&ChannelGroupMonitorEffectsView::Listener::monitorEffectsEnableChanged, this);
        }

    }
    else {
        bool wason = processor.getInputMonitorEffectsActive(groupIndex);

        processor.setInputReverbSend(groupIndex, revlevel, false);

        bool ison = processor.getInputMonitorEffectsActive(groupIndex);
        if (wason != ison) {
            listeners.call (&ChannelGroupMonitorEffectsView::Listener::monitorEffectsEnableChanged, this);
        }
        
        // Send OSC message for Input Group Mon Reverb Send change
        if (groupIndex >= 0 && processor.getOSCEnabled()) {
            processor.getOSCManager().sendMessage("/InputGroup" + String(groupIndex + 1) + "MonReverbSend", revlevel);
        }
    }
}


void ChannelGroupMonitorEffectsView::monitorDelayParamsChanged(MonitorDelayView *comp, SonoAudio::DelayParams & params)
{
    if (peerMode) {
        /*
        bool wason = processor.getRemotePeerEffectsActive(peerIndex, groupIndex);

        processor.setRemotePeerCompressorParams(peerIndex, groupIndex, params);

        bool ison = processor.getRemotePeerEffectsActive(peerIndex, groupIndex);
        if (wason != ison) {
            listeners.call (&ChannelGroupEffectsView::Listener::effectsEnableChanged, this);
        }
         */
    }
    else {
        bool wason = false;
        bool ison = params.enabled;

        auto deltimems = params.delayTimeMs;
        DelayParams eparam;

        if (groupIndex == -1) {
            processor.getMetronomeMonitorDelayParams(eparam);
            wason = eparam.enabled;
            processor.setMetronomeMonitorDelayParams(params);
        } else if (groupIndex == -2) {
            processor.getFilePlaybackMonitorDelayParams(eparam);
            wason = eparam.enabled;
            processor.setFilePlaybackMonitorDelayParams(params);
        } else if (groupIndex == -3) {
            eparam = processor.getSoundboardProcessor()->getMonitorDelayParams();
            wason = eparam.enabled;
            processor.getSoundboardProcessor()->setMonitorDelayParams(params);
        } else {
            wason = processor.getInputMonitorEffectsActive(groupIndex);
            processor.setInputMonitorDelayParams(groupIndex, params);
            
            // Send OSC messages for Input Group Mon Delay parameters
            if (groupIndex >= 0 && processor.getOSCEnabled()) {
                processor.getOSCManager().sendMessage("/InputGroup" + String(groupIndex + 1) + "MonDelayEnable", params.enabled ? 1.0f : 0.0f);
                processor.getOSCManager().sendMessage("/InputGroup" + String(groupIndex + 1) + "MonDelayTime", params.delayTimeMs);
            }
        }

        if (processor.getLinkMonitoringDelayTimes()) {
            // set for all the others too
            int numgroups = processor.getInputGroupCount();
            for (int i=0; i < numgroups; ++i) {
                processor.getInputMonitorDelayParams(i, eparam);
                if (eparam.delayTimeMs != deltimems) {
                    eparam.delayTimeMs = deltimems;
                    processor.setInputMonitorDelayParams(i, eparam);
                }
            }

            processor.getMetronomeMonitorDelayParams(eparam);
            if (eparam.delayTimeMs != deltimems) {
                eparam.delayTimeMs = deltimems;
                processor.setMetronomeMonitorDelayParams(eparam);
            }

            processor.getFilePlaybackMonitorDelayParams(eparam);
            if (eparam.delayTimeMs != deltimems) {
                eparam.delayTimeMs = deltimems;
                processor.setFilePlaybackMonitorDelayParams(eparam);
            }

            eparam = processor.getSoundboardProcessor()->getMonitorDelayParams();
            if (eparam.delayTimeMs != deltimems) {
                eparam.delayTimeMs = deltimems;
                processor.getSoundboardProcessor()->setMonitorDelayParams(eparam);
            }
            
            // Send OSC message for Link Delay Time (global setting)
            if (processor.getOSCEnabled()) {
                processor.getOSCManager().sendMessage("/InputGroup" + String(groupIndex + 1) + "MonDelayLink", 1.0f);
            }
        } else {
            // Send OSC message for Link Delay Time being disabled
            if (groupIndex >= 0 && processor.getOSCEnabled()) {
                processor.getOSCManager().sendMessage("/InputGroup" + String(groupIndex + 1) + "MonDelayLink", 0.0f);
            }
        }

        if (wason != ison) {
            listeners.call (&ChannelGroupMonitorEffectsView::Listener::monitorEffectsEnableChanged, this);
        }
    }

}


void ChannelGroupMonitorEffectsView::effectsHeaderClicked(EffectsBaseView *comp)
{
    if (comp == delayView.get()) {
        bool changed = effectsConcertina->setPanelSize(delayView.get(), 0, true);
        //changed |= effectsConcertina->expandPanelFully(expanderView.get(), true);
        //changed |= effectsConcertina->expandPanelFully(compressorView.get(), true);
        //if (!changed)
        {
            // toggle it
            DelayParams params;

            if (peerMode) {
                //processor.getRemotePeerCompressorParams(peerIndex, 0, params);
                //params.enabled = !params.enabled;
                //processor.setInputCompressorParams(0, params);
            } else {
                if (groupIndex == -1) {
                    processor.getMetronomeMonitorDelayParams(params);
                    params.enabled = !params.enabled;
                    processor.setMetronomeMonitorDelayParams(params);
                } else if (groupIndex == -2) {
                    processor.getFilePlaybackMonitorDelayParams(params);
                    params.enabled = !params.enabled;
                    processor.setFilePlaybackMonitorDelayParams(params);
                }
                else if (groupIndex == -3) {
                    params = processor.getSoundboardProcessor()->getMonitorDelayParams();
                    params.enabled = !params.enabled;
                    processor.getSoundboardProcessor()->setMonitorDelayParams(params);
                }
                else {
                    processor.getInputMonitorDelayParams(groupIndex, params);
                    params.enabled = !params.enabled;
                    processor.setInputMonitorDelayParams(groupIndex, params);
                }
            }
            updateState();

            listeners.call (&ChannelGroupMonitorEffectsView::Listener::monitorEffectsEnableChanged, this);
        }
    }
}

#pragma ChannelGroupReverbEffectsView

ChannelGroupReverbEffectsView::ChannelGroupReverbEffectsView(SonobusAudioProcessor& proc)
: Component(), processor(proc)
{
    effectsConcertina =  std::make_unique<ConcertinaPanel>();

    reverbView =  std::make_unique<ReverbView>(processor, true);
    reverbView->addListener(this);
    //reverbSendView->addHeaderListener(this);


    effectsConcertina->addPanel(-1, reverbView.get(), false);
    effectsConcertina->setCustomPanelHeader(reverbView.get(), reverbView->getHeaderComponent(), false);


    addAndMakeVisible (effectsConcertina.get());

    setFocusContainerType(FocusContainerType::focusContainer);

    updateLayout();
}

ChannelGroupReverbEffectsView::~ChannelGroupReverbEffectsView()
{
    reverbView->removeListener(this);
    effectsConcertina.reset();
}

juce::Rectangle<int> ChannelGroupReverbEffectsView::getMinimumContentBounds() const {
    auto minrevbounds = reverbView->getMinimumContentBounds();
    auto headrevbounds = reverbView->getMinimumHeaderBounds();


    int defWidth = jmax(minrevbounds.getWidth(), 0) + 12;
    int defHeight = 0;

    defHeight = jmax(minrevbounds.getHeight() , 0) + headrevbounds.getHeight() + 8;

    return Rectangle<int>(0,0,defWidth,defHeight);
}


void ChannelGroupReverbEffectsView::updateState()
{
    reverbView->updateParams();

    if (firstShow) {
        effectsConcertina->setPanelSize(reverbView.get(), 0, false);
        firstShow = false;
    }
}




void ChannelGroupReverbEffectsView::updateLayout()
{
    int minitemheight = 32;
#if JUCE_IOS || JUCE_ANDROID
    // make the button heights a bit more for touchscreen purposes
    minitemheight = 40;
#endif

    auto minrevbounds = reverbView->getMinimumContentBounds();
    auto minrevheadbounds = reverbView->getMinimumHeaderBounds();
    int gcount = 1;


    effectsBox.items.clear();
    effectsBox.flexDirection = FlexBox::Direction::column;
    effectsBox.items.add(FlexItem(4, 2));
    effectsBox.items.add(FlexItem(minrevbounds.getWidth(), jmax(minrevbounds.getHeight(), 0) + gcount*minitemheight, *effectsConcertina).withMargin(1).withFlex(1));

    effectsConcertina->setPanelHeaderSize(reverbView.get(), minrevheadbounds.getHeight());

    effectsConcertina->setMaximumPanelSize(reverbView.get(), minrevbounds.getHeight());

}

void ChannelGroupReverbEffectsView::resized()  {

    effectsBox.performLayout(getLocalBounds().reduced(2, 2));

}




void ChannelGroupReverbEffectsView::effectsHeaderClicked(EffectsBaseView *comp)
{
    /*
    if (comp == reverbView.get()) {
        bool changed = effectsConcertina->setPanelSize(reverbView.get(), 0, true);


        updateState();

        //listeners.call (&ChannelGroupMonitorEffectsView::Listener::monitorEffectsEnableChanged, this);
    }
     */
}


#pragma ChannelGroupView


ChannelGroupView::ChannelGroupView() : smallLnf(12), medLnf(14), sonoSliderLNF(12), panSliderLNF(12)
{
    bgColor = Colour::fromFloatRGBA(0.112f, 0.112f, 0.112f, 1.0f);
    borderColor = Colour::fromFloatRGBA(0.5f, 0.5f, 0.5f, 0.3f);

    sonoSliderLNF.textJustification = Justification::centredLeft;
    panSliderLNF.textJustification = Justification::centredLeft;

    //setFocusContainerType(FocusContainerType::keyboardFocusContainer);

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

    if (useBgColor) {
        g.fillAll (bgColor);
        //g.setColour(bgColor);
        //g.fillRoundedRectangle(getLocalBounds().toFloat(), 6.0f);
    }

    //g.drawRoundedRectangle(getLocalBounds().toFloat(), 6.0f, 0.5f);

    if (showDivider) {
        g.setColour(borderColor);
        g.drawLine(0, 0, getWidth(), 0, 1);
    }

}

void ChannelGroupView::resized()
{
    
    mainbox.performLayout(getLocalBounds());
    
    if (panLabel) {
        panLabel->setBounds(panSlider->getBounds().removeFromTop(16).translated(0, -2));
    }
    

    int triwidth = 10;
    int triheight = 6;
    
    if (levelSlider) {
        levelSlider->setMouseDragSensitivity(jmax(128, levelSlider->getWidth()));
    }

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

    //bgColor = Colour::fromFloatRGBA(0.045f, 0.045f, 0.05f, 1.0f);
    bgColor = Colour::fromFloatRGBA(0.08f, 0.045f, 0.08f, 1.0f);

    mInGainSlider     = std::make_unique<Slider>(Slider::LinearHorizontal,  Slider::TextBoxAbove);
    mInGainSlider->setName("ingain");
    mInGainSlider->setSliderSnapsToMousePosition(processor.getSlidersSnapToMousePosition());
    mInGainSlider->setTextBoxIsEditable(true);
    mInGainSlider->setScrollWheelEnabled(false);

    mAddButton = std::make_unique<TextButton>("+");
    mAddButton->setTitle(TRANS("Add Input Group"));
    mAddButton->onClick = [this] {  addGroupPressed(); };
    mAddButton->setLookAndFeel(&addLnf);
    mAddButton->setTooltip(TRANS("Add New Input Group"));
    addChildComponent(mAddButton.get());

    mClearButton = std::make_unique<TextButton>(TRANS("CLEAR"));
    mClearButton->onClick = [this] {  clearGroupsPressed(); };
    //mClearButton->setLookAndFeel(&addLnf);
    mClearButton->setTooltip(TRANS("Remove all input groups"));
    addChildComponent(mClearButton.get());

    mInReverbButton = std::make_unique<TextButton>(TRANS("In Reverb"));
    //mClearButton->setLookAndFeel(&addLnf);
    mInReverbButton->setTooltip(TRANS("Configure input reverb parameters"));
    addChildComponent(mInReverbButton.get());
    mInReverbButton->onClick = [this]() {
        if (!inReverbCalloutBox) {
            showInputReverbView(true);
        } else {
            showInputReverbView(false);
        }
        // Send OSC message for InReverbButton click
        if (processor.getOSCEnabled()) {
            processor.getOSCManager().sendMessage("/InReverbButton", 1);
        }
    };

    mMonDelayButton = std::make_unique<TextButton>(TRANS("Monitor Delay"));
    mMonDelayButton->setColour(TextButton::buttonOnColourId, Colour::fromFloatRGBA(0.2, 0.5, 0.7, 0.5));
    mMonDelayButton->setTooltip(TRANS("Toggle monitor delay enabled on all input groups"));
    addChildComponent(mMonDelayButton.get());
    mMonDelayButton->onClick = [this]() {
        toggleAllMonitorDelay();
        // Send OSC message for MonDelayButton state change
        if (processor.getOSCEnabled()) {
            processor.getOSCManager().sendMessage("/MonDelayButton", mMonDelayButton->getToggleState() ? 1 : 0);
        }
    };


    mInsertLine = std::make_unique<DrawableRectangle>();
    //mInsertLine->setCornerSize(Point<float>(6,6));
    //mInsertLine->setFill (Colour::fromFloatRGBA(0.07, 0.07, 0.07, 1.0));
    mInsertLine->setFill (Colours::transparentBlack);
    mInsertLine->setStrokeFill (Colour::fromFloatRGBA(0.5, 0.5, 0.5, 0.75));
    mInsertLine->setStrokeThickness(2);
    addChildComponent(mInsertLine.get());

    mDragDrawable = std::make_unique<DrawableImage>();
    mDragDrawable->setAlpha(0.4f);
    mDragDrawable->setAlwaysOnTop(true);
    addChildComponent(mDragDrawable.get());

    mMetFileBg = std::make_unique<DrawableRectangle>();
    mMetFileBg->setFill (Colour::fromFloatRGBA(0.0, 0.0, 0.0, 0.75));
    //mMetFileBg->setStrokeFill (Colour::fromFloatRGBA(0.4, 0.4, 0.4, 0.3));
    mMetFileBg->setStrokeFill (Colours::transparentBlack);
    //mMetFileBg->setStrokeThickness(0.75f);
    mMetFileBg->setCornerSize(Point<float>(8.0f, 8.0f));
    addChildComponent(mMetFileBg.get());


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

    
    slider->setTextBoxStyle(Slider::TextBoxAbove, true, 100, 12);

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
    slider->setWantsKeyboardFocus(true);
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
            slider->textFromValueFunction = [](float v) -> String { return String(TRANS("Pre Level: ")) + Decibels::toString(Decibels::gainToDecibels(v), 1); };
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
    slider->setWantsKeyboardFocus(true);

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
        label->setFont(12 );
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

    int startind = -1;
    Point<int> topleft;
    for (int i=0; i < mChannelViews.size(); ++i) {
        ChannelGroupView * pvf = mChannelViews.getUnchecked(i);
        pvf->resized();

        if (startind < 0) {
            startind = 0;
            topleft = pvf->getPosition();
        }

        if (pvf->chanIndex == pvf->groupChanCount-1) {
            // last in the group finish the bounds
            if (pvf->group >= mChanGroupBounds.size()) {
                mChanGroupBounds.resize(pvf->group+1);
            }
            mChanGroupBounds.getReference(pvf->group) = Rectangle<int>(topleft.getX(), topleft.getY(), pvf->getRight() - topleft.getX(), pvf->getBottom() - topleft.getY());
            startind = -1;
        }
    }

    if (mMainChannelView) {
        mMainChannelView->resized();
    }

    if (mMetChannelView && mMetChannelView->isVisible()) {
        // resize bg border
        auto mfbounds = Rectangle<int>(mMetChannelView->getX() - 3, mMetChannelView->getY(), mMetChannelView->getWidth() + 6, mSoundboardChannelView->getBottom() - mMetChannelView->getY() + 4);
        mMetFileBg->setRectangle (mfbounds.toFloat());
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
    text.setFont(Font(12* SonoLookAndFeel::getFontScale()));
    if (target) {
        popTip->showAt(target, text, timeoutMs);
    }
    else {
        Rectangle<int> topbox(getWidth()/2 - maxwidth/2, 0, maxwidth, 2);
        popTip->showAt(topbox, text, timeoutMs);
    }
    popTip->toFront(false);
    //AccessibilityHandler::postAnnouncement(message, AccessibilityHandler::AnnouncementPriority::high);
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

    pvf->nameEditor = std::make_unique<TextEditor>("name");
    pvf->nameEditor->setFont(15 * SonoLookAndFeel::getFontScale());
    //pvf->nameEditor->setReadOnly(mPeerMode);
    auto edcb = [this,pvf]() {
        auto changroup = pvf->group;
        nameLabelChanged(changroup, pvf->nameEditor->getText());
    };
    pvf->nameEditor->onFocusLost = edcb;

    pvf->nameEditor->onReturnKey = [this,pvf]() {
        auto changroup = pvf->group;
        nameLabelChanged(changroup, pvf->nameEditor->getText());
        pvf->nameEditor->giveAwayKeyboardFocus();
    };

    pvf->nameEditor->onEscapeKey = [this,pvf]() {
        auto changroup = pvf->group;
        if (!mPeerMode) {
            pvf->nameEditor->setText(processor.getInputGroupName(changroup), dontSendNotification);
        }
        
        pvf->nameEditor->giveAwayKeyboardFocus();
    };


    /*
    pvf->nameLabel->setEditable(!mPeerMode);
    pvf->nameLabel->onTextChange = [this,pvf]() {
        auto changroup = pvf->group;
        nameLabelChanged(changroup, pvf->nameLabel->getText());
    };
     */



    if (!mPeerMode) {
        pvf->nameEditor->setColour(TextEditor::outlineColourId, Colour(0x66666666));
        pvf->nameEditor->setColour(TextEditor::backgroundColourId, Colours::black);
        //pvf->nameLabel->setColour(Label::outlineColourId, Colour(0x66666666));
        //pvf->nameLabel->setColour(Label::backgroundColourId, Colours::black);
        pvf->nameLabel->setTooltip(TRANS("Set name for this group that others will see"));
    } else {
        pvf->nameLabel->setTooltip(TRANS("Click to toggle extra information visibility"));
        //pvf->nameLabel->setColour(TextEditor::outlineColourId, Colours::transparentBlack);
        //pvf->nameLabel->setColour(TextEditor::backgroundColourId, Colours::transparentBlack);
    }



    pvf->muteButton = std::make_unique<TextButton>(TRANS("MUTE"));
    pvf->muteButton->addListener(this);
    pvf->muteButton->setLookAndFeel(&pvf->medLnf);
    pvf->muteButton->setClickingTogglesState(true);
    pvf->muteButton->setColour(TextButton::buttonOnColourId, mutedColor);

    if (mPeerMode) {
        if (first) {
            pvf->muteButton->setTooltip(TRANS("Toggles receive muting, preventing audio from being heard for this user"));
        }
        else {
            pvf->muteButton->setTooltip(TRANS("Toggles receive muting, preventing audio from being heard for this user"));
        }
    } else {
        pvf->muteButton->setTooltip(TRANS("Mute this channel for both sending and monitoring"));
    }

    pvf->soloButton = std::make_unique<TextButton>(TRANS("SOLO"));
    pvf->soloButton->addListener(this);
    pvf->soloButton->setLookAndFeel(&pvf->medLnf);
    pvf->soloButton->setClickingTogglesState(true);
    pvf->soloButton->setColour(TextButton::buttonOnColourId, soloColor.withAlpha(0.7f));
    pvf->soloButton->setColour(TextButton::textColourOnId, Colours::darkblue);
    if (mPeerMode) {
        if (first) {
            pvf->soloButton->setTooltip(TRANS("Solo - Listen to only this user, and other soloed users. Alt-click to exclusively solo this user."));
        } else {
            pvf->soloButton->setTooltip(TRANS("Solo - Listen to only this channel for this user"));
        }
    } else {
        pvf->soloButton->setTooltip(TRANS("Solo - Listen to only this channel, does not affect sending"));
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
    pvf->monitorSlider->setName("monitor");
    pvf->monitorSlider->addListener(this);

    configLevelSlider(pvf->monitorSlider.get(), true);
    pvf->monitorSlider->setLookAndFeel(&pvf->panSliderLNF);
    pvf->monitorSlider->setTextBoxStyle(Slider::NoTextBox, true, 60, 14);
    //pvf->monitorSlider->setTooltip(TRANS("Monitor output level"));


    pvf->panLabel = std::make_unique<Label>("pan", TRANS("Pan"));
    configLabel(pvf->panLabel.get(), LabelTypeSmall);
    pvf->panLabel->setJustificationType(Justification::centredTop);
    pvf->panLabel->setAccessible(false);

    pvf->panSlider     = std::make_unique<Slider>(Slider::LinearHorizontal,  Slider::NoTextBox);
    //pvf->panSlider->setTextBoxStyle(Slider::TextBoxAbove, true, 60, 12);
    pvf->panSlider->setTitle(TRANS("Pan"));
    pvf->panSlider->setName(first ? "firstpan1": "pan1");
    pvf->panSlider->addListener(this);
    pvf->panSlider->getProperties().set ("fromCentre", true);
    pvf->panSlider->getProperties().set ("noFill", true);
    pvf->panSlider->setRange(-1, 1, 0.0f);
    pvf->panSlider->setDoubleClickReturnValue(true, 0.0);
    pvf->panSlider->setTextBoxIsEditable(true);
    pvf->panSlider->setSliderSnapsToMousePosition(false);
    pvf->panSlider->setScrollWheelEnabled(false);
    pvf->panSlider->setMouseDragSensitivity(100);
    pvf->panSlider->setWantsKeyboardFocus(true);

    pvf->panSlider->textFromValueFunction =  [](double v) -> String { if (fabs(v) < 0.01) return String(TRANS("Pan: Center")); return String(TRANS("Pan: ")) +  String((int)rint(abs(v*100.0f))) + ((v > 0 ? "% R" : "% L")) ; };
    pvf->panSlider->valueFromTextFunction =  [](const String& s) -> double { return s.getDoubleValue()*1e-2f; };
    pvf->panSlider->setValue(0.1, dontSendNotification);
    pvf->panSlider->setValue(0.0, dontSendNotification);
    pvf->panSlider->setLookAndFeel(&pvf->panSliderLNF);
    pvf->singlePanner = true;

    std::unique_ptr<Drawable> destimg(Drawable::createFromImageData(BinaryData::chevron_forward_svg, BinaryData::chevron_forward_svgSize));
    std::unique_ptr<Drawable> linkimg(Drawable::createFromImageData(BinaryData::link_svg, BinaryData::link_svgSize));
    std::unique_ptr<Drawable> trirightimg(Drawable::createFromImageData(BinaryData::expand_arrow_inactive_svg, BinaryData::expand_arrow_inactive_svgSize));
    std::unique_ptr<Drawable> triimg(Drawable::createFromImageData(BinaryData::expand_arrow_active_svg, BinaryData::expand_arrow_active_svgSize));

    pvf->linkButton = std::make_unique<SonoDrawableButton>("", DrawableButton::ButtonStyle::ImageRightOfTextLabel);


    pvf->linkButton->setColour(SonoTextButton::outlineColourId, Colours::transparentBlack);
    pvf->linkButton->setColour(DrawableButton::backgroundOnColourId, Colour::fromFloatRGBA(0.4, 0.35, 0.4, 0.7));
    pvf->linkButton->setColour(DrawableButton::backgroundColourId, Colour::fromFloatRGBA(0.2, 0.2, 0.2, 0.6));
    pvf->linkButton->setClickingTogglesState(false);
    pvf->linkButton->setTriggeredOnMouseDown(false);
    pvf->linkButton->setLookAndFeel(&pvf->smallLnf);
    pvf->linkButton->addListener(this);
    pvf->linkButton->setForegroundImageRatio(0.35f);


    //pvf->linkButton->setAlpha(0.6f);
    if (mPeerMode) {
        pvf->linkButton->setTooltip(TRANS("Change channel layout"));
        pvf->linkButton->setTitle(TRANS("Channel Layout"));

        pvf->nameLabel->setInterceptsMouseClicks(true, false);
        pvf->nameLabel->addMouseListener(this, false);
    } else {
        pvf->linkButton->setTooltip(TRANS("Select Input channel source (or drag to rearrange)"));
        pvf->linkButton->setTitle(TRANS("Input Source"));
        pvf->linkButton->addMouseListener(this, false);
        pvf->nameLabel->addMouseListener(this, false);
        pvf->nameEditor->addMouseListener(this, false);
    }

    if (mPeerMode) {
        if (!first) {
            pvf->linkButton->setForegroundImageRatio(0.4f);
            pvf->linkButton->setImages(linkimg.get());
        }
        else {
            // expand button
            pvf->linkButton->setButtonStyle(DrawableButton::ButtonStyle::ImageLeftOfTextLabel);
            pvf->linkButton->setForegroundImageRatio(0.35f);
            pvf->linkButton->setImages(trirightimg.get(), nullptr, nullptr, nullptr,triimg.get());
        }
    }
    else {
        pvf->linkButton->setImages(destimg.get());
    }



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

    pvf->destButton->setForegroundImageRatio(0.35f);
    //pvf->destButton->setColour(SonoTextButton::outlineColourId, Colour::fromFloatRGBA(0.4, 0.4, 0.4, 0.6));
    //pvf->destButton->setColour(DrawableButton::backgroundOnColourId, Colour::fromFloatRGBA(0.4, 0.35, 0.4, 0.7));
    pvf->destButton->setColour(DrawableButton::backgroundColourId, Colour::fromFloatRGBA(0.2, 0.2, 0.2, 0.6));
    pvf->destButton->setLookAndFeel(&pvf->smallLnf);
    pvf->destButton->addListener(this);
    if (mPeerMode) {
        pvf->destButton->setTooltip(TRANS("Choose destination output channels"));
        pvf->destButton->setTitle(TRANS("Output Destination"));
    } else {
        pvf->destButton->setTooltip(TRANS("Choose destination monitoring channels"));
        pvf->destButton->setTitle(TRANS("Monitor Destination"));
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
        pvf->fxButton->setTitle(TRANS("Input Effects"));
    } else {
        pvf->fxButton->setTooltip(TRANS("Edit effects"));
        pvf->fxButton->setTitle(TRANS("Effects"));
    }

    pvf->monfxButton = std::make_unique<TextButton>(TRANS("M.FX"));
    pvf->monfxButton->setColour(TextButton::buttonOnColourId, Colour::fromFloatRGBA(0.2, 0.5, 0.7, 0.5));
    pvf->monfxButton->addListener(this);
    pvf->monfxButton->setLookAndFeel(&pvf->medLnf);
    if (!mPeerMode) {
        pvf->monfxButton->setTooltip(TRANS("Edit input monitoring effects (applied to local monitoring only)"));
        pvf->monfxButton->setTitle(TRANS("Input Monitoring Effects"));
    } else {
        pvf->monfxButton->setTooltip(TRANS("Edit monitoring effects"));
        pvf->monfxButton->setTitle(TRANS("Monitoring Effects"));
    }

    

    // meters
    auto flags = foleys::LevelMeter::Minimal /* | foleys::LevelMeter::SingleChannel */;
    
    pvf->meter = std::make_unique<foleys::LevelMeter>(flags);
    pvf->meter->setLookAndFeel(&(pvf->rmeterLnf));
    pvf->meter->setRefreshRateHz(metersActive ? 8 : 0);
    pvf->meter->addMouseListener(this, false);

    if (!mPeerMode) {
        pvf->premeter = std::make_unique<foleys::LevelMeter>(flags);
        pvf->premeter->setLookAndFeel(&(pvf->rmeterLnf));
        pvf->premeter->setRefreshRateHz(metersActive ? 8 : 0);
        pvf->premeter->addMouseListener(this, false);
    }

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

    if (mMetChannelView) {
        mMetChannelView->meter->setRefreshRateHz(rate);
    }
    if (mFileChannelView) {
        mFileChannelView->meter->setRefreshRateHz(rate);
    }
    if (mSoundboardChannelView) {
        mSoundboardChannelView->meter->setRefreshRateHz(rate);
    }


    for (int i=0; i < mChannelViews.size(); ++i) {
        ChannelGroupView * pvf = mChannelViews.getUnchecked(i);
        if (pvf->meter) {
            pvf->meter->setRefreshRateHz(subrate);
        }
        if (pvf->premeter) {
            pvf->premeter->setRefreshRateHz(subrate);
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
            //mMainChannelView->nameLabel->setColour(TextEditor::backgroundColourId, Colours::transparentBlack);
            //mMainChannelView->nameLabel->setColour(TextEditor::outlineColourId, Colours::transparentBlack);
            mMainChannelView->nameLabel->setWantsKeyboardFocus(true);

            mMainChannelView->linkButton->onClick = [this]() {
                processor.setRemotePeerViewExpanded(mPeerIndex, mMainChannelView->linkButton->getToggleState());
                updateChannelViews();
                updateLayout();
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

            mMainChannelView->monfxButton->onClick = [this]() {
                if (!monEffectsCalloutBox) {
                    showMonitorEffects(0, true, mMainChannelView->monfxButton.get());
                } else {
                    showMonitorEffects(0, false);
                }
            };

            mMainChannelView->destButton->onClick = [this]() {
                // when shown it will be for the first one
                showDestSelectionMenu(mMainChannelView->destButton.get(), 0);
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

        if (!mMetChannelView) {
            mMetChannelView.reset(createChannelGroupView(true));
            mMetChannelView->nameLabel->setText(TRANS("Metronome"), dontSendNotification);
            mMetChannelView->nameLabel->setEditable(false);
            mMetChannelView->nameLabel->setColour(Label::backgroundColourId, Colours::transparentBlack);
            mMetChannelView->nameLabel->setColour(Label::outlineColourId, Colours::transparentBlack);
            //mMetChannelView->nameLabel->setReadOnly(true);
            //mMetChannelView->nameLabel->setColour(TextEditor::backgroundColourId, Colours::transparentBlack);
            //mMetChannelView->nameLabel->setColour(TextEditor::outlineColourId, Colours::transparentBlack);

            mMetLevelAttachment   = std::make_unique<AudioProcessorValueTreeState::SliderAttachment> (processor.getValueTreeState(), SonobusAudioProcessor::paramMetGain, *mMetChannelView->levelSlider);


            std::unique_ptr<Drawable> grpimg(Drawable::createFromImageData(BinaryData::send_group_small_svg, BinaryData::send_group_small_svgSize));
            mMetChannelView->linkButton->setButtonStyle(DrawableButton::ButtonStyle::ImageOnButtonBackground);
            mMetChannelView->linkButton->setTitle(TRANS("Send Metronome"));
            mMetChannelView->linkButton->setImages(grpimg.get());
            mMetChannelView->linkButton->setClickingTogglesState(true);
            mMetSendAttachment = std::make_unique<AudioProcessorValueTreeState::ButtonAttachment> (processor.getValueTreeState(), SonobusAudioProcessor::paramSendMetAudio, *mMetChannelView->linkButton);
            mMetChannelView->linkButton->setForegroundImageRatio(1.0f);
            mMetChannelView->linkButton->setColour(TextButton::buttonOnColourId, Colour::fromFloatRGBA(0.2, 0.5, 0.7, 0.65));
            mMetChannelView->linkButton->setColour(TextButton::buttonColourId, Colours::transparentBlack);
            mMetChannelView->linkButton->setTooltip(TRANS("Send Metronome to All"));



            mMetChannelView->soloButton->onClick = [this]() {
            };

            mMetChannelView->muteButton->onClick = [this]() {
            };


            mMetChannelView->fxButton->onClick = [this]() {
                /*
                if (!effectsCalloutBox) {
                    showEffects(0, true, mMetChannelView->fxButton.get());
                } else {
                    showEffects(0, false);
                }
                 */
            };

            mMetChannelView->monfxButton->onClick = [this]() {
                if (!monEffectsCalloutBox) {
                    showMonitorEffects(-1, true, mMetChannelView->monfxButton.get());
                } else {
                    showMonitorEffects(-1, false);
                }
            };

            mMetChannelView->destButton->onClick = [this]() {
                // when shown it will be for the first one
                showDestSelectionMenu(mMetChannelView->destButton.get(), -1);
            };

            //mMetChannelView->levelSlider->onValueChange = [this]() {
            //    processor.setMetronomeGain(mMetChannelView->levelSlider->getValue());
            //};

            mMetChannelView->panSlider->onValueChange = [this]() {
                processor.setMetronomePan(mMetChannelView->panSlider->getValue());
                // Send OSC message for MetPanSlider value change
                if (processor.getOSCEnabled()) {
                    processor.getOSCManager().sendMessage("/MetPanSlider", static_cast<float>(mMetChannelView->panSlider->getValue()));
                }
            };

            mMetChannelView->monitorSlider->onValueChange = [this]() {
                processor.setMetronomeMonitor(mMetChannelView->monitorSlider->getValue());
                // Send OSC message for MetMonitorSlider value change
                if (processor.getOSCEnabled()) {
                    processor.getOSCManager().sendMessage("/MetMonitorSlider", static_cast<float>(mMetChannelView->monitorSlider->getValue()));
                }
            };

            setupChildren(mMetChannelView.get());

            mMetChannelView->muteButton->setVisible(false);
            mMetChannelView->soloButton->setVisible(false);
            mMetChannelView->fxButton->setVisible(false);
            mMetChannelView->premeter->setVisible(false);
            mMetChannelView->premeter->setRefreshRateHz(0);
            //mMetChannelView->premeter->setVisible(false);
            //mMetChannelView->premeter->setRefreshRateHz(0);

        }

        if (!mFileChannelView) {
            mFileChannelView.reset(createChannelGroupView(true));
            mFileChannelView->nameLabel->setEditable(false);
            mFileChannelView->nameLabel->setText(TRANS("File Playback"), dontSendNotification);
            mFileChannelView->nameLabel->setColour(Label::backgroundColourId, Colours::transparentBlack);
            mFileChannelView->nameLabel->setColour(Label::outlineColourId, Colours::transparentBlack);
            //mFileChannelView->nameLabel->setColour(TextEditor::backgroundColourId, Colours::transparentBlack);
            //mFileChannelView->nameLabel->setColour(TextEditor::outlineColourId, Colours::transparentBlack);
            //mFileChannelView->nameLabel->setReadOnly(true);

            mFileChannelView->linkButton->setClickingTogglesState(true);
            std::unique_ptr<Drawable> grpimg(Drawable::createFromImageData(BinaryData::send_group_small_svg, BinaryData::send_group_small_svgSize));
            mFileChannelView->linkButton->setTitle(TRANS("Send File Playback"));
            mFileChannelView->linkButton->setImages(grpimg.get());
            mFileChannelView->linkButton->setClickingTogglesState(true);
            mFileSendAttachment = std::make_unique<AudioProcessorValueTreeState::ButtonAttachment> (processor.getValueTreeState(), SonobusAudioProcessor::paramSendFileAudio, *mFileChannelView->linkButton);
            mFileChannelView->linkButton->setButtonStyle(DrawableButton::ButtonStyle::ImageOnButtonBackground);
            mFileChannelView->linkButton->setForegroundImageRatio(1.0f);
            mFileChannelView->linkButton->setColour(TextButton::buttonOnColourId, Colour::fromFloatRGBA(0.2, 0.5, 0.7, 0.65));
            mFileChannelView->linkButton->setColour(TextButton::buttonColourId, Colours::transparentBlack);
            mFileChannelView->linkButton->setTooltip(TRANS("Send File Playback to All"));


            mFileChannelView->soloButton->onClick = [this]() {
            };

            mFileChannelView->muteButton->onClick = [this]() {
            };


            mFileChannelView->fxButton->onClick = [this]() {
                /*
                if (!effectsCalloutBox) {
                    showEffects(0, true, mFileChannelView->fxButton.get());
                } else {
                    showEffects(0, false);
                }
                 */
            };

            mFileChannelView->monfxButton->onClick = [this]() {
                if (!monEffectsCalloutBox) {
                    showMonitorEffects(-2, true, mFileChannelView->monfxButton.get());
                } else {
                    showMonitorEffects(-2, false);
                }
            };

            mFileChannelView->destButton->onClick = [this]() {
                // when shown it will be for the first one
                showDestSelectionMenu(mFileChannelView->destButton.get(), -2);
            };

            mFileChannelView->levelSlider->onValueChange = [this]() {
                processor.setFilePlaybackGain(mFileChannelView->levelSlider->getValue());
                // Send OSC message for File Playback Pre Level change
                if (processor.getOSCEnabled()) {
                    processor.getOSCManager().sendMessage("/FilePlaybackPreLevel", static_cast<float>(mFileChannelView->levelSlider->getValue()));
                }
            };

            //mFileChannelView->panSlider->onValueChange = [this]() {
            //    processor.setFilePlaybackPan(mFileChannelView->panSlider->getValue());
            //};

            mFileChannelView->monitorSlider->onValueChange = [this]() {
                processor.setFilePlaybackMonitor(mFileChannelView->monitorSlider->getValue());
                // Send OSC message for FileMonitorSlider value change
                if (processor.getOSCEnabled()) {
                    processor.getOSCManager().sendMessage("/FileMonitorSlider", static_cast<float>(mFileChannelView->monitorSlider->getValue()));
                }
            };

            setupChildren(mFileChannelView.get());

            mFileChannelView->muteButton->setVisible(false);
            mFileChannelView->soloButton->setVisible(false);
            mFileChannelView->fxButton->setVisible(false);
            mFileChannelView->panSlider->setVisible(false);

            mFileChannelView->premeter->setVisible(false);
            mFileChannelView->premeter->setRefreshRateHz(0);
            //mFileChannelView->premeter->setVisible(false);
            //mFileChannelView->premeter->setRefreshRateHz(0);

        }
        if (!mSoundboardChannelView) {
            mSoundboardChannelView.reset(createChannelGroupView(true));
            mSoundboardChannelView->nameLabel->setEditable(false);
            mSoundboardChannelView->nameLabel->setText(TRANS("Soundboard"), dontSendNotification);
            mSoundboardChannelView->nameLabel->setColour(Label::backgroundColourId, Colours::transparentBlack);
            mSoundboardChannelView->nameLabel->setColour(Label::outlineColourId, Colours::transparentBlack);

            mSoundboardChannelView->linkButton->setClickingTogglesState(true);
            std::unique_ptr<Drawable> grpimg(Drawable::createFromImageData(BinaryData::send_group_small_svg, BinaryData::send_group_small_svgSize));
            mSoundboardChannelView->linkButton->setTitle(TRANS("Send Soundboard"));
            mSoundboardChannelView->linkButton->setImages(grpimg.get());
            mSoundboardChannelView->linkButton->setClickingTogglesState(true);
            mSoundboardSendAttachment = std::make_unique<AudioProcessorValueTreeState::ButtonAttachment> (processor.getValueTreeState(), SonobusAudioProcessor::paramSendSoundboardAudio, *mSoundboardChannelView->linkButton);
            mSoundboardChannelView->linkButton->setButtonStyle(DrawableButton::ButtonStyle::ImageOnButtonBackground);
            mSoundboardChannelView->linkButton->setForegroundImageRatio(1.0f);
            mSoundboardChannelView->linkButton->setColour(TextButton::buttonOnColourId, Colour::fromFloatRGBA(0.2, 0.5, 0.7, 0.65));
            mSoundboardChannelView->linkButton->setColour(TextButton::buttonColourId, Colours::transparentBlack);
            mSoundboardChannelView->linkButton->setTooltip(TRANS("Send Soundboard to All"));

            mSoundboardChannelView->soloButton->onClick = []() {};
            mSoundboardChannelView->muteButton->onClick = []() {};
            mSoundboardChannelView->fxButton->onClick = []() {};

            mSoundboardChannelView->monfxButton->onClick = [this]() {
               if (!monEffectsCalloutBox) {
                   showMonitorEffects(-3, true, mSoundboardChannelView->monfxButton.get());
               }
               else {
                   showMonitorEffects(-3, false);
               }
            };

            mSoundboardChannelView->destButton->onClick = [this]() {
                // when shown it will be for the first one
                showDestSelectionMenu(mSoundboardChannelView->destButton.get(), -3);
            };

            mSoundboardChannelView->levelSlider->onValueChange = [this]() {
                processor.getSoundboardProcessor()->setGain(mSoundboardChannelView->levelSlider->getValue());
                // Send OSC message for SoundboardLevelSlider value change
                if (processor.getOSCEnabled()) {
                    processor.getOSCManager().sendMessage("/SoundboardLevelSlider", static_cast<float>(mSoundboardChannelView->levelSlider->getValue()));
                }
            };

            //mSoundboardChannelView->panSlider->onValueChange = [this]() {
            //    processor.setSoundboardPlaybackPan(mSoundboardChannelView->panSlider->getValue());
            //};

            mSoundboardChannelView->monitorSlider->onValueChange = [this]() {
                processor.getSoundboardProcessor()->setMonitorGain(mSoundboardChannelView->monitorSlider->getValue());
                // Send OSC message for SoundboardMonitorSlider value change
                if (processor.getOSCEnabled()) {
                    processor.getOSCManager().sendMessage("/SoundboardMonitorSlider", static_cast<float>(mSoundboardChannelView->monitorSlider->getValue()));
                }
            };

            setupChildren(mSoundboardChannelView.get());

            mSoundboardChannelView->muteButton->setVisible(false);
            mSoundboardChannelView->soloButton->setVisible(false);
            mSoundboardChannelView->fxButton->setVisible(false);
            mSoundboardChannelView->panSlider->setVisible(false);

            mSoundboardChannelView->premeter->setVisible(false);
            mSoundboardChannelView->premeter->setRefreshRateHz(0);
            //mSoundboardChannelView->premeter->setVisible(false);
            //mSoundboardChannelView->premeter->setRefreshRateHz(0);
        }

        mMetFileBg->setVisible(true);
    }

    while (mChannelViews.size() < numchans) {
        mChannelViews.add(createChannelGroupView(!mPeerMode && mChannelViews.size() == 0));
    }
    while (mChannelViews.size() > numchans) {
        mChannelViews.removeLast();
    }


    for (int i= (mPeerMode ? -1 : 0); i < mChannelViews.size(); ++i) {
        ChannelGroupView * pvf = i < 0 ? mMainChannelView.get() : mChannelViews.getUnchecked(i);

        setupChildren(pvf);
    }
    
    updateChannelViews();
    updateLayout(notify);
    resized();
}

void ChannelGroupsView::setupChildren(ChannelGroupView * pvf)
{
    Component* dw = this->findParentComponentOfClass<AudioProcessorEditor>();
    if (!dw) dw = this->findParentComponentOfClass<Component>();
    if (!dw) dw = this;

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
    pvf->addChildComponent(pvf->nameEditor.get());

    pvf->addAndMakeVisible(pvf->meter.get());
    if (pvf->premeter) {
        pvf->addAndMakeVisible(pvf->premeter.get());
    }
    pvf->addAndMakeVisible(pvf->fxButton.get());

    pvf->addAndMakeVisible(pvf->monfxButton.get());

    pvf->addAndMakeVisible(pvf->panSlider.get());

    pvf->panSlider->setPopupDisplayEnabled(true, true, dw);

    pvf->monitorSlider->setPopupDisplayEnabled(true, true, dw);

    addAndMakeVisible(pvf);
}

void ChannelGroupsView::updateLayoutForRemotePeer(bool notify)
{
    int minitemheight = 34 ;
    int mincheckheight = 32;
    int minPannerWidth = isNarrow ? 56 : 64;
    int minButtonWidth = 60;
    int maxPannerWidth = 130;
    int compactMaxPannerWidth = 90;
    int minSliderWidth = isNarrow ? 90 : 100;
    int meterwidth = 10;
    int mainmeterwidth = 10;
    int muteminbuttwidth = isNarrow ? 30 : 52;
    int mutebuttwidth = isNarrow ? 42 : 52;
    int linkbuttwidth = 50;
    int destbuttwidth = 44;
    int destminbuttwidth = isNarrow ? 36 : 44;
    int monsliderwidth =  0 ;
    int namewidth = isNarrow ? 88 :  110;
    int addrowheight = minitemheight - 2;

#if JUCE_IOS || JUCE_ANDROID
    // make the button heights a bit more for touchscreen purposes
    minitemheight = 42;
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
    totaloutchans = processor.getTotalNumOutputChannels();

    int estwidth = mEstimatedWidth > 13 ? mEstimatedWidth - 13 : 320;

    changroups = processor.getRemotePeerChannelGroupCount(mPeerIndex);

    totalchans = processor.getRemotePeerRecvChannelCount(mPeerIndex);

    processor.getRemotePeerChannelGroupStartAndCount(mPeerIndex, changroup, chstart, chcnt);
    processor.getRemotePeerChannelGroupDestStartAndCount(mPeerIndex, changroup, deststart, destcnt);
    destcnt = jmin(totaloutchans, destcnt);


    if ((destcnt != 2) || chcnt == 0) {
        pannervisible = false;
    }

    mainmeterwidth = totalchans * meterwidth;
    if (totalchans > 2) {
        mainmeterwidth = 6 * totalchans;
    }


    // Main connected peer views


    for (int i = -1; i < mChannelViews.size(); ++i, ++chi) {
        if (i==0) {
            chi = 0; // ensure this
        }

        if (chi >= chcnt && changroup < changroups-1) {
            changroup++;
            processor.getRemotePeerChannelGroupStartAndCount(mPeerIndex, changroup, chstart, chcnt);
            processor.getRemotePeerChannelGroupDestStartAndCount(mPeerIndex, changroup, deststart, destcnt);
            destcnt = jmin(totaloutchans, destcnt);

            if ((destcnt != 2) || chcnt == 0) {
                pannervisible = false;
            }
            else {
                pannervisible = true;
            }
            chi = 0;
        }

        ChannelGroupView * pvf = i < 0 ? mMainChannelView.get() : mChannelViews.getUnchecked(i);

        //pvf->updateLayout();
        bool viewexpanded = processor.getRemotePeerViewExpanded(mPeerIndex);

        if (!viewexpanded && i >= 0) {
            // skip this one, not expanded
            continue;
        }


        bool destbuttvisible = true;

        if (chi == 0 || (totalchans > 1) )
        {

            pvf->namebox.items.clear();
            pvf->namebox.flexDirection = FlexBox::Direction::row;
            //.withAlignSelf(FlexItem::AlignSelf::center));
            pvf->namebox.items.add(FlexItem(namewidth, minitemheight, *pvf->nameLabel).withMargin(0).withFlex(1));

            pvf->inbox.items.clear();
            pvf->inbox.flexDirection = FlexBox::Direction::row;
            // pvf->inbox.items.add(FlexItem(20, minitemheight, *pvf->chanLabel).withMargin(0).withFlex(0));
            // pvf->inbox.items.add(FlexItem(3, 3));

            if (totalchans == 1) {
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
                pvf->monbox.items.add(FlexItem(muteminbuttwidth, minitemheight, *pvf->muteButton).withMargin(0).withFlex(1).withMaxWidth(mutebuttwidth));
                pvf->monbox.items.add(FlexItem(3, 3));
                pvf->monbox.items.add(FlexItem(muteminbuttwidth, minitemheight, *pvf->soloButton).withMargin(0).withFlex(1).withMaxWidth(mutebuttwidth));
                pvf->monbox.items.add(FlexItem(3, 3));

                pvf->monbox.items.add(FlexItem(muteminbuttwidth, minitemheight, *pvf->fxButton).withMargin(0).withFlex(1).withMaxWidth(mutebuttwidth));

                pvf->monbox.items.add(FlexItem(3, 3).withFlex(0.25));

                if (i < 0 ) {
                    pvf->monbox.items.add(FlexItem(mainmeterwidth, minitemheight, *pvf->meter).withMargin(0).withFlex(0));
                    if (totalchans == 1) {
                        // add gap
                        pvf->monbox.items.add(FlexItem(meterwidth, minitemheight).withMargin(0).withFlex(0));
                    }
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


                if (destbuttvisible) {
                    pvf->monbox.items.add(FlexItem(destminbuttwidth, minitemheight, *pvf->destButton).withMargin(0).withFlex(1).withMaxWidth(destbuttwidth));
                    pvf->monbox.items.add(FlexItem(2, 3));
                }

            }
            else {
                if (i < 0) {
                    pvf->monbox.items.add(FlexItem(mainmeterwidth, minitemheight, *pvf->meter).withMargin(0).withFlex(0));
                    if (totalchans == 1) {
                        // add gap
                        pvf->monbox.items.add(FlexItem(meterwidth, minitemheight).withMargin(0).withFlex(0));
                    }
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

            bool dotopgap = (chi == 0) && (i >= 0 );

            if (i == -1) {
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
                    pvf->maincontentbox.items.add(FlexItem(mpw, mph , pvf->monbox).withMargin(0).withFlex(1).withMaxWidth(maxPannerWidth + mutebuttwidth  + (destbuttvisible ? destbuttwidth + 2 : 0) + 2));
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


    // for input mode add met and file playback rows
    if (!mPeerMode) {

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

void ChannelGroupsView::updateLayoutForInput(bool notify)
{
    int minitemheight =  30;
    int mincheckheight = 32;
    int minPannerWidth = isNarrow ? 50 : 64;
    int minButtonWidth = 60;
    int maxPannerWidth = 130;
    int compactMaxPannerWidth = 90;
    int minSliderWidth = isNarrow ? 90 : 100;
    int meterwidth = 10;
    int mainmeterwidth = 10;
    int mutebuttwidth = isNarrow ? 42 : 52;
    int muteminbuttwidth = isNarrow ? 30 : 52;
    int linkbuttwidth = 50;
    int destbuttwidth = 44;
    int destminbuttwidth = isNarrow ? 36 : 44;
    int monsliderwidth =  40;
    int namewidth = isNarrow ? 88 : 100;
    int addrowheight = minitemheight - 2;

#if JUCE_IOS || JUCE_ANDROID
    // make the button heights a bit more for touchscreen purposes
    minitemheight =  38;
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


    sendcnt = (int) processor.getValueTreeState().getParameter(SonobusAudioProcessor::paramSendChannels)->convertFrom0to1( processor.getValueTreeState().getParameter(SonobusAudioProcessor::paramSendChannels)->getValue());

    if (sendcnt == 0) {
        sendcnt = processor.getTotalNumInputChannels(); // processor.getMainBusNumInputChannels();
    }

    totalchans = processor.getTotalNumInputChannels();
    changroups = processor.getInputGroupCount();

    totaloutchans = processor.getTotalNumOutputChannels();

    processor.getInputGroupChannelStartAndCount(changroup, chstart, chcnt);
    processor.getInputGroupChannelDestStartAndCount(changroup, deststart, destcnt);

    destcnt = jmin(totaloutchans, destcnt);
    if ((sendcnt != 2 && destcnt != 2) || chcnt == 0) {
        pannervisible = false;
    }


    // Main connected peer views

    addrowBox.items.clear();
    addrowBox.flexDirection = FlexBox::Direction::row;
    addrowBox.items.add(FlexItem(4, 2).withMargin(0));
    addrowBox.items.add(FlexItem(linkbuttwidth, addrowheight, *mAddButton).withMargin(0).withFlex(0));
    addrowBox.items.add(FlexItem(6, 2).withMargin(0).withFlex(1));
    addrowBox.items.add(FlexItem(minButtonWidth, addrowheight, *mInReverbButton).withMargin(0).withFlex(0));
    addrowBox.items.add(FlexItem(6, 2).withMargin(0).withFlex(1));
    addrowBox.items.add(FlexItem(minButtonWidth, addrowheight, *mMonDelayButton).withMargin(0).withFlex(0));
    addrowBox.items.add(FlexItem(6, 2).withMargin(0).withFlex(1));
    addrowBox.items.add(FlexItem(mutebuttwidth, addrowheight, *mClearButton).withMargin(0).withFlex(0));
    addrowBox.items.add(FlexItem(4, 2).withMargin(0));

    int gaph = 6 ; //changroups > 0 ? 2 : 6;
    int bgaph = changroups > 0 ? 4 : 6;
    channelsBox.items.add(FlexItem(8, gaph).withMargin(0));
    channelsBox.items.add(FlexItem(100, addrowheight, addrowBox).withMargin(0).withFlex(0));
    channelsBox.items.add(FlexItem(8, bgaph).withMargin(0));
    peersheight += addrowheight + gaph + bgaph;

    // all the inputs, plus three extra (met and file playback and soundboard)
    for (int i =  0; i < mChannelViews.size() + 3; ++i, ++chi) {
        if (i==0) {
            chi = 0; // ensure this
        }

        if (i >= mChannelViews.size()) {
            // one of met or file playback
            chi = 0;
        }
        else if (chi >= chcnt && changroup < changroups-1) {
            changroup++;
            processor.getInputGroupChannelStartAndCount(changroup, chstart, chcnt);

            processor.getInputGroupChannelDestStartAndCount(changroup, deststart, destcnt);

            destcnt = jmin(totaloutchans, destcnt);
            if ((sendcnt != 2 && destcnt != 2) || chcnt == 0) {
                pannervisible = false;
            } else {
                pannervisible = true;
            }
            chi = 0;
        }

        bool ismetorfileorsoundboard = false;

        ChannelGroupView * pvf;
        if (i == mChannelViews.size()) {
            pvf = mMetChannelView.get();
            ismetorfileorsoundboard = true;
        } else if (i == mChannelViews.size() + 1) {
            pvf = mFileChannelView.get();
            auto numfilechan = processor.getFilePlaybackMeterSource().getNumChannels();
            mainmeterwidth = numfilechan * (numfilechan > 2 ? 6 : meterwidth);
            ismetorfileorsoundboard = true;
        }
        else if (i > mChannelViews.size()) {
            pvf = mSoundboardChannelView.get();
            auto numsoundboardchan = processor.getSoundboardProcessor()->getMeterSource().getNumChannels();
            mainmeterwidth = numsoundboardchan * (numsoundboardchan > 2 ? 6 : meterwidth);
            ismetorfileorsoundboard = true;
        }
        else {
            pvf = mChannelViews.getUnchecked(i);
        }

        //pvf->updateLayout();
        bool viewexpanded = true;

        if (!viewexpanded && i >= 0) {
            // skip this one, not expanded
            continue;
        }


        bool destbuttvisible = true;

        //if (chi == 0 || !mPeerMode || (totalchans > 1) )
        {

            pvf->namebox.items.clear();
            pvf->namebox.flexDirection = FlexBox::Direction::row;
            //.withAlignSelf(FlexItem::AlignSelf::center));

            if (i >= mChannelViews.size()) {
                pvf->namebox.items.add(FlexItem(namewidth, minitemheight, *pvf->nameLabel).withMargin(0).withFlex(1));
            } else {
                pvf->namebox.items.add(FlexItem(namewidth, minitemheight, *pvf->nameEditor).withMargin(0).withFlex(1));
            }

            pvf->inbox.items.clear();
            pvf->inbox.flexDirection = FlexBox::Direction::row;
            // pvf->inbox.items.add(FlexItem(20, minitemheight, *pvf->chanLabel).withMargin(0).withFlex(0));
            // pvf->inbox.items.add(FlexItem(3, 3));

            pvf->inbox.items.add(FlexItem(linkbuttwidth, minitemheight, *pvf->linkButton).withMargin(0).withFlex(0));

            pvf->inbox.items.add(FlexItem(3, 3));

            pvf->inbox.items.add(FlexItem(meterwidth, minitemheight, *pvf->premeter).withMargin(0).withFlex(0));
            pvf->inbox.items.add(FlexItem(3, 3));


            pvf->inbox.items.add(FlexItem(namewidth, minitemheight, pvf->namebox).withMargin(0).withFlex(0));
            if (!isNarrow && !ismetorfileorsoundboard) {
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
                if (!ismetorfileorsoundboard) {
                    pvf->monbox.items.add(FlexItem(3, 3).withFlex(0.25));
                    pvf->monbox.items.add(FlexItem(muteminbuttwidth, minitemheight, *pvf->muteButton).withMargin(0).withFlex(1).withMaxWidth(mutebuttwidth));
                    pvf->monbox.items.add(FlexItem(3, 3));
                    pvf->monbox.items.add(FlexItem(muteminbuttwidth, minitemheight, *pvf->soloButton).withMargin(0).withFlex(1).withMaxWidth(mutebuttwidth));
                    pvf->monbox.items.add(FlexItem(3, 3));
                    pvf->monbox.items.add(FlexItem(muteminbuttwidth, minitemheight, *pvf->fxButton).withMargin(0).withFlex(1).withMaxWidth(mutebuttwidth));
                }

                pvf->monbox.items.add(FlexItem(3, 3).withFlex(0.25));

                pvf->monbox.items.add(FlexItem(mainmeterwidth, minitemheight, *pvf->meter).withMargin(0).withFlex(0));

                pvf->monbox.items.add(FlexItem(4, 3));

                //if (pannervisible)
                {
                    pvf->monbox.items.add(FlexItem(2, 3));
                    pvf->monbox.items.add(FlexItem(minPannerWidth, minitemheight, *pvf->panSlider).withMargin(0).withFlex(1).withMaxWidth(maxPannerWidth));
                    pvf->monbox.items.add(FlexItem(1, 3).withFlex(0.1).withMaxWidth(meterwidth + 10));
                }

                pvf->monbox.items.add(FlexItem(monsliderwidth, minitemheight, *pvf->monitorSlider).withMargin(0).withFlex(0));
                pvf->monbox.items.add(FlexItem(2, 3));

                pvf->monbox.items.add(FlexItem(muteminbuttwidth, minitemheight, *pvf->monfxButton).withMargin(0).withFlex(1).withMaxWidth(mutebuttwidth));
                pvf->monbox.items.add(FlexItem(2, 3));


                if (destbuttvisible) {
                    pvf->monbox.items.add(FlexItem(destminbuttwidth, minitemheight, *pvf->destButton).withMargin(0).withFlex(1).withMaxWidth(destbuttwidth));
                    pvf->monbox.items.add(FlexItem(2, 3));
                }

            }
            else {
                pvf->monbox.items.add(FlexItem(mainmeterwidth, minitemheight, *pvf->meter).withMargin(0).withFlex(0));
                pvf->monbox.items.add(FlexItem(4, 3));

                //if (pannervisible) {
                    pvf->monbox.items.add(FlexItem(minPannerWidth, minitemheight, *pvf->panSlider).withMargin(0).withFlex(0.25).withMaxWidth(maxPannerWidth));
                    pvf->monbox.items.add(FlexItem(3, 3));
                //}
                //pvf->monbox.items.add(FlexItem(mutebuttwidth, minitemheight, *pvf->fxButton).withMargin(0).withFlex(0));
                //pvf->monbox.items.add(FlexItem(2, 3));

                pvf->monbox.items.add(FlexItem(monsliderwidth, minitemheight, *pvf->monitorSlider).withMargin(0).withFlex(0));
                pvf->monbox.items.add(FlexItem(2, 3));
                pvf->monbox.items.add(FlexItem(mutebuttwidth, minitemheight, *pvf->monfxButton).withMargin(0).withFlex(0));
                pvf->monbox.items.add(FlexItem(2, 3));

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

            bool dotopgap = (chi == 0) || i >= mChannelViews.size();


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
                    pvf->maincontentbox.items.add(FlexItem(mpw, mph , pvf->monbox).withMargin(0).withFlex(1).withMaxWidth(maxPannerWidth + mutebuttwidth + (monsliderwidth) + (destbuttvisible ? destbuttwidth + 2 : 0) + 2));
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

            if (i < mChannelViews.size()+2) {
                channelsBox.items.add(FlexItem(3, 4));
                peersheight += 4;
            }

        }

    }


    // for input mode add met and file playback rows


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


void ChannelGroupsView::updateLayout(bool notify)
{
    if (mPeerMode) {
        updateLayoutForRemotePeer(notify);
    } else {
        updateLayoutForInput(notify);
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

    if (mMainChannelView) {
        routine(mMainChannelView->levelSlider.get());
        routine(mMainChannelView->panSlider.get());
        routine(mMainChannelView->monitorSlider.get());
    }

    if (mFileChannelView) {
        routine(mFileChannelView->levelSlider.get());
        routine(mFileChannelView->panSlider.get());
        routine(mFileChannelView->monitorSlider.get());
    }

    if (mMetChannelView) {
        routine(mMetChannelView->levelSlider.get());
        routine(mMetChannelView->panSlider.get());
        routine(mMetChannelView->monitorSlider.get());
    }

    if (mSoundboardChannelView) {
        routine(mSoundboardChannelView->levelSlider.get());
        routine(mSoundboardChannelView->panSlider.get());
        routine(mSoundboardChannelView->monitorSlider.get());
    }
}

void ChannelGroupsView::updateChannelViews(int specific)
{
    if (mPeerMode) {
        updatePeerModeChannelViews(specific);

        mAddButton->setVisible(false);
        mClearButton->setVisible(false);
        mInReverbButton->setVisible(false);
        mMonDelayButton->setVisible(false);
    } else {
        updateInputModeChannelViews(specific);

        mAddButton->setVisible(true);
        mClearButton->setVisible(true);
        mInReverbButton->setVisible(true);
        mMonDelayButton->setVisible(true);
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

    int totaloutchans = processor.getTotalNumOutputChannels();
    int changroups = processor.getInputGroupCount();
    int chi = 0;


    // met and file playback and soundboard
    if (mMetChannelView) {
        String desttext;
        int destcnt, deststart;
        processor.getMetronomeChannelDestStartAndCount(deststart, destcnt);
        if (destcnt == 1) {
            desttext << deststart + 1;
        } else {
            desttext << deststart + 1 << "-" << deststart+destcnt;
        }
        mMetChannelView->destButton->setButtonText(desttext);
        mMetChannelView->monitorSlider->setVisible(true);
        mMetChannelView->monitorSlider->setValue(processor.getMetronomeMonitor(), dontSendNotification);
        mMetChannelView->levelSlider->setValue(processor.getMetronomeGain(), dontSendNotification);
        mMetChannelView->panSlider->setValue(processor.getMetronomePan(), dontSendNotification);
        mMetChannelView->showDivider = true;
        mMetChannelView->nameEditor->setVisible(false);

        mMetChannelView->meter->setMeterSource (&processor.getMetronomeMeterSource());
        mMetChannelView->meter->setSelectedChannel(0);

        SonoAudio::DelayParams eparams;
        processor.getMetronomeMonitorDelayParams(eparams);
        mMetChannelView->monfxButton->setToggleState(eparams.enabled, dontSendNotification);

    }

    if (mFileChannelView) {
        String desttext;
        int destcnt, deststart;
        processor.getFilePlaybackDestStartAndCount(deststart, destcnt);
        if (destcnt == 1) {
            desttext << deststart + 1;
        } else {
            desttext << deststart + 1 << "-" << deststart+destcnt;
        }
        mFileChannelView->destButton->setButtonText(desttext);
        mFileChannelView->monitorSlider->setVisible(true);
        mFileChannelView->monitorSlider->setValue(processor.getFilePlaybackMonitor(), dontSendNotification);
        mFileChannelView->levelSlider->setValue(processor.getFilePlaybackGain(), dontSendNotification);
        mFileChannelView->panSlider->setVisible(false);
        mFileChannelView->panLabel->setVisible(false);
        //mFileChannelView->panSlider->setValue(processor.getFilPlaybackPan(), dontSendNotification);
        mFileChannelView->showDivider = true;
        mFileChannelView->nameEditor->setVisible(false);

        mFileChannelView->meter->setMeterSource (&processor.getFilePlaybackMeterSource());
        mFileChannelView->meter->setSelectedChannel(0);

        SonoAudio::DelayParams eparams;
        processor.getFilePlaybackMonitorDelayParams(eparams);
        mFileChannelView->monfxButton->setToggleState(eparams.enabled, dontSendNotification);

    }

    if (mSoundboardChannelView) {
        String desttext;
        int destcnt, deststart;
        processor.getSoundboardProcessor()->getDestStartAndCount(deststart, destcnt);
        if (destcnt == 1) {
            desttext << deststart + 1;
        } else {
            desttext << deststart + 1 << "-" << deststart+destcnt;
        }
        mSoundboardChannelView->destButton->setButtonText(desttext);
        mSoundboardChannelView->monitorSlider->setVisible(true);
        mSoundboardChannelView->monitorSlider->setValue(processor.getSoundboardProcessor()->getMonitorGain(), dontSendNotification);
        mSoundboardChannelView->levelSlider->setValue(processor.getSoundboardProcessor()->getGain(), dontSendNotification);
        mSoundboardChannelView->panSlider->setVisible(false);
        mSoundboardChannelView->panLabel->setVisible(false);
        //mSoundboardChannelView->panSlider->setValue(processor.getSoundboardPan(), dontSendNotification);
        mSoundboardChannelView->showDivider = true;
        mSoundboardChannelView->nameEditor->setVisible(false);

        mSoundboardChannelView->meter->setMeterSource(&processor.getSoundboardProcessor()->getMeterSource());
        mSoundboardChannelView->meter->setSelectedChannel(0);

        SonoAudio::DelayParams eparams = processor.getSoundboardProcessor()->getMonitorDelayParams();
        mSoundboardChannelView->monfxButton->setToggleState(eparams.enabled, dontSendNotification);
    }


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

        pvf->group = changroup;
        pvf->groupChanCount = chcnt;
        pvf->chanIndex = chi;


        String name = processor.getInputGroupName(changroup);

        //DBG("Got username: '" << username << "'");

        //pvf->nameLabel->setText(name, dontSendNotification);
        pvf->nameEditor->setText(name, dontSendNotification);

        String chantext;
        if (chstart + chi >= totalchans) {
            // out of range now
            chantext << "--";
        }
        else {
            if (chi == 0 && chcnt > 1) {
                chantext << chstart+1 << "-" << chstart+chcnt;
            } else {
                chantext << chstart + chi + 1;
            }
        }
        //pvf->chanLabel->setText(String::formatted("%d", i+1), dontSendNotification);

        pvf->linkButton->setButtonText(chantext);

        bool aresoloed = processor.getInputGroupSoloed(changroup);
        bool aremuted = processor.getInputGroupMuted(changroup);

        pvf->showDivider = chi == 0 ; // || i == 0;

        pvf->muteButton->setToggleState(aremuted , dontSendNotification);
        pvf->soloButton->setToggleState(aresoloed , dontSendNotification);


        bool infxon = processor.getInputEffectsActive(changroup);
        pvf->fxButton->setToggleState(infxon, dontSendNotification);

        bool inmonfxon = processor.getInputMonitorEffectsActive(changroup);
        pvf->monfxButton->setToggleState(inmonfxon, dontSendNotification);


        if (!pvf->levelSlider->isMouseOverOrDragging()) {
            pvf->levelSlider->setValue(processor.getInputGroupGain(changroup), dontSendNotification);
        }


        if (!pvf->monitorSlider->isMouseOverOrDragging()) {
            pvf->monitorSlider->setValue(processor.getInputMonitor(changroup), dontSendNotification);
        }


        pvf->meter->setMeterSource (&processor.getPostInputMeterSource());
        pvf->meter->setSelectedChannel(i);
        pvf->meter->setFixedNumChannels(1);

        pvf->premeter->setMeterSource (&processor.getInputMeterSource());
        pvf->premeter->setSelectedChannel(chstart + chi);
        pvf->premeter->setFixedNumChannels(1);


        int deststart = 0;
        int destcnt = 2;
        processor.getInputGroupChannelDestStartAndCount(changroup, deststart, destcnt);
        destcnt = jmin(totaloutchans, destcnt);

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

        } else {
            pvf->panLabel->setVisible(true);
            pvf->panSlider->setDoubleClickReturnValue(true, (chi & 2) ? 1.0f : -1.0f); // double click defaults to alternating left/right
            pvf->panSlider->setVisible(true);

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
        pvf->destButton->setVisible(destbuttvisible);
        pvf->monitorSlider->setVisible(isprimary);
        pvf->monfxButton->setVisible(isprimary);
        pvf->nameEditor->setVisible(isprimary);
        pvf->nameLabel->setVisible(false);


        // effects aren't used if channel count is above 2, right now
        pvf->fxButton->setVisible(isprimary && chcnt <= 2);

        pvf->linkButton->setVisible(isprimary);
        pvf->monoButton->setVisible(false);

        pvf->repaint();
    }

    updateMonDelayButton();

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
    int totaloutchans = processor.getTotalNumOutputChannels();

    int chstart = 0;
    int chcnt = 0;
    processor.getRemotePeerChannelGroupStartAndCount(mPeerIndex, changroup, chstart, chcnt);

    int deststart = 0;
    int destcnt = 2;
    processor.getRemotePeerChannelGroupDestStartAndCount(mPeerIndex, changroup, deststart, destcnt);
    destcnt = jmin(totaloutchans, destcnt);

    bool expanded = processor.getRemotePeerViewExpanded(mPeerIndex);

    // deal with main strip

    bool connected = processor.getRemotePeerConnected(mPeerIndex);
    String username = processor.getRemotePeerUserName(mPeerIndex);

    mMainChannelView->nameLabel->setText(username, dontSendNotification);
    if (username.length() > 32) {
        mMainChannelView->nameLabel->setTooltip(username);
    } else {
        mMainChannelView->nameLabel->setTooltip("");
    }

    String chantext;
    chantext << totalchans << TRANS("ch");
    mMainChannelView->linkButton->setButtonText(chantext);

    bool safetymuted = processor.getRemotePeerSafetyMuted(mPeerIndex);
    bool recvactive = processor.getRemotePeerRecvActive(mPeerIndex);
    bool recvallow = processor.getRemotePeerRecvAllow(mPeerIndex);

    bool mainanysoloed = processor.isAnythingSoloed();
    bool mainsoloed = processor.getRemotePeerSoloed(mPeerIndex);
    bool mainmuted = !processor.getRemotePeerRecvAllow(mPeerIndex);
    if (safetymuted || (mainanysoloed && !mainsoloed && !mainmuted)) {
        mMainChannelView->muteButton->setColour(TextButton::buttonColourId, mutedBySoloColor);
    } else {
        mMainChannelView->muteButton->removeColour(TextButton::buttonColourId);
    }

    if (chi == 0 && totalchans==1) {
        mMainChannelView->monoButton->setVisible(true);
        mMainChannelView->linkButton->setVisible(false);
    } else
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

    if (expanded || changroups > 1 || (destcnt != 2)) {
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
        auto pan1 = processor.getRemotePeerChannelPan(mPeerIndex, changroup, 0);
        auto pan2 = processor.getRemotePeerChannelPan(mPeerIndex, changroup, 1);
        if (pan1 != mMainChannelView->panSlider->getMinValue() || pan2 != mMainChannelView->panSlider->getMaxValue()) {
            mMainChannelView->panSlider->setMinAndMaxValues(pan1, pan2, dontSendNotification);
        }
    }
    else {
        auto pan = processor.getRemotePeerChannelPan(mPeerIndex, changroup, chi);
        if (pan != mMainChannelView->panSlider->getValue()) {
            mMainChannelView->panSlider->setValue(pan, dontSendNotification);
        }
    }

    bool maindestbuttvisible = !expanded && changroups == 1 /*&& chcnt < totaloutchans */;
    mMainChannelView->destButton->setVisible(maindestbuttvisible);

    if (maindestbuttvisible) {
        String desttext;
        if (destcnt == 1) {
            desttext << deststart + 1;
        } else {
            desttext << deststart + 1 << "-" << deststart+destcnt;
        }
        mMainChannelView->destButton->setButtonText(desttext);
    }

    mMainChannelView->monitorSlider->setVisible(false);

    float disalpha = 0.4;
    mMainChannelView->nameLabel->setAlpha(connected ? 1.0 : 0.8);
    mMainChannelView->levelSlider->setAlpha((recvactive && !safetymuted) ? 1.0 : disalpha);

    // effects aren't used if channel count is above 2, right now
    mMainChannelView->fxButton->setVisible(!expanded && changroups == 1 && chcnt <= 2);
    bool infxon = processor.getRemotePeerEffectsActive(mPeerIndex, changroup);
    mMainChannelView->fxButton->setToggleState(infxon, dontSendNotification);

    mMainChannelView->linkButton->setToggleState(expanded, dontSendNotification);

    mMainChannelView->monfxButton->setVisible(false);
    mMainChannelView->nameEditor->setVisible(false);

    if (!expanded) {
        // hide all the subchannel views
        for (int i=0; i < mChannelViews.size(); ++i) {
            if (specific >= 0 && specific != i) continue;
            ChannelGroupView * pvf = mChannelViews.getUnchecked(i);
            pvf->setVisible(false);
        }
    }


    for (int i=0; expanded && i < mChannelViews.size(); ++i, ++chi) {
        if (specific >= 0 && specific != i) continue;


        if (chi >= chcnt && changroup < changroups-1) {
            changroup++;
            chstart += chcnt;

            processor.getRemotePeerChannelGroupStartAndCount(mPeerIndex, changroup, chstart, chcnt);
            chi = 0;
        }


        ChannelGroupView * pvf = mChannelViews.getUnchecked(i);
        pvf->setVisible(true);
        pvf->group = changroup;
        pvf->groupChanCount = chcnt;
        pvf->chanIndex = chi;

        String name = processor.getRemotePeerChannelGroupName (mPeerIndex, changroup);

        //DBG("Got username: '" << username << "'");
        String dispname = name ; //(i==0) ? username : "";
        //if (name.isNotEmpty()) {
        //    dispname << " | " << name;
        //}

        pvf->nameLabel->setText(dispname, dontSendNotification);

        //pvf->chanLabel->setText(String::formatted("%d", i+1), dontSendNotification);

        String chantext;
        if (processor.getLayoutFormatChangedForRemotePeer(mPeerIndex)) {
            chantext << "*";
        }
        chantext << chstart+chi + 1; //<< "-" << chstart+chcnt;
        pvf->linkButton->setButtonText(chantext);


        int chstart=0, chcnt=0;
        processor.getRemotePeerChannelGroupStartAndCount(mPeerIndex, changroup, chstart, chcnt);

        bool anysoloed = processor.getRemotePeerChannelSoloed(mPeerIndex, -1);
        bool aresoloed = processor.getRemotePeerChannelSoloed(mPeerIndex, changroup);
        bool aremuted = !processor.getRemotePeerRecvAllow(mPeerIndex) || processor.getRemotePeerChannelMuted(mPeerIndex, changroup);

        if (safetymuted || (anysoloed && !aresoloed && !aremuted) || (mainanysoloed && !mainsoloed && !mainmuted)) {
            pvf->muteButton->setColour(TextButton::buttonColourId, mutedBySoloColor);
        } else {
            pvf->muteButton->removeColour(TextButton::buttonColourId);
        }

        pvf->showDivider = chi == 0; //  && i != 0 ; //&& i < mChannelViews.size()-1; // || i == 0;


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
        destcnt = jmin(totaloutchans, destcnt);

        String desttext;
        if (destcnt == 1) {
            desttext << deststart + 1;
        } else {
            desttext << deststart + 1 << "-" << deststart+destcnt;
        }
        pvf->destButton->setButtonText(desttext);

        if (destcnt != 2) {
            pvf->panSlider->setVisible(false);
            pvf->panLabel->setVisible(false);
        }
        else {
            pvf->panLabel->setVisible(true);
            pvf->panSlider->setVisible(true);
            pvf->panSlider->setDoubleClickReturnValue(true, 0.0);

            if (!pvf->singlePanner) {
                pvf->panSlider->setSliderStyle(Slider::LinearHorizontal); // LinearBar
                pvf->panSlider->setTextBoxStyle(Slider::NoTextBox, true, 10, 2); // TextBoxAbove

                pvf->singlePanner = true;
            }
                        
        }

        if (pvf->panSlider->isTwoValue()) {
            pvf->panSlider->setMinAndMaxValues(processor.getRemotePeerChannelPan(mPeerIndex, changroup, 0), processor.getRemotePeerChannelPan(mPeerIndex, changroup, 1), dontSendNotification);
        }
        else {        
            pvf->panSlider->setValue(processor.getRemotePeerChannelPan(mPeerIndex, changroup, chi), dontSendNotification);
        }

        bool isprimary = chi == 0;
        bool destbuttvisible = isprimary /*&& chcnt < totaloutchans */;
        pvf->levelSlider->setVisible(isprimary);
        pvf->soloButton->setVisible(isprimary);
        pvf->muteButton->setVisible(isprimary);
        pvf->nameLabel->setVisible(isprimary);
        pvf->destButton->setVisible(destbuttvisible);
        pvf->monitorSlider->setVisible(false);
        pvf->monfxButton->setVisible(false);
        pvf->nameEditor->setVisible(false);

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

}

void ChannelGroupsView::updateMonDelayButton()
{

    DelayParams metparams;
    processor.getMetronomeMonitorDelayParams(metparams);

    DelayParams fileparams;
    processor.getFilePlaybackMonitorDelayParams(fileparams);

    DelayParams sbparams = processor.getSoundboardProcessor()->getMonitorDelayParams();

    DelayParams eparam;

    // if any are on, turn them off... otherwise turn them all on
    bool anyenabled = metparams.enabled || fileparams.enabled || sbparams.enabled;

    if (!anyenabled) {
        int numgroups = processor.getInputGroupCount();
        for (int i=0; i < numgroups; ++i) {
            processor.getInputMonitorDelayParams(i, eparam);
            if (eparam.enabled) {
                anyenabled = true;
                break;
            }
        }
    }

    mMonDelayButton->setToggleState(anyenabled, dontSendNotification);
}

void ChannelGroupsView::toggleAllMonitorDelay()
{
    int numgroups = processor.getInputGroupCount();

    DelayParams metparams;
    processor.getMetronomeMonitorDelayParams(metparams);

    DelayParams fileparams;
    processor.getFilePlaybackMonitorDelayParams(fileparams);

    DelayParams sbparams = processor.getSoundboardProcessor()->getMonitorDelayParams();

    DelayParams eparam;

    // if any are on, turn them off... otherwise turn them all on
    bool doDisable = metparams.enabled || fileparams.enabled || sbparams.enabled;

    if (!doDisable) {
        for (int i=0; i < numgroups; ++i) {
            processor.getInputMonitorDelayParams(i, eparam);
            if (eparam.enabled) {
                doDisable = true;
                break;
            }
        }
    }

    // set them all
    metparams.enabled = !doDisable;
    processor.setMetronomeMonitorDelayParams(metparams);

    fileparams.enabled = !doDisable;
    processor.setFilePlaybackMonitorDelayParams(fileparams);

    sbparams.enabled = !doDisable;
    processor.getSoundboardProcessor()->setMonitorDelayParams(sbparams);


    for (int i=0; i < numgroups; ++i) {
        processor.getInputMonitorDelayParams(i, eparam);
        eparam.enabled = !doDisable;
        processor.setInputMonitorDelayParams(i, eparam);
    }

    updateChannelViews();
}


void ChannelGroupsView::choiceButtonSelected(SonoChoiceButton *comp, int index, int ident)
{
    for (int i=0; i < mChannelViews.size(); ++i) {
        ChannelGroupView * pvf = mChannelViews.getUnchecked(i);

    }
}

void ChannelGroupsView::buttonClicked (Button* buttonThatWasClicked)
{

    if (mPeerMode) {
        int changroups = processor.getRemotePeerChannelGroupCount(mPeerIndex);

        for (int i=0; i < mChannelViews.size(); ++i) {

            ChannelGroupView * pvf = mChannelViews.getUnchecked(i);
            int changroup = pvf->group;

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

                showChangePeerChannelsLayout(i, buttonThatWasClicked);

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
            else if (pvf->monfxButton.get() == buttonThatWasClicked) {

                if (!monEffectsCalloutBox) {
                    showMonitorEffects(changroup, true, pvf->monfxButton.get());
                } else {
                    showMonitorEffects(changroup, false);
                }

                break;
            }
        }

    }
    else {
        int changroups = processor.getInputGroupCount();

        for (int i=0; i < mChannelViews.size(); ++i)
        {
            ChannelGroupView * pvf = mChannelViews.getUnchecked(i);
            int changroup = pvf->group;

            if (pvf->muteButton.get() == buttonThatWasClicked) {
                processor.setInputGroupMuted(changroup, buttonThatWasClicked->getToggleState());
                processor.getOSCManager().sendMessage("/InputGroup" + String(changroup + 1) + "Mute", buttonThatWasClicked->getToggleState() ? 1.0f : 0.0f);
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
                        // Send OSC for each group
                        processor.getOSCManager().sendMessage("/InputGroup" + String(j + 1) + "Solo", (newsolo && changroup == j) ? 1.0f : 0.0f);
                    }

                    // change solo for main monitor too
                    //processor.getValueTreeState().getParameter(SonobusAudioProcessor::paramMainMonitorSolo)->setValueNotifyingHost(newsolo ? 1.0f : 0.0);

                    updateChannelViews();
                } else {
                    bool newsolo = buttonThatWasClicked->getToggleState();
                    processor.setInputGroupSoloed (changroup, newsolo);
                    processor.getOSCManager().sendMessage("/InputGroup" + String(changroup + 1) + "Solo", newsolo ? 1.0f : 0.0f);

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
            else if (pvf->monfxButton.get() == buttonThatWasClicked) {

                if (!monEffectsCalloutBox) {
                    showMonitorEffects(changroup, true, pvf->monfxButton.get());
                } else {
                    showMonitorEffects(changroup, false);
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
        safeThis->processor.updateRemotePeerUserFormat();

        safeThis->rebuildChannelViews(true);
    };

    GenericItemChooser::launchPopupChooser(items, bounds, dw, callback, -1, dw ? dw->getHeight()-30 : 0);
}

void ChannelGroupsView::addGroupPressed()
{
    if (mPeerMode) return; // not used for it

    Array<GenericItemChooserItem> items;

    int totalouts = processor.getTotalNumOutputChannels();
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
        if (!safeThis || index == 0) return;
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
            safeThis->processor.setInputGroupName(insAt, ""); // clear the name
            safeThis->processor.updateRemotePeerUserFormat();
            safeThis->rebuildChannelViews(true);
        }
    };

    GenericItemChooser::launchPopupChooser(items, bounds, dw, callback, -1, dw ? dw->getHeight()-30 : 0);
}

void ChannelGroupsView::showChangeGroupChannels(int changroup, Component * showfrom)
{
    if (mPeerMode) return; // not used for it

    Array<GenericItemChooserItem> items;

    int totalouts = processor.getTotalNumOutputChannels();
    int totalins = processor.getTotalNumInputChannels();

    totalins = jmin(totalins, MAX_CHANNELS);

    items.add(GenericItemChooserItem(TRANS("CHANGE CHANNEL LAYOUT:"), {}, nullptr, false, true));


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
    SafePointer<Component> safeSource(showfrom);

    auto callback = [safeThis,changroup,totalins,totalouts,safeSource](GenericItemChooser* chooser,int index) mutable {
        if (!safeThis || index == 0) return;
        int chstart = 0;
        int chcnt;
        safeThis->processor.getInputGroupChannelStartAndCount(changroup, chstart, chcnt);
        // just set the new channel count
        safeThis->processor.setInputGroupChannelStartAndCount(changroup, chstart, index);
        safeThis->processor.updateRemotePeerUserFormat();
        safeThis->rebuildChannelViews(true);

        Timer::callAfterDelay(100, [safeSource](){
            if (safeSource) {
                safeSource->grabKeyboardFocus();
            }
        });
    };

    GenericItemChooser::launchPopupChooser(items, bounds, dw, callback, -1, dw ? dw->getHeight()-30 : 0);
}

void ChannelGroupsView::showChangePeerChannelsLayout(int chindex, Component * showfrom)
{
    if (!mPeerMode) return; // not used for it

    Array<GenericItemChooserItem> items;

    int totalins = processor.getRemotePeerRecvChannelCount(mPeerIndex);

    totalins = jmin(totalins, MAX_CHANNELS);

    int changroup = getChanGroupFromIndex(chindex);

    int chstart = 0;
    int chcnt = 0;
    processor.getRemotePeerChannelGroupStartAndCount(mPeerIndex, changroup, chstart, chcnt);

    int usableins = totalins - (chindex);
    int selindex = chindex == chstart ? chcnt : -1;

    items.add(GenericItemChooserItem(TRANS("CHANGE CHANNEL LAYOUT:"), {}, nullptr, false, true));

    for (int i=0; i < usableins; ++i) {
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


    if (processor.getLayoutFormatChangedForRemotePeer(mPeerIndex)) {
        auto udata = std::make_shared<CmdListItemData>(1);
        items.add(GenericItemChooserItem(TRANS("<Restore Original Layout>"), {}, udata, true));
    }


    Component* dw = showfrom->findParentComponentOfClass<AudioProcessorEditor>();
    if (!dw) dw = showfrom->findParentComponentOfClass<Component>();
    Rectangle<int> bounds =  dw->getLocalArea(nullptr, showfrom->getScreenBounds());

    SafePointer<ChannelGroupsView> safeThis(this);
    SafePointer<Component> safeSource(showfrom);

    auto callback = [safeThis,changroup,chindex,totalins,safeSource](GenericItemChooser* chooser,int index) mutable {
        if (!safeThis || index == 0) return;
        // jlcc
        auto & selitem = chooser->getItems().getReference(index);
        auto dclitem = std::dynamic_pointer_cast<CmdListItemData>(selitem.userdata);
        if (dclitem) {
            // this is restore
            safeThis->processor.restoreLayoutFormatForRemotePeer(safeThis->mPeerIndex);
            safeThis->rebuildChannelViews(true);
            return;
        }

        int newcount = index;
        int chstart = 0;
        int chcnt;
        safeThis->processor.getRemotePeerChannelGroupStartAndCount(safeThis->mPeerIndex, changroup, chstart, chcnt);
        int changroups = safeThis->processor.getRemotePeerChannelGroupCount(safeThis->mPeerIndex);

        // set the new channel count for this group
        if (chindex == chstart) {
            // this was the first in the group, just adjust the count
            chcnt = newcount;
            safeThis->processor.setRemotePeerChannelGroupStartAndCount(safeThis->mPeerIndex, changroup, chstart, chcnt);
        }
        else {
            // need to split this group, by inserting a new one
            int ncnt = jmax(1, chindex - chstart);
            safeThis->processor.setRemotePeerChannelGroupStartAndCount(safeThis->mPeerIndex, changroup, chstart, ncnt);
            chcnt = newcount;
            changroup++;
            safeThis->processor.insertRemotePeerChannelGroup(safeThis->mPeerIndex, changroup, chindex, chcnt);
            changroups++;
        }

        // now we need to possibly delete the following groups that were within the range, or insert a new one
        int endch = chindex + chcnt;

        if (changroup == changroups-1) {
            if (endch < totalins) {
                // this was the last one, possibly insert one
                int nlen = totalins - endch;
                safeThis->processor.insertRemotePeerChannelGroup(safeThis->mPeerIndex, changroup+1, endch, nlen);
                changroups++;
            }
        }
        else {
            for (int cgi=changroup+1; cgi < changroups; ++cgi) {
                int nchstart=0, nchcnt=0;
                safeThis->processor.getRemotePeerChannelGroupStartAndCount(safeThis->mPeerIndex, cgi, nchstart, nchcnt);
                if (nchstart < endch) {
                    // the new one overlaps
                    if (nchstart + nchcnt <= endch) {
                        // fully subsumed, remove it
                        safeThis->processor.removeRemotePeerChannelGroup(safeThis->mPeerIndex, cgi);
                        changroups--;
                        cgi--;
                    }
                    else {
                        // partially subsumed, adjust it and done
                        int adjstart = endch;
                        int adjlen = (nchstart + nchcnt) - adjstart;
                        safeThis->processor.setRemotePeerChannelGroupStartAndCount(safeThis->mPeerIndex, cgi, adjstart, adjlen);

                        break;
                    }
                } else if (endch < nchstart) {
                    // the new one leaves a gap, insert one to fill it
                    int nlen = nchstart - endch;
                    safeThis->processor.insertRemotePeerChannelGroup(safeThis->mPeerIndex, cgi, endch, nlen);
                    changroups++;

                    break;
                }
            }
        }

        safeThis->processor.setRemotePeerChannelGroupCount(safeThis->mPeerIndex, changroups);


        safeThis->rebuildChannelViews(true);

        Timer::callAfterDelay(100, [safeSource](){
            if (safeSource) {
                safeSource->grabKeyboardFocus();
            }
        });
    };

    GenericItemChooser::launchPopupChooser(items, bounds, dw, callback, selindex, dw ? dw->getHeight()-30 : 0);
}

int ChannelGroupsView::getChanGroupFromIndex(int index)
{
    if (index >= 0 && index < mChannelViews.size()) {
        return mChannelViews.getUnchecked(index)->group;
    }

    return 0;
}

int ChannelGroupsView::getChanGroupForPoint(Point<int> pos, bool inbetween)
{
    int i=0;
    for (; i < mChanGroupBounds.size(); ++i) {
        auto bounds = mChanGroupBounds.getUnchecked(i);

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

Rectangle<int> ChannelGroupsView::getBoundsForChanGroup(int chgroup)
{
    if (chgroup >= 0 && chgroup < mChanGroupBounds.size()) {
        return mChanGroupBounds.getUnchecked(chgroup);
    }
    // otherwise return a line after the last of them
    if (!mChanGroupBounds.isEmpty()) {
        auto lastone = mChanGroupBounds.getLast();
        return Rectangle<int>(lastone.getX(), lastone.getBottom(), lastone.getWidth(), 0);
    }
    return {};
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

    items.add(GenericItemChooserItem(chcnt > 1 ? TRANS("SELECT INPUTS:") : TRANS("SELECT INPUT:"), {}, nullptr, false, true));


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

            if (cc == 1) {
                if (mPeerMode) {
                    name << i+1;
                } else {
                    if (i < inputnames.size()) {
                        name << "[" << i+1 << "] " << inputnames[i];
                    }
                    else {
                        name << i+1;
                    }
                }
            }
            else if (cc > 1) {

                if (mPeerMode) {
                    name << i+1;
                } else {
                    if (i+cc-1 < inputnames.size()) {
                        name << "[" << i+1 << "-" << i+cc << "] " << inputnames[i] << " - " << inputnames[i+cc-1];
                    }
                    else {
                        name << i+1 << " - " << i+cc;
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
    SafePointer<Component> safeSource(source);

    auto callback = [safeThis,changroup,inputnames, safeSource](GenericItemChooser* chooser,int index) mutable {
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
                    safeThis->processor.updateRemotePeerUserFormat();
                }
            }

            safeThis->rebuildChannelViews(true);
            return;
        }
        else if (index == items.size()-2 && safeSource) {
            // second to last is change size
            safeThis->showChangeGroupChannels(changroup, safeSource);
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
            safeThis->processor.updateRemotePeerUserFormat();
        }

        safeThis->updateChannelViews();
        safeThis->updateLayout();
        safeThis->resized();

        Timer::callAfterDelay(100, [safeSource](){
            if (safeSource) {
                safeSource->grabKeyboardFocus();
            }
        });
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
        mEffectsView->grabKeyboardFocus();
    }
    else {
        // dismiss it
        if (CallOutBox * box = dynamic_cast<CallOutBox*>(effectsCalloutBox.get())) {
            box->dismiss();
            effectsCalloutBox = nullptr;
        }
    }
}

void ChannelGroupsView::showMonitorEffects(int index, bool flag, Component * fromView)
{
    if (flag && monEffectsCalloutBox == nullptr) {

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

        if (!mMonEffectsView) {
            mMonEffectsView = std::make_unique<ChannelGroupMonitorEffectsView>(processor, mPeerMode);
            mMonEffectsView->addListener(this);
        }

        mMonEffectsView->peerMode = mPeerMode;
        mMonEffectsView->peerIndex = mPeerIndex;
        mMonEffectsView->groupIndex = index;

        auto minbounds = mMonEffectsView->getMinimumContentBounds();
        defWidth = minbounds.getWidth();
        defHeight = minbounds.getHeight();


        int extrawidth = 0;
        if (defHeight > dw->getHeight() - 24) {
            extrawidth = wrap->getScrollBarThickness() + 1;
        }

        wrap->setSize(jmin(defWidth + extrawidth, dw->getWidth() - 10), jmin(defHeight, dw->getHeight() - 24));


        mMonEffectsView->updateLayout();

        mMonEffectsView->setBounds(Rectangle<int>(0,0,defWidth,defHeight));

        mMonEffectsView->updateState();

        wrap->setViewedComponent(mMonEffectsView.get(), false);
        mMonEffectsView->setVisible(true);

        Rectangle<int> bounds =  dw->getLocalArea(nullptr, fromView->getScreenBounds());
        DBG("effect callout bounds: " << bounds.toString());
        monEffectsCalloutBox = & CallOutBox::launchAsynchronously (std::move(wrap), bounds , dw, false);
        if (CallOutBox * box = dynamic_cast<CallOutBox*>(monEffectsCalloutBox.get())) {
            box->setDismissalMouseClicksAreAlwaysConsumed(true);
        }

        mMonEffectsView->setFocusContainerType(FocusContainerType::keyboardFocusContainer);

        mMonEffectsView->grabKeyboardFocus();

    }
    else {
        // dismiss it
        if (CallOutBox * box = dynamic_cast<CallOutBox*>(monEffectsCalloutBox.get())) {
            box->dismiss();
            monEffectsCalloutBox = nullptr;
        }
    }
}

void ChannelGroupsView::showInputReverbView(bool flag, Component * fromView)
{
    if (flag && inReverbCalloutBox == nullptr) {

        if (!fromView) {
            fromView = mInReverbButton.get();
        }

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

        if (!mInputReverbView) {
            mInputReverbView = std::make_unique<ChannelGroupReverbEffectsView>(processor);
            //mInputReverbView->addListener(this);
        }


        auto minbounds = mInputReverbView->getMinimumContentBounds();
        defWidth = minbounds.getWidth();
        defHeight = minbounds.getHeight();


        int extrawidth = 0;
        if (defHeight > dw->getHeight() - 24) {
            extrawidth = wrap->getScrollBarThickness() + 1;
        }

        wrap->setSize(jmin(defWidth + extrawidth, dw->getWidth() - 10), jmin(defHeight, dw->getHeight() - 24));


        mInputReverbView->updateLayout();

        mInputReverbView->setBounds(Rectangle<int>(0,0,defWidth,defHeight));

        mInputReverbView->updateState();

        wrap->setViewedComponent(mInputReverbView.get(), false);
        mInputReverbView->setVisible(true);

        Rectangle<int> bounds =  dw->getLocalArea(nullptr, fromView->getScreenBounds());
        DBG("in reverb callout bounds: " << bounds.toString());
        inReverbCalloutBox = & CallOutBox::launchAsynchronously (std::move(wrap), bounds , dw, false);
        if (CallOutBox * box = dynamic_cast<CallOutBox*>(inReverbCalloutBox.get())) {
            box->setDismissalMouseClicksAreAlwaysConsumed(true);
        }
        inReverbCalloutBox->setFocusContainerType(FocusContainerType::keyboardFocusContainer);
        mInputReverbView->grabKeyboardFocus();

    }
    else {
        // dismiss it
        if (CallOutBox * box = dynamic_cast<CallOutBox*>(inReverbCalloutBox.get())) {
            box->dismiss();
            inReverbCalloutBox = nullptr;
        }
    }
}

bool ChannelGroupsView::isDraggable(Component * comp) const
{
    if (comp == this || comp == mMainChannelView.get() || comp == mMainChannelView->nameLabel.get() || comp == mMainChannelView->linkButton.get()) {
        return true;
    }
    return false;
}


void ChannelGroupsView::mouseDown (const MouseEvent& event)
{
    if (mMainChannelView) {
        if (event.eventComponent == mMainChannelView->meter.get()) {
            clearClipIndicators();
            return;
        }
    }

    if (mMetChannelView) {
        if (event.eventComponent == mMetChannelView->meter.get()) {
            clearClipIndicators();
            return;
        }
    }
    if (mFileChannelView) {
        if (event.eventComponent == mFileChannelView->meter.get()) {
            clearClipIndicators();
            return;
        }
    }
    if (mSoundboardChannelView) {
        if (event.eventComponent == mSoundboardChannelView->meter.get()) {
            clearClipIndicators();
            return;
        }
    }

    for (int i=0; i < mChannelViews.size(); ++i) {
        ChannelGroupView * pvf = mChannelViews.getUnchecked(i);

        if (event.eventComponent == pvf->linkButton.get()) {
            mDraggingSourceGroup = pvf->group;
            break;
        }
        else if (event.eventComponent == pvf->meter.get() || (event.eventComponent == pvf->premeter.get())) {
            clearClipIndicators();
            break;
        }
    }
}

void ChannelGroupsView::mouseDrag (const MouseEvent& event)
{
    for (int i=0; i < mChannelViews.size(); ++i) {
        ChannelGroupView * pvf = mChannelViews.getUnchecked(i);

        if (!mPeerMode &&
            (event.eventComponent == pvf->linkButton.get()
            || event.eventComponent == pvf->nameLabel.get()
            || event.eventComponent == pvf->nameEditor.get())) {
            auto adjpos =  getLocalPoint(event.eventComponent, event.getPosition());
            DBG("Dragging link button: " << adjpos.toString());
            if (abs(event.getDistanceFromDragStartY()) > 4 && !mDraggingActive) {
                // start drag behavior
                mDraggingSourceGroup = pvf->group;
                mDraggingActive = true;
                mDraggingGroupPos = getChanGroupForPoint(adjpos, true);
                auto groupbounds = getBoundsForChanGroup(mDraggingSourceGroup);
                mDragImage = createComponentSnapshot(groupbounds);
                mDragDrawable->setImage(mDragImage);
                mDragDrawable->setVisible(true);
                mDragDrawable->setBounds(groupbounds.getX(), adjpos.getY() - groupbounds.getHeight()/2, groupbounds.getWidth(), groupbounds.getHeight());
            }
            else if (mDraggingActive) {
                // adjust drag indicator
                int changroup = getChanGroupForPoint(adjpos, true);
                DBG("In changroup: " << changroup);

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

                if (changroup != mDraggingGroupPos) {
                    // insert point changed, update it
                    mDraggingGroupPos = changroup;

                    auto groupbounds = getBoundsForChanGroup(mDraggingGroupPos);
                    groupbounds.setHeight(0);
                    groupbounds.setWidth(getWidth() - 16);
                    groupbounds.setX(7);
                    mInsertLine->setRectangle (groupbounds.toFloat());

                    int delta = mDraggingGroupPos - mDraggingSourceGroup;
                    bool canmove = delta > 1 || delta < 0;
                    mInsertLine->setVisible(canmove);
                }
            }
            break;
        }
    }
}

void ChannelGroupsView::mouseUp (const MouseEvent& event)
{
    if (mMainChannelView) {
        if (event.eventComponent == mMainChannelView->nameLabel.get()) {
            listeners.call (&ChannelGroupsView::Listener::nameLabelClicked, this);
            return;
        }
    }

    for (int i=0; i < mChannelViews.size(); ++i) {
        ChannelGroupView * pvf = mChannelViews.getUnchecked(i);

        if (event.eventComponent == pvf->linkButton.get()
            || event.eventComponent == pvf->nameLabel.get()
            || event.eventComponent == pvf->nameEditor.get()) {
            if (mDraggingActive) {
                DBG("Mouse up after drag: " << event.getPosition().toString());
                // commit it
                int delta = mDraggingGroupPos - mDraggingSourceGroup;
                bool canmove = delta > 1 || delta < 0;

                if (canmove && processor.moveInputChannelGroupTo(mDraggingSourceGroup, mDraggingGroupPos)) {
                    // moved it
                    processor.updateRemotePeerUserFormat();
                    rebuildChannelViews();
                }

                mInsertLine->setVisible(false);
                mDragDrawable->setVisible(false);
                mDraggingActive = false;
                mAutoscrolling = false;
            }

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
        if (pvf->premeter) {
            pvf->premeter->clearClipIndicator(-1);
            pvf->premeter->clearMaxLevelDisplay(-1);
        }
        pvf->meter->clearClipIndicator(-1);
        pvf->meter->clearMaxLevelDisplay(-1);
    }

    if (mMainChannelView) {
        mMainChannelView->meter->clearClipIndicator();
        mMainChannelView->meter->clearMaxLevelDisplay(-1);
    }

    if (mMetChannelView) {
        mMetChannelView->premeter->clearClipIndicator();
        mMetChannelView->premeter->clearMaxLevelDisplay(-1);
        mMetChannelView->meter->clearClipIndicator();
        mMetChannelView->meter->clearMaxLevelDisplay(-1);
    }

    if (mFileChannelView) {
        mFileChannelView->premeter->clearClipIndicator();
        mFileChannelView->premeter->clearMaxLevelDisplay(-1);
        mFileChannelView->meter->clearClipIndicator();
        mFileChannelView->meter->clearMaxLevelDisplay(-1);
    }

    if (mSoundboardChannelView) {
        mSoundboardChannelView->premeter->clearClipIndicator();
        mSoundboardChannelView->premeter->clearMaxLevelDisplay(-1);
        mSoundboardChannelView->meter->clearClipIndicator();
        mSoundboardChannelView->meter->clearMaxLevelDisplay(-1);
    }

}


void ChannelGroupsView::showDestSelectionMenu(Component * source, int index)
{
    if (index >= mChannelViews.size()) return;

    ChannelGroupView * pvf = nullptr;
    bool ismet = false, isfile = false, issoundboard = false;

    if (index == -1) {
        pvf = mMetChannelView.get();
        ismet = true;
    } else if (index == -2) {
        pvf = mFileChannelView.get();
        isfile = true;
    } else if (index == -3) {
        pvf = mSoundboardChannelView.get();
        issoundboard = true;
    } else {
        pvf = mChannelViews.getUnchecked(index);
    }

    Array<GenericItemChooserItem> items;
    items.add(GenericItemChooserItem(TRANS("SELECT MONITOR OUT:"), {}, nullptr, false, true));

    int chstart=0, chcnt=0;
    int totalouts = 0;
    int destst=0, destcnt=0;

    int changroup =  getChanGroupFromIndex(index);
    int maxchcnt = 0;

    if (mPeerMode) {
        totalouts = processor.getTotalNumOutputChannels();
        processor.getRemotePeerChannelGroupStartAndCount(mPeerIndex, changroup, chstart, chcnt);
        processor.getRemotePeerChannelGroupDestStartAndCount(mPeerIndex, changroup, destst, destcnt);
        chcnt = jmin(chcnt, totalouts);
        maxchcnt = chcnt;
    }
    else {
        // input mode
        totalouts = processor.getTotalNumOutputChannels();
        if (ismet) {
            chcnt = maxchcnt = 1;
            processor.getMetronomeChannelDestStartAndCount(destst, destcnt);
        }
        else if (isfile) {
            processor.getFilePlaybackDestStartAndCount(destst, destcnt);
            chcnt = processor.getFilePlaybackMeterSource().getNumChannels();
            maxchcnt = chcnt;
            chcnt = jmin(2, chcnt, totalouts);
        }
        else if (issoundboard) {
            processor.getSoundboardProcessor()->getDestStartAndCount(destst, destcnt);
            chcnt = processor.getSoundboardProcessor()->getMeterSource().getNumChannels();
            maxchcnt = chcnt;
            chcnt = jmin(2, chcnt, totalouts);
        }
        else {
            processor.getInputGroupChannelStartAndCount(changroup, chstart, chcnt);
            processor.getInputGroupChannelDestStartAndCount(changroup, destst, destcnt);
            maxchcnt = chcnt = jmin(chcnt, totalouts);
        }
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
                    //name << "[" << ind + 1 << "] " << allnames[ni];
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
                //name << "[" << i+1 << "] " << chname << " " << bch+1;
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
    for (int cc=chcnt; cc <= jmin( jmax(2, maxchcnt), totalouts); ++cc) {
        for (int i=0; i < totalouts - (cc - 1); ++i) {
            String name;
            if (cc == 1) {
                if (i < outputnames.size()) {
                    name << "[" << i+1 << "] " << outputnames[i];
                    //name << outputnames[i];
                } else {
                    name << i+1;
                }
            } else {
                if (i+cc-1 < outputnames.size()) {
                    name << "[" << i+1 << "-" << i+cc <<  "] " << outputnames[i] << " - " << outputnames[i+cc-1];
                    //name << outputnames[i] << " - " << outputnames[i+cc-1];
                } else {
                    name << i+1 << " - " << i+cc;
                }
            }
            auto udata = std::make_shared<DestChannelListItemData>(i, cc);
            items.add(GenericItemChooserItem(name, Image(), udata, i==0));

            if (i == destst && cc == destcnt) {
                selindex = ind;
            }
            ++ind;
        }
    }

    SafePointer<ChannelGroupsView> safeThis(this);
    SafePointer<Component> safeSource(source);

    auto callback = [safeThis,safeSource,changroup,ismet,isfile,issoundboard](GenericItemChooser* chooser,int index) mutable {
        auto & items = chooser->getItems();
        auto & selitem = items.getReference(index);
        auto dclitem = std::dynamic_pointer_cast<DestChannelListItemData>(selitem.userdata);
        if (!dclitem) {
            DBG("Error getting user data");
            return;
        }

        // change src chan stuff

        if (ismet) {
            safeThis->processor.setMetronomeChannelDestStartAndCount(dclitem->startIndex, dclitem->count);
        }
        else if (isfile) {
            safeThis->processor.setFilePlaybackDestStartAndCount(dclitem->startIndex, dclitem->count);
        }
        else if (issoundboard) {
            safeThis->processor.getSoundboardProcessor()->setDestStartAndCount(dclitem->startIndex, dclitem->count);
        }
        else if (safeThis->mPeerMode) {
            safeThis->processor.setRemotePeerChannelGroupDestStartAndCount(safeThis->mPeerIndex, changroup, dclitem->startIndex, dclitem->count);
        }
        else {
            safeThis->processor.setInputGroupChannelDestStartAndCount(changroup, dclitem->startIndex, dclitem->count);
        }

        safeThis->updateChannelViews();
        safeThis->updateLayout();
        safeThis->resized();

        Timer::callAfterDelay(100, [safeSource](){
            if (safeSource) {
                safeSource->grabKeyboardFocus();
            }
        });
    };


    Component* dw = source->findParentComponentOfClass<AudioProcessorEditor>();
    if (!dw) dw = source->findParentComponentOfClass<Component>();

    Rectangle<int> bounds =  dw->getLocalArea(nullptr, source->getScreenBounds());
    
    GenericItemChooser::launchPopupChooser(items, bounds, dw, callback, selindex, dw ? dw->getHeight()-30 : 0);
}

void ChannelGroupsView::genericItemChooserSelected(GenericItemChooser *comp, int index)
{

}

void ChannelGroupsView::nameLabelChanged (int changroup, const String & name) {

    if (mPeerMode) {
        processor.setRemotePeerChannelGroupName(mPeerIndex, changroup, name);
    }
    else {
        if (processor.getInputGroupName(changroup) != name) {
            processor.setInputGroupName(changroup, name);
            processor.updateRemotePeerUserFormat();
        }
    }
}


void ChannelGroupsView::effectsEnableChanged(ChannelGroupEffectsView *comp)
{
    updateChannelViews();
}

void ChannelGroupsView::monitorEffectsEnableChanged(ChannelGroupMonitorEffectsView *comp)
{
    updateChannelViews();
}



void ChannelGroupsView::sliderValueChanged (Slider* slider)
{

    if (mPeerMode) {
        int changroup = 0;
        int chi = 0;

        if (slider == mMainChannelView->levelSlider.get()) {
            processor.setRemotePeerLevelGain(mPeerIndex, mMainChannelView->levelSlider->getValue());
            
            // Send OSC feedback for peer level slider (only for first 16 peers)
            if (processor.getOSCEnabled() && mPeerIndex < 16) {
                processor.getOSCManager().sendMessage("/Peer" + String(mPeerIndex + 1) + "Level", static_cast<float>(mMainChannelView->levelSlider->getValue()));
            }
            return;
        }
        else if (slider == mMainChannelView->panSlider.get()) {
            if (mMainChannelView->panSlider->isTwoValue()) {
                float pan1 = mMainChannelView->panSlider->getMinValue();
                float pan2 = mMainChannelView->panSlider->getMaxValue();
                processor.setRemotePeerChannelPan(mPeerIndex, changroup, 0, pan1);
                processor.setRemotePeerChannelPan(mPeerIndex, changroup, 1, pan2);
                
                // Send OSC feedback for pan (only for first 16 peers, using first channel value)
                if (processor.getOSCEnabled() && mPeerIndex < 16) {
                    processor.getOSCManager().sendMessage("/Peer" + String(mPeerIndex + 1) + "Pan", static_cast<float>(pan1));
                }
            }
            else {
                processor.setRemotePeerChannelPan(mPeerIndex, changroup, chi, mMainChannelView->panSlider->getValue());
                
                // Send OSC feedback for pan (only for first 16 peers)
                if (processor.getOSCEnabled() && mPeerIndex < 16) {
                    processor.getOSCManager().sendMessage("/Peer" + String(mPeerIndex + 1) + "Pan", static_cast<float>(mMainChannelView->panSlider->getValue()));
                }
            }
            return;
        }

        for (int i=0; i < mChannelViews.size(); ++i)
        {
            ChannelGroupView * pvf = mChannelViews.getUnchecked(i);
            changroup = pvf->group;
            chi = pvf->chanIndex;

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



        for (int i=0; i < mChannelViews.size(); ++i)
        {

            ChannelGroupView * pvf = mChannelViews.getUnchecked(i);
            int changroup = pvf->group;
            int chi = pvf->chanIndex;

            if (pvf->levelSlider.get() == slider) {
                processor.setInputGroupGain(changroup, pvf->levelSlider->getValue());
                // Send OSC message for Input Group Pre Level change
                if (processor.getOSCEnabled()) {
                    processor.getOSCManager().sendMessage("/InputGroup" + String(changroup + 1) + "PreLevel", static_cast<float>(pvf->levelSlider->getValue()));
                }
                break;
            }
            else if (pvf->monitorSlider.get() == slider) {
                processor.setInputMonitor(changroup, pvf->monitorSlider->getValue());
                // Send OSC message for Input Group Monitor change
                if (processor.getOSCEnabled()) {
                    processor.getOSCManager().sendMessage("/InputGroup" + String(changroup + 1) + "Monitor", static_cast<float>(pvf->monitorSlider->getValue()));
                }
                break;
            }
            else if (pvf->panSlider.get() == slider) {
                if (pvf->panSlider->isTwoValue()) {
                    float pan1 = pvf->panSlider->getMinValue();
                    float pan2 = pvf->panSlider->getMaxValue();
                    processor.setInputChannelPan(changroup, 0, pan1);
                    processor.setInputChannelPan(changroup, 1, pan2);
                    // Send OSC message for Input Group Pan (dual channel)
                    if (processor.getOSCEnabled()) {
                        processor.getOSCManager().sendMessage("/InputGroup" + String(changroup + 1) + "PanLeft", pan1);
                        processor.getOSCManager().sendMessage("/InputGroup" + String(changroup + 1) + "PanRight", pan2);
                    }
                }
                else {
                    processor.setInputChannelPan(changroup, chi, pvf->panSlider->getValue());
                    // Send OSC message for Input Group Pan (single channel)
                    if (processor.getOSCEnabled()) {
                        processor.getOSCManager().sendMessage("/InputGroup" + String(changroup + 1) + "Pan", static_cast<float>(pvf->panSlider->getValue()));
                    }
                }
                break;
            }

        }

    }

}
