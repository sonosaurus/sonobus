// SPDX-License-Identifier: GPLv3-or-later WITH Appstore-exception
// Copyright (C) 2024 SoundFlip

#pragma once

#include "JuceHeader.h"

// Forward declarations
class SessionManager;
class SonobusAudioProcessor;

class ActiveSessionView : public Component,
                          public ChangeListener
{
public:
    ActiveSessionView(SessionManager* sessionManager = nullptr,
                      SonobusAudioProcessor* processor = nullptr);
    ~ActiveSessionView() override;

    void paint(Graphics& g) override;
    void resized() override;
    void changeListenerCallback(ChangeBroadcaster* source) override;
    
    /** Set managers after construction if needed */
    void setSessionManager(SessionManager* sm);
    void setProcessor(SonobusAudioProcessor* proc) { processor = proc; }
    
    /** Update session info displayed in the view */
    void setSessionInfo(const String& name, const String& inviteUrl);
    
    /** Refresh participant list */
    void refreshParticipants();

    // Callbacks
    std::function<void()> onEndClicked;
    std::function<void()> onRecordClicked;
    std::function<void()> onChatClicked;
    std::function<void()> onInviteClicked;
    std::function<void()> onSessionEnded;

private:
    void handleInviteClicked();
    void handleEndSession();
    void updateParticipantsUI();
    
    // Managers
    SessionManager* sessionManager = nullptr;
    SonobusAudioProcessor* processor = nullptr;
    
    // Session info
    String currentSessionName;
    String currentInviteUrl;
    
    // UI Components
    Label titleLabel;
    Label statusLabel;
    Label participantsLabel;
    Label participantListLabel;
    TextButton endSessionButton;
    TextButton recordButton;
    TextButton chatButton;
    TextButton inviteButton;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ActiveSessionView)
};