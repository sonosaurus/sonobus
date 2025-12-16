// SPDX-License-Identifier: GPLv3-or-later WITH Appstore-exception
// Copyright (C) 2024 SoundFlip

#pragma once

#include "JuceHeader.h"
#include "SoundFlipAuth.h"
#include "SoundFlipWebSocket.h"

class SoundFlipAPI
{
public:
    SoundFlipAPI(SoundFlipAuth& auth);
    ~SoundFlipAPI();

    //==============================================================================
    // Connection Info (returned when creating/joining sessions)
    
    struct ConnectionInfo
    {
        String server;
        int port;
        String group;
        String password;
    };

    //==============================================================================
    // Participant Info
    
    struct Participant
    {
        String odId;
        String username;
        String avatar;
        int64 joinedAt;
        int64 leftAt;  // 0 if still active
    };

    //==============================================================================
    // Collab Session Management
    
    struct CollabSession
    {
        String id;
        String inviteCode;
        String name;
        String status;  // "active", "ended", "archived"
        String inviteUrl;
        ConnectionInfo connection;
        String createdById;
        String createdByUsername;
        String createdByAvatar;
        Array<Participant> participants;
        int stemCount;
        int durationSeconds;
        int64 createdAt;
        int64 endedAt;  // 0 if still active
    };
    
    /** Create a new collab session */
    CollabSession createCollabSession(const String& name = "");
    
    /** Get collab session by ID */
    CollabSession getCollabSession(const String& sessionId);
    
    /** Get collab session by invite code */
    CollabSession getCollabSessionByInviteCode(const String& inviteCode);
    
    /** Join a collab session using session ID or invite code */
    CollabSession joinCollabSession(const String& sessionIdOrInviteCode);
    
    /** Leave a collab session */
    bool leaveCollabSession(const String& sessionId);
    
    /** Update a collab session (name, status) - creator only */
    CollabSession updateCollabSession(const String& sessionId, 
                                      const String& name = "", 
                                      const String& status = "");
    
    /** List user's collab sessions */
    Array<CollabSession> listCollabSessions(const String& status = "", 
                                            int limit = 20, 
                                            int offset = 0);

    //==============================================================================
    // Stem Management
    
    struct UploadUrlResponse
    {
        String uploadUrl;
        String stemId;
        String s3Key;
        int expiresIn;
    };
    
    struct Stem
    {
        String id;
        String filename;
        String uploadedById;
        String uploadedByUsername;
        String uploadedByAvatar;
        String downloadUrl;
        int64 sizeBytes;
        int durationSeconds;
        int64 createdAt;
    };
    
    /** Request presigned URL for stem upload */
    UploadUrlResponse requestStemUploadUrl(const String& sessionId, 
                                           const String& filename, 
                                           const String& contentType,
                                           int64 sizeBytes);
    
    /** Mark stem upload as complete */
    Stem completeStemUpload(const String& sessionId, 
                            const String& stemId, 
                            int durationSeconds = 0);
    
    /** List stems for a session */
    Array<Stem> listSessionStems(const String& sessionId);
    
    /** Delete a stem */
    bool deleteStem(const String& sessionId, const String& stemId);

    //==============================================================================
    // Helper: Upload file directly to S3 using presigned URL
    
    bool uploadFileToS3(const String& presignedUrl, 
                        const File& file, 
                        const String& contentType);

    //==============================================================================
    // WebSocket Access
    
    /** Get the WebSocket instance for real-time events */
    SoundFlipWebSocket& getWebSocket() { return webSocket; }
    
    /** Get WebSocket server URL */
    String getWebSocketUrl() const { return wsBaseUrl; }

    //==============================================================================
    // Error handling
    
    String getLastError() const { return lastError; }
    int getLastStatusCode() const { return lastStatusCode; }
    String getAuthToken() const { return auth.getAccessToken(); }

private:
    var makeRequest(const String& endpoint, 
                    const String& method = "GET",
                    const var& body = var());
    
    CollabSession parseCollabSession(const var& json);
    Participant parseParticipant(const var& json);
    ConnectionInfo parseConnectionInfo(const var& json);
    Stem parseStem(const var& json);
    UploadUrlResponse parseUploadUrlResponse(const var& json);
    
    SoundFlipAuth& auth;
    SoundFlipWebSocket webSocket;
    
    //==============================================================================
    // URLs - Development vs Production
    
    // Development URL (local testing)
    String apiBaseUrl = "http://localhost:4400";
    String wsBaseUrl = "ws://localhost:4401";
    
    // Production URL (uncomment for production)
    // String apiBaseUrl = "https://api.soundflip.xyz";
    // String wsBaseUrl = "wss://api.soundflip.xyz/4401";

    String lastError;
    int lastStatusCode = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SoundFlipAPI)
};