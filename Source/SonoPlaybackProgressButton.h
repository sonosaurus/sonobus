// SPDX-License-Identifier: GPLv3-or-later WITH Appstore-exception
// Copyright (C) 2020 Jesse Chappell


#pragma once

#include <JuceHeader.h>
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

    void setShowEditArea(bool show);
    bool getShowEditArea() const { return showEditArea; }

    /**
     * Function that gets called whenever the button is clicked with the primary button.
     * onClick also gets called before this callback.
     */
    std::function<void(const ModifierKeys& mods)> onPrimaryClick;

    /**
     * Function that gets called whenever the button is clicked with the secondary button.
     * onClick also gets called before this callback.
     */
    std::function<void(const ModifierKeys& mods)> onSecondaryClick;

    void internalClickCallback(const ModifierKeys& mods) override
    {
        TextButton::internalClickCallback(mods);

        if (!ignoreNextClick) {
            if (mods.isLeftButtonDown()) {
                if (clickIsEdit) {
                    if (onSecondaryClick) onSecondaryClick(mods);
                } else {
                    if (onPrimaryClick) onPrimaryClick(mods);
                }
            }
            else if (mods.isRightButtonDown()) {
                if (onSecondaryClick) onSecondaryClick(mods);
            }
        }

        ignoreNextClick = false;
    }

    void mouseDown (const MouseEvent& e) override
    {
        clickIsEdit = false;

        if (showEditArea && editBounds.contains(e.getPosition())) {
            clickIsEdit = true;
        }
        TextButton::mouseDown(e);
    }

    void mouseMove (const MouseEvent& e) override
    {
        if (showEditArea) {
            bool inarea = editBounds.contains(e.getPosition());
            if (inarea != overEditArea) {
                overEditArea = inarea;
                repaint();
            }
        }
        TextButton::mouseMove(e);
    }

    void mouseExit (const MouseEvent& e) override
    {
        if (overEditArea) {
            overEditArea = false;
            repaint();
        }

        TextButton::mouseExit(e);
    }

    void paintButton(Graphics& graphics, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override;

    void onPlaybackPositionChanged(const SamplePlaybackManager& samplePlaybackManager) override;

    void attachToPlaybackManager(std::shared_ptr<SamplePlaybackManager> playbackManager);

    void setMouseListener(std::unique_ptr<MouseListener> listener);

    SamplePlaybackManager * getPlaybackManager() const { return playbackManager.get(); }

    void setIgnoreNextClick() { ignoreNextClick = true; }

    bool isClickEdit() const { return clickIsEdit; }

    void setPositionDragging(bool flag) { posDragging = flag; }
    bool isPositionDragging() const { return posDragging; }
    
private:

    void initStuff();

    constexpr static const uint32 PROGRESS_COLOUR = 0x33FFFFFF;
    constexpr static const uint32 PROGRESS_PLAY_OUTLINE_COLOUR = 0x99AAAAFF;

    double playbackPosition = 0.0;
    bool isPlaying = false;
    bool ignoreNextClick = false;
    bool clickIsEdit = false;
    bool overEditArea = false;

    bool showEditArea = true;
    bool posDragging = false;
    
    /**
     * The RGB value of the button background color with an alpha of 0.
     */
    uint32 buttonColour = SoundboardButtonColors::DEFAULT_BUTTON_COLOUR;

    std::shared_ptr<SamplePlaybackManager> playbackManager;

    std::unique_ptr<MouseListener> mouseListener;

    std::unique_ptr<Drawable> editImage;
    Rectangle<int> editBounds;
};
