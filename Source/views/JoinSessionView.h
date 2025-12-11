// SPDX-License-Identifier: GPLv3-or-later WITH Appstore-exception
// Copyright (C) 2024 SoundFlip

#pragma once

#include <JuceHeader.h>

class JoinSessionView : public Component,
                        public Button::Listener
{
public:
    JoinSessionView();
    ~JoinSessionView() override;

    void paint(Graphics&) override;
    void resized() override;
    void buttonClicked(Button* buttonThatWasClicked) override;

    std::function<void()> onBackClicked;
    std::function<void()> onJoinClicked;

private:
    std::unique_ptr<TextButton> backButton;
    std::unique_ptr<Label> titleLabel;
    std::unique_ptr<Label> inviteLinkLabel;
    std::unique_ptr<TextEditor> inviteLinkInput;
    std::unique_ptr<Label> audioInputLabel;
    std::unique_ptr<ComboBox> audioInputCombo;
    std::unique_ptr<Label> inputLevelLabel;
    std::unique_ptr<TextButton> joinButton;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(JoinSessionView)
};