// SPDX-License-Identifier: GPLv3-or-later WITH Appstore-exception
// Copyright (C) 2024 SoundFlip

#include "SoundFlipWebSocket.h"
#include <random>
#include <iostream>

#define WS_LOG(msg) std::cerr << "[SoundFlipWebSocket] " << msg << std::endl

SoundFlipWebSocket::SoundFlipWebSocket()
    : Thread("SoundFlipWebSocket")
{
    WS_LOG("Constructor called");
}

SoundFlipWebSocket::~SoundFlipWebSocket()
{
    WS_LOG("Destructor called");
    disconnect();
}

//==============================================================================
// Connection Management

bool SoundFlipWebSocket::connect(const String& url, const String& token)
{
    WS_LOG("connect() called with URL: " << url.toStdString());
    
    ScopedLock sl(lock);
    
    if (connected.load())
    {
        WS_LOG("Already connected, returning true");
        DBG("SoundFlipWebSocket: Already connected");
        return true;
    }
    
    authToken = token;
    stopRequested = false;
    shouldReconnect = true;
    reconnectAttempts = 0;
    
    // Parse URL: ws://host:port/path or wss://host:port/path
    String urlToParse = url;
    
    if (urlToParse.startsWith("wss://"))
    {
        useSSL = true;
        urlToParse = urlToParse.substring(6);
        serverPort = 443;
        WS_LOG("Using SSL (wss://)");
    }
    else if (urlToParse.startsWith("ws://"))
    {
        useSSL = false;
        urlToParse = urlToParse.substring(5);
        serverPort = 80;
        WS_LOG("Using plain WebSocket (ws://)");
    }
    
    // Extract host, port, and path
    int pathStart = urlToParse.indexOf("/");
    String hostPort;
    
    if (pathStart >= 0)
    {
        hostPort = urlToParse.substring(0, pathStart);
        serverPath = urlToParse.substring(pathStart);
    }
    else
    {
        hostPort = urlToParse;
        serverPath = "/";
    }
    
    int colonPos = hostPort.indexOf(":");
    if (colonPos >= 0)
    {
        serverHost = hostPort.substring(0, colonPos);
        serverPort = hostPort.substring(colonPos + 1).getIntValue();
    }
    else
    {
        serverHost = hostPort;
    }
    
    // Add token to path
    if (serverPath.contains("?"))
        serverPath += "&token=" + URL::addEscapeChars(token, true);
    else
        serverPath += "?token=" + URL::addEscapeChars(token, true);
    
    WS_LOG("Parsed - Host: " << serverHost.toStdString() << ", Port: " << serverPort << ", Path length: " << serverPath.length());
    DBG("SoundFlipWebSocket: Connecting to " + serverHost + ":" + String(serverPort));
    
    // Create socket
    WS_LOG("Creating StreamingSocket...");
    socket = std::make_unique<StreamingSocket>();
    
    WS_LOG("Attempting TCP connection to " << serverHost.toStdString() << ":" << serverPort);
    if (!socket->connect(serverHost, serverPort, 5000))
    {
        WS_LOG("ERROR: Failed to connect TCP socket!");
        DBG("SoundFlipWebSocket: Failed to connect socket");
        socket.reset();
        return false;
    }
    WS_LOG("TCP socket connected successfully");
    
    // Perform WebSocket handshake
    WS_LOG("Performing WebSocket handshake...");
    if (!performWebSocketHandshake())
    {
        WS_LOG("ERROR: WebSocket handshake failed!");
        DBG("SoundFlipWebSocket: WebSocket handshake failed");
        socket.reset();
        return false;
    }
    WS_LOG("WebSocket handshake successful");
    
    // Start the message processing thread
    WS_LOG("Starting message processing thread...");
    startThread();
    WS_LOG("Thread started, connect() returning true");
    
    // Start ping timer
    startTimer(PING_INTERVAL_MS);
    
    return true;
}

void SoundFlipWebSocket::disconnect()
{
    WS_LOG("disconnect() called");
    DBG("SoundFlipWebSocket: Disconnecting");
    
    stopTimer();
    
    {
        ScopedLock sl(lock);
        stopRequested = true;
        shouldReconnect = false;
    }
    
    if (currentSessionId.isNotEmpty())
    {
        leaveSession();
    }
    
    if (socket != nullptr)
    {
        socket->close();
    }
    
    stopThread(2000);
    
    {
        ScopedLock sl(lock);
        socket.reset();
        connected = false;
        currentSessionId = "";
    }
    
    WS_LOG("disconnect() complete");
    DBG("SoundFlipWebSocket: Disconnected");
}

//==============================================================================
// WebSocket Handshake

String SoundFlipWebSocket::generateWebSocketKey()
{
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 255);
    
    uint8 keyBytes[16];
    for (int i = 0; i < 16; ++i)
        keyBytes[i] = static_cast<uint8>(dis(gen));
    
    return Base64::toBase64(keyBytes, 16);
}

bool SoundFlipWebSocket::performWebSocketHandshake()
{
    if (socket == nullptr)
    {
        WS_LOG("performWebSocketHandshake: socket is null!");
        return false;
    }
    
    String wsKey = generateWebSocketKey();
    
    // Build HTTP upgrade request
    String request;
    request << "GET " << serverPath << " HTTP/1.1\r\n";
    request << "Host: " << serverHost << ":" << serverPort << "\r\n";
    request << "Upgrade: websocket\r\n";
    request << "Connection: Upgrade\r\n";
    request << "Sec-WebSocket-Key: " << wsKey << "\r\n";
    request << "Sec-WebSocket-Version: 13\r\n";
    request << "\r\n";
    
    WS_LOG("Sending handshake request (" << request.getNumBytesAsUTF8() << " bytes)...");
    
    // Send request
    int bytesSent = socket->write(request.toRawUTF8(), (int)request.getNumBytesAsUTF8());
    if (bytesSent < 0)
    {
        WS_LOG("ERROR: Failed to send handshake request (write returned " << bytesSent << ")");
        DBG("SoundFlipWebSocket: Failed to send handshake request");
        return false;
    }
    WS_LOG("Sent " << bytesSent << " bytes");
    
    // Wait for response to be available (5 second timeout)
    WS_LOG("Waiting for handshake response...");
    int readyStatus = socket->waitUntilReady(true, 5000);
    
    if (readyStatus < 0)
    {
        WS_LOG("ERROR: Socket error while waiting for handshake response");
        return false;
    }
    else if (readyStatus == 0)
    {
        WS_LOG("ERROR: Timeout waiting for handshake response");
        return false;
    }
    
    WS_LOG("Socket ready, reading response...");
    
    // Read response (non-blocking since we know data is available)
    char buffer[4096];
    int bytesRead = socket->read(buffer, sizeof(buffer) - 1, false);
    
    if (bytesRead <= 0)
    {
        WS_LOG("ERROR: No handshake response received (bytesRead=" << bytesRead << ")");
        DBG("SoundFlipWebSocket: No handshake response received");
        return false;
    }
    
    buffer[bytesRead] = '\0';
    String response(buffer);
    
    WS_LOG("Received handshake response (" << bytesRead << " bytes):");
    WS_LOG(response.substring(0, 150).toStdString());
    
    // Check for successful upgrade
    if (!response.contains("101") || !response.containsIgnoreCase("Upgrade"))
    {
        WS_LOG("ERROR: Handshake failed - response doesn't contain 101 or Upgrade");
        WS_LOG("Full response: " << response.toStdString());
        DBG("SoundFlipWebSocket: Handshake failed: " + response);
        return false;
    }
    
    WS_LOG("Handshake validated - got 101 Switching Protocols");
    DBG("SoundFlipWebSocket: Handshake successful");
    return true;
}

//==============================================================================
// WebSocket Frame Handling

bool SoundFlipWebSocket::sendWebSocketFrame(const String& message)
{
    if (socket == nullptr || !socket->isConnected())
        return false;
    
    MemoryOutputStream frame;
    
    const char* data = message.toRawUTF8();
    size_t len = message.getNumBytesAsUTF8();
    
    // First byte: FIN bit + opcode (text frame = 0x81)
    frame.writeByte((char)0x81);
    
    // Second byte: MASK bit + payload length
    if (len < 126)
    {
        frame.writeByte((char)(0x80 | len));  // Client must mask
    }
    else if (len < 65536)
    {
        frame.writeByte((char)(0x80 | 126));
        frame.writeByte((char)((len >> 8) & 0xFF));
        frame.writeByte((char)(len & 0xFF));
    }
    else
    {
        frame.writeByte((char)(0x80 | 127));
        for (int i = 7; i >= 0; --i)
            frame.writeByte((char)((len >> (i * 8)) & 0xFF));
    }
    
    // Masking key (random 4 bytes)
    uint8 maskKey[4];
    Random random;
    for (int i = 0; i < 4; ++i)
        maskKey[i] = (uint8)random.nextInt(256);
    
    frame.write(maskKey, 4);
    
    // Masked payload
    for (size_t i = 0; i < len; ++i)
    {
        frame.writeByte(data[i] ^ maskKey[i % 4]);
    }
    
    const MemoryBlock& block = frame.getMemoryBlock();
    int written = socket->write(block.getData(), (int)block.getSize());
    
    return written == (int)block.getSize();
}

String SoundFlipWebSocket::receiveWebSocketFrame()
{
    if (socket == nullptr || !socket->isConnected())
        return {};
    
    // Check if data is available
    if (!socket->waitUntilReady(true, 100))
        return {};
    
    uint8 header[2];
    if (socket->read(header, 2, true) != 2)
        return {};
    
    bool fin = (header[0] & 0x80) != 0;
    int opcode = header[0] & 0x0F;
    bool masked = (header[1] & 0x80) != 0;
    uint64 payloadLen = header[1] & 0x7F;
    
    // Handle close frame
    if (opcode == 0x08)
    {
        WS_LOG("Received close frame");
        DBG("SoundFlipWebSocket: Received close frame");
        return {};
    }
    
    // Handle ping frame
    if (opcode == 0x09)
    {
        // Send pong
        uint8 pong[2] = { 0x8A, 0x00 };
        socket->write(pong, 2);
        return {};
    }
    
    // Handle pong frame
    if (opcode == 0x0A)
    {
        lastPongTime = Time::currentTimeMillis();
        return {};
    }
    
    // Extended payload length
    if (payloadLen == 126)
    {
        uint8 extLen[2];
        if (socket->read(extLen, 2, true) != 2)
            return {};
        payloadLen = (extLen[0] << 8) | extLen[1];
    }
    else if (payloadLen == 127)
    {
        uint8 extLen[8];
        if (socket->read(extLen, 8, true) != 8)
            return {};
        payloadLen = 0;
        for (int i = 0; i < 8; ++i)
            payloadLen = (payloadLen << 8) | extLen[i];
    }
    
    // Read mask if present
    uint8 maskKey[4] = {0, 0, 0, 0};
    if (masked)
    {
        if (socket->read(maskKey, 4, true) != 4)
            return {};
    }
    
    // Read payload
    if (payloadLen > 1024 * 1024)  // 1MB limit
    {
        WS_LOG("ERROR: Payload too large: " << payloadLen);
        DBG("SoundFlipWebSocket: Payload too large");
        return {};
    }
    
    HeapBlock<char> payload(payloadLen + 1);
    if (socket->read(payload.getData(), (int)payloadLen, true) != (int)payloadLen)
        return {};
    
    // Unmask if needed
    if (masked)
    {
        for (uint64 i = 0; i < payloadLen; ++i)
            payload[i] ^= maskKey[i % 4];
    }
    
    payload[payloadLen] = '\0';
    
    return String::fromUTF8(payload.getData(), (int)payloadLen);
}

//==============================================================================
// Room Management

void SoundFlipWebSocket::joinSession(const String& sessionId)
{
    WS_LOG("joinSession() called with sessionId: " << sessionId.toStdString());
    
    if (!connected.load())
    {
        WS_LOG("ERROR: Cannot join session - not connected (connected=" << connected.load() << ")");
        DBG("SoundFlipWebSocket: Cannot join session - not connected");
        return;
    }
    
    if (sessionId.isEmpty())
    {
        WS_LOG("ERROR: Cannot join session - empty sessionId");
        DBG("SoundFlipWebSocket: Cannot join session - empty sessionId");
        return;
    }
    
    if (currentSessionId.isNotEmpty() && currentSessionId != sessionId)
    {
        WS_LOG("Leaving current session before joining new one");
        leaveSession();
    }
    
    WS_LOG("Sending join_session message for " << sessionId.toStdString());
    DBG("SoundFlipWebSocket: Joining session " + sessionId);
    
    DynamicObject::Ptr data = new DynamicObject();
    data->setProperty("sessionId", sessionId);
    
    sendMessage("join_session", var(data.get()));
    
    currentSessionId = sessionId;
    WS_LOG("joinSession() complete, currentSessionId set");
}

void SoundFlipWebSocket::leaveSession()
{
    WS_LOG("leaveSession() called");
    
    if (currentSessionId.isEmpty())
    {
        WS_LOG("No current session to leave");
        return;
    }
    
    if (!connected.load())
    {
        WS_LOG("Not connected, just clearing currentSessionId");
        currentSessionId = "";
        return;
    }
    
    WS_LOG("Sending leave_session message for " << currentSessionId.toStdString());
    DBG("SoundFlipWebSocket: Leaving session " + currentSessionId);
    
    DynamicObject::Ptr data = new DynamicObject();
    data->setProperty("sessionId", currentSessionId);
    
    sendMessage("leave_session", var(data.get()));
    
    currentSessionId = "";
}

//==============================================================================
// Thread - Message Processing Loop

void SoundFlipWebSocket::run()
{
    WS_LOG("=== Thread run() STARTED ===");
    DBG("SoundFlipWebSocket: Thread started");
    
    connected = true;
    WS_LOG("Set connected = true");
    
    WS_LOG("Calling onConnected callback via MessageManager::callAsync...");
    MessageManager::callAsync([this]() {
        WS_LOG(">>> Inside MessageManager::callAsync for onConnected");
        if (onConnected)
        {
            WS_LOG(">>> onConnected callback exists, calling it now");
            onConnected();
            WS_LOG(">>> onConnected callback completed");
        }
        else
        {
            WS_LOG(">>> WARNING: onConnected callback is NULL!");
        }
    });
    
    WS_LOG("Entering message receive loop...");
    
    while (!threadShouldExit() && !stopRequested.load())
    {
        if (socket == nullptr || !socket->isConnected())
        {
            WS_LOG("Socket disconnected in loop");
            
            if (connected.load())
            {
                connected = false;
                
                MessageManager::callAsync([this]() {
                    WS_LOG("Calling onDisconnected callback");
                    if (onDisconnected)
                        onDisconnected();
                });
            }
            
            if (shouldReconnect.load() && !stopRequested.load())
            {
                WS_LOG("Attempting reconnect...");
                attemptReconnect();
            }
            else
            {
                WS_LOG("Not reconnecting, breaking loop");
                break;
            }
            
            continue;
        }
        
        String message = receiveWebSocketFrame();
        
        if (message.isNotEmpty())
        {
            WS_LOG("Received message: " << message.substring(0, 100).toStdString());
            processMessage(message);
        }
    }
    
    WS_LOG("=== Thread run() EXITING ===");
    DBG("SoundFlipWebSocket: Thread exiting");
}

//==============================================================================
// Message Handling

void SoundFlipWebSocket::processMessage(const String& message)
{
    DBG("SoundFlipWebSocket: Received: " + message);
    
    var json;
    Result parseResult = JSON::parse(message, json);
    
    if (parseResult.failed())
    {
        WS_LOG("ERROR: Failed to parse JSON: " << parseResult.getErrorMessage().toStdString());
        DBG("SoundFlipWebSocket: Failed to parse message: " + parseResult.getErrorMessage());
        return;
    }
    
    String event = json.getProperty("event", "").toString();
    var data = json.getProperty("data", var());
    
    if (event.isEmpty())
    {
        WS_LOG("WARNING: Message missing event field");
        DBG("SoundFlipWebSocket: Message missing event field");
        return;
    }
    
    WS_LOG("Processing event: " << event.toStdString());
    handleEvent(event, data);
}

void SoundFlipWebSocket::handleEvent(const String& event, const var& data)
{
    WS_LOG("handleEvent: " << event.toStdString());
    DBG("SoundFlipWebSocket: Handling event: " + event);
    
    if (event == "connected")
    {
        WS_LOG("Server acknowledged connection");
        DBG("SoundFlipWebSocket: Server acknowledged connection");
    }
    else if (event == "pong")
    {
        lastPongTime = Time::currentTimeMillis();
    }
    else if (event == "session_joined")
    {
        String sessionId = data.getProperty("sessionId", "").toString();
        WS_LOG("session_joined event for: " << sessionId.toStdString());
        
        MessageManager::callAsync([this, sessionId]() {
            if (onSessionJoined)
                onSessionJoined(sessionId);
        });
    }
    else if (event == "session_left")
    {
        String sessionId = data.getProperty("sessionId", "").toString();
        WS_LOG("session_left event for: " << sessionId.toStdString());
        
        MessageManager::callAsync([this, sessionId]() {
            if (onSessionLeft)
                onSessionLeft(sessionId);
        });
    }
    else if (event == "participant_joined")
    {
        String sessionId = data.getProperty("sessionId", "").toString();
        String odId = data.getProperty("userId", "").toString();
        String username = data.getProperty("username", "").toString();
        String avatar = data.getProperty("avatar", "").toString();
        
        WS_LOG("participant_joined: " << username.toStdString() << " in session " << sessionId.toStdString());
        
        MessageManager::callAsync([this, sessionId, odId, username, avatar]() {
            WS_LOG(">>> Dispatching onParticipantJoined for " << username.toStdString());
            if (onParticipantJoined)
            {
                WS_LOG(">>> Calling onParticipantJoined callback");
                onParticipantJoined(sessionId, odId, username, avatar);
            }
            else
            {
                WS_LOG(">>> WARNING: onParticipantJoined callback is NULL!");
            }
        });
    }
    else if (event == "participant_left")
    {
        String sessionId = data.getProperty("sessionId", "").toString();
        String odId = data.getProperty("userId", "").toString();
        String username = data.getProperty("username", "").toString();
        
        WS_LOG("participant_left: " << username.toStdString());
        
        MessageManager::callAsync([this, sessionId, odId, username]() {
            if (onParticipantLeft)
                onParticipantLeft(sessionId, odId, username);
        });
    }
    else if (event == "stem_uploaded")
    {
        String sessionId = data.getProperty("sessionId", "").toString();
        String stemId = data.getProperty("stemId", "").toString();
        String filename = data.getProperty("filename", "").toString();
        String uploadedById = data.getProperty("uploadedById", "").toString();
        String uploadedByUsername = data.getProperty("uploadedByUsername", "").toString();
        int64 sizeBytes = (int64)data.getProperty("sizeBytes", 0);
        int durationSeconds = (int)data.getProperty("durationSeconds", 0);
        
        WS_LOG("stem_uploaded: " << filename.toStdString());
        
        MessageManager::callAsync([this, sessionId, stemId, filename, uploadedById, 
                                   uploadedByUsername, sizeBytes, durationSeconds]() {
            if (onStemUploaded)
                onStemUploaded(sessionId, stemId, filename, uploadedById, 
                              uploadedByUsername, sizeBytes, durationSeconds);
        });
    }
    else if (event == "stem_deleted")
    {
        String sessionId = data.getProperty("sessionId", "").toString();
        String stemId = data.getProperty("stemId", "").toString();
        String deletedById = data.getProperty("deletedById", "").toString();
        
        WS_LOG("stem_deleted: " << stemId.toStdString());
        
        MessageManager::callAsync([this, sessionId, stemId, deletedById]() {
            if (onStemDeleted)
                onStemDeleted(sessionId, stemId, deletedById);
        });
    }
    else if (event == "session_updated")
    {
        String sessionId = data.getProperty("sessionId", "").toString();
        String name = data.getProperty("name", "").toString();
        String status = data.getProperty("status", "").toString();
        
        WS_LOG("session_updated: " << name.toStdString() << " status=" << status.toStdString());
        
        MessageManager::callAsync([this, sessionId, name, status]() {
            if (onSessionUpdated)
                onSessionUpdated(sessionId, name, status);
        });
    }
    else if (event == "error")
    {
        String errorMessage = data.getProperty("message", "Unknown error").toString();
        WS_LOG("ERROR from server: " << errorMessage.toStdString());
        DBG("SoundFlipWebSocket: Server error: " + errorMessage);
        
        MessageManager::callAsync([this, errorMessage]() {
            if (onConnectionError)
                onConnectionError(errorMessage);
        });
    }
    else
    {
        WS_LOG("Unknown event: " << event.toStdString());
        DBG("SoundFlipWebSocket: Unknown event: " + event);
    }
}

void SoundFlipWebSocket::sendMessage(const String& event, const var& data)
{
    if (!connected.load() || socket == nullptr)
    {
        WS_LOG("ERROR: Cannot send message - not connected");
        DBG("SoundFlipWebSocket: Cannot send message - not connected");
        return;
    }
    
    DynamicObject::Ptr message = new DynamicObject();
    message->setProperty("event", event);
    message->setProperty("data", data);
    
    String jsonStr = JSON::toString(var(message.get()));
    
    WS_LOG("Sending: " << jsonStr.substring(0, 100).toStdString());
    DBG("SoundFlipWebSocket: Sending: " + jsonStr);
    
    sendWebSocketFrame(jsonStr);
}

void SoundFlipWebSocket::sendPing()
{
    if (!connected.load())
        return;
    
    lastPingTime = Time::currentTimeMillis();
    sendMessage("ping", var());
}

//==============================================================================
// Timer - Ping/Pong

void SoundFlipWebSocket::timerCallback()
{
    if (connected.load())
    {
        sendPing();
        
        if (lastPingTime > 0 && lastPongTime > 0)
        {
            int64 timeSinceLastPong = Time::currentTimeMillis() - lastPongTime;
            
            if (timeSinceLastPong > PING_INTERVAL_MS * 2)
            {
                WS_LOG("WARNING: No pong received for " << timeSinceLastPong << "ms, connection may be dead");
                DBG("SoundFlipWebSocket: No pong received, connection may be dead");
                
                if (socket != nullptr)
                {
                    socket->close();
                }
            }
        }
    }
}

//==============================================================================
// Reconnection

void SoundFlipWebSocket::attemptReconnect()
{
    if (stopRequested.load() || !shouldReconnect.load())
        return;
    
    reconnectAttempts++;
    
    if (reconnectAttempts > MAX_RECONNECT_ATTEMPTS)
    {
        WS_LOG("Max reconnect attempts reached (" << MAX_RECONNECT_ATTEMPTS << ")");
        DBG("SoundFlipWebSocket: Max reconnect attempts reached");
        
        MessageManager::callAsync([this]() {
            if (onConnectionError)
                onConnectionError("Failed to reconnect after " + String(MAX_RECONNECT_ATTEMPTS) + " attempts");
        });
        
        shouldReconnect = false;
        return;
    }
    
    int delayMs = RECONNECT_DELAY_MS * reconnectAttempts;
    WS_LOG("Reconnecting in " << delayMs << "ms (attempt " << reconnectAttempts << ")");
    DBG("SoundFlipWebSocket: Reconnecting in " + String(delayMs) + "ms (attempt " + String(reconnectAttempts) + ")");
    
    Thread::sleep(delayMs);
    
    if (stopRequested.load())
        return;
    
    // Create new socket and reconnect
    {
        ScopedLock sl(lock);
        socket = std::make_unique<StreamingSocket>();
    }
    
    if (!socket->connect(serverHost, serverPort, 5000))
    {
        WS_LOG("Reconnect: TCP socket connect failed");
        DBG("SoundFlipWebSocket: Reconnect socket failed");
        socket.reset();
        return;
    }
    
    if (!performWebSocketHandshake())
    {
        WS_LOG("Reconnect: handshake failed");
        DBG("SoundFlipWebSocket: Reconnect handshake failed");
        socket.reset();
        return;
    }
    
    connected = true;
    reconnectAttempts = 0;
    
    WS_LOG("Reconnected successfully!");
    DBG("SoundFlipWebSocket: Reconnected successfully");
    
    MessageManager::callAsync([this]() {
        if (onConnected)
            onConnected();
    });
    
    // Rejoin session if we were in one
    if (currentSessionId.isNotEmpty())
    {
        String sessionToRejoin = currentSessionId;
        currentSessionId = "";
        WS_LOG("Rejoining session: " << sessionToRejoin.toStdString());
        joinSession(sessionToRejoin);
    }
}