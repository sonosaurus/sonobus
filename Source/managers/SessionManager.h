// SPDX-License-Identifier: GPLv3-or-later WITH Appstore-exception
// Copyright (C) 2024 SoundFlip

#pragma once

#include <JuceHeader.h>
#include "../api/SoundFlipAPI.h"

struct SessionConnectionInfo
{
    String server;
    int port = 10999;
    String group;
    String password;
};

struct SessionParticipant
{
    String odid;
    String username;
    String odidLinked;
};

// Recent session info (for HomeView)
struct RecentSessionInfo
{
    String id;
    String name;
    String status;
    int stemCount = 0;
    int64 createdAt = 0;
    Array<SessionParticipant> participants;
};

class SessionManager : public ChangeBroadcaster
{
public:
    enum class State
    {
        Idle,
        CreatingSession,
        JoiningSession,
        Connecting,
        Connected,
        Disconnecting,
        Error
    };
    
    SessionManager(SoundFlipAPI& api);
    ~SessionManager() = default;
    
    // Session lifecycle
    bool createSession(const String& name = "");
    bool joinSession(const String& inviteCodeOrUrl);
    void leaveSession();
    
    // State queries
    bool isConnected() const { return currentState == State::Connected; }
    State getState() const { return currentState; }
    String getLastError() const { return lastError; }
    
    // Session info
    String getCurrentSessionId() const { return currentSessionId; }
    String getCurrentSessionName() const { return currentSessionName; }
    String getInviteUrl() const { return currentInviteUrl; }
    SessionConnectionInfo getConnectionInfo() const { return connectionInfo; }
    const Array<SessionParticipant>& getParticipants() const { return participants; }
    
    // Recent sessions (for HomeView)
    void fetchRecentSessions(int limit = 5);
    const Array<RecentSessionInfo>& getRecentSessions() const { return recentSessions; }
    
    // Called by editor when connection events occur
    void onSessionConnected();
    void onSessionDisconnected();
    void onConnectionFailed(const String& error);
    void onPeerJoined(const String& username);
    void onPeerLeft(const String& username);
    
    // Upload stem to current session
    void uploadStem(const URL& audioFile, std::function<void(bool success, const String& message)> callback);
    
    // Callbacks for UI - set by editor
    std::function<void()> onSessionConnectedCallback;
    std::function<void()> onSessionDisconnectedCallback;
    std::function<void(const String&)> onConnectionFailedCallback;
    
private:
    void setState(State newState);
    void clearSession();
    String extractSessionCode(const String& input);
    
    SoundFlipAPI& api;
    
    State currentState = State::Idle;
    String lastError;
    
    String currentSessionId;
    String currentSessionName;
    String currentInviteUrl;
    SessionConnectionInfo connectionInfo;
    Array<SessionParticipant> participants;
    
    // Recent sessions cache
    Array<RecentSessionInfo> recentSessions;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SessionManager)
};