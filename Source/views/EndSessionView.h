// SPDX-License-Identifier: GPLv3-or-later WITH Appstore-exception
// Copyright (C) 2024 SoundFlip

#pragma once

#include <JuceHeader.h>

class SessionManager;

class EndSessionView : public Component,
                       public Button::Listener
{
public:
    EndSessionView();
    EndSessionView(SessionManager* sessionManager);
    ~EndSessionView() override;

    void paint(Graphics&) override;
    void resized() override;
    void buttonClicked(Button* buttonThatWasClicked) override;
    
    // Set recording info from the ended session
    void setRecordingInfo(const URL& recordedFile, double durationSeconds);
    
    // Clear state for reuse
    void reset();

    std::function<void()> onUploadClicked;
    std::function<void()> onSaveLocallyClicked;
    std::function<void()> onDiscardClicked;
    std::function<void(const URL&, std::function<void(bool, const String&)>)> onUploadFile;

private:
    void updateUI();
    
    SessionManager* sessionManager = nullptr;
    
    URL lastRecordedFile;
    double recordingDuration = 0.0;
    bool hasRecording = false;
    bool isUploading = false;

    std::unique_ptr<Label> titleLabel;
    std::unique_ptr<Label> recordingInfoLabel;
    std::unique_ptr<Label> uploadProgressLabel;
    std::unique_ptr<TextButton> uploadButton;
    std::unique_ptr<TextButton> saveLocallyButton;
    std::unique_ptr<TextButton> discardButton;
    std::unique_ptr<TextButton> doneButton;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(EndSessionView)
};