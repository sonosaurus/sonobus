// SPDX-License-Identifier: GPLv3-or-later WITH Appstore-exception
// Copyright (C) 2024 SoundFlip

#include "SessionDetailView.h"

SessionDetailView::SessionDetailView()
{
    backButton = std::make_unique<TextButton>("< Back");
    backButton->addListener(this);
    addAndMakeVisible(backButton.get());

    sessionNameLabel = std::make_unique<Label>("name", "Friday night cookup");
    sessionNameLabel->setFont(Font(24.0f, Font::bold));
    sessionNameLabel->setColour(Label::textColourId, Colours::white);
    addAndMakeVisible(sessionNameLabel.get());

    sessionInfoLabel = std::make_unique<Label>("info", "Dec 8, 2024 - 45 min");
    sessionInfoLabel->setFont(Font(14.0f));
    sessionInfoLabel->setColour(Label::textColourId, Colours::grey);
    addAndMakeVisible(sessionInfoLabel.get());

    participantsLabel = std::make_unique<Label>("participants", "with @mike");
    participantsLabel->setFont(Font(14.0f));
    participantsLabel->setColour(Label::textColourId, Colours::grey);
    addAndMakeVisible(participantsLabel.get());

    stemsLabel = std::make_unique<Label>("stems", "Stems");
    stemsLabel->setFont(Font(18.0f, Font::bold));
    stemsLabel->setColour(Label::textColourId, Colours::white);
    addAndMakeVisible(stemsLabel.get());

    stem1Button = std::make_unique<TextButton>("vocal_take_1.wav\n@mike - 3:24 - Download");
    stem1Button->addListener(this);
    addAndMakeVisible(stem1Button.get());

    stem2Button = std::make_unique<TextButton>("beat_v2.wav\nYou - 2:48 - Download");
    stem2Button->addListener(this);
    addAndMakeVisible(stem2Button.get());

    openInSoundFlipButton = std::make_unique<TextButton>("Open in SoundFlip");
    openInSoundFlipButton->addListener(this);
    openInSoundFlipButton->setColour(TextButton::buttonColourId, Colour(0xff6c5ce7));
    addAndMakeVisible(openInSoundFlipButton.get());
}

SessionDetailView::~SessionDetailView()
{
}

void SessionDetailView::paint(Graphics& g)
{
    g.fillAll(Colour(0xff1a1a2e));

    auto bounds = getLocalBounds().reduced(20);

    // Stem card backgrounds
    g.setColour(Colour(0xff2d2d44));
    g.fillRoundedRectangle(20.0f, 195.0f, (float)bounds.getWidth(), 55.0f, 8.0f);
    g.fillRoundedRectangle(20.0f, 260.0f, (float)bounds.getWidth(), 55.0f, 8.0f);
}

void SessionDetailView::resized()
{
    auto bounds = getLocalBounds().reduced(20);

    backButton->setBounds(20, 20, 80, 30);

    sessionNameLabel->setBounds(20, 60, bounds.getWidth(), 30);
    sessionInfoLabel->setBounds(20, 95, bounds.getWidth(), 20);
    participantsLabel->setBounds(20, 115, bounds.getWidth(), 20);

    stemsLabel->setBounds(20, 160, 100, 25);

    stem1Button->setBounds(25, 200, bounds.getWidth() - 10, 45);
    stem2Button->setBounds(25, 265, bounds.getWidth() - 10, 45);

    openInSoundFlipButton->setBounds(20, bounds.getHeight() - 20, bounds.getWidth(), 45);
}

void SessionDetailView::buttonClicked(Button* buttonThatWasClicked)
{
    if (buttonThatWasClicked == backButton.get())
    {
        if (onBackClicked)
            onBackClicked();
    }
    else if (buttonThatWasClicked == openInSoundFlipButton.get())
    {
        if (onOpenInSoundFlipClicked)
            onOpenInSoundFlipClicked();
    }
    else if (buttonThatWasClicked == stem1Button.get())
    {
        if (onDownloadStemClicked)
            onDownloadStemClicked(0);
    }
    else if (buttonThatWasClicked == stem2Button.get())
    {
        if (onDownloadStemClicked)
            onDownloadStemClicked(1);
    }
}