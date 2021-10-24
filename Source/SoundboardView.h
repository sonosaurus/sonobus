// SPDX-License-Identifier: GPLv3-or-later WITH Appstore-exception
// Copyright (C) 2021 Jesse Chappell


#pragma once

#include <JuceHeader.h>

#include "SonobusPluginProcessor.h"
#include "SonoDrawableButton.h"

/**
 * User Interface element for the Soundboard.
 *
 * @author Hannah Schellekens, Sten Wessel
 */
class SoundboardView : public Component
{
public:
    SoundboardView();

    void paint(Graphics&) override;
    void resized() override;

private:
    constexpr static const float MENU_BUTTON_WIDTH = 36;
    constexpr static const float TITLE_LABEL_WIDTH = 90;
    constexpr static const float TITLE_HEIGHT = 32;
    constexpr static const float TITLE_FONT_HEIGHT = 18;
    constexpr static const float ELEMENT_MARGIN = 4;

    /**
     * The outer soundboard panel box.
     */
    FlexBox mainBox;

    /**
     * Wrapper for soundboard UI contents.
     */
    FlexBox soundboardContainerBox;

    /**
     * Title bar of the soundboard panel.
     */
    FlexBox titleBox;

    /**
     * Label showing the panel title.
     */
    std::unique_ptr<Label> mTitleLabel;

    /**
     * Button that closes the soundboard panel.
     */
    std::unique_ptr<SonoDrawableButton> mCloseButton;

    /**
     * Creates the outer UI panels.
     */
    void createBasePanels();

    /**
     * Creates the title UI elements of the soundboard panel.
     */
    void createSoundboardTitle();

    /**
     * Creates the title label.
     */
    void createSoundboardTitleLabel();

    /**
     * Creates the close button of the title panel.
     */
    void createSoundboardTitleCloseButton();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SoundboardView)
};