// SPDX-License-Identifier: GPLv3-or-later WITH Appstore-exception
// Copyright (C) 2024 SoundFlip

#include "LoginView.h"

LoginView::LoginView(SoundFlipAuth& authRef)
    : auth(authRef)
{
    auth.addListener(this);

    titleLabel.setText("SoundFlip Connect", dontSendNotification);
    titleLabel.setFont(Font(32.0f, Font::bold));
    titleLabel.setJustificationType(Justification::centred);
    addAndMakeVisible(titleLabel);

    subtitleLabel.setText("Sign in to start collaborating", dontSendNotification);
    subtitleLabel.setFont(Font(16.0f));
    subtitleLabel.setJustificationType(Justification::centred);
    subtitleLabel.setColour(Label::textColourId, Colours::grey);
    addAndMakeVisible(subtitleLabel);

    signInButton.setButtonText("Sign In with Google");
    signInButton.onClick = [this]() {
        statusLabel.setText("Opening browser...", dontSendNotification);
        auth.startOAuthFlow();
    };
    addAndMakeVisible(signInButton);

    statusLabel.setText("", dontSendNotification);
    statusLabel.setFont(Font(14.0f));
    statusLabel.setJustificationType(Justification::centred);
    statusLabel.setColour(Label::textColourId, Colours::grey);
    addAndMakeVisible(statusLabel);
}

LoginView::~LoginView()
{
    auth.removeListener(this);
}

void LoginView::paint(Graphics& g)
{
    g.fillAll(getLookAndFeel().findColour(ResizableWindow::backgroundColourId));
}

void LoginView::resized()
{
    auto bounds = getLocalBounds();
    auto contentBounds = bounds.reduced(40);

    int centerY = contentBounds.getCentreY();

    titleLabel.setBounds(contentBounds.getX(), centerY - 100, contentBounds.getWidth(), 40);
    subtitleLabel.setBounds(contentBounds.getX(), centerY - 50, contentBounds.getWidth(), 30);
    
    int buttonWidth = jmin(300, contentBounds.getWidth() - 40);
    signInButton.setBounds((getWidth() - buttonWidth) / 2, centerY, buttonWidth, 50);
    
    statusLabel.setBounds(contentBounds.getX(), centerY + 70, contentBounds.getWidth(), 30);
}

void LoginView::authenticationSucceeded()
{
    // Call on message thread
    MessageManager::callAsync([this]() {
        statusLabel.setText("Success!", dontSendNotification);
        if (onLoginSuccess)
            onLoginSuccess();
    });
}

void LoginView::authenticationFailed(const String& error)
{
    MessageManager::callAsync([this, error]() {
        statusLabel.setText("Error: " + error, dontSendNotification);
        statusLabel.setColour(Label::textColourId, Colours::red);
    });
}