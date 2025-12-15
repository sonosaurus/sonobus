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
        
        // Step 1: Get file info
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
        String contentType = "audio/flac"; // Default, could detect from extension
        
        // Detect content type from extension
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
        
        // Step 2: Request presigned upload URL from backend
        auto uploadInfo = api.requestStemUploadUrl(sessionId, filename, contentType, fileSize);
        
        // FIX: Use uploadUrl instead of presignedUrl
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
        
        // Step 3: Upload file to S3 - FIX: Use uploadUrl
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
        
        // Step 4: Mark upload as complete in backend
        // FIX: completeStemUpload returns Stem, not bool
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