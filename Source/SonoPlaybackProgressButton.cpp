// SPDX-License-Identifier: GPLv3-or-later WITH Appstore-exception
// Copyright (C) 2020 Jesse Chappell


#include "SonoPlaybackProgressButton.h"

#include <utility>

SonoPlaybackProgressButton::SonoPlaybackProgressButton() : TextButton()
{}

SonoPlaybackProgressButton::SonoPlaybackProgressButton(const String& buttonName) : TextButton(buttonName)
{}

SonoPlaybackProgressButton::SonoPlaybackProgressButton(const String& buttonName, const String& toolTip)
        : TextButton(buttonName, toolTip)
{}

SonoPlaybackProgressButton::~SonoPlaybackProgressButton() noexcept
{
    if (playbackManager != nullptr) {
        playbackManager->detach(*this);
    }
}

void SonoPlaybackProgressButton::setPlaybackPosition(double value)
{
    playbackPosition = value;
}

void SonoPlaybackProgressButton::setButtonColour(int newRgb)
{
    buttonColour = newRgb;
}

void SonoPlaybackProgressButton::paintButton(Graphics& graphics,
                                             bool shouldDrawButtonAsHighlighted,
                                             bool shouldDrawButtonAsDown)
{
    auto& lf = getLookAndFeel();

    lf.drawButtonBackground(
            graphics,
            *this,
            Colour(buttonColour | SoundboardButtonColors::DEFAULT_BUTTON_COLOUR_ALPHA),
            shouldDrawButtonAsHighlighted, shouldDrawButtonAsDown
    );

    // Probably not the best place for this code
    auto cornerSize = 6.0f;
    auto bounds = getLocalBounds().toFloat().reduced(0.5f, 0.5f);
    bounds = bounds.withWidth(playbackPosition * bounds.getWidth());
    auto colour = Colour(PROGRESS_COLOUR);

    graphics.setColour(colour);
    graphics.fillRoundedRectangle(bounds, cornerSize);


    lf.drawButtonText(graphics, *this, shouldDrawButtonAsHighlighted, shouldDrawButtonAsDown);
}

void SonoPlaybackProgressButton::onPlaybackPositionChanged(const SamplePlaybackManager& manager)
{
    auto position = manager.getLength() != 0.0
        ? manager.getCurrentPosition() / manager.getLength()
        : 0.0;

    setPlaybackPosition(position);
    repaint();
}

void SonoPlaybackProgressButton::attachToPlaybackManager(std::shared_ptr<SamplePlaybackManager> playbackManager_)
{
    playbackManager = std::move(playbackManager_);
    playbackManager->attach(*this);
}
