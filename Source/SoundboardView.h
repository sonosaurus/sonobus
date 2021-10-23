// SPDX-License-Identifier: GPLv3-or-later WITH Appstore-exception
// Copyright (C) 2021 Jesse Chappell


#pragma once

#include <JuceHeader.h>

#include "SonobusPluginProcessor.h"

class SoundboardView : public Component
{
public:
    SoundboardView();

    void paint(Graphics&) override;
    void resized() override;

private:
    FlexBox mainBox;

    std::unique_ptr<Label> mTitleLabel;


    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SoundboardView)
};