// SPDX-License-Identifier: GPLv3-or-later WITH Appstore-exception
// Copyright (C) 2021 Jesse Chappell



#pragma once

#include <JuceHeader.h>
#include "Soundboard.h"
#include "SonoTextButton.h"

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
    constexpr static const float DEFAULT_VIEW_HEIGHT = 160;

    /**
     * @param callback Function with the actual selected name that gets called when the submit button is pressed.
     * @param soundSample The sample that must be edited, or null when a new sample must be created.
     */
    explicit SampleEditView(std::function<void (SampleEditView&)> callback, const SoundSample* soundSample = nullptr);

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

    void paint(Graphics&) override;
    void resized() override;

private:
    constexpr static const float ELEMENT_MARGIN = 4;
    constexpr static const float CONTROL_HEIGHT = 24;

    /**
     * true if the dialog is in rename mode, or false when the dialog is in create mode.
     */
    bool editModeEnabled;

    /**
     * The name that is shown upon opening the dialog.
     */
    String initialName = "";

    /**
     * The file path that is shown upon opening the dialog.
     */
    String initialFilePath = "";

    /**
     * Function to call whenever the submit button is clicked.
     * Parameter is the inputted name.
     */
    std::function<void (SampleEditView&)> submitCallback;

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
     * Button that saves the sound sample/submits the dialog.
     */
    std::unique_ptr<SonoTextButton> mSubmitButton;

    /**
     * Button that deletes the sound sample.
     */
    std::unique_ptr<SonoTextButton> mDeleteButton;

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

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SampleEditView)
};