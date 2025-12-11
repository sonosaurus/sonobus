// SPDX-License-Identifier: GPLv3-or-later WITH Appstore-exception
// Copyright (C) 2024 SoundFlip

#pragma once

#include <JuceHeader.h>

class EndSessionView : public Component,
                       public Button::Listener
{
public:
    EndSessionView();
    ~EndSessionView() override;

    void paint(Graphics&) override;
    void resized() override;
    void buttonClicked(Button* buttonThatWasClicked) override;

    std::function<void()> onUploadClicked;
    std::function<void()> onSaveLocallyClicked;
    std::function<void()> onDiscardClicked;

private:
    std::unique_ptr<Label> titleLabel;
    std::unique_ptr<Label> recordingInfoLabel;
    std::unique_ptr<TextButton> uploadButton;
    std::unique_ptr<TextButton> saveLocallyButton;
    std::unique_ptr<TextButton> discardButton;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(EndSessionView)
};