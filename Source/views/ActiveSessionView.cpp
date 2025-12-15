// SPDX-License-Identifier: GPLv3-or-later WITH Appstore-exception
// Copyright (C) 2024 SoundFlip

#include "ActiveSessionView.h"
#include "../managers/SessionManager.h"
#include "../SonobusPluginEditor.h"
#include "../SonobusPluginProcessor.h"
#include "../api/SoundFlipAPI.h"
#include "../SonoUtility.h"
#include "../SonoLookAndFeel.h"

ActiveSessionView::ActiveSessionView()
    : sessionManager(nullptr), editor(nullptr), api(nullptr), processor(nullptr)
{
    setupUI();
    setupMeters();
}

ActiveSessionView::ActiveSessionView(SessionManager* sm, SonobusAudioProcessorEditor* ed)
    : sessionManager(sm), editor(ed), api(nullptr), processor(nullptr)
{
    if (sessionManager)
        sessionManager->addChangeListener(this);
    
    setupUI();
    setupMeters();
    
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
    : sessionManager(sm), editor(ed), api(apiRef), processor(nullptr)
{
    if (sessionManager)
        sessionManager->addChangeListener(this);
    
    setupUI();
    setupMeters();
    
    if (sessionManager && sessionManager->isConnected())
    {
        currentSessionId = sessionManager->getCurrentSessionId();
        setSessionInfo(sessionManager->getCurrentSessionName(),
                       sessionManager->getInviteUrl());
        startTimer(ParticipantPollTimerId, pollIntervalMs);
        updateParticipantsUI();
    }
}

ActiveSessionView::~ActiveSessionView()
{
    stopTimer(ParticipantPollTimerId);
    stopTimer(RecordingTimerId);
    stopTimer(MeterUpdateTimerId);
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
    
    recordingTimeLabel.setText("", dontSendNotification);
    recordingTimeLabel.setFont(Font(16.0f, Font::bold));
    recordingTimeLabel.setJustificationType(Justification::centred);
    recordingTimeLabel.setColour(Label::textColourId, Colour(0xffff6b6b));
    recordingTimeLabel.setVisible(false);
    addAndMakeVisible(recordingTimeLabel);
    
    inputLevelLabel.setText("In", dontSendNotification);
    inputLevelLabel.setFont(Font(12.0f));
    inputLevelLabel.setJustificationType(Justification::centred);
    inputLevelLabel.setColour(Label::textColourId, Colour(0xffaaaaaa));
    addAndMakeVisible(inputLevelLabel);
    
    outputLevelLabel.setText("Out", dontSendNotification);
    outputLevelLabel.setFont(Font(12.0f));
    outputLevelLabel.setJustificationType(Justification::centred);
    outputLevelLabel.setColour(Label::textColourId, Colour(0xffaaaaaa));
    addAndMakeVisible(outputLevelLabel);

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

void ActiveSessionView::setupMeters()
{
    auto flags = foleys::LevelMeter::Minimal;
    
    inputMeter = std::make_unique<foleys::LevelMeter>(flags);
    inputMeter->setRefreshRateHz(30);
    addAndMakeVisible(inputMeter.get());
    
    outputMeter = std::make_unique<foleys::LevelMeter>(flags);
    outputMeter->setRefreshRateHz(30);
    addAndMakeVisible(outputMeter.get());
    
    // Start meter update timer
    startTimer(MeterUpdateTimerId, meterUpdateMs);
}

void ActiveSessionView::setProcessor(SonobusAudioProcessor* proc)
{
    processor = proc;
    
    if (processor)
    {
        // Connect meters to processor meter sources
        inputMeter->setMeterSource(&processor->getSendMeterSource());
        outputMeter->setMeterSource(&processor->getOutputMeterSource());
    }
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
                startTimer(ParticipantPollTimerId, pollIntervalMs);
            
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

void ActiveSessionView::updateRecordingState(bool recording, double elapsedTime)
{
    isRecording = recording;
    
    if (recording)
    {
        recordingStartTime = Time::getMillisecondCounterHiRes() * 0.001 - elapsedTime;
        recordButton.setButtonText("Stop");
        recordButton.setColour(TextButton::buttonColourId, Colour(0xffe74c3c));
        recordingTimeLabel.setVisible(true);
        updateRecordingTimeDisplay();
        startTimer(RecordingTimerId, recordingUpdateMs);
    }
    else
    {
        stopTimer(RecordingTimerId);
        recordButton.setButtonText("Record");
        recordButton.setColour(TextButton::buttonColourId, Colour(0xff6c5ce7));
        recordingTimeLabel.setVisible(false);
    }
}

void ActiveSessionView::updateRecordingTimeDisplay()
{
    if (isRecording)
    {
        double elapsed = Time::getMillisecondCounterHiRes() * 0.001 - recordingStartTime;
        String timeStr = SonoUtility::durationToString(elapsed, true);
        recordingTimeLabel.setText("REC " + timeStr, dontSendNotification);
    }
}

void ActiveSessionView::updateMeters()
{
    // Meters auto-update from their sources
    // This timer callback can be used for any additional meter-related processing
}

void ActiveSessionView::updatePeerNamesFromProcessor()
{
    if (!processor)
        return;
    
    int numPeers = processor->getNumberRemotePeers();
    StringArray newPeerNames;
    
    for (int i = 0; i < numPeers; ++i)
    {
        String peerName = processor->getRemotePeerUserName(i);
        if (peerName.isNotEmpty())
        {
            newPeerNames.add(peerName);
        }
    }
    
    // Only update if peer names changed
    if (newPeerNames != peerNames)
    {
        peerNames = newPeerNames;
    }
}

void ActiveSessionView::refreshParticipants()
{
    fetchAndUpdateParticipants();
}

void ActiveSessionView::timerCallback(int timerId)
{
    if (timerId == ParticipantPollTimerId)
    {
        if (api && !currentSessionId.isEmpty())
            fetchAndUpdateParticipants();
    }
    else if (timerId == RecordingTimerId)
    {
        updateRecordingTimeDisplay();
    }
    else if (timerId == MeterUpdateTimerId)
    {
        updateMeters();
    }
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
                startTimer(ParticipantPollTimerId, pollIntervalMs);
            
            fetchAndUpdateParticipants();
            statusLabel.setText("Connected", dontSendNotification);
            statusLabel.setColour(Label::textColourId, Colours::green);
        }
        else
        {
            stopTimer(ParticipantPollTimerId);
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

    // Update peer names from processor if available (for real-time peer tracking)
    if (processor)
    {
        updatePeerNamesFromProcessor();
    }

    // Always try to fetch from API for the authoritative participant list
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
    stopTimer(ParticipantPollTimerId);
    stopTimer(RecordingTimerId);
    stopTimer(MeterUpdateTimerId);
    
    if (isRecording && onRecordClicked)
    {
        onRecordClicked();
    }
    
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
    
    bounds.removeFromTop(20);
    
    // Level meters section
    int meterWidth = 30;
    int meterHeight = 100;
    int meterSpacing = 60;
    
    auto metersArea = bounds.removeFromTop(meterHeight + 20);
    int metersStartX = (metersArea.getWidth() - (meterWidth * 2 + meterSpacing)) / 2;
    
    inputLevelLabel.setBounds(metersArea.getX() + metersStartX, metersArea.getY(), meterWidth, 16);
    inputMeter->setBounds(metersArea.getX() + metersStartX, metersArea.getY() + 18, meterWidth, meterHeight);
    
    outputLevelLabel.setBounds(metersArea.getX() + metersStartX + meterWidth + meterSpacing, metersArea.getY(), meterWidth, 16);
    outputMeter->setBounds(metersArea.getX() + metersStartX + meterWidth + meterSpacing, metersArea.getY() + 18, meterWidth, meterHeight);
    
    bounds.removeFromTop(10);
    
    recordingTimeLabel.setBounds(bounds.removeFromTop(30));
    
    bounds.removeFromTop(10);
    
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