// SPDX-License-Identifier: GPLv3-or-later WITH Appstore-exception
// Copyright (C) 2024 SoundFlip

#include "ActiveSessionView.h"

ActiveSessionView::ActiveSessionView()
{
    titleLabel.setText("Active Session", dontSendNotification);
    titleLabel.setFont(Font(24.0f, Font::bold));
    titleLabel.setJustificationType(Justification::centred);
    addAndMakeVisible(titleLabel);

    statusLabel.setText("Connected", dontSendNotification);
    statusLabel.setJustificationType(Justification::centred);
    statusLabel.setColour(Label::textColourId, Colours::green);
    addAndMakeVisible(statusLabel);

    recordButton.setButtonText("Record");
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
    inviteButton.onClick = [this]() {
        if (onInviteClicked)
            onInviteClicked();
    };
    addAndMakeVisible(inviteButton);

    endSessionButton.setButtonText("End Session");
    endSessionButton.onClick = [this]() {
        if (onEndClicked)
            onEndClicked();
        if (onSessionEnded)
            onSessionEnded();
    };
    addAndMakeVisible(endSessionButton);
}

void ActiveSessionView::paint(Graphics& g)
{
    g.fillAll(getLookAndFeel().findColour(ResizableWindow::backgroundColourId));
}

void ActiveSessionView::resized()
{
    auto bounds = getLocalBounds().reduced(40);
    
    titleLabel.setBounds(bounds.removeFromTop(40));
    bounds.removeFromTop(20);
    statusLabel.setBounds(bounds.removeFromTop(30));
    
    bounds.removeFromTop(40);
    
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