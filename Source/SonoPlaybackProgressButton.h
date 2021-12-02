// SPDX-License-Identifier: GPLv3-or-later WITH Appstore-exception
// Copyright (C) 2020 Jesse Chappell


#pragma once

#include "JuceHeader.h"

class SonoPlaybackProgressButton : public TextButton
{
public:
    /**
     * The default button colour RGB value with alpha 0.
     */
    constexpr static const int DEFAULT_BUTTON_COLOUR = 0x252525;

    /**
     * Alpha value of the button background color with RGB value of 0.
     */
    constexpr static const int DEFAULT_BUTTON_COLOUR_ALPHA = 0x77000000;

    /**
     * RGB value for the default red button colour with alpha 0.
     */
    constexpr static const int /* GIRL IN */ RED = 0xBF2E26;

    /**
     * RGB value for the default orange button colour with alpha 0.
     */
    constexpr static const int ORANGE = 0xE6851E;

    /**
     * RGB value for the default yellow button colour with alpha 0.
     */
    constexpr static const int YELLOW = 0xD6BD14;

    /**
     * RGB value for the default yellow green button colour with alpha 0.
     */
    constexpr static const int YELLOW_GREEN = 0x9AC742;

    /**
     * RGB value for the default green button colour with alpha 0.
     */
    constexpr static const int GREEN /* DAY */ = 0x4A8235;

    /**
     * RGB value for the default cyan button colour with alpha 0.
     */
    constexpr static const int CYAN = 0x5CC2AE;

    /**
     * RGB value for the default blue button colour with alpha 0.
     */
    constexpr static const int BLUE = 0x36639A;

    /**
     * RGB value for the default purple button colour with alpha 0.
     */
    constexpr static const int PURPLE /* DISCO MACHINE */ = 0x802F9F;

    /**
     * RGB value for the default pink button colour with alpha 0.
     */
    constexpr static const int PINK = 0xD264A1;

    /**
     * RGB value for the default white button colour with alpha 0.
     */
    constexpr static const int WHITE = 0xDEDEDE;

    SonoPlaybackProgressButton();
    explicit SonoPlaybackProgressButton(const String& buttonName);
    SonoPlaybackProgressButton(const String& buttonName, const String& toolTip);

    void setPlaybackPosition(double value);

    /**
     * @param newRgb The new button background colour with an alpha of 0.
     */
    void setButtonColour(int newRgb);

    /**
     * Function that gets called whenever the button is clicked with the primary button.
     * onClick also gets called before this callback.
     */
    std::function<void()> onPrimaryClick;

    /**
     * Function that gets called whenever the button is clicked with the secondary button.
     * onClick also gets called before this callback.
     */
    std::function<void()> onSecondaryClick;

    void internalClickCallback(const ModifierKeys& mods) override
    {
        TextButton::internalClickCallback(mods);

        if (mods.isLeftButtonDown()) {
            onPrimaryClick();
        }
        else if (mods.isRightButtonDown()) {
            onSecondaryClick();
        }
    }

    void paintButton(Graphics& graphics, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override;

private:
    constexpr static const int PROGRESS_COLOUR = 0x22FFFFFF;

    double playbackPosition = 0.0;

    /**
     * The RGB value of the button background color with an alpha of 0.
     */
    int buttonColour = DEFAULT_BUTTON_COLOUR;
};
