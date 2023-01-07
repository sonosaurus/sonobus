// SPDX-License-Identifier: GPLv3-or-later WITH Appstore-exception
// Copyright (C) 2020 Jesse Chappell


#include "SonoPlaybackProgressButton.h"

#include <utility>

SonoPlaybackProgressButton::SonoPlaybackProgressButton() : TextButton()
{
    initStuff();
}

SonoPlaybackProgressButton::SonoPlaybackProgressButton(const String& buttonName) : TextButton(buttonName)
{
    initStuff();
}

SonoPlaybackProgressButton::SonoPlaybackProgressButton(const String& buttonName, const String& toolTip)
        : TextButton(buttonName, toolTip)
{
    initStuff();
}

SonoPlaybackProgressButton::~SonoPlaybackProgressButton()
{
    if (playbackManager != nullptr) {
        playbackManager->detach(*this);
    }
}

void SonoPlaybackProgressButton::initStuff()
{
    editImage = Drawable::createFromImageData(BinaryData::dots_svg, BinaryData::dots_svgSize);
}

void SonoPlaybackProgressButton::setPlaybackPosition(double value)
{
    playbackPosition = value;
}

void SonoPlaybackProgressButton::setButtonColour(int newRgb)
{
    buttonColour = newRgb;
}

void SonoPlaybackProgressButton::setShowEditArea(bool show)
{
    if (showEditArea != show) {
        showEditArea = show;
        repaint();
    }
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
    if (isPlaying) {
        graphics.setColour(Colour(PROGRESS_PLAY_OUTLINE_COLOUR));
        graphics.drawRoundedRectangle(bounds, cornerSize, 2.0f);
    }

    lf.drawButtonText(graphics, *this, shouldDrawButtonAsHighlighted, shouldDrawButtonAsDown);

    if (showEditArea) {
        auto imgbounds = getLocalBounds().toFloat().reduced(2).withLeft(getLocalBounds().getRight() - getLocalBounds().getHeight());
        editBounds = imgbounds.toNearestIntEdges();
        if (overEditArea) {
            graphics.setColour(colour);
            graphics.fillRoundedRectangle(imgbounds, 6.0f);
        }
        editImage->drawWithin(graphics, imgbounds, RectanglePlacement(RectanglePlacement::centred), 0.6f);
    }
}

void SonoPlaybackProgressButton::onPlaybackPositionChanged(const SamplePlaybackManager& manager)
{
    auto position = manager.getLength() != 0.0
        ? manager.getCurrentPosition() / manager.getLength()
        : 0.0;

    if (!ignoreNextClick) {
        if (abs(playbackPosition - position) > 1e-10 || manager.isPlaying() != isPlaying) {
            isPlaying = manager.isPlaying();
            setPlaybackPosition(position);
            repaint();
        }
    }
}

void SonoPlaybackProgressButton::attachToPlaybackManager(std::shared_ptr<SamplePlaybackManager> playbackManager_)
{
    playbackManager = std::move(playbackManager_);
    playbackManager->detach(*this);
    playbackManager->attach(*this);
}

void SonoPlaybackProgressButton::setMouseListener(std::unique_ptr<MouseListener> listener) {
    mouseListener = std::move(listener);
    addMouseListener(mouseListener.get(), false);
}
