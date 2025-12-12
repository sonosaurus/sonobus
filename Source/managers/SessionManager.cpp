// SPDX-License-Identifier: GPLv3-or-later WITH Appstore-exception
// Copyright (C) 2024 SoundFlip

#include "SessionManager.h"

SessionManager::SessionManager(SoundFlipAPI& apiRef)
    : api(apiRef)
{
}

SessionManager::~SessionManager()
{
}

//==============================================================================
// State Management

void SessionManager::setState(State newState)
{
    if (currentState != newState)
    {
        currentState = newState;
        sendChangeMessage();
    }
}

void SessionManager::clearCurrentSession()
{
    currentSession = SoundFlipAPI::CollabSession();
}

//==============================================================================
// Session Management

bool SessionManager::createSession(const String& name)
{
    lastError = "";
    setState(State::Creating);
    
    auto session = api.createCollabSession(name);
    
    if (session.id.isEmpty())
    {
        lastError = api.getLastError();
        setState(State::Disconnected);
        return false;
    }
    
    currentSession = session;
    setState(State::Connected);
    sendChangeMessage();
    
    return true;
}

bool SessionManager::joinSession(const String& inviteCodeOrUrl)
{
    lastError = "";
    setState(State::Joining);
    
    // Parse invite code from URL if needed
    String inviteCode = parseInviteCode(inviteCodeOrUrl);
    
    if (inviteCode.isEmpty())
    {
        lastError = "Invalid invite code or URL";
        setState(State::Disconnected);
        return false;
    }
    
    auto session = api.joinCollabSession(inviteCode);
    
    if (session.id.isEmpty())
    {
        lastError = api.getLastError();
        setState(State::Disconnected);
        return false;
    }
    
    currentSession = session;
    setState(State::Connected);
    sendChangeMessage();
    
    return true;
}

void SessionManager::leaveSession()
{
    if (currentSession.id.isEmpty())
        return;
    
    // Call API to leave
    api.leaveCollabSession(currentSession.id);
    
    clearCurrentSession();
    setState(State::Disconnected);
    sendChangeMessage();
}

bool SessionManager::endSession()
{
    if (currentSession.id.isEmpty())
    {
        lastError = "No active session";
        return false;
    }
    
    setState(State::Ending);
    
    // Update session status to ended
    auto updatedSession = api.updateCollabSession(currentSession.id, "", "ended");
    
    if (updatedSession.id.isEmpty())
    {
        lastError = api.getLastError();
        setState(State::Connected); // Revert state
        return false;
    }
    
    clearCurrentSession();
    setState(State::Disconnected);
    sendChangeMessage();
    
    return true;
}

//==============================================================================
// Recent Sessions

Array<SoundFlipAPI::CollabSession> SessionManager::fetchRecentSessions(int limit)
{
    recentSessions = api.listCollabSessions("", limit, 0);
    sendChangeMessage();
    return recentSessions;
}

//==============================================================================
// Stem Management

bool SessionManager::uploadStem(const File& audioFile, int durationSeconds)
{
    if (currentSession.id.isEmpty())
    {
        lastError = "No active session";
        return false;
    }
    
    if (!audioFile.existsAsFile())
    {
        lastError = "File does not exist";
        return false;
    }
    
    // Determine content type from file extension
    String extension = audioFile.getFileExtension().toLowerCase();
    String contentType = "audio/wav"; // Default
    
    if (extension == ".mp3")
        contentType = "audio/mpeg";
    else if (extension == ".ogg")
        contentType = "audio/ogg";
    else if (extension == ".flac")
        contentType = "audio/flac";
    else if (extension == ".aiff" || extension == ".aif")
        contentType = "audio/aiff";
    
    // Step 1: Request upload URL
    auto uploadInfo = api.requestStemUploadUrl(
        currentSession.id,
        audioFile.getFileName(),
        contentType,
        audioFile.getSize()
    );
    
    if (uploadInfo.uploadUrl.isEmpty())
    {
        lastError = api.getLastError();
        return false;
    }
    
    // Step 2: Upload to S3
    bool uploaded = api.uploadFileToS3(uploadInfo.uploadUrl, audioFile, contentType);
    
    if (!uploaded)
    {
        lastError = api.getLastError();
        return false;
    }
    
    // Step 3: Confirm upload complete
    auto stem = api.completeStemUpload(currentSession.id, uploadInfo.stemId, durationSeconds);
    
    if (stem.id.isEmpty())
    {
        lastError = api.getLastError();
        return false;
    }
    
    // Update stem count in current session
    currentSession.stemCount++;
    sendChangeMessage();
    
    return true;
}

Array<SoundFlipAPI::Stem> SessionManager::fetchSessionStems()
{
    if (currentSession.id.isEmpty())
        return Array<SoundFlipAPI::Stem>();
    
    return api.listSessionStems(currentSession.id);
}

Array<SoundFlipAPI::Stem> SessionManager::fetchSessionStems(const String& sessionId)
{
    return api.listSessionStems(sessionId);
}

bool SessionManager::downloadStem(const SoundFlipAPI::Stem& stem, const File& destinationFile)
{
    if (stem.downloadUrl.isEmpty())
    {
        lastError = "No download URL for stem";
        return false;
    }
    
    // Download from presigned URL
    URL url(stem.downloadUrl);
    
    auto options = URL::InputStreamOptions(URL::ParameterHandling::inAddress)
        .withConnectionTimeoutMs(300000); // 5 min timeout
    
    auto stream = url.createInputStream(options);
    
    if (stream == nullptr)
    {
        lastError = "Failed to connect to download URL";
        return false;
    }
    
    // Write to file
    FileOutputStream outputStream(destinationFile);
    
    if (!outputStream.openedOk())
    {
        lastError = "Failed to create output file";
        return false;
    }
    
    outputStream.writeFromInputStream(*stream, -1);
    
    return true;
}

bool SessionManager::deleteStem(const String& stemId)
{
    if (currentSession.id.isEmpty())
    {
        lastError = "No active session";
        return false;
    }
    
    bool success = api.deleteStem(currentSession.id, stemId);
    
    if (!success)
    {
        lastError = api.getLastError();
        return false;
    }
    
    // Update stem count
    if (currentSession.stemCount > 0)
        currentSession.stemCount--;
    
    sendChangeMessage();
    
    return true;
}

//==============================================================================
// Helpers

String SessionManager::parseInviteCode(const String& input)
{
    String trimmed = input.trim();
    
    if (trimmed.isEmpty())
        return "";
    
    // Check if it's a URL
    if (trimmed.containsChar('/'))
    {
        // Try to extract invite code from URL patterns:
        // https://soundflip.xyz/session/ABC123
        // soundflip.xyz/session/ABC123
        // /session/ABC123
        
        // Look for /session/ pattern
        int sessionIndex = trimmed.indexOf("/session/");
        if (sessionIndex >= 0)
        {
            String code = trimmed.substring(sessionIndex + 9); // Length of "/session/"
            
            // Remove any trailing path or query params
            int slashIndex = code.indexOf("/");
            if (slashIndex > 0)
                code = code.substring(0, slashIndex);
            
            int queryIndex = code.indexOf("?");
            if (queryIndex > 0)
                code = code.substring(0, queryIndex);
            
            return code.trim();
        }
        
        // Check for invite= query param
        int inviteIndex = trimmed.indexOf("invite=");
        if (inviteIndex >= 0)
        {
            String code = trimmed.substring(inviteIndex + 7); // Length of "invite="
            
            int ampIndex = code.indexOf("&");
            if (ampIndex > 0)
                code = code.substring(0, ampIndex);
            
            return code.trim();
        }
    }
    
    // Assume it's a raw invite code
    // Validate: should be 8 alphanumeric characters
    if (trimmed.length() == 8 && trimmed.containsOnly("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789"))
    {
        return trimmed;
    }
    
    // If it's longer or different format, still return it and let the API validate
    return trimmed;
}