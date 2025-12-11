// SPDX-License-Identifier: GPLv3-or-later WITH Appstore-exception
// Copyright (C) 2024 SoundFlip

#pragma once

#include <JuceHeader.h>

class StartSessionView : public Component,
                         public Button::Listener
{
public:
    StartSessionView();
    ~StartSessionView() override;

    void paint(Graphics&) override;
    void resized() override;
    void buttonClicked(Button* buttonThatWasClicked) override;

    std::function<void()> onBackClicked;
    std::function<void()> onStartClicked;

private:
    std::unique_ptr<TextButton> backButton;
    std::unique_ptr<Label> titleLabel;
    std::unique_ptr<Label> sessionNameLabel;
    std::unique_ptr<TextEditor> sessionNameInput;
    std::unique_ptr<Label> audioInputLabel;
    std::unique_ptr<ComboBox> audioInputCombo;
    std::unique_ptr<Label> inputLevelLabel;
    std::unique_ptr<TextButton> startButton;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(StartSessionView)
};