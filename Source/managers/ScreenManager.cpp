// SPDX-License-Identifier: GPLv3-or-later WITH Appstore-exception
// Copyright (C) 2024 SoundFlip

#include "ScreenManager.h"

ScreenManager::ScreenManager(SoundFlipAuth& authRef, SoundFlipAPI& apiRef)
    : auth(authRef), api(apiRef)
{
    auth.addListener(this);
    setupViews();
    
    // Start on login or home based on auth state
    if (auth.isAuthenticated())
    {
        showScreen(Screen::Home);
    }
    else
    {
        showScreen(Screen::Login);
    }
}

ScreenManager::~ScreenManager()
{
    auth.removeListener(this);
}

void ScreenManager::setupViews()
{
    // Create views
    loginView = std::make_unique<LoginView>(auth);
    homeView = std::make_unique<HomeView>();
    startSessionView = std::make_unique<StartSessionView>();
    joinSessionView = std::make_unique<JoinSessionView>();
    activeSessionView = std::make_unique<ActiveSessionView>();
    settingsView = std::make_unique<SettingsView>();

    // Add as children (hidden initially)
    addChildComponent(loginView.get());
    addChildComponent(homeView.get());
    addChildComponent(startSessionView.get());
    addChildComponent(joinSessionView.get());
    addChildComponent(activeSessionView.get());
    addChildComponent(settingsView.get());

    // Setup callbacks
    loginView->onLoginSuccess = [this]() {
        showScreen(Screen::Home);
    };

    homeView->onStartSessionClicked = [this]() {
        showScreen(Screen::StartSession);
    };

    homeView->onJoinSessionClicked = [this]() {
        showScreen(Screen::JoinSession);
    };

    homeView->onSettingsClicked = [this]() {
        showScreen(Screen::Settings);
    };

    startSessionView->onSessionCreated = [this](const String& sessionId) {
        ignoreUnused(sessionId);
        showScreen(Screen::ActiveSession);
    };

    startSessionView->onBackClicked = [this]() {
        showScreen(Screen::Home);
    };

    joinSessionView->onSessionJoined = [this](const String& sessionId) {
        ignoreUnused(sessionId);
        showScreen(Screen::ActiveSession);
    };

    joinSessionView->onBackClicked = [this]() {
        showScreen(Screen::Home);
    };

    activeSessionView->onSessionEnded = [this]() {
        showScreen(Screen::Home);
    };

    settingsView->onBackClicked = [this]() {
        showScreen(Screen::Home);
    };

    settingsView->onSignOutClicked = [this]() {
        auth.logout();
        showScreen(Screen::Login);
    };
}

void ScreenManager::hideAllViews()
{
    loginView->setVisible(false);
    homeView->setVisible(false);
    startSessionView->setVisible(false);
    joinSessionView->setVisible(false);
    activeSessionView->setVisible(false);
    settingsView->setVisible(false);
}

void ScreenManager::showScreen(Screen screen)
{
    hideAllViews();
    currentScreen = screen;

    Component* viewToShow = nullptr;

    switch (screen)
    {
        case Screen::Login:
            viewToShow = loginView.get();
            break;
        case Screen::Home:
            viewToShow = homeView.get();
            break;
        case Screen::StartSession:
            viewToShow = startSessionView.get();
            break;
        case Screen::JoinSession:
            viewToShow = joinSessionView.get();
            break;
        case Screen::ActiveSession:
            viewToShow = activeSessionView.get();
            break;
        case Screen::Settings:
            viewToShow = settingsView.get();
            break;
    }

    if (viewToShow != nullptr)
    {
        viewToShow->setVisible(true);
        viewToShow->setBounds(getLocalBounds());
    }
}

void ScreenManager::resized()
{
    auto bounds = getLocalBounds();

    loginView->setBounds(bounds);
    homeView->setBounds(bounds);
    startSessionView->setBounds(bounds);
    joinSessionView->setBounds(bounds);
    activeSessionView->setBounds(bounds);
    settingsView->setBounds(bounds);
}

void ScreenManager::authenticationSucceeded()
{
    MessageManager::callAsync([this]() {
        showScreen(Screen::Home);
    });
}

void ScreenManager::authenticationFailed(const String& error)
{
    ignoreUnused(error);
    // LoginView handles displaying the error
}

void ScreenManager::authenticationLoggedOut()
{
    MessageManager::callAsync([this]() {
        showScreen(Screen::Login);
    });
}