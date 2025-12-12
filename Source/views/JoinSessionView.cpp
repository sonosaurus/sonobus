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
        stopTimer();
        if (onBackClicked)
            onBackClicked();
    };
    addAndMakeVisible(backButton);
}

void JoinSessionView::setInviteCode(const String& code)
{
    codeEditor.setText(code, dontSendNotification);
}

void JoinSessionView::handleJoinSession()
{
    if (isJoiningSession || isWaitingForConnection)
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
    
    // Store for connection check
    pendingSessionId = sessionManager->getCurrentSessionId();
    pendingGroupName = connectionInfo.group;
    
    // Get current username from processor or use a default
    String username = processor->getCurrentUsername();
    if (username.isEmpty())
    {
        username = "User_" + String(Random::getSystemRandom().nextInt(9999));
    }
    
    // Connect to AOO server
    bool connected = processor->connectToServer(
        connectionInfo.server,
        connectionInfo.port,
        username,
        ""
    );
    
    if (!connected)
    {
        isJoiningSession = false;
        setUIEnabled(true);
        showError("Failed to connect to server");
        return;
    }
    
    // Join the group with the session password
    showStatus("Joining group...");
    
    bool joined = processor->joinServerGroup(
        connectionInfo.group,
        connectionInfo.password,
        false
    );
    
    if (!joined)
    {
        processor->disconnectFromServer();
        isJoiningSession = false;
        setUIEnabled(true);
        showError("Failed to join session group");
        return;
    }
    
    // Start polling for connection confirmation
    isJoiningSession = false;
    isWaitingForConnection = true;
    connectionCheckCount = 0;
    startTimer(100);
}

void JoinSessionView::timerCallback()
{
    checkConnectionStatus();
}

void JoinSessionView::checkConnectionStatus()
{
    connectionCheckCount++;
    
    if (!processor)
    {
        stopTimer();
        isWaitingForConnection = false;
        setUIEnabled(true);
        showError("Lost connection to processor");
        return;
    }
    
    // Check if we're connected and in the right group
    if (processor->isConnectedToServer() && 
        processor->getCurrentJoinedGroup() == pendingGroupName)
    {
        stopTimer();
        isWaitingForConnection = false;
        
        showStatus("Connected!");
        statusLabel.setColour(Label::textColourId, Colours::green);
        
        // Small delay to show success message
        Timer::callAfterDelay(300, [this]() {
            if (onJoinClicked)
                onJoinClicked();
            
            if (onSessionJoined)
                onSessionJoined(pendingSessionId);
        });
        
        return;
    }
    
    // Timeout check
    if (connectionCheckCount >= maxConnectionChecks)
    {
        stopTimer();
        isWaitingForConnection = false;
        setUIEnabled(true);
        
        // Clean up
        if (processor->isConnectedToServer())
        {
            processor->leaveServerGroup(pendingGroupName);
            processor->disconnectFromServer();
        }
        
        showError("Connection timed out");
        return;
    }
    
    // Still waiting...
    int dots = (connectionCheckCount / 5) % 4;
    String dotStr = String::repeatedString(".", dots);
    showStatus("Connecting" + dotStr);
}

void JoinSessionView::showError(const String& message)
{
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