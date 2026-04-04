// SPDX-License-Identifier: GPLv3-or-later WITH Appstore-exception
// Copyright (C) 2021 Jesse Chappell



#pragma once

#include <JuceHeader.h>
#include "SonoDrawableButton.h"

/**
 * Handles selecting a custom colour.
 *
 * @author Hannah Schellekens
 */
class SoundSampleButtonColourPicker : public ChangeListener
{
public:
    explicit SoundSampleButtonColourPicker(
            uint32* theSelectedColour,
            SonoDrawableButton* thePickerButton
    ) : selectedColour(theSelectedColour),
        pickerButton(thePickerButton)
    {}

    /**
     * Callback that is called when color is changed.
     */
    std::function<void()> colourChangedCallback;

    /**
     * Shows the colour picker.
     *
     * @param bounds Screen location of the picker.
     */
    void show(const Rectangle<int>& bounds);

private:
    /**
     * Where the currently selected colour is stored.
     */
    uint32* selectedColour;

    /**
     * Pointer to the button that should reflect the selected colour.
     */
    SonoDrawableButton* pickerButton;

    /**
     * Reports the newly selected colour.
     *
     * @param newColour The newly selected colour (may contain alpha value).
     */
    void updateSelectedColour(uint32 newColour);

    /**
     * Updates the picker button to display the newly selected colour.
     * Also removes the checkmark from the previously selected button.
     *
     * @param newColour The newly selected colour (may contain alpha value).
     */
    void updatePickerButton(uint32 newColour);

    void changeListenerCallback(ChangeBroadcaster* source) override
    {
        auto* colourSelector = dynamic_cast<ColourSelector*>(source);
        auto colour = colourSelector->getCurrentColour().getARGB();

        updateSelectedColour(colour);
        updatePickerButton(colour);
        if (colourChangedCallback)
            colourChangedCallback();
    }
};
