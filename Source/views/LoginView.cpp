// SPDX-License-Identifier: GPLv3-or-later WITH Appstore-exception
// Copyright (C) 2024 SoundFlip

#include "LoginView.h"

LoginView::LoginView()
{
    titleLabel = std::make_unique<Label>("title", "SoundFlip Connect");
    titleLabel->setFont(Font(32.0f, Font::bold));
    titleLabel->setJustificationType(Justification::centred);
    titleLabel->setColour(Label::textColourId, Colours::white);
    addAndMakeVisible(titleLabel.get());

    subtitleLabel = std::make_unique<Label>("subtitle", "Connect & Create Together");
    subtitleLabel->setFont(Font(16.0f));
    subtitleLabel->setJustificationType(Justification::centred);
    subtitleLabel->setColour(Label::textColourId, Colours::grey);
    addAndMakeVisible(subtitleLabel.get());

    signInButton = std::make_unique<TextButton>("Sign in with SoundFlip");
    signInButton->addListener(this);
    addAndMakeVisible(signInButton.get());
}

LoginView::~LoginView()
{
}

void LoginView::paint(Graphics& g)
{
    g.fillAll(Colour(0xff1a1a2e));
}

void LoginView::resized()
{
    auto bounds = getLocalBounds();
    auto centerX = bounds.getCentreX();
    auto centerY = bounds.getCentreY();

    titleLabel->setBounds(centerX - 150, centerY - 80, 300, 40);
    subtitleLabel->setBounds(centerX - 150, centerY - 35, 300, 25);
    signInButton->setBounds(centerX - 100, centerY + 20, 200, 40);
}

void LoginView::buttonClicked(Button* buttonThatWasClicked)
{
    if (buttonThatWasClicked == signInButton.get())
    {
        if (onSignInClicked)
            onSignInClicked();
    }
}
