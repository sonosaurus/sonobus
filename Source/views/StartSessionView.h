// SPDX-License-Identifier: GPLv3-or-later WITH Appstore-exception
// Copyright (C) 2024 SoundFlip

#pragma once

#include "JuceHeader.h"
#include "../SonobusPluginProcessor.h"

// Forward declarations
class SessionManager;

class StartSessionView : public Component,
                         public Timer,
                         public SonobusAudioProcessor::ClientListener
{
public:
    StartSessionView(SessionManager* sessionManager = nullptr, 
                     SonobusAudioProcessor* processor = nullptr);
    ~StartSessionView() override;

    void paint(Graphics& g) override;
    void resized() override;
    void timerCallback() override;
    
    /** Set managers after construction if needed */
    void setSessionManager(SessionManager* sm) { sessionManager = sm; }
    void setProcessor(SonobusAudioProcessor* proc);

    // Callbacks
    std::function<void()> onStartClicked;
    std::function<void()> onBackClicked;
    std::function<void(const String& sessionId)> onSessionCreated;
    std::function<void()> onSessionStarted;

    // ClientListener overrides
    void aooClientConnected(SonobusAudioProcessor* processor, bool success, const String& errmesg) override;
    void aooClientDisconnected(SonobusAudioProcessor* processor, bool success, const String& errmesg) override;
    void aooClientGroupJoined(SonobusAudioProcessor* processor, bool success, const String& group, const String& errmesg) override;

private:
    void handleCreateSession();
    void showError(const String& message);
    void showStatus(const String& message);
    void setUIEnabled(bool enabled);
    void cleanupConnection();
    
    // Managers
    SessionManager* sessionManager = nullptr;
    SonobusAudioProcessor* processor = nullptr;
    
    // UI Components
    Label titleLabel;
    Label sessionNameLabel;
    TextEditor sessionNameEditor;
    Label statusLabel;
    TextButton createButton;
    TextButton backButton;
    
    // State
    bool isCreatingSession = false;
    bool isWaitingForConnect = false;
    bool isWaitingForGroupJoin = false;
    String pendingSessionId;
    String pendingGroupName;
    String pendingGroupPassword;
    int connectionCheckCount = 0;
    static constexpr int maxConnectionChecks = 300; // 30 seconds at 100ms intervals

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(StartSessionView)
};