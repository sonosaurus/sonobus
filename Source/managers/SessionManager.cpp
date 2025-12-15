// SPDX-License-Identifier: GPLv3-or-later WITH Appstore-exception
// Copyright (C) 2024 SoundFlip

#include "SessionManager.h"

SessionManager::SessionManager(SoundFlipAPI& apiRef)
    : api(apiRef)
{
    setupWebSocketCallbacks();
}

SessionManager::~SessionManager()
{
    disconnectWebSocket();
}

void SessionManager::setupWebSocketCallbacks()
{
    auto& ws = api.getWebSocket();
    
    ws.onConnected = [this]() {
        DBG("SessionManager: WebSocket connected");
        webSocketConnected = true;
        
        if (currentSessionId.isNotEmpty())
        {
            api.getWebSocket().joinSession(currentSessionId);
        }
    };
    
    ws.onDisconnected = [this]() {
        DBG("SessionManager: WebSocket disconnected");
        webSocketConnected = false;
    };
    
    ws.onConnectionError = [this](const String& error) {
        DBG("SessionManager: WebSocket error - " + error);
        webSocketConnected = false;
    };
    
    ws.onSessionJoined = [this](const String& sessionId) {
        DBG("SessionManager: Joined WebSocket room for session " + sessionId);
    };
    
    ws.onSessionLeft = [this](const String& sessionId) {
        DBG("SessionManager: Left WebSocket room for session " + sessionId);
    };
    
    ws.onParticipantJoined = [this](const String& sessionId, const String& odId, 
                                    const String& username, const String& avatar) {
        DBG("SessionManager: Participant joined via WebSocket - " + username);
        
        bool found = false;
        for (const auto& p : participants)
        {
            if (p.odId == odId)
            {
                found = true;
                break;
            }
        }
        
        if (!found)
        {
            SessionParticipant participant;
            participant.odId = odId;
            participant.username = username;
            participant.avatar = avatar;
            participants.add(participant);
            
            sendChangeMessage();
        }
        
        onPeerJoined(username);
    };
    
    ws.onParticipantLeft = [this](const String& sessionId, const String& odId, 
                                  const String& username) {
        DBG("SessionManager: Participant left via WebSocket - " + username);
        
        for (int i = participants.size() - 1; i >= 0; --i)
        {
            if (participants[i].odId == odId)
            {
                participants.remove(i);
                break;
            }
        }
        
        sendChangeMessage();
        
        onPeerLeft(username);
    };
    
    ws.onStemUploaded = [this](const String& sessionId, const String& stemId,
                               const String& filename, const String& uploadedById,
                               const String& uploadedByUsername, int64 sizeBytes,
                               int durationSeconds) {
        DBG("SessionManager: Stem uploaded via WebSocket - " + filename);
        
        if (onStemUploadedCallback)
            onStemUploadedCallback(sessionId, stemId, filename);
        
        sendChangeMessage();
    };
    
    ws.onStemDeleted = [this](const String& sessionId, const String& stemId,
                              const String& deletedById) {
        DBG("SessionManager: Stem deleted via WebSocket - " + stemId);
        
        if (onStemDeletedCallback)
            onStemDeletedCallback(sessionId, stemId);
        
        sendChangeMessage();
    };
    
    ws.onSessionUpdated = [this](const String& sessionId, const String& name,
                                 const String& status) {
        DBG("SessionManager: Session updated via WebSocket - name: " + name + ", status: " + status);
        
        if (name.isNotEmpty())
            currentSessionName = name;
        
        if (onSessionUpdatedCallback)
            onSessionUpdatedCallback(sessionId, name, status);
        
        sendChangeMessage();
    };
}

void SessionManager::connectWebSocket()
{
    if (webSocketConnected)
        return;
    
    DBG("SessionManager: Connecting WebSocket to " + api.getWebSocketUrl());
}

void SessionManager::disconnectWebSocket()
{
    if (!webSocketConnected)
        return;
    
    api.getWebSocket().disconnect();
    webSocketConnected = false;
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
    
    if (webSocketConnected)
    {
        api.getWebSocket().joinSession(currentSessionId);
    }
    
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
    
    participants.clear();
    for (const auto& p : result.participants)
    {
        SessionParticipant participant;
        participant.odId = p.odId;
        participant.username = p.username;
        participant.avatar = p.avatar;
        participants.add(participant);
    }
    
    DBG("SessionManager: Joined session " + currentSessionId + " (" + currentSessionName + ")");
    
    setState(State::Connecting);
    
    if (webSocketConnected)
    {
        api.getWebSocket().joinSession(currentSessionId);
    }
    
    return true;
}

void SessionManager::leaveSession()
{
    if (currentState == State::Idle)
        return;
    
    setState(State::Disconnecting);
    
    if (webSocketConnected && currentSessionId.isNotEmpty())
    {
        api.getWebSocket().leaveSession();
    }
    
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
        
        for (const auto& p : session.participants)
        {
            SessionParticipant participant;
            participant.odId = p.odId;
            participant.username = p.username;
            participant.avatar = p.avatar;
            info.participants.add(participant);
        }
        
        recentSessions.add(info);
    }
    
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
    
    bool found = false;
    for (const auto& p : participants)
    {
        if (p.username == username)
        {
            found = true;
            break;
        }
    }
    
    if (!found)
    {
        SessionParticipant participant;
        participant.username = username;
        participants.add(participant);
    }
    
    sendChangeMessage();
}

void SessionManager::onPeerLeft(const String& username)
{
    DBG("SessionManager: Peer left - " + username);
    
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
    
    Thread::launch([this, audioFile, callback, sessionId = currentSessionId]() {
        DBG("SessionManager: Starting upload for session " + sessionId);
        
        File localFile;
        if (audioFile.isLocalFile())
        {
            localFile = audioFile.getLocalFile();
        }
        else
        {
            DBG("SessionManager: Upload failed - not a local file");
            if (callback)
            {
                MessageManager::callAsync([callback]() {
                    callback(false, "Only local files can be uploaded");
                });
            }
            return;
        }
        
        if (!localFile.existsAsFile())
        {
            DBG("SessionManager: Upload failed - file does not exist");
            if (callback)
            {
                MessageManager::callAsync([callback]() {
                    callback(false, "File does not exist");
                });
            }
            return;
        }
        
        String filename = localFile.getFileName();
        int64 fileSize = localFile.getSize();
        String contentType = "audio/flac";
        
        String extension = localFile.getFileExtension().toLowerCase();
        if (extension == ".wav")
            contentType = "audio/wav";
        else if (extension == ".mp3")
            contentType = "audio/mpeg";
        else if (extension == ".ogg")
            contentType = "audio/ogg";
        else if (extension == ".aif" || extension == ".aiff")
            contentType = "audio/aiff";
        
        DBG("SessionManager: Requesting upload URL for " + filename + " (" + String(fileSize) + " bytes)");
        
        auto uploadInfo = api.requestStemUploadUrl(sessionId, filename, contentType, fileSize);
        
        if (uploadInfo.uploadUrl.isEmpty() || uploadInfo.stemId.isEmpty())
        {
            String errorMsg = api.getLastError().isEmpty() ? "Failed to get upload URL" : api.getLastError();
            DBG("SessionManager: " + errorMsg);
            if (callback)
            {
                MessageManager::callAsync([callback, errorMsg]() {
                    callback(false, errorMsg);
                });
            }
            return;
        }
        
        DBG("SessionManager: Got presigned URL, stemId: " + uploadInfo.stemId);
        
        bool s3Success = api.uploadFileToS3(uploadInfo.uploadUrl, localFile, contentType);
        
        if (!s3Success)
        {
            String errorMsg = api.getLastError().isEmpty() ? "Failed to upload to S3" : api.getLastError();
            DBG("SessionManager: S3 upload failed - " + errorMsg);
            if (callback)
            {
                MessageManager::callAsync([callback, errorMsg]() {
                    callback(false, errorMsg);
                });
            }
            return;
        }
        
        DBG("SessionManager: S3 upload successful, marking complete");
        
        auto completedStem = api.completeStemUpload(sessionId, uploadInfo.stemId);
        bool completeSuccess = completedStem.id.isNotEmpty();
        
        if (!completeSuccess)
        {
            String errorMsg = api.getLastError().isEmpty() ? "Failed to complete upload" : api.getLastError();
            DBG("SessionManager: Complete upload failed - " + errorMsg);
            if (callback)
            {
                MessageManager::callAsync([callback, errorMsg]() {
                    callback(false, errorMsg);
                });
            }
            return;
        }
        
        DBG("SessionManager: Upload completed successfully");
        
        if (callback)
        {
            MessageManager::callAsync([callback]() {
                callback(true, "Upload complete");
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
    
    if (trimmed.containsChar('/'))
    {
        int lastSlash = trimmed.lastIndexOf("/");
        if (lastSlash >= 0 && lastSlash < trimmed.length() - 1)
        {
            return trimmed.substring(lastSlash + 1);
        }
    }
    
    return trimmed;
}