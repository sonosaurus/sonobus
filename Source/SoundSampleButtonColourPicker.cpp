// SPDX-License-Identifier: GPLv3-or-later WITH Appstore-exception
// Copyright (C) 2021 Jesse Chappell



#include <JuceHeader.h>
#include "SoundSampleButtonColourPicker.h"
#include "SonoPlaybackProgressButton.h"
#include "SonoTextButton.h"

void SoundSampleButtonColourPicker::show(const Rectangle<int>& bounds)
{
    auto defaultColour = selectedColour == nullptr
            ? SoundboardButtonColors::DEFAULT_BUTTON_COLOUR
            : *selectedColour & 0xFFFFFF;

    auto colourSelector = std::make_unique<ColourSelector>();
    colourSelector->setName(TRANS("Custom Button Colour"));
    colourSelector->setCurrentColour(Colour(defaultColour | SoundboardButtonColors::DEFAULT_BUTTON_COLOUR_ALPHA));
    colourSelector->addChangeListener(this);
    colourSelector->setColour(ColourSelector::backgroundColourId, Colours::transparentBlack);
    colourSelector->setSize(300, 400);

    Component* dw = nullptr;
    if (pickerButton) {
        dw = pickerButton->findParentComponentOfClass<AudioProcessorEditor>();
        if (!dw) dw = pickerButton->findParentComponentOfClass<Component>();
    }
    Rectangle<int> abounds =  dw ? dw->getLocalArea(nullptr, bounds) : bounds;

    CallOutBox::launchAsynchronously (std::move (colourSelector), abounds, dw);
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
            Colour(colourWithoutAlpha | SoundboardButtonColors::DEFAULT_BUTTON_COLOUR_ALPHA)
    );
}
