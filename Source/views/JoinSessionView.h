// SPDX-License-Identifier: GPLv3-or-later WITH Appstore-exception
// Copyright (C) 2024 SoundFlip

#pragma once

#include "JuceHeader.h"
#include "../SonobusPluginProcessor.h"

// Forward declarations
class SessionManager;

class JoinSessionView : public Component,
                        public Timer,
                        public SonobusAudioProcessor::ClientListener
{
public:
    JoinSessionView(SessionManager* sessionManager = nullptr,
                    SonobusAudioProcessor* processor = nullptr);
    ~JoinSessionView() override;

    void paint(Graphics& g) override;
    void resized() override;
    void timerCallback() override;
    
    /** Set managers after construction if needed */
    void setSessionManager(SessionManager* sm) { sessionManager = sm; }
    void setProcessor(SonobusAudioProcessor* proc);
    
    /** Pre-fill the invite code (e.g., from deep link) */
    void setInviteCode(const String& code);

    // ClientListener callbacks
    void aooClientConnected(SonobusAudioProcessor* processor, bool success, const String& errmesg) override;
    void aooClientDisconnected(SonobusAudioProcessor* processor, bool success, const String& errmesg) override;
    void aooClientGroupJoined(SonobusAudioProcessor* processor, bool success, const String& group, const String& errmesg) override;

    // Callbacks
    std::function<void()> onJoinClicked;
    std::function<void()> onBackClicked;
    std::function<void(const String& sessionId)> onSessionJoined;

private:
    void handleJoinSession();
    void showError(const String& message);
    void showStatus(const String& message);
    void setUIEnabled(bool enabled);
    void cleanupConnection();
    
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
    bool isWaitingForConnect = false;
    bool isWaitingForGroupJoin = false;
    String pendingSessionId;
    String pendingGroupName;
    String pendingGroupPassword;
    int connectionCheckCount = 0;
    static constexpr int maxConnectionChecks = 100; // 10 seconds at 100ms intervals

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(JoinSessionView)
};