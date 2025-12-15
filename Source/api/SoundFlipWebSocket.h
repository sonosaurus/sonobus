// SPDX-License-Identifier: GPLv3-or-later WITH Appstore-exception
// Copyright (C) 2024 SoundFlip

#pragma once

#include "JuceHeader.h"

class SoundFlipWebSocket : private Thread,
                           private Timer
{
public:
    SoundFlipWebSocket();
    ~SoundFlipWebSocket() override;

    //==============================================================================
    // Connection Management
    
    bool connect(const String& serverUrl, const String& authToken);
    void disconnect();
    bool isConnected() const { return connected.load(); }

    //==============================================================================
    // Room Management
    
    void joinSession(const String& sessionId);
    void leaveSession();
    String getCurrentSessionId() const { return currentSessionId; }

    //==============================================================================
    // Event Callbacks
    
    std::function<void()> onConnected;
    std::function<void()> onDisconnected;
    std::function<void(const String& error)> onConnectionError;
    std::function<void(const String& sessionId)> onSessionJoined;
    std::function<void(const String& sessionId)> onSessionLeft;
    std::function<void(const String& sessionId, 
                       const String& odId, 
                       const String& username,
                       const String& avatar)> onParticipantJoined;
    std::function<void(const String& sessionId, 
                       const String& odId, 
                       const String& username)> onParticipantLeft;
    std::function<void(const String& sessionId,
                       const String& stemId,
                       const String& filename,
                       const String& uploadedById,
                       const String& uploadedByUsername,
                       int64 sizeBytes,
                       int durationSeconds)> onStemUploaded;
    std::function<void(const String& sessionId,
                       const String& stemId,
                       const String& deletedById)> onStemDeleted;
    std::function<void(const String& sessionId,
                       const String& name,
                       const String& status)> onSessionUpdated;

private:
    void run() override;
    void timerCallback() override;

    void processMessage(const String& message);
    void sendMessage(const String& event, const var& data);
    void sendPing();
    void attemptReconnect();
    void handleEvent(const String& event, const var& data);
    
    bool performWebSocketHandshake();
    bool sendWebSocketFrame(const String& message);
    String receiveWebSocketFrame();
    String generateWebSocketKey();
    
    //==============================================================================
    // Socket connection
    std::unique_ptr<StreamingSocket> socket;
    
    //==============================================================================
    // Connection state
    std::atomic<bool> connected { false };
    std::atomic<bool> shouldReconnect { false };
    std::atomic<bool> stopRequested { false };
    
    String serverHost;
    int serverPort = 443;
    String serverPath;
    String authToken;
    String currentSessionId;
    bool useSSL = false;
    
    //==============================================================================
    // Reconnection handling
    int reconnectAttempts = 0;
    static constexpr int MAX_RECONNECT_ATTEMPTS = 5;
    static constexpr int RECONNECT_DELAY_MS = 2000;
    static constexpr int PING_INTERVAL_MS = 25000;
    
    //==============================================================================
    // Thread safety
    CriticalSection lock;
    
    //==============================================================================
    // Ping tracking
    int64 lastPingTime = 0;
    int64 lastPongTime = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SoundFlipWebSocket)
};