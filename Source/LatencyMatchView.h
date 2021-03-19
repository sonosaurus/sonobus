// SPDX-License-Identifier: GPLv3-or-later
// Copyright (C) 2021 Jesse Chappell


#pragma once

#include <JuceHeader.h>

#include "SonobusPluginProcessor.h"
#include "SonoDrawableButton.h"
#include "SonoLookAndFeel.h"

class LatencyMatchView : public Component, public MultiTimer
{
public:
    LatencyMatchView(SonobusAudioProcessor& proc);
    ~LatencyMatchView();

    void paint (Graphics&) override;
    void resized() override;

    void startLatMatchProcess();
    void stopLatMatchProcess();
    
    void updatePeerSliders();

    void timerCallback(int timerid) override;

protected:
    SonoBigTextLookAndFeel sonoSliderLNF;

    Slider * createPeerLatSlider();
    Label * createPeerLabel();

    SonobusAudioProcessor& processor;


    std::unique_ptr<Viewport> mViewport;
    std::unique_ptr<Component> mRowsContainer;
    OwnedArray<Slider> mPeerSliders;
    OwnedArray<Label> mPeerLabels;

    std::unique_ptr<Slider> mMainSlider;
    std::unique_ptr<Label> mMainSliderLabel;
    std::unique_ptr<TextButton> mRequestMatchButton;

    std::unique_ptr<Label> mTitleLabel;
    std::unique_ptr<SonoDrawableButton> mCloseButton;


    FlexBox mainBox;
    FlexBox titleBox;
    FlexBox peerSlidersBox;
    FlexBox mainSliderBox;
    FlexBox matchButtBox;

    bool initialCheckDone = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LatencyMatchView)
};
