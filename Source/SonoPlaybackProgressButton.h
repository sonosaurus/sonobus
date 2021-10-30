// SPDX-License-Identifier: GPLv3-or-later WITH Appstore-exception
// Copyright (C) 2020 Jesse Chappell


#pragma once

#include "JuceHeader.h"

class SonoPlaybackProgressButton : public TextButton
{
public:
    SonoPlaybackProgressButton();
    explicit SonoPlaybackProgressButton(const String& buttonName);
    SonoPlaybackProgressButton(const String& buttonName, const String& toolTip);

    void setPlaybackPosition(double value);

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

};
