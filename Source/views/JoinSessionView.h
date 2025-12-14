// SPDX-License-Identifier: GPLv3-or-later WITH Appstore-exception
// Copyright (C) 2024 SoundFlip

#pragma once

#include <JuceHeader.h>

class SessionManager;
class SonobusAudioProcessorEditor;

class JoinSessionView : public Component
{
public:
    // Default constructor (for ScreenManager usage)
    JoinSessionView();
    
    // Full constructor (for SonobusPluginEditor usage)
    JoinSessionView(SessionManager* sessionManager, SonobusAudioProcessorEditor* editor);
    
    ~JoinSessionView() override = default;

    void paint(Graphics& g) override;
    void resized() override;
    
    void setInviteCode(const String& code);

    // Callbacks
    std::function<void()> onBackClicked;
    std::function<void()> onJoinClicked;
    std::function<void(const String&)> onSessionJoined;

private:
    void setupUI();
    void handleJoinSession();
    void showError(const String& message);
    void showStatus(const String& message);
    void setUIEnabled(bool enabled);

    SessionManager* sessionManager = nullptr;
    SonobusAudioProcessorEditor* editor = nullptr;

    Label titleLabel;
    Label codeLabel;
    TextEditor codeEditor;
    Label statusLabel;
    TextButton joinButton;
    TextButton backButton;

    bool isJoiningSession = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(JoinSessionView)
};