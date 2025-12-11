// SPDX-License-Identifier: GPLv3-or-later WITH Appstore-exception
// Copyright (C) 2024 SoundFlip

#pragma once

#include <JuceHeader.h>

class LoginView : public Component,
                  public Button::Listener
{
public:
    LoginView();
    ~LoginView() override;

    void paint(Graphics&) override;
    void resized() override;
    void buttonClicked(Button* buttonThatWasClicked) override;

    std::function<void()> onSignInClicked;

private:
    std::unique_ptr<Label> titleLabel;
    std::unique_ptr<Label> subtitleLabel;
    std::unique_ptr<TextButton> signInButton;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LoginView)
};