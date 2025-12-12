// SPDX-License-Identifier: GPLv3-or-later WITH Appstore-exception
// Copyright (C) 2024 SoundFlip

#include "ActiveSessionView.h"
#include "../managers/SessionManager.h"
#include "../SonobusPluginProcessor.h"

ActiveSessionView::ActiveSessionView(SessionManager* sm, SonobusAudioProcessor* proc)
    : sessionManager(sm), processor(proc)
{
    if (sessionManager)
        sessionManager->addChangeListener(this);
    
    titleLabel.setText("Active Session", dontSendNotification);
    titleLabel.setFont(Font(24.0f, Font::bold));
    titleLabel.setJustificationType(Justification::centred);
    titleLabel.setColour(Label::textColourId, Colours::white);
    addAndMakeVisible(titleLabel);

    statusLabel.setText("Connected", dontSendNotification);
    statusLabel.setJustificationType(Justification::centred);
    statusLabel.setColour(Label::textColourId, Colours::green);
    addAndMakeVisible(statusLabel);
    
    participantsLabel.setText("Participants:", dontSendNotification);
    participantsLabel.setFont(Font(14.0f, Font::bold));
    participantsLabel.setColour(Label::textColourId, Colours::white);
    addAndMakeVisible(participantsLabel);
    
    participantListLabel.setText("Loading...", dontSendNotification);
    participantListLabel.setColour(Label::textColourId, Colour(0xffaaaaaa));
    addAndMakeVisible(participantListLabel);

    recordButton.setButtonText("Record");
    recordButton.setColour(TextButton::buttonColourId, Colour(0xff6c5ce7));
    recordButton.onClick = [this]() {
        if (onRecordClicked)
            onRecordClicked();
    };
    addAndMakeVisible(recordButton);

    chatButton.setButtonText("Chat");
    chatButton.onClick = [this]() {
        if (onChatClicked)
            onChatClicked();
    };
    addAndMakeVisible(chatButton);

    inviteButton.setButtonText("Invite");
    inviteButton.setColour(TextButton::buttonColourId, Colour(0xff00cec9));
    inviteButton.onClick = [this]() {
        handleInviteClicked();
    };
    addAndMakeVisible(inviteButton);

    endSessionButton.setButtonText("Leave Session");
    endSessionButton.setColour(TextButton::buttonColourId, Colour(0xffe74c3c));
    endSessionButton.onClick = [this]() {
        handleEndSession();
    };
    addAndMakeVisible(endSessionButton);
    
    // Update UI with session info if available
    if (sessionManager && sessionManager->isConnected())
    {
        setSessionInfo(sessionManager->getCurrentSessionName(),
                       sessionManager->getInviteUrl());
        updateParticipantsUI();
    }
}

ActiveSessionView::~ActiveSessionView()
{
    if (sessionManager)
        sessionManager->removeChangeListener(this);
}

void ActiveSessionView::setSessionManager(SessionManager* sm)
{
    if (sessionManager)
        sessionManager->removeChangeListener(this);
    
    sessionManager = sm;
    
    if (sessionManager)
    {
        sessionManager->addChangeListener(this);
        
        if (sessionManager->isConnected())
        {
            setSessionInfo(sessionManager->getCurrentSessionName(),
                           sessionManager->getInviteUrl());
            updateParticipantsUI();
        }
    }
}

void ActiveSessionView::setSessionInfo(const String& name, const String& inviteUrl)
{
    currentSessionName = name;
    currentInviteUrl = inviteUrl;
    
    if (currentSessionName.isNotEmpty())
        titleLabel.setText(currentSessionName, dontSendNotification);
    else
        titleLabel.setText("Active Session", dontSendNotification);
}

void ActiveSessionView::refreshParticipants()
{
    updateParticipantsUI();
}

void ActiveSessionView::changeListenerCallback(ChangeBroadcaster* source)
{
    if (source == sessionManager)
    {
        // Session state changed - update UI
        if (sessionManager->isConnected())
        {
            setSessionInfo(sessionManager->getCurrentSessionName(),
                           sessionManager->getInviteUrl());
            updateParticipantsUI();
            statusLabel.setText("Connected", dontSendNotification);
            statusLabel.setColour(Label::textColourId, Colours::green);
        }
        else
        {
            statusLabel.setText("Disconnected", dontSendNotification);
            statusLabel.setColour(Label::textColourId, Colour(0xffe74c3c));
        }
    }
}

void ActiveSessionView::updateParticipantsUI()
{
    if (!sessionManager)
    {
        participantListLabel.setText("No session", dontSendNotification);
        return;
    }
    
    const auto& participants = sessionManager->getParticipants();
    
    if (participants.isEmpty())
    {
        participantListLabel.setText("Just you", dontSendNotification);
        return;
    }
    
    String participantText;
    for (int i = 0; i < participants.size(); ++i)
    {
        if (i > 0) participantText += ", ";
        participantText += "@" + participants[i].username;
    }
    
    participantListLabel.setText(participantText, dontSendNotification);
}

void ActiveSessionView::handleInviteClicked()
{
    if (currentInviteUrl.isNotEmpty())
    {
        SystemClipboard::copyTextToClipboard(currentInviteUrl);
        
        // Show feedback
        inviteButton.setButtonText("Copied!");
        
        // Reset button text after 2 seconds
        Timer::callAfterDelay(2000, [this]() {
            if (inviteButton.isShowing())
                inviteButton.setButtonText("Invite");
        });
    }
    
    if (onInviteClicked)
        onInviteClicked();
}

void ActiveSessionView::handleEndSession()
{
    if (onEndClicked)
        onEndClicked();
    
    // Leave the AOO group and disconnect
    if (processor && sessionManager)
    {
        auto connectionInfo = sessionManager->getConnectionInfo();
        
        // Leave the group first
        processor->leaveServerGroup(connectionInfo.group);
        
        // Then disconnect from server
        processor->disconnectFromServer();
    }
    
    // Update session manager state
    if (sessionManager)
    {
        sessionManager->leaveSession();
    }
    
    if (onSessionEnded)
        onSessionEnded();
}

void ActiveSessionView::paint(Graphics& g)
{
    g.fillAll(Colour(0xff1a1a2e));
}

void ActiveSessionView::resized()
{
    auto bounds = getLocalBounds().reduced(40);
    
    titleLabel.setBounds(bounds.removeFromTop(40));
    bounds.removeFromTop(10);
    statusLabel.setBounds(bounds.removeFromTop(25));
    
    bounds.removeFromTop(20);
    
    // Participants section
    participantsLabel.setBounds(bounds.removeFromTop(20));
    participantListLabel.setBounds(bounds.removeFromTop(25));
    
    bounds.removeFromTop(30);
    
    int buttonWidth = 120;
    int buttonHeight = 40;
    int spacing = 15;
    
    // Action buttons row
    auto buttonRow = bounds.removeFromTop(buttonHeight);
    int totalButtonWidth = buttonWidth * 3 + spacing * 2;
    int startX = (buttonRow.getWidth() - totalButtonWidth) / 2;
    
    recordButton.setBounds(buttonRow.getX() + startX, buttonRow.getY(), buttonWidth, buttonHeight);
    chatButton.setBounds(buttonRow.getX() + startX + buttonWidth + spacing, buttonRow.getY(), buttonWidth, buttonHeight);
    inviteButton.setBounds(buttonRow.getX() + startX + (buttonWidth + spacing) * 2, buttonRow.getY(), buttonWidth, buttonHeight);
    
    bounds.removeFromTop(30);
    
    // End session button centered
    endSessionButton.setBounds((getWidth() - 150) / 2, bounds.getY(), 150, buttonHeight);
}