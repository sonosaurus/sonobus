// SPDX-License-Identifier: GPLv3-or-later WITH Appstore-exception
// Copyright (C) 2021 Jesse Chappell



#pragma once

#include <JuceHeader.h>
#include "SoundSampleButtonColourPicker.h"
#include "SonoPlaybackProgressButton.h"
#include "SonoTextButton.h"

void SoundSampleButtonColourPicker::show(const Rectangle<int>& bounds)
{
    auto defaultColour = selectedColour == nullptr
            ? SonoPlaybackProgressButton::DEFAULT_BUTTON_COLOUR
            : *selectedColour & 0xFFFFFF;

    auto colourSelector = std::make_unique<ColourSelector>();
    colourSelector->setName(TRANS("Custom Button Colour"));
    colourSelector->setCurrentColour(Colour(defaultColour | SonoPlaybackProgressButton::DEFAULT_BUTTON_COLOUR_ALPHA));
    colourSelector->addChangeListener(this);
    colourSelector->setColour(ColourSelector::backgroundColourId, Colours::transparentBlack);
    colourSelector->setSize(300, 400);

    CallOutBox::launchAsynchronously (std::move (colourSelector), bounds, nullptr);
}

void SoundSampleButtonColourPicker::updateSelectedColour(unsigned int newColour)
{
    if (selectedColour == nullptr) {
        return;
    }

    *selectedColour = newColour & 0xFFFFFF;
}

void SoundSampleButtonColourPicker::updatePickerButton(unsigned int newColour)
{
    if (pickerButton == nullptr) {
        return;
    }

    auto colourWithoutAlpha = newColour & 0xFFFFFF;
    pickerButton->setColour(
            SonoTextButton::buttonColourId,
            Colour(colourWithoutAlpha | SonoPlaybackProgressButton::DEFAULT_BUTTON_COLOUR_ALPHA)
    );
}