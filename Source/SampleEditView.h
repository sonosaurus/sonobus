// SPDX-License-Identifier: GPLv3-or-later WITH Appstore-exception
// Copyright (C) 2021 Jesse Chappell



#pragma once

#include <JuceHeader.h>
#include "Soundboard.h"
#include "SonoTextButton.h"
#include "SonoDrawableButton.h"
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
    constexpr static const float DEFAULT_VIEW_WIDTH = 380;

    /**
     * The recommended height of the Sample edit view.
     */
    constexpr static const float DEFAULT_VIEW_HEIGHT = 354;

    /**
     * @param callback Function with the actual selected name that gets called when the submit button is pressed.
     * @param soundSample The sample that must be edited, or null when a new sample must be created.
     * @param lastOpenedDirectoryString Where to store the directory that was last opened using the browse button,
     *              or nullptr when the last directory should not be stored.
     */
    explicit SampleEditView(
            std::function<void(SampleEditView&)> callback,
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
    [[nodiscard]] bool isLoop() const { return mLoopButton->getToggleState(); }

    void paint(Graphics&) override;

    void resized() override;

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
    int selectedColour = SoundboardButtonColors::DEFAULT_BUTTON_COLOUR;

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
    bool initialLoop;

    /**
     * The directory that was last opened by the file chooser.
     * nullptr when this should not be stored.
     */
    String* lastOpenedDirectory = nullptr;

    /**
     * Function to call whenever the submit button is clicked.
     * Parameter is the inputted name.
     */
    std::function<void(SampleEditView&)> submitCallback;

    /**
     * Outer layout.
     */
    FlexBox mainBox;

    /**
     * Wrapper for soundboard UI contents.
     */
    FlexBox contentBox;

    /**
     * Box for the dialog buttons.
     */
    FlexBox buttonBox;

    /**
     * Box for the file path input and browse button.
     */
    FlexBox filePathBox;

    /**
     * Box for the colour select buttons.
     */
    FlexBox colourButtonBox;

    /**
     * Box for the top colour button row.
     */
    FlexBox colourButtonRowTopBox;

    /**
     * Box for the bottom colour button row.
     */
    FlexBox colourButtonRowBottomBox;

    /**
     * Box for the playback option inputs.
     */
    FlexBox playbackOptionsRowBox;

    /**
     * Label for the SoundSample name field.
     */
    std::unique_ptr<Label> mNameLabel;

    /**
     * Text input for the SoundSample name field.
     */
    std::unique_ptr<TextEditor> mNameInput;

    /**
     * Label for the file path input field.
     */
    std::unique_ptr<Label> mFilePathLabel;

    /**
     * Text input for the SoundSample name field.
     */
    std::unique_ptr<TextEditor> mFilePathInput;

    /**
     * Button that prompts the user for a file to be put in the file path input.
     */
    std::unique_ptr<SonoTextButton> mBrowseFilePathButton;

    /**
     * Dialog box to choose the sound sample.
     */
    std::unique_ptr<FileChooser> mFileChooser;

    /**
     * Label for the sample button colour input.
     */
    std::unique_ptr<Label> mColourInputLabel;

    /**
     * Label for playback options.
     */
    std::unique_ptr<Label> mPlaybackOptionsLabel;

    /**
     * Contains all the button objects for the colour buttons.
     */
    std::vector<std::unique_ptr<SonoDrawableButton>> mColourButtons;

    /**
     * Button that saves the sound sample/submits the dialog.
     */
    std::unique_ptr<SonoTextButton> mSubmitButton;

    /**
     * Button that deletes the sound sample.
     */
    std::unique_ptr<SonoTextButton> mDeleteButton;

    /**
     * Control for selecting a custom button colour.
     */
    std::unique_ptr<SoundSampleButtonColourPicker> mColourPicker;

    /**
     * Control to toggle the loop playback option.
     */
    std::unique_ptr<SonoDrawableButton> mLoopButton;

    /**
     * Initialises all layout elements.
     */
    void initialiseLayouts();

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
     * Creates the loop playback option toggle.
     */
    void createLoopButton();

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
    void submitDialog();

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