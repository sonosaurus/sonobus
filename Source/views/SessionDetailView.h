// SPDX-License-Identifier: GPLv3-or-later WITH Appstore-exception
// Copyright (C) 2024 SoundFlip

#pragma once

#include <JuceHeader.h>
#include "../api/SoundFlipAPI.h"

class SessionDetailView : public Component,
                          public Button::Listener
{
public:
    SessionDetailView();
    ~SessionDetailView() override;

    void paint(Graphics&) override;
    void resized() override;
    void buttonClicked(Button* buttonThatWasClicked) override;

    void setSession(const String& sessionId, SoundFlipAPI* apiRef);
    void setSessionInfo(const String& name, const String& date, const String& duration, const StringArray& participants);
    
    void fetchStems();
    void refreshStemList();

    std::function<void()> onBackClicked;
    std::function<void()> onOpenInSoundFlipClicked;
    std::function<void(int)> onDownloadStemClicked;

private:
    void createStemRow(const SoundFlipAPI::Stem& stem, int index);
    void clearStemRows();
    String formatDuration(int seconds);
    String formatDate(const String& isoDate);

    SoundFlipAPI* api = nullptr;
    String currentSessionId;
    String inviteCode;
    Array<SoundFlipAPI::Stem> stems;

    std::unique_ptr<TextButton> backButton;
    std::unique_ptr<Label> sessionNameLabel;
    std::unique_ptr<Label> sessionInfoLabel;
    std::unique_ptr<Label> participantsLabel;
    std::unique_ptr<Label> stemsLabel;
    std::unique_ptr<Label> noStemsLabel;
    std::unique_ptr<TextButton> openInSoundFlipButton;
    
    struct StemRow
    {
        std::unique_ptr<Label> infoLabel;
        std::unique_ptr<TextButton> downloadButton;
    };
    OwnedArray<StemRow> stemRows;
    
    String sessionName;
    String sessionDate;
    String sessionDuration;
    StringArray sessionParticipants;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SessionDetailView)
};