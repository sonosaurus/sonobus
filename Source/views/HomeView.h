// SPDX-License-Identifier: GPLv3-or-later WITH Appstore-exception
// Copyright (C) 2024 SoundFlip

#pragma once

#include <JuceHeader.h>

class HomeView : public Component,
                 public Button::Listener
{
public:
    HomeView();
    ~HomeView() override;

    void paint(Graphics&) override;
    void resized() override;
    void buttonClicked(Button* buttonThatWasClicked) override;

    std::function<void()> onStartSessionClicked;
    std::function<void()> onJoinSessionClicked;
    std::function<void()> onSettingsClicked;
    std::function<void(int)> onRecentSessionClicked;

private:
    std::unique_ptr<Label> usernameLabel;
    std::unique_ptr<TextButton> settingsButton;
    std::unique_ptr<TextButton> startSessionButton;
    std::unique_ptr<TextButton> joinSessionButton;
    std::unique_ptr<Label> recentSessionsLabel;
    std::unique_ptr<TextButton> recentSession1Button;
    std::unique_ptr<TextButton> recentSession2Button;

    void drawAvatar(Graphics& g, int x, int y, int size);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(HomeView)
};