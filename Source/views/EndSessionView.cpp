// SPDX-License-Identifier: GPLv3-or-later WITH Appstore-exception
// Copyright (C) 2024 SoundFlip

#include "EndSessionView.h"
#include "../managers/SessionManager.h"
#include "../SonoUtility.h"

EndSessionView::EndSessionView()
    : sessionManager(nullptr)
{
    titleLabel = std::make_unique<Label>("title", "Session ended");
    titleLabel->setFont(Font(28.0f, Font::bold));
    titleLabel->setColour(Label::textColourId, Colours::white);
    titleLabel->setJustificationType(Justification::centred);
    addAndMakeVisible(titleLabel.get());

    recordingInfoLabel = std::make_unique<Label>("info", "No recording from this session");
    recordingInfoLabel->setFont(Font(16.0f));
    recordingInfoLabel->setColour(Label::textColourId, Colours::grey);
    recordingInfoLabel->setJustificationType(Justification::centred);
    addAndMakeVisible(recordingInfoLabel.get());
    
    uploadProgressLabel = std::make_unique<Label>("progress", "");
    uploadProgressLabel->setFont(Font(14.0f));
    uploadProgressLabel->setColour(Label::textColourId, Colour(0xff00cec9));
    uploadProgressLabel->setJustificationType(Justification::centred);
    uploadProgressLabel->setVisible(false);
    addAndMakeVisible(uploadProgressLabel.get());

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
    
    doneButton = std::make_unique<TextButton>("Done");
    doneButton->addListener(this);
    doneButton->setColour(TextButton::buttonColourId, Colour(0xff6c5ce7));
    doneButton->setVisible(false);
    addAndMakeVisible(doneButton.get());
    
    updateUI();
}

EndSessionView::EndSessionView(SessionManager* sm)
    : sessionManager(sm)
{
    titleLabel = std::make_unique<Label>("title", "Session ended");
    titleLabel->setFont(Font(28.0f, Font::bold));
    titleLabel->setColour(Label::textColourId, Colours::white);
    titleLabel->setJustificationType(Justification::centred);
    addAndMakeVisible(titleLabel.get());

    recordingInfoLabel = std::make_unique<Label>("info", "No recording from this session");
    recordingInfoLabel->setFont(Font(16.0f));
    recordingInfoLabel->setColour(Label::textColourId, Colours::grey);
    recordingInfoLabel->setJustificationType(Justification::centred);
    addAndMakeVisible(recordingInfoLabel.get());
    
    uploadProgressLabel = std::make_unique<Label>("progress", "");
    uploadProgressLabel->setFont(Font(14.0f));
    uploadProgressLabel->setColour(Label::textColourId, Colour(0xff00cec9));
    uploadProgressLabel->setJustificationType(Justification::centred);
    uploadProgressLabel->setVisible(false);
    addAndMakeVisible(uploadProgressLabel.get());

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
    
    doneButton = std::make_unique<TextButton>("Done");
    doneButton->addListener(this);
    doneButton->setColour(TextButton::buttonColourId, Colour(0xff6c5ce7));
    doneButton->setVisible(false);
    addAndMakeVisible(doneButton.get());
    
    updateUI();
}

EndSessionView::~EndSessionView()
{
}

void EndSessionView::setRecordingInfo(const URL& recordedFile, double durationSeconds)
{
    lastRecordedFile = recordedFile;
    recordingDuration = durationSeconds;
    hasRecording = !recordedFile.isEmpty() && durationSeconds > 0;
    
    updateUI();
}

void EndSessionView::reset()
{
    lastRecordedFile = URL();
    recordingDuration = 0.0;
    hasRecording = false;
    isUploading = false;
    
    updateUI();
}

void EndSessionView::updateUI()
{
    if (hasRecording)
    {
        String durationStr = SonoUtility::durationToString(recordingDuration, true);
        String filename = lastRecordedFile.getFileName();
        recordingInfoLabel->setText("Recorded " + durationStr + " of audio\n" + filename, dontSendNotification);
        
        uploadButton->setVisible(!isUploading);
        saveLocallyButton->setVisible(!isUploading);
        discardButton->setVisible(!isUploading);
        doneButton->setVisible(false);
        uploadProgressLabel->setVisible(isUploading);
    }
    else
    {
        recordingInfoLabel->setText("No recording from this session", dontSendNotification);
        
        uploadButton->setVisible(false);
        saveLocallyButton->setVisible(false);
        discardButton->setVisible(false);
        doneButton->setVisible(true);
        uploadProgressLabel->setVisible(false);
    }
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

    titleLabel->setBounds(centerX - 150, centerY - 140, 300, 35);
    recordingInfoLabel->setBounds(centerX - 150, centerY - 95, 300, 50);
    uploadProgressLabel->setBounds(centerX - 150, centerY - 35, 300, 25);

    uploadButton->setBounds(centerX - 120, centerY - 5, 240, 45);
    saveLocallyButton->setBounds(centerX - 120, centerY + 50, 240, 45);
    discardButton->setBounds(centerX - 120, centerY + 105, 240, 45);
    
    doneButton->setBounds(centerX - 120, centerY - 5, 240, 45);
}

void EndSessionView::buttonClicked(Button* buttonThatWasClicked)
{
    if (buttonThatWasClicked == uploadButton.get())
    {
        if (hasRecording && onUploadFile)
        {
            isUploading = true;
            uploadProgressLabel->setText("Uploading...", dontSendNotification);
            updateUI();
            
            onUploadFile(lastRecordedFile, [this](bool success, const String& message) {
                MessageManager::callAsync([this, success, message]() {
                    isUploading = false;
                    
                    if (success)
                    {
                        uploadProgressLabel->setText("Upload complete!", dontSendNotification);
                        uploadProgressLabel->setColour(Label::textColourId, Colours::green);
                        hasRecording = false;
                        
                        Timer::callAfterDelay(1500, [this]() {
                            if (onUploadClicked)
                                onUploadClicked();
                        });
                    }
                    else
                    {
                        uploadProgressLabel->setText("Upload failed: " + message, dontSendNotification);
                        uploadProgressLabel->setColour(Label::textColourId, Colour(0xffe74c3c));
                        updateUI();
                    }
                });
            });
        }
        else if (onUploadClicked)
        {
            onUploadClicked();
        }
    }
    else if (buttonThatWasClicked == saveLocallyButton.get())
    {
        // Recording is already saved locally, just acknowledge and go home
        if (onSaveLocallyClicked)
            onSaveLocallyClicked();
    }
    else if (buttonThatWasClicked == discardButton.get())
    {
        // Optionally delete the file
        if (hasRecording && lastRecordedFile.isLocalFile())
        {
            File localFile = lastRecordedFile.getLocalFile();
            if (localFile.existsAsFile())
            {
                localFile.deleteFile();
            }
        }
        
        hasRecording = false;
        updateUI();
        
        if (onDiscardClicked)
            onDiscardClicked();
    }
    else if (buttonThatWasClicked == doneButton.get())
    {
        // No recording case - just go home
        if (onSaveLocallyClicked)
            onSaveLocallyClicked();
    }
}