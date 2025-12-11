// SPDX-License-Identifier: GPLv3-or-later WITH Appstore-exception
// Copyright (C) 2024 SoundFlip

#pragma once

#include "JuceHeader.h"
#include "../api/SoundFlipAuth.h"
#include "../api/SoundFlipAPI.h"
#include "../views/LoginView.h"
#include "../views/HomeView.h"
#include "../views/StartSessionView.h"
#include "../views/JoinSessionView.h"
#include "../views/ActiveSessionView.h"
#include "../views/SettingsView.h"

class ScreenManager : public Component,
                      public SoundFlipAuth::Listener
{
public:
    enum class Screen
    {
        Login,
        Home,
        StartSession,
        JoinSession,
        ActiveSession,
        Settings
    };

    ScreenManager(SoundFlipAuth& auth, SoundFlipAPI& api);
    ~ScreenManager() override;

    void showScreen(Screen screen);
    Screen getCurrentScreen() const { return currentScreen; }

    void resized() override;

    // SoundFlipAuth::Listener
    void authenticationSucceeded() override;
    void authenticationFailed(const String& error) override;
    void authenticationLoggedOut() override;

private:
    void setupViews();
    void hideAllViews();

    SoundFlipAuth& auth;
    SoundFlipAPI& api;
    Screen currentScreen = Screen::Login;

    std::unique_ptr<LoginView> loginView;
    std::unique_ptr<HomeView> homeView;
    std::unique_ptr<StartSessionView> startSessionView;
    std::unique_ptr<JoinSessionView> joinSessionView;
    std::unique_ptr<ActiveSessionView> activeSessionView;
    std::unique_ptr<SettingsView> settingsView;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ScreenManager)
};