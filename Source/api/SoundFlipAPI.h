// SPDX-License-Identifier: GPLv3-or-later WITH Appstore-exception
// Copyright (C) 2024 SoundFlip

#pragma once

#include "JuceHeader.h"
#include "SoundFlipAuth.h"

class SoundFlipAPI
{
public:
    SoundFlipAPI(SoundFlipAuth& auth);
    ~SoundFlipAPI();

    //==============================================================================
    // Session Management
    
    struct Session
    {
        String id;
        String name;
        String description;
        String hostUserId;
        String connectionCode;
        String status;
        int64 createdAt;
        int64 updatedAt;
    };
    
    /** Create a new session */
    Session createSession(const String& name, const String& description = "");
    
    /** Get session by ID */
    Session getSession(const String& sessionId);
    
    /** Join a session using invite code */
    Session joinSession(const String& inviteCode);
    
    /** Leave current session */
    bool leaveSession(const String& sessionId);
    
    /** End a session (host only) */
    bool endSession(const String& sessionId);
    
    /** List user's sessions */
    Array<Session> listSessions();

    //==============================================================================
    // Stem Management
    
    struct Stem
    {
        String id;
        String sessionId;
        String userId;
        String fileName;
        String fileUrl;
        int64 fileSize;
        String status;
        int64 createdAt;
    };
    
    /** Get upload URL for a stem */
    String getUploadUrl(const String& sessionId, const String& fileName, int64 fileSize);
    
    /** Mark upload as complete */
    Stem completeUpload(const String& sessionId, const String& uploadId);
    
    /** List stems for a session */
    Array<Stem> listStems(const String& sessionId);
    
    /** Get download URL for a stem */
    String getDownloadUrl(const String& stemId);
    
    /** Delete a stem */
    bool deleteStem(const String& stemId);

    //==============================================================================
    // Error handling
    
    String getLastError() const { return lastError; }
    int getLastStatusCode() const { return lastStatusCode; }

private:
    var makeRequest(const String& endpoint, 
                    const String& method = "GET",
                    const var& body = var());
    
    Session parseSession(const var& json);
    Stem parseStem(const var& json);
    
    SoundFlipAuth& auth;
    
    //==============================================================================
    // URLs - Development vs Production
    
    // Development URL (local testing)
    String apiBaseUrl = "http://localhost:4400/api";  // NestJS API
    
    // Production URL (uncomment this and comment above for production)
    // String apiBaseUrl = "https://api.soundflip.xyz";  // NestJS API

    String lastError;
    int lastStatusCode = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SoundFlipAPI)
};