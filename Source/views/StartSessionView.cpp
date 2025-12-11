// SPDX-License-Identifier: GPLv3-or-later WITH Appstore-exception
// Copyright (C) 2024 SoundFlip

#include "StartSessionView.h"

StartSessionView::StartSessionView()
{
    backButton = std::make_unique<TextButton>("< Back");
    backButton->addListener(this);
    addAndMakeVisible(backButton.get());

    titleLabel = std::make_unique<Label>("title", "Start Session");
    titleLabel->setFont(Font(24.0f, Font::bold));
    titleLabel->setColour(Label::textColourId, Colours::white);
    addAndMakeVisible(titleLabel.get());

    sessionNameLabel = std::make_unique<Label>("nameLabel", "Session Name (optional)");
    sessionNameLabel->setFont(Font(14.0f));
    sessionNameLabel->setColour(Label::textColourId, Colours::grey);
    addAndMakeVisible(sessionNameLabel.get());

    sessionNameInput = std::make_unique<TextEditor>("nameInput");
    sessionNameInput->setMultiLine(false);
    sessionNameInput->setText("Friday night cookup");
    addAndMakeVisible(sessionNameInput.get());

    audioInputLabel = std::make_unique<Label>("audioLabel", "Audio Input");
    audioInputLabel->setFont(Font(14.0f));
    audioInputLabel->setColour(Label::textColourId, Colours::grey);
    addAndMakeVisible(audioInputLabel.get());

    audioInputCombo = std::make_unique<ComboBox>("audioInput");
    audioInputCombo->addItem("Scarlett 2i2 USB", 1);
    audioInputCombo->addItem("Built-in Microphone", 2);
    audioInputCombo->addItem("BlackHole 2ch", 3);
    audioInputCombo->setSelectedId(1);
    addAndMakeVisible(audioInputCombo.get());

    inputLevelLabel = std::make_unique<Label>("levelLabel", "Input Level: [====____]");
    inputLevelLabel->setFont(Font(14.0f));
    inputLevelLabel->setColour(Label::textColourId, Colours::green);
    addAndMakeVisible(inputLevelLabel.get());

    startButton = std::make_unique<TextButton>("Start Session");
    startButton->addListener(this);
    addAndMakeVisible(startButton.get());
}

StartSessionView::~StartSessionView()
{
}

void StartSessionView::paint(Graphics& g)
{
    g.fillAll(Colour(0xff1a1a2e));
}

void StartSessionView::resized()
{
    auto bounds = getLocalBounds().reduced(20);

    backButton->setBounds(20, 20, 80, 30);
    titleLabel->setBounds(20, 60, 200, 30);

    sessionNameLabel->setBounds(20, 110, bounds.getWidth(), 20);
    sessionNameInput->setBounds(20, 135, bounds.getWidth(), 35);

    audioInputLabel->setBounds(20, 190, bounds.getWidth(), 20);
    audioInputCombo->setBounds(20, 215, bounds.getWidth(), 35);

    inputLevelLabel->setBounds(20, 270, bounds.getWidth(), 25);

    startButton->setBounds(20, 330, bounds.getWidth(), 45);
}

void StartSessionView::buttonClicked(Button* buttonThatWasClicked)
{
    if (buttonThatWasClicked == backButton.get())
    {
        if (onBackClicked)
            onBackClicked();
    }
    else if (buttonThatWasClicked == startButton.get())
    {
        if (onStartClicked)
            onStartClicked();
    }
}