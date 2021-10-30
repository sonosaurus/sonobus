// SPDX-License-Identifier: GPLv3-or-later WITH Appstore-exception
// Copyright (C) 2021 Jesse Chappell



#pragma once

#include <JuceHeader.h>
#include "Soundboard.h"
#include "SonoTextButton.h"

/**
 * Dialog for creating/renaming a soundboard.
 * Meant for use in a CallOutBox.
 *
 * @author Hannah Schellekens
 */
class SoundboardEditView : public Component
{
public:
    /**
     * The recommended width of the SoundboardEditView.
     */
    constexpr static const float DEFAULT_VIEW_WIDTH = 256;

    /**
     * The recommended height of the SoundboardEditView.
     */
    constexpr static const float DEFAULT_VIEW_HEIGHT = 106;

    /**
     * @param callback Function with the actual selected name that gets called when the submit button is pressed.
     * @param soundboard The soundboard that must be edited, or null when a new soundboard must be created.
     */
    explicit SoundboardEditView(std::function<void (String)> callback, const Soundboard* soundboard = nullptr);

    /**
     * @return The soundboard name that was entered.
     */
    [[nodiscard]] String getInputName() const;

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
    std::function<void (String)> submitCallback;

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
     * Label showing the prompt to the user.
     */
    std::unique_ptr<Label> mMessageLabel;

    /**
     * Text box for entering the soundboard name.
     */
    std::unique_ptr<TextEditor> mInputField;

    /**
     * The button that submits the info in the dialog.
     */
    std::unique_ptr<SonoTextButton> mSubmitButton;

    /**
     * The button that cancels the dialog and closes without modifying state.
     */
    std::unique_ptr<SonoTextButton> mCancelButton;

    /**
     * Submit the input.
     */
    void submitDialog();

    /**
     * Closes the dialog.
     */
    void dismissDialog();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SoundboardEditView)
};