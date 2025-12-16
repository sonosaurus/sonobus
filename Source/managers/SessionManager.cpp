// SPDX-License-Identifier: GPLv3-or-later WITH Appstore-exception
// Copyright (C) 2024 SoundFlip

#include "SessionManager.h"
#include <iostream>

// Use this macro for logging that works in both Debug and Release
#define SM_LOG(msg) std::cerr << "[SessionManager] " << msg << std::endl

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
        SM_LOG("WebSocket connected");
        DBG("SessionManager: WebSocket connected");
        webSocketConnected = true;
        
        if (currentSessionId.isNotEmpty())
        {
            SM_LOG("Auto-joining session room: " << currentSessionId.toStdString());
            DBG("SessionManager: Auto-joining session room: " + currentSessionId);
            api.getWebSocket().joinSession(currentSessionId);
        }
    };
    
    ws.onDisconnected = [this]() {
        SM_LOG("WebSocket disconnected");
        DBG("SessionManager: WebSocket disconnected");
        webSocketConnected = false;
    };
    
    ws.onConnectionError = [this](const String& error) {
        SM_LOG("WebSocket error - " << error.toStdString());
        DBG("SessionManager: WebSocket error - " + error);
        webSocketConnected = false;
    };
    
    ws.onSessionJoined = [this](const String& sessionId) {
        SM_LOG("Joined WebSocket room for session " << sessionId.toStdString());
        DBG("SessionManager: Joined WebSocket room for session " + sessionId);
    };
    
    ws.onSessionLeft = [this](const String& sessionId) {
        SM_LOG("Left WebSocket room for session " << sessionId.toStdString());
        DBG("SessionManager: Left WebSocket room for session " + sessionId);
    };
    
    ws.onParticipantJoined = [this](const String& sessionId, const String& odId, 
                                    const String& username, const String& avatar) {
        SM_LOG("Participant joined via WebSocket - " << username.toStdString());
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
        SM_LOG("Participant left via WebSocket - " << username.toStdString());
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
        SM_LOG("Stem uploaded via WebSocket - " << filename.toStdString());
        DBG("SessionManager: Stem uploaded via WebSocket - " + filename);
        
        if (onStemUploadedCallback)
            onStemUploadedCallback(sessionId, stemId, filename);
        
        sendChangeMessage();
    };
    
    ws.onStemDeleted = [this](const String& sessionId, const String& stemId,
                              const String& deletedById) {
        SM_LOG("Stem deleted via WebSocket - " << stemId.toStdString());
        DBG("SessionManager: Stem deleted via WebSocket - " + stemId);
        
        if (onStemDeletedCallback)
            onStemDeletedCallback(sessionId, stemId);
        
        sendChangeMessage();
    };
    
    ws.onSessionUpdated = [this](const String& sessionId, const String& name,
                                 const String& status) {
        SM_LOG("Session updated via WebSocket - name: " << name.toStdString() << ", status: " << status.toStdString());
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
    SM_LOG("connectWebSocket() called");
    
    if (webSocketConnected)
    {
        SM_LOG("WebSocket already connected, skipping connect");
        DBG("SessionManager: WebSocket already connected, skipping connect");
        return;
    }
    
    String wsUrl = api.getWebSocketUrl();
    String token = api.getAuthToken();
    
    SM_LOG("wsUrl = " << wsUrl.toStdString());
    SM_LOG("token length = " << token.length());
    SM_LOG("token preview = " << (token.length() > 20 ? token.substring(0, 20).toStdString() + "..." : token.toStdString()));
    
    DBG("SessionManager: connectWebSocket() called");
    DBG("SessionManager: wsUrl = " + wsUrl);
    DBG("SessionManager: token length = " + String(token.length()));
    
    if (token.isEmpty())
    {
        SM_LOG("Cannot connect WebSocket - no auth token");
        DBG("SessionManager: Cannot connect WebSocket - no auth token");
        return;
    }
    
    if (wsUrl.isEmpty())
    {
        SM_LOG("Cannot connect WebSocket - no WebSocket URL");
        DBG("SessionManager: Cannot connect WebSocket - no WebSocket URL");
        return;
    }
    
    SM_LOG("Connecting WebSocket to " << wsUrl.toStdString());
    DBG("SessionManager: Connecting WebSocket to " + wsUrl);
    
    api.getWebSocket().connect(wsUrl, token);
}

void SessionManager::disconnectWebSocket()
{
    if (!webSocketConnected)
        return;
    
    SM_LOG("Disconnecting WebSocket");
    DBG("SessionManager: Disconnecting WebSocket");
    api.getWebSocket().disconnect();
    webSocketConnected = false;
}

bool SessionManager::createSession(const String& name)
{
    SM_LOG("createSession() called with name: " << name.toStdString());
    DBG("SessionManager: createSession() called with name: " + name);
    
    if (currentState != State::Idle && currentState != State::Error)
    {
        lastError = "Already in a session";
        SM_LOG("createSession failed - " << lastError.toStdString());
        DBG("SessionManager: createSession failed - " + lastError);
        return false;
    }
    
    setState(State::CreatingSession);
    
    String sessionName = name.isEmpty() ? "SoundFlip Session" : name;
    
    auto result = api.createCollabSession(sessionName);
    
    if (result.id.isEmpty())
    {
        lastError = api.getLastError().isEmpty() ? "Failed to create session" : api.getLastError();
        SM_LOG("createSession API failed - " << lastError.toStdString());
        DBG("SessionManager: createSession API failed - " + lastError);
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
    
    SM_LOG("Created session " << currentSessionId.toStdString());
    SM_LOG("Connection - " << connectionInfo.server.toStdString() << ":" << connectionInfo.port << " group: " << connectionInfo.group.toStdString());
    DBG("SessionManager: Created session " + currentSessionId);
    DBG("SessionManager: Connection - " + connectionInfo.server + ":" + 
        String(connectionInfo.port) + " group: " + connectionInfo.group);
    
    setState(State::Connecting);
    
    // Connect WebSocket if not already connected
    SM_LOG("webSocketConnected = " << (webSocketConnected ? "true" : "false"));
    DBG("SessionManager: webSocketConnected = " + String(webSocketConnected ? "true" : "false"));
    
    if (!webSocketConnected)
    {
        SM_LOG("Calling connectWebSocket() from createSession");
        DBG("SessionManager: Calling connectWebSocket() from createSession");
        connectWebSocket();
    }
    else
    {
        SM_LOG("WebSocket already connected, joining session room");
        DBG("SessionManager: WebSocket already connected, joining session room");
        api.getWebSocket().joinSession(currentSessionId);
    }
    
    return true;
}

bool SessionManager::joinSession(const String& inviteCodeOrUrl)
{
    SM_LOG("joinSession() called with: " << inviteCodeOrUrl.toStdString());
    DBG("SessionManager: joinSession() called with: " + inviteCodeOrUrl);
    
    if (currentState != State::Idle && currentState != State::Error)
    {
        lastError = "Already in a session";
        SM_LOG("joinSession failed - " << lastError.toStdString());
        DBG("SessionManager: joinSession failed - " + lastError);
        return false;
    }
    
    setState(State::JoiningSession);
    
    String sessionCode = extractSessionCode(inviteCodeOrUrl);
    SM_LOG("Extracted session code: " << sessionCode.toStdString());
    DBG("SessionManager: Extracted session code: " + sessionCode);
    
    if (sessionCode.isEmpty())
    {
        lastError = "Invalid invite code or URL";
        SM_LOG("joinSession failed - " << lastError.toStdString());
        DBG("SessionManager: joinSession failed - " + lastError);
        setState(State::Error);
        return false;
    }
    
    auto result = api.joinCollabSession(sessionCode);
    
    if (result.id.isEmpty())
    {
        lastError = api.getLastError().isEmpty() ? "Failed to join session" : api.getLastError();
        SM_LOG("joinSession API failed - " << lastError.toStdString());
        DBG("SessionManager: joinSession API failed - " + lastError);
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
    
    SM_LOG("Joined session " << currentSessionId.toStdString() << " (" << currentSessionName.toStdString() << ")");
    SM_LOG("Participants from API: " << participants.size());
    DBG("SessionManager: Joined session " + currentSessionId + " (" + currentSessionName + ")");
    DBG("SessionManager: Participants from API: " + String(participants.size()));
    
    setState(State::Connecting);
    
    // FIX: Connect WebSocket if not already connected (was missing!)
    SM_LOG("webSocketConnected = " << (webSocketConnected ? "true" : "false"));
    DBG("SessionManager: webSocketConnected = " + String(webSocketConnected ? "true" : "false"));
    
    if (!webSocketConnected)
    {
        SM_LOG("Calling connectWebSocket() from joinSession");
        DBG("SessionManager: Calling connectWebSocket() from joinSession");
        connectWebSocket();
    }
    else
    {
        SM_LOG("WebSocket already connected, joining session room");
        DBG("SessionManager: WebSocket already connected, joining session room");
        api.getWebSocket().joinSession(currentSessionId);
    }
    
    return true;
}

void SessionManager::leaveSession()
{
    SM_LOG("leaveSession() called");
    DBG("SessionManager: leaveSession() called");
    
    if (currentState == State::Idle)
    {
        SM_LOG("Already idle, nothing to leave");
        DBG("SessionManager: Already idle, nothing to leave");
        return;
    }
    
    setState(State::Disconnecting);
    
    if (webSocketConnected && currentSessionId.isNotEmpty())
    {
        SM_LOG("Leaving WebSocket session room");
        DBG("SessionManager: Leaving WebSocket session room");
        api.getWebSocket().leaveSession();
    }
    
    if (currentSessionId.isNotEmpty())
    {
        SM_LOG("Calling API to leave session");
        DBG("SessionManager: Calling API to leave session");
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
    SM_LOG("Session connected (AOO)");
    DBG("SessionManager: Session connected (AOO)");
    setState(State::Connected);
    
    if (onSessionConnectedCallback)
        onSessionConnectedCallback();
    
    sendChangeMessage();
}

void SessionManager::onSessionDisconnected()
{
    SM_LOG("Session disconnected (AOO)");
    DBG("SessionManager: Session disconnected (AOO)");
    
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
    SM_LOG("Connection failed - " << error.toStdString());
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
    SM_LOG("Peer joined - " << username.toStdString());
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
    SM_LOG("Peer left - " << username.toStdString());
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
        SM_LOG("Starting upload for session " << sessionId.toStdString());
        DBG("SessionManager: Starting upload for session " + sessionId);
        
        File localFile;
        if (audioFile.isLocalFile())
        {
            localFile = audioFile.getLocalFile();
        }
        else
        {
            SM_LOG("Upload failed - not a local file");
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
            SM_LOG("Upload failed - file does not exist");
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
        
        SM_LOG("Requesting upload URL for " << filename.toStdString() << " (" << fileSize << " bytes)");
        DBG("SessionManager: Requesting upload URL for " + filename + " (" + String(fileSize) + " bytes)");
        
        auto uploadInfo = api.requestStemUploadUrl(sessionId, filename, contentType, fileSize);
        
        if (uploadInfo.uploadUrl.isEmpty() || uploadInfo.stemId.isEmpty())
        {
            String errorMsg = api.getLastError().isEmpty() ? "Failed to get upload URL" : api.getLastError();
            SM_LOG(errorMsg.toStdString());
            DBG("SessionManager: " + errorMsg);
            if (callback)
            {
                MessageManager::callAsync([callback, errorMsg]() {
                    callback(false, errorMsg);
                });
            }
            return;
        }
        
        SM_LOG("Got presigned URL, stemId: " << uploadInfo.stemId.toStdString());
        DBG("SessionManager: Got presigned URL, stemId: " + uploadInfo.stemId);
        
        bool s3Success = api.uploadFileToS3(uploadInfo.uploadUrl, localFile, contentType);
        
        if (!s3Success)
        {
            String errorMsg = api.getLastError().isEmpty() ? "Failed to upload to S3" : api.getLastError();
            SM_LOG("S3 upload failed - " << errorMsg.toStdString());
            DBG("SessionManager: S3 upload failed - " + errorMsg);
            if (callback)
            {
                MessageManager::callAsync([callback, errorMsg]() {
                    callback(false, errorMsg);
                });
            }
            return;
        }
        
        SM_LOG("S3 upload successful, marking complete");
        DBG("SessionManager: S3 upload successful, marking complete");
        
        auto completedStem = api.completeStemUpload(sessionId, uploadInfo.stemId);
        bool completeSuccess = completedStem.id.isNotEmpty();
        
        if (!completeSuccess)
        {
            String errorMsg = api.getLastError().isEmpty() ? "Failed to complete upload" : api.getLastError();
            SM_LOG("Complete upload failed - " << errorMsg.toStdString());
            DBG("SessionManager: Complete upload failed - " + errorMsg);
            if (callback)
            {
                MessageManager::callAsync([callback, errorMsg]() {
                    callback(false, errorMsg);
                });
            }
            return;
        }
        
        SM_LOG("Upload completed successfully");
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
        SM_LOG("State change " << (int)currentState << " -> " << (int)newState);
        DBG("SessionManager: State change " + String((int)currentState) + " -> " + String((int)newState));
        currentState = newState;
    }
}

void SessionManager::clearSession()
{
    SM_LOG("Clearing session data");
    DBG("SessionManager: Clearing session data");
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