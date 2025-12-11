// SPDX-License-Identifier: GPLv3-or-later WITH Appstore-exception
// Copyright (C) 2024 SoundFlip

#include "JoinSessionView.h"

JoinSessionView::JoinSessionView()
{
    backButton = std::make_unique<TextButton>("< Back");
    backButton->addListener(this);
    addAndMakeVisible(backButton.get());

    titleLabel = std::make_unique<Label>("title", "Join Session");
    titleLabel->setFont(Font(24.0f, Font::bold));
    titleLabel->setColour(Label::textColourId, Colours::white);
    addAndMakeVisible(titleLabel.get());

    inviteLinkLabel = std::make_unique<Label>("linkLabel", "Paste invite link or code");
    inviteLinkLabel->setFont(Font(14.0f));
    inviteLinkLabel->setColour(Label::textColourId, Colours::grey);
    addAndMakeVisible(inviteLinkLabel.get());

    inviteLinkInput = std::make_unique<TextEditor>("linkInput");
    inviteLinkInput->setMultiLine(false);
    inviteLinkInput->setTextToShowWhenEmpty("soundflip.com/session/XYZ789", Colours::darkgrey);
    addAndMakeVisible(inviteLinkInput.get());

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

    joinButton = std::make_unique<TextButton>("Join Session");
    joinButton->addListener(this);
    addAndMakeVisible(joinButton.get());
}

JoinSessionView::~JoinSessionView()
{
}

void JoinSessionView::paint(Graphics& g)
{
    g.fillAll(Colour(0xff1a1a2e));
}

void JoinSessionView::resized()
{
    auto bounds = getLocalBounds().reduced(20);

    backButton->setBounds(20, 20, 80, 30);
    titleLabel->setBounds(20, 60, 200, 30);

    inviteLinkLabel->setBounds(20, 110, bounds.getWidth(), 20);
    inviteLinkInput->setBounds(20, 135, bounds.getWidth(), 35);

    audioInputLabel->setBounds(20, 190, bounds.getWidth(), 20);
    audioInputCombo->setBounds(20, 215, bounds.getWidth(), 35);

    inputLevelLabel->setBounds(20, 270, bounds.getWidth(), 25);

    joinButton->setBounds(20, 330, bounds.getWidth(), 45);
}

void JoinSessionView::buttonClicked(Button* buttonThatWasClicked)
{
    if (buttonThatWasClicked == backButton.get())
    {
        if (onBackClicked)
            onBackClicked();
    }
    else if (buttonThatWasClicked == joinButton.get())
    {
        if (onJoinClicked)
            onJoinClicked();
    }
}