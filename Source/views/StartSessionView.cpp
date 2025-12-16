// SPDX-License-Identifier: GPLv3-or-later WITH Appstore-exception
// Copyright (C) 2024 SoundFlip

#include "StartSessionView.h"
#include "../managers/SessionManager.h"
#include "../SonobusPluginEditor.h"

StartSessionView::StartSessionView()
    : sessionManager(nullptr), editor(nullptr)
{
    setupUI();
}

StartSessionView::StartSessionView(SessionManager* sm, SonobusAudioProcessorEditor* ed)
    : sessionManager(sm), editor(ed)
{
    setupUI();
}

void StartSessionView::setupUI()
{
    titleLabel.setText("Start New Session", dontSendNotification);
    titleLabel.setFont(Font(24.0f, Font::bold));
    titleLabel.setJustificationType(Justification::centred);
    titleLabel.setColour(Label::textColourId, Colours::white);
    addAndMakeVisible(titleLabel);

    sessionNameLabel.setText("Session Name:", dontSendNotification);
    sessionNameLabel.setColour(Label::textColourId, Colours::white);
    addAndMakeVisible(sessionNameLabel);

    sessionNameEditor.setTextToShowWhenEmpty("Friday night cookup...", Colours::grey);
    addAndMakeVisible(sessionNameEditor);
    
    statusLabel.setText("", dontSendNotification);
    statusLabel.setJustificationType(Justification::centred);
    statusLabel.setColour(Label::textColourId, Colours::orange);
    addAndMakeVisible(statusLabel);

    createButton.setButtonText("Create Session");
    createButton.setColour(TextButton::buttonColourId, Colour(0xff6c5ce7));
    createButton.onClick = [this]() {
        handleCreateSession();
    };
    addAndMakeVisible(createButton);

    backButton.setButtonText("Back");
    backButton.onClick = [this]() {
        if (onBackClicked)
            onBackClicked();
    };
    addAndMakeVisible(backButton);
}

void StartSessionView::handleCreateSession()
{
    if (isCreatingSession)
        return;
    
    String sessionName = sessionNameEditor.getText().trim();
    
    // If we have editor integration, use the full flow
    if (sessionManager && editor)
    {
        // Update UI state
        isCreatingSession = true;
        setUIEnabled(false);
        showStatus("Creating session...");
        
        // Create session via API
        bool success = sessionManager->createSession(sessionName);
        
        if (!success)
        {
            isCreatingSession = false;
            setUIEnabled(true);
            showError(sessionManager->getLastError());
            return;
        }
        
        DBG("Session created: " + sessionManager->getCurrentSessionId());
        showStatus("Connecting to server...");
        
        // Get connection info from SessionManager
        auto connectionInfo = sessionManager->getConnectionInfo();
        
        // Get username
        String username = "User_" + String(Random::getSystemRandom().nextInt(9999));
        
        // Tell the editor to connect
        editor->connectToSoundFlipSession(
            connectionInfo.server,
            connectionInfo.port,
            connectionInfo.group,
            connectionInfo.password,
            username
        );
        
        isCreatingSession = false;
        
        if (onSessionCreated)
            onSessionCreated(sessionManager->getCurrentSessionId());
    }
    else
    {
        // Fallback: just trigger callback (for ScreenManager path)
        if (onSessionCreated)
            onSessionCreated("placeholder_session_id");
    }
}

void StartSessionView::reset()
{
    isCreatingSession = false;
    sessionNameEditor.clear();
    statusLabel.setText("", dontSendNotification);
    setUIEnabled(true);
}

void StartSessionView::showError(const String& message)
{
    DBG("StartSessionView Error: " + message);
    statusLabel.setText(message.isEmpty() ? "An error occurred" : message, dontSendNotification);
    statusLabel.setColour(Label::textColourId, Colour(0xffe74c3c));
    setUIEnabled(true);
}

void StartSessionView::showStatus(const String& message)
{
    statusLabel.setText(message, dontSendNotification);
    statusLabel.setColour(Label::textColourId, Colours::white);
}

void StartSessionView::setUIEnabled(bool enabled)
{
    sessionNameEditor.setEnabled(enabled);
    createButton.setEnabled(enabled);
    backButton.setEnabled(enabled);
    
    createButton.setButtonText(enabled ? "Create Session" : "Creating...");
}

void StartSessionView::paint(Graphics& g)
{
    g.fillAll(Colour(0xff1a1a2e));
}

void StartSessionView::resized()
{
    auto bounds = getLocalBounds().reduced(40);
    
    titleLabel.setBounds(bounds.removeFromTop(40));
    bounds.removeFromTop(30);
    
    auto row = bounds.removeFromTop(30);
    sessionNameLabel.setBounds(row.removeFromLeft(120));
    sessionNameEditor.setBounds(row);
    
    bounds.removeFromTop(20);
    
    statusLabel.setBounds(bounds.removeFromTop(25));
    
    bounds.removeFromTop(20);
    
    int buttonWidth = 150;
    auto buttonArea = bounds.removeFromTop(40);
    int totalWidth = buttonWidth * 2 + 20;
    int startX = (buttonArea.getWidth() - totalWidth) / 2;
    
    backButton.setBounds(buttonArea.getX() + startX, buttonArea.getY(), buttonWidth, 40);
    createButton.setBounds(buttonArea.getX() + startX + buttonWidth + 20, buttonArea.getY(), buttonWidth, 40);
}