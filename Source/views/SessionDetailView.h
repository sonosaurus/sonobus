// SPDX-License-Identifier: GPLv3-or-later WITH Appstore-exception
// Copyright (C) 2024 SoundFlip

#pragma once

#include <JuceHeader.h>

class SessionDetailView : public Component,
                          public Button::Listener
{
public:
    SessionDetailView();
    ~SessionDetailView() override;

    void paint(Graphics&) override;
    void resized() override;
    void buttonClicked(Button* buttonThatWasClicked) override;

    std::function<void()> onBackClicked;
    std::function<void()> onOpenInSoundFlipClicked;
    std::function<void(int)> onDownloadStemClicked;

private:
    std::unique_ptr<TextButton> backButton;
    std::unique_ptr<Label> sessionNameLabel;
    std::unique_ptr<Label> sessionInfoLabel;
    std::unique_ptr<Label> participantsLabel;
    std::unique_ptr<Label> stemsLabel;
    std::unique_ptr<TextButton> stem1Button;
    std::unique_ptr<TextButton> stem2Button;
    std::unique_ptr<TextButton> openInSoundFlipButton;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SessionDetailView)
};