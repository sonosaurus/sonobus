// SPDX-License-Identifier: GPLv3-or-later WITH Appstore-exception
// Copyright (C) 2024 SoundFlip

#include "JoinSessionView.h"

JoinSessionView::JoinSessionView()
{
    titleLabel.setText("Join Session", dontSendNotification);
    titleLabel.setFont(Font(24.0f, Font::bold));
    titleLabel.setJustificationType(Justification::centred);
    addAndMakeVisible(titleLabel);

    codeLabel.setText("Session Code:", dontSendNotification);
    addAndMakeVisible(codeLabel);

    codeEditor.setTextToShowWhenEmpty("Enter code...", Colours::grey);
    addAndMakeVisible(codeEditor);

    joinButton.setButtonText("Join");
    joinButton.onClick = [this]() {
        if (onJoinClicked)
            onJoinClicked();
            
        String code = codeEditor.getText().trim();
        if (code.isNotEmpty() && onSessionJoined)
        {
            // In real implementation, this would call the API
            onSessionJoined("joined-session-id");
        }
    };
    addAndMakeVisible(joinButton);

    backButton.setButtonText("Back");
    backButton.onClick = [this]() {
        if (onBackClicked)
            onBackClicked();
    };
    addAndMakeVisible(backButton);
}

void JoinSessionView::paint(Graphics& g)
{
    g.fillAll(getLookAndFeel().findColour(ResizableWindow::backgroundColourId));
}

void JoinSessionView::resized()
{
    auto bounds = getLocalBounds().reduced(40);
    
    titleLabel.setBounds(bounds.removeFromTop(40));
    bounds.removeFromTop(30);
    
    auto row = bounds.removeFromTop(30);
    codeLabel.setBounds(row.removeFromLeft(120));
    codeEditor.setBounds(row);
    
    bounds.removeFromTop(20);
    
    int buttonWidth = 150;
    auto buttonArea = bounds.removeFromTop(40);
    int totalWidth = buttonWidth * 2 + 20;
    int startX = (buttonArea.getWidth() - totalWidth) / 2;
    
    backButton.setBounds(buttonArea.getX() + startX, buttonArea.getY(), buttonWidth, 40);
    joinButton.setBounds(buttonArea.getX() + startX + buttonWidth + 20, buttonArea.getY(), buttonWidth, 40);
}