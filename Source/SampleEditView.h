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
    constexpr static const float DEFAULT_VIEW_WIDTH = 350;

    /**
     * The recommended height of the Sample edit view.
     */
    constexpr static const float DEFAULT_VIEW_HEIGHT = 100;

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
     * Label for the SoundSample name field.
     */
    std::unique_ptr<Label> mNameLabel;

    /**
     * Text input for the SoundSample name field.
     */
    std::unique_ptr<TextEditor> mNameInput;

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
     * Creates the button bar elements.
     */
    void createButtonBar();

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