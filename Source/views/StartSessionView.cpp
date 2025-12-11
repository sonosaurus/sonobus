// SPDX-License-Identifier: GPLv3-or-later WITH Appstore-exception
// Copyright (C) 2024 SoundFlip

#include "StartSessionView.h"

StartSessionView::StartSessionView()
{
    titleLabel.setText("Start New Session", dontSendNotification);
    titleLabel.setFont(Font(24.0f, Font::bold));
    titleLabel.setJustificationType(Justification::centred);
    addAndMakeVisible(titleLabel);

    sessionNameLabel.setText("Session Name:", dontSendNotification);
    addAndMakeVisible(sessionNameLabel);

    sessionNameEditor.setTextToShowWhenEmpty("Enter session name...", Colours::grey);
    addAndMakeVisible(sessionNameEditor);

    createButton.setButtonText("Create Session");
    createButton.onClick = [this]() {
        if (onStartClicked)
            onStartClicked();
        
        String sessionName = sessionNameEditor.getText().trim();
        if (sessionName.isNotEmpty() && onSessionCreated)
        {
            // In real implementation, this would call the API
            onSessionCreated("new-session-id");
        }
    };
    addAndMakeVisible(createButton);

    backButton.setButtonText("Back");
    backButton.onClick = [this]() {
        if (onBackClicked)
            onBackClicked();
    };
    addAndMakeVisible(backButton);
}

void StartSessionView::paint(Graphics& g)
{
    g.fillAll(getLookAndFeel().findColour(ResizableWindow::backgroundColourId));
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
    
    int buttonWidth = 150;
    auto buttonArea = bounds.removeFromTop(40);
    int totalWidth = buttonWidth * 2 + 20;
    int startX = (buttonArea.getWidth() - totalWidth) / 2;
    
    backButton.setBounds(buttonArea.getX() + startX, buttonArea.getY(), buttonWidth, 40);
    createButton.setBounds(buttonArea.getX() + startX + buttonWidth + 20, buttonArea.getY(), buttonWidth, 40);
}