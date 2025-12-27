// SPDX-License-Identifier: GPLv3-or-later WITH Appstore-exception
// Copyright (C) 2024 SoundFlip

#pragma once

#include "JuceHeader.h"
#include "../api/SoundFlipAuth.h"

class LoginView : public Component,
                  public SoundFlipAuth::Listener
{
public:
    LoginView(SoundFlipAuth& auth);
    ~LoginView() override;

    void paint(Graphics& g) override;
    void resized() override;

    // SoundFlipAuth::Listener
    void authenticationSucceeded() override;
    void authenticationFailed(const String& error) override;

    // Callbacks
    std::function<void()> onLoginSuccess;

private:
    SoundFlipAuth& auth;

    Label titleLabel;
    Label subtitleLabel;
    TextButton signInButton;
    Label statusLabel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LoginView)
};