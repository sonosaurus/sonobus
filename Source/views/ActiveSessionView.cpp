#include "ActiveSessionView.h"
#include "../managers/SessionManager.h"
#include "../SonobusPluginEditor.h"
#include "../api/SoundFlipAPI.h"

ActiveSessionView::ActiveSessionView()
    : sessionManager(nullptr), editor(nullptr), api(nullptr)
{
    setupUI();
}

ActiveSessionView::ActiveSessionView(SessionManager* sm, SonobusAudioProcessorEditor* ed)
    : sessionManager(sm), editor(ed), api(nullptr)
{
    if (sessionManager)
        sessionManager->addChangeListener(this);
    
    setupUI();
    
    if (sessionManager && sessionManager->isConnected())
    {
        currentSessionId = sessionManager->getCurrentSessionId();
        setSessionInfo(sessionManager->getCurrentSessionName(),
                       sessionManager->getInviteUrl());
        updateParticipantsUI();
    }
}

ActiveSessionView::ActiveSessionView(SessionManager* sm, 
                                     SonobusAudioProcessorEditor* ed,
                                     SoundFlipAPI* apiRef)
    : sessionManager(sm), editor(ed), api(apiRef)
{
    if (sessionManager)
        sessionManager->addChangeListener(this);
    
    setupUI();
    
    if (sessionManager && sessionManager->isConnected())
    {
        currentSessionId = sessionManager->getCurrentSessionId();
        setSessionInfo(sessionManager->getCurrentSessionName(),
                       sessionManager->getInviteUrl());
        startTimer(pollIntervalMs);
        updateParticipantsUI();
    }
}

ActiveSessionView::~ActiveSessionView()
{
    stopTimer();
    if (sessionManager)
        sessionManager->removeChangeListener(this);
}

void ActiveSessionView::setupUI()
{
    titleLabel.setText("Active Session", dontSendNotification);
    titleLabel.setFont(Font(24.0f, Font::bold));
    titleLabel.setJustificationType(Justification::centred);
    titleLabel.setColour(Label::textColourId, Colours::white);
    addAndMakeVisible(titleLabel);

    statusLabel.setText("Connected", dontSendNotification);
    statusLabel.setJustificationType(Justification::centred);
    statusLabel.setColour(Label::textColourId, Colours::green);
    addAndMakeVisible(statusLabel);
    
    participantsLabel.setText("Participants:", dontSendNotification);
    participantsLabel.setFont(Font(14.0f, Font::bold));
    participantsLabel.setColour(Label::textColourId, Colours::white);
    addAndMakeVisible(participantsLabel);
    
    participantListLabel.setText("Loading...", dontSendNotification);
    participantListLabel.setColour(Label::textColourId, Colour(0xffaaaaaa));
    addAndMakeVisible(participantListLabel);

    recordButton.setButtonText("Record");
    recordButton.setColour(TextButton::buttonColourId, Colour(0xff6c5ce7));
    recordButton.onClick = [this]() {
        if (onRecordClicked)
            onRecordClicked();
    };
    addAndMakeVisible(recordButton);

    chatButton.setButtonText("Chat");
    chatButton.onClick = [this]() {
        if (onChatClicked)
            onChatClicked();
    };
    addAndMakeVisible(chatButton);

    inviteButton.setButtonText("Invite");
    inviteButton.setColour(TextButton::buttonColourId, Colour(0xff00cec9));
    inviteButton.onClick = [this]() {
        handleInviteClicked();
    };
    addAndMakeVisible(inviteButton);

    endSessionButton.setButtonText("Leave Session");
    endSessionButton.setColour(TextButton::buttonColourId, Colour(0xffe74c3c));
    endSessionButton.onClick = [this]() {
        handleEndSession();
    };
    addAndMakeVisible(endSessionButton);
}

void ActiveSessionView::setSoundFlipAPI(SoundFlipAPI* apiRef)
{
    api = apiRef;
}

void ActiveSessionView::setSessionManager(SessionManager* sm)
{
    if (sessionManager)
        sessionManager->removeChangeListener(this);
    
    sessionManager = sm;
    
    if (sessionManager)
    {
        sessionManager->addChangeListener(this);
        
        if (sessionManager->isConnected())
        {
            currentSessionId = sessionManager->getCurrentSessionId();
            setSessionInfo(sessionManager->getCurrentSessionName(),
                           sessionManager->getInviteUrl());
            
            if (api && !currentSessionId.isEmpty())
                startTimer(pollIntervalMs);
            
            updateParticipantsUI();
        }
    }
}

void ActiveSessionView::setSessionInfo(const String& name, const String& inviteUrl)
{
    currentSessionName = name;
    currentInviteUrl = inviteUrl;
    
    if (currentSessionName.isNotEmpty())
        titleLabel.setText(currentSessionName, dontSendNotification);
    else
        titleLabel.setText("Active Session", dontSendNotification);
}

void ActiveSessionView::refreshParticipants()
{
    fetchAndUpdateParticipants();
}

void ActiveSessionView::timerCallback()
{
    if (api && !currentSessionId.isEmpty())
        fetchAndUpdateParticipants();
}

void ActiveSessionView::changeListenerCallback(ChangeBroadcaster* source)
{
    if (source == sessionManager)
    {
        if (sessionManager->isConnected())
        {
            currentSessionId = sessionManager->getCurrentSessionId();
            setSessionInfo(sessionManager->getCurrentSessionName(),
                           sessionManager->getInviteUrl());
            
            if (api && !currentSessionId.isEmpty())
                startTimer(pollIntervalMs);
            
            fetchAndUpdateParticipants();
            statusLabel.setText("Connected", dontSendNotification);
            statusLabel.setColour(Label::textColourId, Colours::green);
        }
        else
        {
            stopTimer();
            statusLabel.setText("Disconnected", dontSendNotification);
            statusLabel.setColour(Label::textColourId, Colour(0xffe74c3c));
        }
    }
}

void ActiveSessionView::fetchAndUpdateParticipants()
{
    DBG("=== fetchAndUpdateParticipants called ===");
    DBG("API pointer: " + String(api == nullptr ? "NULL" : "valid"));
    DBG("Session ID: " + currentSessionId);

    if (!api || currentSessionId.isEmpty())
    {
        updateParticipantsUI();
        return;
    }
    
    SoundFlipAPI::CollabSession session = api->getCollabSession(currentSessionId);
    
    DBG("API Status Code: " + String(api->getLastStatusCode()));
    DBG("Participants count from API: " + String(session.participants.size()));

    if (api->getLastStatusCode() != 200)
    {
        DBG("Failed to fetch session participants: " + api->getLastError());
        updateParticipantsUI();
        return;
    }
    
    if (session.participants.isEmpty())
    {
        participantListLabel.setText("Just you", dontSendNotification);
        return;
    }
    
    String participantText = "(" + String(session.participants.size()) + ") ";
    for (int i = 0; i < session.participants.size(); ++i)
    {
        if (i > 0) participantText += ", ";
        participantText += "@" + session.participants[i].username;
    }
    
    participantListLabel.setText(participantText, dontSendNotification);
}

void ActiveSessionView::updateParticipantsUI()
{
    if (!sessionManager)
    {
        participantListLabel.setText("No session", dontSendNotification);
        return;
    }
    
    const auto& participants = sessionManager->getParticipants();
    
    if (participants.isEmpty())
    {
        participantListLabel.setText("Just you", dontSendNotification);
        return;
    }
    
    String participantText = "(" + String(participants.size()) + ") ";
    for (int i = 0; i < participants.size(); ++i)
    {
        if (i > 0) participantText += ", ";
        participantText += "@" + participants[i].username;
    }
    
    participantListLabel.setText(participantText, dontSendNotification);
}

void ActiveSessionView::handleInviteClicked()
{
    if (currentInviteUrl.isNotEmpty())
    {
        SystemClipboard::copyTextToClipboard(currentInviteUrl);
        
        inviteButton.setButtonText("Copied!");
        
        Timer::callAfterDelay(2000, [this]() {
            if (inviteButton.isShowing())
                inviteButton.setButtonText("Invite");
        });
    }
    
    if (onInviteClicked)
        onInviteClicked();
}

void ActiveSessionView::handleEndSession()
{
    stopTimer();
    
    if (onEndClicked)
        onEndClicked();
    
    if (editor)
    {
        editor->disconnectSoundFlipSession();
    }
    
    if (sessionManager)
    {
        sessionManager->leaveSession();
    }
    
    if (onSessionEnded)
        onSessionEnded();
}

void ActiveSessionView::paint(Graphics& g)
{
    g.fillAll(Colour(0xff1a1a2e));
}

void ActiveSessionView::resized()
{
    auto bounds = getLocalBounds().reduced(40);
    
    titleLabel.setBounds(bounds.removeFromTop(40));
    bounds.removeFromTop(10);
    statusLabel.setBounds(bounds.removeFromTop(25));
    
    bounds.removeFromTop(20);
    
    participantsLabel.setBounds(bounds.removeFromTop(20));
    participantListLabel.setBounds(bounds.removeFromTop(25));
    
    bounds.removeFromTop(30);
    
    int buttonWidth = 120;
    int buttonHeight = 40;
    int spacing = 15;
    
    auto buttonRow = bounds.removeFromTop(buttonHeight);
    int totalButtonWidth = buttonWidth * 3 + spacing * 2;
    int startX = (buttonRow.getWidth() - totalButtonWidth) / 2;
    
    recordButton.setBounds(buttonRow.getX() + startX, buttonRow.getY(), buttonWidth, buttonHeight);
    chatButton.setBounds(buttonRow.getX() + startX + buttonWidth + spacing, buttonRow.getY(), buttonWidth, buttonHeight);
    inviteButton.setBounds(buttonRow.getX() + startX + (buttonWidth + spacing) * 2, buttonRow.getY(), buttonWidth, buttonHeight);
    
    bounds.removeFromTop(30);
    
    endSessionButton.setBounds((getWidth() - 150) / 2, bounds.getY(), 150, buttonHeight);
}