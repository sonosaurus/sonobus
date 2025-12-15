#pragma once

#include <JuceHeader.h>

class SessionManager;
class SonobusAudioProcessorEditor;
class SoundFlipAPI;

class ActiveSessionView : public Component,
                          public ChangeListener,
                          public MultiTimer  // Changed from Timer
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
    void timerCallback(int timerId) override;  // MultiTimer signature
    
    void setSessionManager(SessionManager* sm);
    void setSoundFlipAPI(SoundFlipAPI* api);
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
    void handleEndSession();
    void handleInviteClicked();
    void updateParticipantsUI();
    void fetchAndUpdateParticipants();
    void updateRecordingTimeDisplay();

    SessionManager* sessionManager = nullptr;
    SonobusAudioProcessorEditor* editor = nullptr;
    SoundFlipAPI* api = nullptr;
    
    String currentSessionId;
    
    enum TimerIds {
        ParticipantPollTimerId = 1,
        RecordingTimerId = 2
    };
    static constexpr int pollIntervalMs = 60000;
    static constexpr int recordingUpdateMs = 500;

    Label titleLabel;
    Label statusLabel;
    Label participantsLabel;
    Label participantListLabel;
    Label recordingTimeLabel;
    TextButton recordButton;
    TextButton chatButton;
    TextButton inviteButton;
    TextButton endSessionButton;
    
    String currentSessionName;
    String currentInviteUrl;
    
    bool isRecording = false;
    double recordingStartTime = 0.0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ActiveSessionView)
};