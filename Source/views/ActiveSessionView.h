#pragma once

#include <JuceHeader.h>

class SessionManager;
class SonobusAudioProcessorEditor;
class SoundFlipAPI;

class ActiveSessionView : public Component,
                          public ChangeListener,
                          public Timer
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
    void timerCallback() override;
    
    void setSessionManager(SessionManager* sm);
    void setSoundFlipAPI(SoundFlipAPI* api);
    void setSessionInfo(const String& name, const String& inviteUrl);
    void refreshParticipants();
    
    void changeListenerCallback(ChangeBroadcaster* source) override;

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

    SessionManager* sessionManager = nullptr;
    SonobusAudioProcessorEditor* editor = nullptr;
    SoundFlipAPI* api = nullptr;
    
    String currentSessionId;
    static constexpr int pollIntervalMs = 60000;  // 60 seconds

    Label titleLabel;
    Label statusLabel;
    Label participantsLabel;
    Label participantListLabel;
    TextButton recordButton;
    TextButton chatButton;
    TextButton inviteButton;
    TextButton endSessionButton;
    
    String currentSessionName;
    String currentInviteUrl;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ActiveSessionView)
};