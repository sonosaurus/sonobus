// SPDX-License-Identifier: GPLv3-or-later WITH Appstore-exception
// Copyright (C) 2024 SoundFlip

#include "SessionManager.h"

SessionManager::SessionManager(SoundFlipAPI& apiRef)
    : api(apiRef)
{
}

bool SessionManager::createSession(const String& name)
{
    if (currentState != State::Idle && currentState != State::Error)
    {
        lastError = "Already in a session";
        return false;
    }
    
    setState(State::CreatingSession);
    
    String sessionName = name.isEmpty() ? "SoundFlip Session" : name;
    
    // Use the API's createCollabSession method
    auto result = api.createCollabSession(sessionName);
    
    if (result.id.isEmpty())
    {
        lastError = api.getLastError().isEmpty() ? "Failed to create session" : api.getLastError();
        setState(State::Error);
        return false;
    }
    
    currentSessionId = result.id;
    currentSessionName = result.name;
    currentInviteUrl = result.inviteUrl;
    
    connectionInfo.server = result.connection.server;
    connectionInfo.port = result.connection.port;
    connectionInfo.group = result.connection.group;
    connectionInfo.password = result.connection.password;
    
    DBG("SessionManager: Created session " + currentSessionId);
    DBG("SessionManager: Connection - " + connectionInfo.server + ":" + 
        String(connectionInfo.port) + " group: " + connectionInfo.group);
    
    setState(State::Connecting);
    
    return true;
}

bool SessionManager::joinSession(const String& inviteCodeOrUrl)
{
    if (currentState != State::Idle && currentState != State::Error)
    {
        lastError = "Already in a session";
        return false;
    }
    
    setState(State::JoiningSession);
    
    String sessionCode = extractSessionCode(inviteCodeOrUrl);
    
    if (sessionCode.isEmpty())
    {
        lastError = "Invalid invite code or URL";
        setState(State::Error);
        return false;
    }
    
    // Use the API's joinCollabSession method
    auto result = api.joinCollabSession(sessionCode);
    
    if (result.id.isEmpty())
    {
        lastError = api.getLastError().isEmpty() ? "Failed to join session" : api.getLastError();
        setState(State::Error);
        return false;
    }
    
    currentSessionId = result.id;
    currentSessionName = result.name;
    currentInviteUrl = result.inviteUrl;
    
    connectionInfo.server = result.connection.server;
    connectionInfo.port = result.connection.port;
    connectionInfo.group = result.connection.group;
    connectionInfo.password = result.connection.password;
    
    DBG("SessionManager: Joined session " + currentSessionId + " (" + currentSessionName + ")");
    
    setState(State::Connecting);
    
    return true;
}

void SessionManager::leaveSession()
{
    if (currentState == State::Idle)
        return;
    
    setState(State::Disconnecting);
    
    // Notify API that we're leaving (for tracking purposes)
    if (currentSessionId.isNotEmpty())
    {
        api.leaveCollabSession(currentSessionId);
    }
    
    clearSession();
    setState(State::Idle);
    
    sendChangeMessage();
}

void SessionManager::fetchRecentSessions(int limit)
{
    // Fetch from API
    auto sessions = api.listCollabSessions("", limit, 0);
    
    recentSessions.clear();
    
    for (const auto& session : sessions)
    {
        RecentSessionInfo info;
        info.id = session.id;
        info.name = session.name;
        info.status = session.status;
        info.stemCount = session.stemCount;
        info.createdAt = session.createdAt;
        
        // Convert participants
        for (const auto& p : session.participants)
        {
            SessionParticipant participant;
            participant.odid = p.userId;
            participant.username = p.username;
            info.participants.add(participant);
        }
        
        recentSessions.add(info);
    }
    
    // Notify listeners that data has changed
    sendChangeMessage();
}

void SessionManager::onSessionConnected()
{
    DBG("SessionManager: Session connected");
    setState(State::Connected);
    
    if (onSessionConnectedCallback)
        onSessionConnectedCallback();
    
    sendChangeMessage();
}

void SessionManager::onSessionDisconnected()
{
    DBG("SessionManager: Session disconnected");
    
    State previousState = currentState;
    clearSession();
    setState(State::Idle);
    
    // Only trigger callback if we were previously connected
    if (previousState == State::Connected)
    {
        if (onSessionDisconnectedCallback)
            onSessionDisconnectedCallback();
    }
    
    sendChangeMessage();
}

void SessionManager::onConnectionFailed(const String& error)
{
    DBG("SessionManager: Connection failed - " + error);
    lastError = error;
    
    clearSession();
    setState(State::Error);
    
    if (onConnectionFailedCallback)
        onConnectionFailedCallback(error);
    
    sendChangeMessage();
}

void SessionManager::onPeerJoined(const String& username)
{
    DBG("SessionManager: Peer joined - " + username);
    
    // Add to participants list
    SessionParticipant participant;
    participant.username = username;
    participants.add(participant);
    
    sendChangeMessage();
}

void SessionManager::onPeerLeft(const String& username)
{
    DBG("SessionManager: Peer left - " + username);
    
    // Remove from participants list
    for (int i = participants.size() - 1; i >= 0; --i)
    {
        if (participants[i].username == username)
        {
            participants.remove(i);
            break;
        }
    }
    
    sendChangeMessage();
}

void SessionManager::uploadStem(const URL& audioFile, std::function<void(bool success, const String& message)> callback)
{
    if (currentSessionId.isEmpty())
    {
        if (callback)
            callback(false, "No active session");
        return;
    }
    
    if (audioFile.isEmpty())
    {
        if (callback)
            callback(false, "No audio file specified");
        return;
    }
    
    // Run upload in background thread
    Thread::launch([this, audioFile, callback, sessionId = currentSessionId]() {
        DBG("SessionManager: Starting upload for session " + sessionId);
        
        // TODO: Implement actual upload via SoundFlipAPI
        // For now, this is a stub that simulates success after a delay
        // 
        // Future implementation would call something like:
        // bool success = api.uploadStemToSession(sessionId, audioFile);
        
        // Simulate upload delay
        Thread::sleep(2000);
        
        // For now, report success (stub)
        // In real implementation, check api.getLastStatusCode() etc.
        bool success = true;
        String message = success ? "Upload complete" : "Upload failed";
        
        DBG("SessionManager: Upload " + String(success ? "succeeded" : "failed"));
        
        if (callback)
        {
            MessageManager::callAsync([callback, success, message]() {
                callback(success, message);
            });
        }
    });
}

void SessionManager::setState(State newState)
{
    if (currentState != newState)
    {
        DBG("SessionManager: State change " + String((int)currentState) + " -> " + String((int)newState));
        currentState = newState;
    }
}

void SessionManager::clearSession()
{
    currentSessionId = "";
    currentSessionName = "";
    currentInviteUrl = "";
    connectionInfo = SessionConnectionInfo();
    participants.clear();
    lastError = "";
}

String SessionManager::extractSessionCode(const String& input)
{
    String trimmed = input.trim();
    
    // Check if it's a URL
    if (trimmed.containsChar('/'))
    {
        // Extract last path component as session code
        int lastSlash = trimmed.lastIndexOf("/");
        if (lastSlash >= 0 && lastSlash < trimmed.length() - 1)
        {
            return trimmed.substring(lastSlash + 1);
        }
    }
    
    // Assume it's a direct code
    return trimmed;
}