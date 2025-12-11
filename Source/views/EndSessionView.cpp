// SPDX-License-Identifier: GPLv3-or-later WITH Appstore-exception
// Copyright (C) 2024 SoundFlip

#include "EndSessionView.h"

EndSessionView::EndSessionView()
{
    titleLabel = std::make_unique<Label>("title", "Session ended");
    titleLabel->setFont(Font(28.0f, Font::bold));
    titleLabel->setColour(Label::textColourId, Colours::white);
    titleLabel->setJustificationType(Justification::centred);
    addAndMakeVisible(titleLabel.get());

    recordingInfoLabel = std::make_unique<Label>("info", "You recorded 12:34 of audio");
    recordingInfoLabel->setFont(Font(16.0f));
    recordingInfoLabel->setColour(Label::textColourId, Colours::grey);
    recordingInfoLabel->setJustificationType(Justification::centred);
    addAndMakeVisible(recordingInfoLabel.get());

    uploadButton = std::make_unique<TextButton>("Upload to this session");
    uploadButton->addListener(this);
    uploadButton->setColour(TextButton::buttonColourId, Colour(0xff6c5ce7));
    addAndMakeVisible(uploadButton.get());

    saveLocallyButton = std::make_unique<TextButton>("Save locally only");
    saveLocallyButton->addListener(this);
    addAndMakeVisible(saveLocallyButton.get());

    discardButton = std::make_unique<TextButton>("Discard recording");
    discardButton->addListener(this);
    discardButton->setColour(TextButton::buttonColourId, Colour(0xff555555));
    addAndMakeVisible(discardButton.get());
}

EndSessionView::~EndSessionView()
{
}

void EndSessionView::paint(Graphics& g)
{
    g.fillAll(Colour(0xff1a1a2e));
}

void EndSessionView::resized()
{
    auto bounds = getLocalBounds();
    auto centerX = bounds.getCentreX();
    auto centerY = bounds.getCentreY();

    titleLabel->setBounds(centerX - 150, centerY - 120, 300, 35);
    recordingInfoLabel->setBounds(centerX - 150, centerY - 75, 300, 25);

    uploadButton->setBounds(centerX - 120, centerY - 20, 240, 45);
    saveLocallyButton->setBounds(centerX - 120, centerY + 35, 240, 45);
    discardButton->setBounds(centerX - 120, centerY + 90, 240, 45);
}

void EndSessionView::buttonClicked(Button* buttonThatWasClicked)
{
    if (buttonThatWasClicked == uploadButton.get())
    {
        if (onUploadClicked)
            onUploadClicked();
    }
    else if (buttonThatWasClicked == saveLocallyButton.get())
    {
        if (onSaveLocallyClicked)
            onSaveLocallyClicked();
    }
    else if (buttonThatWasClicked == discardButton.get())
    {
        if (onDiscardClicked)
            onDiscardClicked();
    }
}