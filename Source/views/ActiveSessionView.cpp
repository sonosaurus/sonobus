// SPDX-License-Identifier: GPLv3-or-later WITH Appstore-exception
// Copyright (C) 2024 SoundFlip

#include "ActiveSessionView.h"

ActiveSessionView::ActiveSessionView()
{
    sessionNameLabel = std::make_unique<Label>("sessionName", "Friday night cookup");
    sessionNameLabel->setFont(Font(20.0f, Font::bold));
    sessionNameLabel->setColour(Label::textColourId, Colours::white);
    addAndMakeVisible(sessionNameLabel.get());

    endButton = std::make_unique<TextButton>("End");
    endButton->addListener(this);
    endButton->setColour(TextButton::buttonColourId, Colour(0xffe74c3c));
    addAndMakeVisible(endButton.get());

    menuButton = std::make_unique<TextButton>("...");
    menuButton->addListener(this);
    addAndMakeVisible(menuButton.get());

    youLabel = std::make_unique<Label>("you", "YOU");
    youLabel->setFont(Font(16.0f, Font::bold));
    youLabel->setColour(Label::textColourId, Colours::white);
    addAndMakeVisible(youLabel.get());

    youLevelLabel = std::make_unique<Label>("youLevel", "[======__]");
    youLevelLabel->setFont(Font(14.0f));
    youLevelLabel->setColour(Label::textColourId, Colours::green);
    addAndMakeVisible(youLevelLabel.get());

    youMuteButton = std::make_unique<TextButton>("Mute");
    youMuteButton->addListener(this);
    addAndMakeVisible(youMuteButton.get());

    peerLabel = std::make_unique<Label>("peer", "@mike");
    peerLabel->setFont(Font(16.0f, Font::bold));
    peerLabel->setColour(Label::textColourId, Colours::white);
    addAndMakeVisible(peerLabel.get());

    peerLevelLabel = std::make_unique<Label>("peerLevel", "[====____]");
    peerLevelLabel->setFont(Font(14.0f));
    peerLevelLabel->setColour(Label::textColourId, Colours::green);
    addAndMakeVisible(peerLevelLabel.get());

    peerVolumeSlider = std::make_unique<Slider>(Slider::LinearHorizontal, Slider::NoTextBox);
    peerVolumeSlider->setRange(0.0, 1.0, 0.01);
    peerVolumeSlider->setValue(0.75);
    peerVolumeSlider->addListener(this);
    addAndMakeVisible(peerVolumeSlider.get());

    connectionStatusLabel = std::make_unique<Label>("status", "Connected - 24ms latency");
    connectionStatusLabel->setFont(Font(14.0f));
    connectionStatusLabel->setColour(Label::textColourId, Colour(0xff2ecc71));
    connectionStatusLabel->setJustificationType(Justification::centred);
    addAndMakeVisible(connectionStatusLabel.get());

    recordButton = std::make_unique<TextButton>("Record");
    recordButton->addListener(this);
    addAndMakeVisible(recordButton.get());

    chatButton = std::make_unique<TextButton>("Chat");
    chatButton->addListener(this);
    addAndMakeVisible(chatButton.get());

    inviteButton = std::make_unique<TextButton>("Invite");
    inviteButton->addListener(this);
    addAndMakeVisible(inviteButton.get());
}

ActiveSessionView::~ActiveSessionView()
{
}

void ActiveSessionView::paint(Graphics& g)
{
    g.fillAll(Colour(0xff1a1a2e));

    auto bounds = getLocalBounds().reduced(20);

    // You card background
    g.setColour(Colour(0xff2d2d44));
    g.fillRoundedRectangle(20.0f, 70.0f, (float)bounds.getWidth(), 70.0f, 8.0f);

    // Peer card background
    g.setColour(Colour(0xff2d2d44));
    g.fillRoundedRectangle(20.0f, 155.0f, (float)bounds.getWidth(), 70.0f, 8.0f);

    // Connection status background
    g.setColour(Colour(0xff1e1e32));
    g.fillRoundedRectangle(20.0f, 240.0f, (float)bounds.getWidth(), 35.0f, 8.0f);
}

void ActiveSessionView::resized()
{
    auto bounds = getLocalBounds().reduced(20);

    sessionNameLabel->setBounds(20, 20, bounds.getWidth() - 120, 30);
    endButton->setBounds(bounds.getWidth() - 70, 20, 50, 30);
    menuButton->setBounds(bounds.getWidth() - 15, 20, 35, 30);

    // You section
    youLabel->setBounds(30, 80, 60, 25);
    youLevelLabel->setBounds(100, 80, 100, 25);
    youMuteButton->setBounds(bounds.getWidth() - 50, 85, 60, 30);

    // Peer section
    peerLabel->setBounds(30, 165, 80, 25);
    peerLevelLabel->setBounds(120, 165, 100, 25);
    peerVolumeSlider->setBounds(30, 195, bounds.getWidth() - 20, 20);

    // Connection status
    connectionStatusLabel->setBounds(20, 245, bounds.getWidth(), 25);

    // Bottom buttons
    int buttonWidth = (bounds.getWidth() - 20) / 3;
    int buttonY = bounds.getHeight() - 30;
    recordButton->setBounds(20, buttonY, buttonWidth, 40);
    chatButton->setBounds(30 + buttonWidth, buttonY, buttonWidth, 40);
    inviteButton->setBounds(40 + buttonWidth * 2, buttonY, buttonWidth, 40);
}

void ActiveSessionView::buttonClicked(Button* buttonThatWasClicked)
{
    if (buttonThatWasClicked == endButton.get())
    {
        if (onEndClicked)
            onEndClicked();
    }
    else if (buttonThatWasClicked == recordButton.get())
    {
        isRecording = !isRecording;
        recordButton->setButtonText(isRecording ? "Stop (0:12)" : "Record");
        recordButton->setColour(TextButton::buttonColourId, 
            isRecording ? Colour(0xffe74c3c) : getLookAndFeel().findColour(TextButton::buttonColourId));
        
        if (onRecordClicked)
            onRecordClicked();
    }
    else if (buttonThatWasClicked == chatButton.get())
    {
        if (onChatClicked)
            onChatClicked();
    }
    else if (buttonThatWasClicked == inviteButton.get())
    {
        if (onInviteClicked)
            onInviteClicked();
    }
    else if (buttonThatWasClicked == youMuteButton.get())
    {
        isMuted = !isMuted;
        youMuteButton->setButtonText(isMuted ? "Unmute" : "Mute");
        youLevelLabel->setColour(Label::textColourId, isMuted ? Colours::grey : Colours::green);
    }
}

void ActiveSessionView::sliderValueChanged(Slider* slider)
{
    if (slider == peerVolumeSlider.get())
    {
        // Volume changed - will connect to actual audio later
    }
}