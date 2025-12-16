// SPDX-License-Identifier: GPLv3-or-later WITH Appstore-exception
// Copyright (C) 2024 SoundFlip

#include "JoinSessionView.h"
#include "../managers/SessionManager.h"
#include "../SonobusPluginEditor.h"

JoinSessionView::JoinSessionView()
    : sessionManager(nullptr), editor(nullptr)
{
    setupUI();
}

JoinSessionView::JoinSessionView(SessionManager* sm, SonobusAudioProcessorEditor* ed)
    : sessionManager(sm), editor(ed)
{
    setupUI();
}

void JoinSessionView::setupUI()
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
        if (onBackClicked)
            onBackClicked();
    };
    addAndMakeVisible(backButton);
}

void JoinSessionView::reset()
{
    isJoiningSession = false;
    codeEditor.clear();
    statusLabel.setText("", dontSendNotification);
    setUIEnabled(true);
}

void JoinSessionView::setInviteCode(const String& code)
{
    codeEditor.setText(code, dontSendNotification);
}

void JoinSessionView::handleJoinSession()
{
    if (isJoiningSession)
        return;
    
    String inviteInput = codeEditor.getText().trim();
    
    if (inviteInput.isEmpty())
    {
        showError("Please enter an invite link or code");
        return;
    }
    
    // If we have editor integration, use the full flow
    if (sessionManager && editor)
    {
        isJoiningSession = true;
        setUIEnabled(false);
        showStatus("Looking up session...");
        
        bool success = sessionManager->joinSession(inviteInput);
        
        if (!success)
        {
            isJoiningSession = false;
            setUIEnabled(true);
            showError(sessionManager->getLastError());
            return;
        }
        
        DBG("Joined session via API: " + sessionManager->getCurrentSessionId());
        showStatus("Connecting to server...");
        
        auto connectionInfo = sessionManager->getConnectionInfo();
        String username = "User_" + String(Random::getSystemRandom().nextInt(9999));
        
        editor->connectToSoundFlipSession(
            connectionInfo.server,
            connectionInfo.port,
            connectionInfo.group,
            connectionInfo.password,
            username
        );
        
        isJoiningSession = false;
        
        if (onSessionJoined)
            onSessionJoined(sessionManager->getCurrentSessionId());
    }
    else
    {
        // Fallback: just trigger callback
        if (onSessionJoined)
            onSessionJoined("placeholder_session_id");
    }
}

void JoinSessionView::showError(const String& message)
{
    DBG("JoinSessionView Error: " + message);
    statusLabel.setText(message.isEmpty() ? "An error occurred" : message, dontSendNotification);
    statusLabel.setColour(Label::textColourId, Colour(0xffe74c3c));
    setUIEnabled(true);
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