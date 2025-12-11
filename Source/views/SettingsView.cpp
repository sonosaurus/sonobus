// SPDX-License-Identifier: GPLv3-or-later WITH Appstore-exception
// Copyright (C) 2024 SoundFlip

#include "SettingsView.h"

SettingsView::SettingsView()
{
    backButton = std::make_unique<TextButton>("< Back");
    backButton->addListener(this);
    addAndMakeVisible(backButton.get());

    titleLabel = std::make_unique<Label>("title", "Settings");
    titleLabel->setFont(Font(24.0f, Font::bold));
    titleLabel->setColour(Label::textColourId, Colours::white);
    titleLabel->setJustificationType(Justification::centredRight);
    addAndMakeVisible(titleLabel.get());

    audioInputLabel = std::make_unique<Label>("inputLabel", "Audio Input");
    audioInputLabel->setFont(Font(14.0f));
    audioInputLabel->setColour(Label::textColourId, Colours::grey);
    addAndMakeVisible(audioInputLabel.get());

    audioInputCombo = std::make_unique<ComboBox>("audioInput");
    audioInputCombo->addItem("Scarlett 2i2 USB", 1);
    audioInputCombo->addItem("Built-in Microphone", 2);
    audioInputCombo->addItem("BlackHole 2ch", 3);
    audioInputCombo->setSelectedId(1);
    addAndMakeVisible(audioInputCombo.get());

    audioOutputLabel = std::make_unique<Label>("outputLabel", "Audio Output");
    audioOutputLabel->setFont(Font(14.0f));
    audioOutputLabel->setColour(Label::textColourId, Colours::grey);
    addAndMakeVisible(audioOutputLabel.get());

    audioOutputCombo = std::make_unique<ComboBox>("audioOutput");
    audioOutputCombo->addItem("Scarlett 2i2 USB", 1);
    audioOutputCombo->addItem("Built-in Speakers", 2);
    audioOutputCombo->addItem("AirPods Pro", 3);
    audioOutputCombo->setSelectedId(1);
    addAndMakeVisible(audioOutputCombo.get());

    audioQualityLabel = std::make_unique<Label>("qualityLabel", "Audio Quality");
    audioQualityLabel->setFont(Font(14.0f));
    audioQualityLabel->setColour(Label::textColourId, Colours::grey);
    addAndMakeVisible(audioQualityLabel.get());

    audioQualityCombo = std::make_unique<ComboBox>("audioQuality");
    audioQualityCombo->addItem("High (more latency)", 1);
    audioQualityCombo->addItem("Medium (balanced)", 2);
    audioQualityCombo->addItem("Low (less latency)", 3);
    audioQualityCombo->setSelectedId(2);
    addAndMakeVisible(audioQualityCombo.get());

    recordingFolderLabel = std::make_unique<Label>("folderLabel", "Recording Folder");
    recordingFolderLabel->setFont(Font(14.0f));
    recordingFolderLabel->setColour(Label::textColourId, Colours::grey);
    addAndMakeVisible(recordingFolderLabel.get());

    recordingFolderPath = std::make_unique<Label>("folderPath", "/Users/you/Music/SoundFlip");
    recordingFolderPath->setFont(Font(12.0f));
    recordingFolderPath->setColour(Label::textColourId, Colours::white);
    addAndMakeVisible(recordingFolderPath.get());

    changeFolderButton = std::make_unique<TextButton>("Change");
    changeFolderButton->addListener(this);
    addAndMakeVisible(changeFolderButton.get());

    signOutButton = std::make_unique<TextButton>("Sign Out");
    signOutButton->addListener(this);
    signOutButton->setColour(TextButton::buttonColourId, Colour(0xffe74c3c));
    addAndMakeVisible(signOutButton.get());
}

SettingsView::~SettingsView()
{
}

void SettingsView::paint(Graphics& g)
{
    g.fillAll(Colour(0xff1a1a2e));
}

void SettingsView::resized()
{
    auto bounds = getLocalBounds().reduced(20);

    backButton->setBounds(20, 20, 80, 30);
    titleLabel->setBounds(bounds.getWidth() - 80, 20, 80, 30);

    audioInputLabel->setBounds(20, 70, bounds.getWidth(), 20);
    audioInputCombo->setBounds(20, 95, bounds.getWidth(), 35);

    audioOutputLabel->setBounds(20, 145, bounds.getWidth(), 20);
    audioOutputCombo->setBounds(20, 170, bounds.getWidth(), 35);

    audioQualityLabel->setBounds(20, 220, bounds.getWidth(), 20);
    audioQualityCombo->setBounds(20, 245, bounds.getWidth(), 35);

    recordingFolderLabel->setBounds(20, 295, bounds.getWidth(), 20);
    recordingFolderPath->setBounds(20, 315, bounds.getWidth() - 80, 20);
    changeFolderButton->setBounds(bounds.getWidth() - 50, 312, 70, 28);

    signOutButton->setBounds(20, bounds.getHeight() - 20, bounds.getWidth(), 45);
}

void SettingsView::buttonClicked(Button* buttonThatWasClicked)
{
    if (buttonThatWasClicked == backButton.get())
    {
        if (onBackClicked)
            onBackClicked();
    }
    else if (buttonThatWasClicked == signOutButton.get())
    {
        if (onSignOutClicked)
            onSignOutClicked();
    }
    else if (buttonThatWasClicked == changeFolderButton.get())
    {
        if (onChangeRecordingFolderClicked)
            onChangeRecordingFolderClicked();
    }
}