// SPDX-License-Identifier: GPLv3-or-later WITH Appstore-exception
// Copyright (C) 2021 Jesse Chappell



#pragma once

#include <JuceHeader.h>
#include "Soundboard.h"
#include "SonoTextButton.h"
#include "SonoDrawableButton.h"
#include "SonoMultiStateDrawableButton.h"
#include "SoundSampleButtonColourPicker.h"

/**
 * Dialog for creating/editing soundboard samples.
 * Meant for use in a CallOutBox.
 *
 * @author Hannah Schellekens
 */
class SampleEditView : public Component
{
public:
    /**
     * The recommended width of the Sample edit view.
     */
    constexpr static const float DEFAULT_VIEW_WIDTH = 350;

    /**
     * The recommended height of the Sample edit view.
     */
    constexpr static const float DEFAULT_VIEW_HEIGHT = 500;

    /**
     * @param submitcallback Function with the actual selected name that gets called when the submit button is pressed.
     * @param gaincallback Function that will get called for gain changes
     * @param soundSample The sample that must be edited, or null when a new sample must be created.
     * @param lastOpenedDirectoryString Where to store the directory that was last opened using the browse button,
     *              or nullptr when the last directory should not be stored.
     */
    explicit SampleEditView(
                            std::function<void(SampleEditView&)> submitcallback,
                            std::function<void(SampleEditView&)> gaincallback,
                            const SoundSample* soundSample = nullptr,
                            String* lastOpenedDirectoryString = nullptr
    );

    /**
     * @return The sample name that was entered.
     */
    [[nodiscard]] String getSampleName() const;

    /**
     * @return The absolute file path to the sound file of the sound sample.
     */
    [[nodiscard]] String getAbsoluteFilePath() const;

    /**
     * Whether the dialog is in edit mode.
     *
     * @return true if the dialog is in edit mode.
     */
    [[nodiscard]] bool isEditMode() const { return editModeEnabled; }

    void setEditMode(bool flag);

    /**
     * Whether the dialog is in create mode.
     *
     * @return true if the dialog is in create mode.
     */
    [[nodiscard]] bool isCreateMode() const { return !editModeEnabled; }

    /**
     * @return true if the sample must be deleted.
     */
    [[nodiscard]] bool isDeleteSample() const { return deleteSample; }

    /**
     * @return The selected button colour without alpha value.
     */
    [[nodiscard]] int getButtonColour() const { return selectedColour; }

    /**
     * @return true if the loop button is in the 'toggle on' state.
     */
    [[nodiscard]] bool isLoop() const { return mLoopButton->getState() == 1; }

    /**
     * @return The selected playback behaviour.
     */
    [[nodiscard]] SoundSample::PlaybackBehaviour getPlaybackBehaviour() const { return static_cast<SoundSample::PlaybackBehaviour>(mPlaybackBehaviourButton->getState()); }

    [[nodiscard]] SoundSample::ButtonBehaviour getButtonBehaviour() const { return static_cast<SoundSample::ButtonBehaviour>(mButtonBehaviourButton->getState()); }

    [[nodiscard]] SoundSample::ReplayBehaviour getReplayBehaviour() const { return static_cast<SoundSample::ReplayBehaviour>(mReplayBehaviourButton->getState()); }

    /**
     * @return The selected gain.
     */
    [[nodiscard]] float getGain() const;

    /**
     * @return The key code for the hotkey.
     */
    [[nodiscard]] int getHotkeyCode() const { return hotkeyCode; };

    bool keyPressed(const KeyPress& key) override;

    void paint(Graphics&) override;

    void resized() override;

    /**
     * Function to call whenever the submit button is clicked.
     * Parameter is the inputted name.
     */
    std::function<void(SampleEditView&)> submitCallback;

    /**
     * Function to call whenever the gain/volume is changed
     */
    std::function<void(SampleEditView&)> gainChangeCallback;

    
    int getMinimumContentWidth() const { return minContentWidth; }
    int getMinimumContentHeight() const { return minContentHeight; }

private:
    constexpr static const float ELEMENT_MARGIN = 4;
    constexpr static const float CONTROL_HEIGHT = 24;

    /**
     * Constant for indicating that a custom colour will be used.
     */
    constexpr static const int CUSTOM_COLOUR = -1;

    /**
     * All the button colours that can be selected (in order).
     */
    const std::vector<int> BUTTON_COLOURS = {
            SoundboardButtonColors::DEFAULT_BUTTON_COLOUR,
            SoundboardButtonColors::RED,
            SoundboardButtonColors::ORANGE,
            SoundboardButtonColors::YELLOW,
            SoundboardButtonColors::YELLOW_GREEN,
            SoundboardButtonColors::GREEN,
            SoundboardButtonColors::CYAN,
            SoundboardButtonColors::BLUE,
            SoundboardButtonColors::PURPLE,
            SoundboardButtonColors::PINK,
            SoundboardButtonColors::WHITE,
            CUSTOM_COLOUR
    };

    /**
     * true if the dialog is in rename mode, or false when the dialog is in create mode.
     */
    bool editModeEnabled;

    /**
     * Whether the sample must be deleted.
     */
    bool deleteSample = false;

    /**
     * The selected button colour without alpha value.
     */
    uint32 selectedColour = SoundboardButtonColors::DEFAULT_BUTTON_COLOUR;

    /**
     * The name that is shown upon opening the dialog.
     */
    String initialName = "";

    /**
     * The file path that is shown upon opening the dialog.
     */
    String initialFilePath = "";

    /**
     * Loop playback option that is chosen upon opening the dialog.
     */
    bool initialLoop = false;

    /**
     * Playback behaviour option that is chosen upon opening the dialog.
     */
    SoundSample::PlaybackBehaviour initialPlaybackBehaviour = SoundSample::PlaybackBehaviour::SIMULTANEOUS;

    SoundSample::ButtonBehaviour initialButtonBehaviour = SoundSample::ButtonBehaviour::TOGGLE;

    SoundSample::ReplayBehaviour initialReplayBehaviour = SoundSample::ReplayBehaviour::REPLAY_FROM_START;

    /**
     * Gain option that is chosen upon opening the dialog.
     */
    float initialGain = 1.0;

    /**
     * The key code of the hotkey for the sample, -1 for no hotkey.
     */
    int hotkeyCode = -1;

    /**
     * The directory that was last opened by the file chooser.
     * nullptr when this should not be stored.
     */
    String* lastOpenedDirectory = nullptr;

    int minContentWidth = 320;
    int minContentHeight = 300;

    FlexBox buttonBox;
    FlexBox colourButtonBox;
    FlexBox colourButtonRowTopBox;
    FlexBox colourButtonRowBottomBox;


    std::unique_ptr<Label> mNameLabel;
    std::unique_ptr<TextEditor> mNameInput;

    std::unique_ptr<Label> mFilePathLabel;
    std::unique_ptr<TextEditor> mFilePathInput;
    std::unique_ptr<SonoTextButton> mBrowseFilePathButton;
    std::unique_ptr<FileChooser> mFileChooser;

    std::unique_ptr<Label> mColourInputLabel;
    std::vector<std::unique_ptr<SonoDrawableButton>> mColourButtons;
    std::unique_ptr<SoundSampleButtonColourPicker> mColourPicker;

    std::unique_ptr<Label> mPlaybackOptionsLabel;

    std::unique_ptr<SonoMultiStateDrawableButton> mLoopButton;

    std::unique_ptr<Label> mVolumeLabel;
    std::unique_ptr<Slider> mVolumeSlider;

    std::unique_ptr<SonoMultiStateDrawableButton> mPlaybackBehaviourButton;
    std::unique_ptr<SonoMultiStateDrawableButton> mButtonBehaviourButton;
    std::unique_ptr<SonoMultiStateDrawableButton> mReplayBehaviourButton;

    std::unique_ptr<Label> mHotkeyLabel;
    std::unique_ptr<SonoTextButton> mHotkeyButton;
    std::unique_ptr<SonoTextButton> mRemoveHotkeyButton;
    std::unique_ptr<SonoTextButton> mSubmitButton;
    std::unique_ptr<SonoTextButton> mDeleteButton;


    /**
     * Creates the input controls for the sample name.
     */
    void createNameInputs();

    /**
     * Creates the input controls for the file path.
     */
    void createFilePathInputs();

    /**
     * Adds the input controls for the sample button colour.
     */
    void createColourInput();

    /**
     * Adds the input controls for playback option configuration.
     */
    void createPlaybackOptionInputs();

    /**
     * Adds the input controls for the volume settings.
     */
    void createVolumeInputs();

    /**
     * Creates the loop playback option toggle.
     */
    void createLoopButton();

    /**
     * Creates the playback behaviour option toggle.
     */
    void createPlaybackBehaviourButton();

    void createButtonBehaviourButton();

    void createReplayBehaviourButton();

    /**
     * Adds the input controls for the hotkey configuration.
     */
    void createHotkeyInput();

    /**
     * Creates a new colour pick button.
     *
     * @param index The ith button (index in BUTTON_COLOURS).
     * @return Reference to the created button.
     */
    std::unique_ptr<SonoDrawableButton> createColourButton(const int index);

    /**
     * Updates the colour buttons to show a checkmark if its colour is slected and hide the checkmark if that colour
     * has not been selected.
     */
    void updateColourButtonCheckmark();

    /**
     * Creates the button bar elements.
     */
    void createButtonBar();

    /**
     * Lets the user browse for a sound file to select.
     */
    void browseFilePath();

    /**
     * Fills in the sample name field based on the given file name.
     */
    void inferSampleName();

    /**
     * Submit the input.
     */
    void submitDialog(bool dismiss=true);

    /**
     * Closes the dialog.
     */
    void dismissDialog();

    /**
     * Get the index of the colour in BUTTON_COLOURS, when not a standard colour, returns the last index in
     * BUTTON_COLOURS.
     *
     * @param colourWithoutAlpha The colour to look up without alpha value, i.e. only RGB bits, rest 0.
     * @return The index of the given colour in BUTTON_COLOURS, or #BUTTON_COLOURS - 1 when it is not a default
     * colour (CUSTOM_COLOUR).
     */
    unsigned int indexOfColour(int colourWithoutAlpha)
    {
        auto colourCount = BUTTON_COLOURS.size();
        for (int i = 0; i < colourCount; ++i) {
            if (BUTTON_COLOURS[i] == colourWithoutAlpha) {
                return i;
            }
        }
        return colourCount - 1;
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SampleEditView)
};
