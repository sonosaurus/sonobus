// SPDX-License-Identifier: GPLv3-or-later WITH Appstore-exception
// Copyright (C) 2024 SoundFlip

#pragma once

#include "JuceHeader.h"

// Forward declarations
class SessionManager;
class SonobusAudioProcessor;

class JoinSessionView : public Component,
                        public Timer
{
public:
    JoinSessionView(SessionManager* sessionManager = nullptr,
                    SonobusAudioProcessor* processor = nullptr);
    ~JoinSessionView() override = default;

    void paint(Graphics& g) override;
    void resized() override;
    void timerCallback() override;
    
    /** Set managers after construction if needed */
    void setSessionManager(SessionManager* sm) { sessionManager = sm; }
    void setProcessor(SonobusAudioProcessor* proc) { processor = proc; }
    
    /** Pre-fill the invite code (e.g., from deep link) */
    void setInviteCode(const String& code);

    // Callbacks
    std::function<void()> onJoinClicked;
    std::function<void()> onBackClicked;
    std::function<void(const String& sessionId)> onSessionJoined;

private:
    void handleJoinSession();
    void showError(const String& message);
    void showStatus(const String& message);
    void setUIEnabled(bool enabled);
    void checkConnectionStatus();
    
    // Managers
    SessionManager* sessionManager = nullptr;
    SonobusAudioProcessor* processor = nullptr;
    
    // UI Components
    Label titleLabel;
    Label codeLabel;
    TextEditor codeEditor;
    Label statusLabel;
    TextButton joinButton;
    TextButton backButton;
    
    // State
    bool isJoiningSession = false;
    bool isWaitingForConnection = false;
    String pendingSessionId;
    String pendingGroupName;
    int connectionCheckCount = 0;
    static constexpr int maxConnectionChecks = 30;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(JoinSessionView)
};