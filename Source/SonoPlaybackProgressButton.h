// SPDX-License-Identifier: GPLv3-or-later WITH Appstore-exception
// Copyright (C) 2020 Jesse Chappell


#pragma once

#include "JuceHeader.h"
#include "SoundboardChannelProcessor.h"
#include "SoundboardButtonColors.h"

class SonoPlaybackProgressButton : public TextButton, public PlaybackPositionListener
{
public:
    SonoPlaybackProgressButton();
    explicit SonoPlaybackProgressButton(const String& buttonName);
    SonoPlaybackProgressButton(const String& buttonName, const String& toolTip);

    ~SonoPlaybackProgressButton() override;

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

    void onPlaybackPositionChanged(const SamplePlaybackManager& samplePlaybackManager) override;

    void attachToPlaybackManager(std::shared_ptr<SamplePlaybackManager> playbackManager);

private:
    constexpr static const int PROGRESS_COLOUR = 0x22FFFFFF;

    double playbackPosition = 0.0;

    /**
     * The RGB value of the button background color with an alpha of 0.
     */
    int buttonColour = SoundboardButtonColors::DEFAULT_BUTTON_COLOUR;

    std::shared_ptr<SamplePlaybackManager> playbackManager;
};
