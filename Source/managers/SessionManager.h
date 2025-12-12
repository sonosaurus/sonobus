// SPDX-License-Identifier: GPLv3-or-later WITH Appstore-exception
// Copyright (C) 2024 SoundFlip

#pragma once

#include "JuceHeader.h"
#include "../api/SoundFlipAPI.h"

class SessionManager : public ChangeBroadcaster
{
public:
    SessionManager(SoundFlipAPI& api);
    ~SessionManager();

    //==============================================================================
    // Session State
    
    enum class State
    {
        Disconnected,
        Creating,
        Joining,
        Connected,
        Ending
    };
    
    State getCurrentState() const { return currentState; }
    bool isConnected() const { return currentState == State::Connected; }
    
    //==============================================================================
    // Session Management
    
    /** Create a new session and connect */
    bool createSession(const String& name = "");
    
    /** Join a session by invite code or URL */
    bool joinSession(const String& inviteCodeOrUrl);
    
    /** Leave the current session */
    void leaveSession();
    
    /** End the current session (host only) */
    bool endSession();
    
    //==============================================================================
    // Current Session Info
    
    String getCurrentSessionId() const { return currentSession.id; }
    String getCurrentSessionName() const { return currentSession.name; }
    String getInviteCode() const { return currentSession.inviteCode; }
    String getInviteUrl() const { return currentSession.inviteUrl; }
    SoundFlipAPI::ConnectionInfo getConnectionInfo() const { return currentSession.connection; }
    const Array<SoundFlipAPI::Participant>& getParticipants() const { return currentSession.participants; }
    
    //==============================================================================
    // Recent Sessions
    
    /** Fetch recent sessions from the API */
    Array<SoundFlipAPI::CollabSession> fetchRecentSessions(int limit = 10);
    
    /** Get cached recent sessions */
    const Array<SoundFlipAPI::CollabSession>& getRecentSessions() const { return recentSessions; }
    
    //==============================================================================
    // Stem Management
    
    /** Upload a recorded file as a stem */
    bool uploadStem(const File& audioFile, int durationSeconds = 0);
    
    /** Fetch stems for current session */
    Array<SoundFlipAPI::Stem> fetchSessionStems();
    
    /** Fetch stems for any session */
    Array<SoundFlipAPI::Stem> fetchSessionStems(const String& sessionId);
    
    /** Download a stem to local file */
    bool downloadStem(const SoundFlipAPI::Stem& stem, const File& destinationFile);
    
    /** Delete a stem */
    bool deleteStem(const String& stemId);
    
    //==============================================================================
    // Helpers
    
    /** Parse invite code from URL or return as-is if already a code */
    static String parseInviteCode(const String& input);
    
    /** Get last error message */
    String getLastError() const { return lastError; }

private:
    void setState(State newState);
    void clearCurrentSession();
    
    SoundFlipAPI& api;
    
    State currentState = State::Disconnected;
    SoundFlipAPI::CollabSession currentSession;
    Array<SoundFlipAPI::CollabSession> recentSessions;
    
    String lastError;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SessionManager)
};