// SPDX-License-Identifier: GPLv3-or-later WITH Appstore-exception
// Copyright (C) 2024 SoundFlip

#include "JoinSessionView.h"
#include "../managers/SessionManager.h"
#include "../SonobusPluginProcessor.h"

JoinSessionView::JoinSessionView(SessionManager* sm, SonobusAudioProcessor* proc)
    : sessionManager(sm), processor(proc)
{
    titleLabel.setText("Join Session", dontSendNotification);
    titleLabel.setFont(Font(24.0f, Font::bold));
    titleLabel.setJustificationType(Justification::centred);
    titleLabel.setColour(Label::textColourId, Colours::white);
    addAndMakeVisible(titleLabel);

    codeLabel.setText("Invite Link or Code:", dontSendNotification);
    codeLabel.setColour(Label::textColourId, Colours::white);
    addAndMakeVisible(codeLabel);

    codeEditor.setTextToShowWhenEmpty("soundflip.xyz/session/ABC123 or ABC123", Colours::grey);
    addAndMakeVisible(codeEditor);
    
    statusLabel.setText("", dontSendNotification);
    statusLabel.setJustificationType(Justification::centred);
    statusLabel.setColour(Label::textColourId, Colours::orange);
    addAndMakeVisible(statusLabel);

    joinButton.setButtonText("Join Session");
    joinButton.setColour(TextButton::buttonColourId, Colour(0xff00cec9));
    joinButton.onClick = [this]() {
        handleJoinSession();
    };
    addAndMakeVisible(joinButton);

    backButton.setButtonText("Back");
    backButton.onClick = [this]() {
        cleanupConnection();
        if (onBackClicked)
            onBackClicked();
    };
    addAndMakeVisible(backButton);

    // Register as listener if processor is available
    if (processor)
    {
        processor->addClientListener(this);
    }
}

JoinSessionView::~JoinSessionView()
{
    stopTimer();
    if (processor)
    {
        processor->removeClientListener(this);
    }
}

void JoinSessionView::setProcessor(SonobusAudioProcessor* proc)
{
    // Unregister from old processor
    if (processor)
    {
        processor->removeClientListener(this);
    }
    
    processor = proc;
    
    // Register with new processor
    if (processor)
    {
        processor->addClientListener(this);
    }
}

void JoinSessionView::setInviteCode(const String& code)
{
    codeEditor.setText(code, dontSendNotification);
}

void JoinSessionView::handleJoinSession()
{
    if (isJoiningSession || isWaitingForConnect || isWaitingForGroupJoin)
        return;
    
    if (!sessionManager)
    {
        showError("Session manager not available");
        return;
    }
    
    if (!processor)
    {
        showError("Audio processor not available");
        return;
    }
    
    String inviteInput = codeEditor.getText().trim();
    
    if (inviteInput.isEmpty())
    {
        showError("Please enter an invite link or code");
        return;
    }
    
    // Update UI state
    isJoiningSession = true;
    setUIEnabled(false);
    showStatus("Looking up session...");
    
    // Join session via API - SessionManager handles URL/code parsing
    bool success = sessionManager->joinSession(inviteInput);
    
    if (!success)
    {
        isJoiningSession = false;
        setUIEnabled(true);
        showError(sessionManager->getLastError());
        return;
    }
    
    DBG("Joined session via API: " + sessionManager->getCurrentSessionId());
    DBG("Session name: " + sessionManager->getCurrentSessionName());
    showStatus("Connecting to server...");
    
    // Get connection info from SessionManager
    auto connectionInfo = sessionManager->getConnectionInfo();
    
    // Store for later use in callbacks
    pendingSessionId = sessionManager->getCurrentSessionId();
    pendingGroupName = connectionInfo.group;
    pendingGroupPassword = connectionInfo.password;
    
    DBG("Connection info - Server: " + connectionInfo.server + 
        ":" + String(connectionInfo.port) + 
        " Group: " + pendingGroupName);
    
    // Get current username from processor or use a default
    String username = processor->getCurrentUsername();
    if (username.isEmpty())
    {
        username = "User_" + String(Random::getSystemRandom().nextInt(9999));
    }
    
    // Disconnect first if already connected to a different server/group
    if (processor->isConnectedToServer())
    {
        DBG("Already connected, disconnecting first...");
        processor->disconnectFromServer();
    }
    
    // Start timeout timer
    connectionCheckCount = 0;
    startTimer(100);
    
    // Initiate connection to AOO server - this is ASYNC!
    isWaitingForConnect = true;
    isJoiningSession = false;
    
    bool initiated = processor->connectToServer(
        connectionInfo.server,
        connectionInfo.port,
        username,
        "" // user password (not group password)
    );
    
    if (!initiated)
    {
        stopTimer();
        isWaitingForConnect = false;
        setUIEnabled(true);
        showError("Failed to initiate server connection");
        return;
    }
    
    DBG("Connection initiated, waiting for callback...");
    // Now we wait for aooClientConnected callback...
}

void JoinSessionView::aooClientConnected(SonobusAudioProcessor* proc, bool success, const String& errmesg)
{
    // Make sure this is for our connection attempt
    if (!isWaitingForConnect || proc != processor)
        return;
    
    isWaitingForConnect = false;
    
    DBG("aooClientConnected callback - success: " + String(success ? "yes" : "no") + " error: " + errmesg);
    
    if (!success)
    {
        stopTimer();
        setUIEnabled(true);
        showError("Connection failed: " + (errmesg.isEmpty() ? "Unknown error" : errmesg));
        return;
    }
    
    DBG("Connected to server, now joining group: " + pendingGroupName);
    showStatus("Joining session...");
    
    // NOW we can join the group since we're connected
    isWaitingForGroupJoin = true;
    
    bool joinInitiated = processor->joinServerGroup(
        pendingGroupName,
        pendingGroupPassword,
        false // not public
    );
    
    if (!joinInitiated)
    {
        stopTimer();
        isWaitingForGroupJoin = false;
        processor->disconnectFromServer();
        setUIEnabled(true);
        showError("Failed to initiate group join");
        return;
    }
    
    DBG("Group join initiated, waiting for callback...");
    // Now we wait for aooClientGroupJoined callback...
}

void JoinSessionView::aooClientDisconnected(SonobusAudioProcessor* proc, bool success, const String& errmesg)
{
    // Handle unexpected disconnection during our connection flow
    if ((isWaitingForConnect || isWaitingForGroupJoin) && proc == processor)
    {
        DBG("Unexpected disconnection during connection flow");
        stopTimer();
        isWaitingForConnect = false;
        isWaitingForGroupJoin = false;
        setUIEnabled(true);
        showError("Disconnected: " + (errmesg.isEmpty() ? "Connection lost" : errmesg));
    }
}

void JoinSessionView::aooClientGroupJoined(SonobusAudioProcessor* proc, bool success, const String& group, const String& errmesg)
{
    // Make sure this is for our group join attempt
    if (!isWaitingForGroupJoin || proc != processor || group != pendingGroupName)
        return;
    
    stopTimer();
    isWaitingForGroupJoin = false;
    
    DBG("aooClientGroupJoined callback - group: " + group + " success: " + String(success ? "yes" : "no") + " error: " + errmesg);
    
    if (!success)
    {
        processor->disconnectFromServer();
        setUIEnabled(true);
        showError("Failed to join session: " + (errmesg.isEmpty() ? "Unknown error" : errmesg));
        return;
    }
    
    DBG("Successfully joined group: " + group);
    
    showStatus("Connected!");
    statusLabel.setColour(Label::textColourId, Colours::green);
    
    // Small delay to show success message before transitioning
    Timer::callAfterDelay(300, [this]() {
        if (onJoinClicked)
            onJoinClicked();
        
        if (onSessionJoined)
            onSessionJoined(pendingSessionId);
    });
}

void JoinSessionView::timerCallback()
{
    connectionCheckCount++;
    
    // Timeout check
    if (connectionCheckCount >= maxConnectionChecks)
    {
        DBG("Connection timeout after " + String(connectionCheckCount * 100) + "ms");
        cleanupConnection();
        showError("Connection timed out");
        return;
    }
    
    // Update status with animated dots
    int dots = (connectionCheckCount / 5) % 4;
    String dotStr = String::repeatedString(".", dots);
    
    if (isWaitingForConnect)
    {
        showStatus("Connecting to server" + dotStr);
    }
    else if (isWaitingForGroupJoin)
    {
        showStatus("Joining session" + dotStr);
    }
}

void JoinSessionView::cleanupConnection()
{
    stopTimer();
    
    bool wasConnecting = isWaitingForConnect || isWaitingForGroupJoin;
    
    isWaitingForConnect = false;
    isWaitingForGroupJoin = false;
    isJoiningSession = false;
    
    if (wasConnecting && processor)
    {
        if (processor->isConnectedToServer())
        {
            if (!pendingGroupName.isEmpty())
            {
                processor->leaveServerGroup(pendingGroupName);
            }
            processor->disconnectFromServer();
        }
    }
    
    setUIEnabled(true);
}

void JoinSessionView::showError(const String& message)
{
    DBG("JoinSessionView Error: " + message);
    statusLabel.setText(message.isEmpty() ? "An error occurred" : message, dontSendNotification);
    statusLabel.setColour(Label::textColourId, Colour(0xffe74c3c));
}

void JoinSessionView::showStatus(const String& message)
{
    statusLabel.setText(message, dontSendNotification);
    statusLabel.setColour(Label::textColourId, Colours::white);
}

void JoinSessionView::setUIEnabled(bool enabled)
{
    codeEditor.setEnabled(enabled);
    joinButton.setEnabled(enabled);
    backButton.setEnabled(enabled);
    
    joinButton.setButtonText(enabled ? "Join Session" : "Joining...");
}

void JoinSessionView::paint(Graphics& g)
{
    g.fillAll(Colour(0xff1a1a2e));
}

void JoinSessionView::resized()
{
    auto bounds = getLocalBounds().reduced(40);
    
    titleLabel.setBounds(bounds.removeFromTop(40));
    bounds.removeFromTop(30);
    
    auto row = bounds.removeFromTop(30);
    codeLabel.setBounds(row.removeFromLeft(140));
    codeEditor.setBounds(row);
    
    bounds.removeFromTop(20);
    
    statusLabel.setBounds(bounds.removeFromTop(25));
    
    bounds.removeFromTop(20);
    
    int buttonWidth = 150;
    auto buttonArea = bounds.removeFromTop(40);
    int totalWidth = buttonWidth * 2 + 20;
    int startX = (buttonArea.getWidth() - totalWidth) / 2;
    
    backButton.setBounds(buttonArea.getX() + startX, buttonArea.getY(), buttonWidth, 40);
    joinButton.setBounds(buttonArea.getX() + startX + buttonWidth + 20, buttonArea.getY(), buttonWidth, 40);
}