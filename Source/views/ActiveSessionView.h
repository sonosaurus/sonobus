// SPDX-License-Identifier: GPLv3-or-later WITH Appstore-exception
// Copyright (C) 2024 SoundFlip

#pragma once

#include <JuceHeader.h>

class SessionManager;
class SonobusAudioProcessorEditor;
class SoundFlipAPI;
class SonobusAudioProcessor;

namespace foleys { class LevelMeter; }

class ActiveSessionView : public Component,
                          public ChangeListener,
                          public MultiTimer
{
public:
    ActiveSessionView();
    
    ActiveSessionView(SessionManager* sessionManager, 
                      SonobusAudioProcessorEditor* editor);
    
    ActiveSessionView(SessionManager* sessionManager, 
                      SonobusAudioProcessorEditor* editor,
                      SoundFlipAPI* api);
    
    ~ActiveSessionView() override;

    void paint(Graphics& g) override;
    void resized() override;
    void timerCallback(int timerId) override;
    
    void setSessionManager(SessionManager* sm);
    void setSoundFlipAPI(SoundFlipAPI* api);
    void setProcessor(SonobusAudioProcessor* proc);
    void setSessionInfo(const String& name, const String& inviteUrl);
    void refreshParticipants();
    
    void changeListenerCallback(ChangeBroadcaster* source) override;
    
    void updateRecordingState(bool recording, double elapsedTime = 0.0);

    std::function<void()> onEndClicked;
    std::function<void()> onRecordClicked;
    std::function<void()> onChatClicked;
    std::function<void()> onInviteClicked;
    std::function<void()> onSessionEnded;

private:
    void setupUI();
    void setupMeters();
    void setupControls();
    void handleEndSession();
    void handleInviteClicked();
    void updateParticipantsUI();
    void fetchAndUpdateParticipants();
    void updateRecordingTimeDisplay();
    void updateMeters();
    void updatePeerNamesFromProcessor();
    void syncControlsWithProcessor();

    SessionManager* sessionManager = nullptr;
    SonobusAudioProcessorEditor* editor = nullptr;
    SoundFlipAPI* api = nullptr;
    SonobusAudioProcessor* processor = nullptr;
    
    String currentSessionId;
    
    enum TimerIds {
        ParticipantPollTimerId = 1,
        RecordingTimerId = 2,
        MeterUpdateTimerId = 3
    };
    static constexpr int pollIntervalMs = 60000;
    static constexpr int recordingUpdateMs = 500;
    static constexpr int meterUpdateMs = 50;

    // Labels
    Label titleLabel;
    Label statusLabel;
    Label participantsLabel;
    Label participantListLabel;
    Label recordingTimeLabel;
    Label inputLevelLabel;
    Label outputLevelLabel;
    Label inputGainLabel;
    Label outputGainLabel;
    
    // Main action buttons
    TextButton recordButton;
    TextButton chatButton;
    TextButton inviteButton;
    TextButton endSessionButton;
    
    // Audio control toggles
    ToggleButton monitorButton;
    ToggleButton muteInputButton;
    ToggleButton muteSendButton;
    ToggleButton muteReceiveButton;
    
    // Level sliders
    Slider inputGainSlider;
    Slider outputLevelSlider;
    
    // Level meters
    std::unique_ptr<foleys::LevelMeter> inputMeter;
    std::unique_ptr<foleys::LevelMeter> outputMeter;
    
    String currentSessionName;
    String currentInviteUrl;
    
    bool isRecording = false;
    double recordingStartTime = 0.0;
    
    // Peer names from processor (for real-time display)
    StringArray peerNames;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ActiveSessionView)
};