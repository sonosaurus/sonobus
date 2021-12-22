// SPDX-License-Identifier: GPLv3-or-later WITH Appstore-exception
// Copyright (C) 2021 Jesse Chappell


#pragma once

#include <JuceHeader.h>

#include "SonobusPluginProcessor.h"
#include "SonoDrawableButton.h"
#include "SonoPlaybackProgressButton.h"
#include "SonoChoiceButton.h"
#include "SoundboardProcessor.h"

/**
 * User Interface element for the Soundboard.
 *
 * @author Hannah Schellekens, Sten Wessel
 */
class SoundboardView : public Component, public SonoChoiceButton::Listener, public FileDragAndDropTarget
{
public:
    explicit SoundboardView(SoundboardChannelProcessor* channelProcessor);

    void paint(Graphics&) override;

    void resized() override;

    void choiceButtonSelected(SonoChoiceButton* choiceButton, int index, int ident) override;

    /**
     * @param keyPress Pressed key code.
     */
    void processKeystroke(const KeyPress& keyPress);

    bool isInterestedInFileDrag(const StringArray& files) override;

    void fileDragEnter(const StringArray& files, int x, int y) override;

    void fileDragMove(const StringArray& files, int x, int y) override;

    void fileDragExit(const StringArray& files) override;

    void filesDropped(const StringArray& files, int x, int y) override;

    /**
     * Plays the sound of the given sample.
     *
     * @param sample The sample to play.
     * @param button The button the play request originated from.
     */
    void playSample(const SoundSample& sample, SonoPlaybackProgressButton* button = nullptr);

    void stopSample(const SoundSample& sample);

private:
    constexpr static const float MENU_BUTTON_WIDTH = 36;
    constexpr static const float TITLE_LABEL_WIDTH = 90;
    constexpr static const float TITLE_HEIGHT = 32;
    constexpr static const float TITLE_FONT_HEIGHT = 18;
    constexpr static const float ELEMENT_MARGIN = 4;

    /**
     * Controller for soundboard view.
     */
    std::unique_ptr<SoundboardProcessor> processor;

    SoundboardProcessor* getSoundboardProcessor() { return processor.get(); };

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
     * Scrollable container for the soundboard buttons.
     */
    Viewport buttonViewport;

    /**
     * Container to hold the soundboard buttons.
     */
    Component buttonContainer;

    /**
     * Layout containing all the buttons that play soundboard
     * sounds.
     */
    FlexBox buttonBox;

    /**
     * Layout for all the soundboard selection and management controls.
     */
    FlexBox soundboardSelectionBox;

    /**
     * Layout for the control bar at the bottom.
     */
    FlexBox controlsBox;

    /**
     * Look and feel for dashed buttons.
     */
    SonoDashedBorderButtonLookAndFeel dashedButtonLookAndFeel;

    /**
     * The parent directory of the last chosen browse directory.
     */
    std::unique_ptr<String> mLastSampleBrowseDirectory;

    /**
     * Label showing the panel title.
     */
    std::unique_ptr<Label> mTitleLabel;

    /**
     * Button that closes the soundboard panel.
     */
    std::unique_ptr<SonoDrawableButton> mCloseButton;

    /**
     * Button that shows soundboard menu options when clicked.
     */
    std::unique_ptr<SonoDrawableButton> mMenuButton;

    /**
     * Combo box where the user can select which available soundboard to use.
     */
    std::unique_ptr<SonoChoiceButton> mBoardSelectComboBox;

    /**
     * All the buttons that can play a sound.
     */
    std::vector<std::unique_ptr<SonoPlaybackProgressButton>> mSoundButtons;

    /**
     * Button that adds a new sample to the current soundboard.
     */
    std::unique_ptr<SonoDrawableButton> mAddSampleButton;

    /**
     * Button for enabling/disabling hotkeys.
     */
    std::unique_ptr<SonoDrawableButton> mHotkeyStateButton;

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

    /**
     * Creates the menu button.
     */
    void createSoundboardMenu();

    /**
     * Creates the control panel for selecting and managing different soundboards.
     */
    void createSoundboardSelectionPanel();

    /**
     * Creates the bottom control button bar.
     */
    void createControlPanel();

    /**
     * Updates the soundboard selector to contain exactly all soundboards that are
     * currently loaded in the soundboard state.
     */
    void updateSoundboardSelector();

    /**
     * Adds buttons for all available sounds for the selected soundboard.
     * Removes buttons of sounds that are not available anymore.
     */
    void updateButtons();

    /**
     * Plays the sample at the given sampleIndex, does nothing when sampleIndex out of bounds.
     * @return true if a sound was played, false if no sound was played.
     */
    bool playSampleAtIndex(int sampleIndex);

    /**
     * Shows the context menu for the SoundBoard View main menu button.
     */
    void showMenuButtonContextMenu();

    /**
     * Call this method whenever the "New Soundboard" option is clicked.
     */
    void clickedAddSoundboard();

    /**
     * Call this method whenever the "Rename Soundboard" option is clicked.
     */
    void clickedRenameSoundboard();

    /**
     * Call this method whenever the "Delete Soundboard" option is clicked.
     */
    void clickedDeleteSoundboard();

    /**
     * Call this method whenever the a new sound sample must be created.
     */
    void clickedAddSoundSample();

    /**
     * Call this method whenever the an existing sound sample must be edited.
     *
     * @param button The sample button.
     * @param sample The sample that must be edited.
     */
    void clickedEditSoundSample(const SonoPlaybackProgressButton& button, SoundSample& sample);

    void fileDraggedAt(int x, int y);

    void fileDragStopped();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SoundboardView)
};

class HoldSampleButtonMouseListener : public MouseListener
{
public:

    HoldSampleButtonMouseListener(SonoPlaybackProgressButton* button, SoundSample* sample, SoundboardView* view);

    void mouseDown(const MouseEvent &event) override;

    void mouseUp(const MouseEvent &event) override;

private:

    SonoPlaybackProgressButton* button;

    SoundSample* sample;

    SoundboardView* view;
};