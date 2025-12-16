// SPDX-License-Identifier: GPLv3-or-later WITH Appstore-exception
// Copyright (C) 2024 SoundFlip

#pragma once

#include <JuceHeader.h>

class SessionManager;
class SonobusAudioProcessorEditor;

class StartSessionView : public Component
{
public:
    // Default constructor (for ScreenManager usage)
    StartSessionView();
    
    // Full constructor (for SonobusPluginEditor usage)
    StartSessionView(SessionManager* sessionManager, SonobusAudioProcessorEditor* editor);
    
    ~StartSessionView() override = default;

    void paint(Graphics& g) override;
    void resized() override;
    void reset();

    // Callbacks
    std::function<void()> onBackClicked;
    std::function<void()> onStartClicked;
    std::function<void(const String&)> onSessionCreated;
    std::function<void()> onSessionStarted;

private:
    void setupUI();
    void handleCreateSession();
    void showError(const String& message);
    void showStatus(const String& message);
    void setUIEnabled(bool enabled);

    SessionManager* sessionManager = nullptr;
    SonobusAudioProcessorEditor* editor = nullptr;

    Label titleLabel;
    Label sessionNameLabel;
    TextEditor sessionNameEditor;
    Label statusLabel;
    TextButton createButton;
    TextButton backButton;

    bool isCreatingSession = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(StartSessionView)
};