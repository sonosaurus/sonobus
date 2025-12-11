// SPDX-License-Identifier: GPLv3-or-later WITH Appstore-exception
// Copyright (C) 2024 SoundFlip

#include "HomeView.h"

HomeView::HomeView()
{
    usernameLabel = std::make_unique<Label>("username", "Producer123");
    usernameLabel->setFont(Font(18.0f, Font::bold));
    usernameLabel->setColour(Label::textColourId, Colours::white);
    addAndMakeVisible(usernameLabel.get());

    settingsButton = std::make_unique<TextButton>("Settings");
    settingsButton->addListener(this);
    addAndMakeVisible(settingsButton.get());

    startSessionButton = std::make_unique<TextButton>("Start Session");
    startSessionButton->addListener(this);
    addAndMakeVisible(startSessionButton.get());

    joinSessionButton = std::make_unique<TextButton>("Join Session");
    joinSessionButton->addListener(this);
    addAndMakeVisible(joinSessionButton.get());

    recentSessionsLabel = std::make_unique<Label>("recent", "Recent Sessions");
    recentSessionsLabel->setFont(Font(16.0f, Font::bold));
    recentSessionsLabel->setColour(Label::textColourId, Colours::white);
    addAndMakeVisible(recentSessionsLabel.get());

    recentSession1Button = std::make_unique<TextButton>("Friday cookup with @mike\nDec 8 - 3 stems");
    recentSession1Button->addListener(this);
    addAndMakeVisible(recentSession1Button.get());

    recentSession2Button = std::make_unique<TextButton>("Beat review with @sarah\nDec 5 - 2 stems");
    recentSession2Button->addListener(this);
    addAndMakeVisible(recentSession2Button.get());
}

HomeView::~HomeView()
{
}

void HomeView::paint(Graphics& g)
{
    g.fillAll(Colour(0xff1a1a2e));
    drawAvatar(g, 20, 15, 40);
}

void HomeView::drawAvatar(Graphics& g, int x, int y, int size)
{
    g.setColour(Colour(0xff6c5ce7));
    g.fillEllipse((float)x, (float)y, (float)size, (float)size);
    g.setColour(Colours::white);
    g.setFont(Font((float)size * 0.5f, Font::bold));
    g.drawText("P", x, y, size, size, Justification::centred);
}

void HomeView::resized()
{
    auto bounds = getLocalBounds().reduced(20);

    usernameLabel->setBounds(70, 20, 150, 30);
    settingsButton->setBounds(bounds.getWidth() - 80, 20, 80, 30);

    auto centerX = bounds.getCentreX();
    startSessionButton->setBounds(20, 80, bounds.getWidth() / 2 - 30, 60);
    joinSessionButton->setBounds(centerX + 10, 80, bounds.getWidth() / 2 - 30, 60);

    recentSessionsLabel->setBounds(20, 160, 200, 25);
    recentSession1Button->setBounds(20, 195, bounds.getWidth(), 50);
    recentSession2Button->setBounds(20, 255, bounds.getWidth(), 50);
}

void HomeView::buttonClicked(Button* buttonThatWasClicked)
{
    if (buttonThatWasClicked == startSessionButton.get())
    {
        if (onStartSessionClicked)
            onStartSessionClicked();
    }
    else if (buttonThatWasClicked == joinSessionButton.get())
    {
        if (onJoinSessionClicked)
            onJoinSessionClicked();
    }
    else if (buttonThatWasClicked == settingsButton.get())
    {
        if (onSettingsClicked)
            onSettingsClicked();
    }
    else if (buttonThatWasClicked == recentSession1Button.get())
    {
        if (onRecentSessionClicked)
            onRecentSessionClicked(0);
    }
    else if (buttonThatWasClicked == recentSession2Button.get())
    {
        if (onRecentSessionClicked)
            onRecentSessionClicked(1);
    }
}