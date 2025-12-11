// SPDX-License-Identifier: GPLv3-or-later WITH Appstore-exception
// Copyright (C) 2024 SoundFlip

#pragma once

#include <JuceHeader.h>

class SettingsView : public Component,
                     public Button::Listener
{
public:
    SettingsView();
    ~SettingsView() override;

    void paint(Graphics&) override;
    void resized() override;
    void buttonClicked(Button* buttonThatWasClicked) override;

    std::function<void()> onBackClicked;
    std::function<void()> onSignOutClicked;
    std::function<void()> onChangeRecordingFolderClicked;

private:
    std::unique_ptr<TextButton> backButton;
    std::unique_ptr<Label> titleLabel;

    std::unique_ptr<Label> audioInputLabel;
    std::unique_ptr<ComboBox> audioInputCombo;

    std::unique_ptr<Label> audioOutputLabel;
    std::unique_ptr<ComboBox> audioOutputCombo;

    std::unique_ptr<Label> audioQualityLabel;
    std::unique_ptr<ComboBox> audioQualityCombo;

    std::unique_ptr<Label> recordingFolderLabel;
    std::unique_ptr<Label> recordingFolderPath;
    std::unique_ptr<TextButton> changeFolderButton;

    std::unique_ptr<TextButton> signOutButton;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SettingsView)
};