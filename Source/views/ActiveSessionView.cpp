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
    setupControls();
}

ActiveSessionView::ActiveSessionView(SessionManager* sm, SonobusAudioProcessorEditor* ed)
    : sessionManager(sm), editor(ed), api(nullptr), processor(nullptr)
{
    if (sessionManager)
        sessionManager->addChangeListener(this);
    
    setupUI();
    setupMeters();
    setupControls();
    
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
    setupControls();
    
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

    // Main action buttons
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

void ActiveSessionView::setupControls()
{
    // Monitor toggle - routes input to output so you can hear yourself
    monitorButton.setButtonText("Monitor");
    monitorButton.setColour(ToggleButton::textColourId, Colours::white);
    monitorButton.setColour(ToggleButton::tickColourId, Colour(0xff00cec9));
    monitorButton.onClick = [this]() {
        if (processor)
        {
            bool enabled = monitorButton.getToggleState();
            // dry parameter controls input monitoring level (0 = off, 1 = full)
            auto* param = processor->getValueTreeState().getParameter("dry");
            if (param)
                param->setValueNotifyingHost(enabled ? 1.0f : 0.0f);
        }
    };
    addAndMakeVisible(monitorButton);
    
    // Mute input toggle - mutes your microphone entirely
    muteInputButton.setButtonText("Mute Mic");
    muteInputButton.setColour(ToggleButton::textColourId, Colours::white);
    muteInputButton.setColour(ToggleButton::tickColourId, Colour(0xffe74c3c));
    muteInputButton.onClick = [this]() {
        if (processor)
        {
            bool muted = muteInputButton.getToggleState();
            auto* param = processor->getValueTreeState().getParameter("mastinmute");
            if (param)
                param->setValueNotifyingHost(muted ? 1.0f : 0.0f);
        }
    };
    addAndMakeVisible(muteInputButton);
    
    // Mute send toggle - stops sending audio to others
    muteSendButton.setButtonText("Mute Send");
    muteSendButton.setColour(ToggleButton::textColourId, Colours::white);
    muteSendButton.setColour(ToggleButton::tickColourId, Colour(0xffe74c3c));
    muteSendButton.onClick = [this]() {
        if (processor)
        {
            bool muted = muteSendButton.getToggleState();
            auto* param = processor->getValueTreeState().getParameter("mastsendmute");
            if (param)
                param->setValueNotifyingHost(muted ? 1.0f : 0.0f);
        }
    };
    addAndMakeVisible(muteSendButton);
    
    // Mute receive toggle - mutes incoming audio from others
    muteReceiveButton.setButtonText("Mute Recv");
    muteReceiveButton.setColour(ToggleButton::textColourId, Colours::white);
    muteReceiveButton.setColour(ToggleButton::tickColourId, Colour(0xffe74c3c));
    muteReceiveButton.onClick = [this]() {
        if (processor)
        {
            bool muted = muteReceiveButton.getToggleState();
            auto* param = processor->getValueTreeState().getParameter("mastrecvmute");
            if (param)
                param->setValueNotifyingHost(muted ? 1.0f : 0.0f);
        }
    };
    addAndMakeVisible(muteReceiveButton);
    
    // Input gain slider
    inputGainLabel.setText("Input", dontSendNotification);
    inputGainLabel.setFont(Font(12.0f));
    inputGainLabel.setColour(Label::textColourId, Colour(0xffaaaaaa));
    inputGainLabel.setJustificationType(Justification::centred);
    addAndMakeVisible(inputGainLabel);
    
    inputGainSlider.setSliderStyle(Slider::LinearHorizontal);
    inputGainSlider.setRange(0.0, 2.0, 0.01);
    inputGainSlider.setValue(1.0);
    inputGainSlider.setTextBoxStyle(Slider::NoTextBox, false, 0, 0);
    inputGainSlider.setColour(Slider::thumbColourId, Colour(0xff00cec9));
    inputGainSlider.setColour(Slider::trackColourId, Colour(0xff444466));
    inputGainSlider.onValueChange = [this]() {
        if (processor)
        {
            float val = (float)inputGainSlider.getValue();
            auto* param = processor->getValueTreeState().getParameter("ingain");
            if (param)
                param->setValueNotifyingHost(param->convertTo0to1(val));
        }
    };
    addAndMakeVisible(inputGainSlider);
    
    // Output level slider
    outputGainLabel.setText("Output", dontSendNotification);
    outputGainLabel.setFont(Font(12.0f));
    outputGainLabel.setColour(Label::textColourId, Colour(0xffaaaaaa));
    outputGainLabel.setJustificationType(Justification::centred);
    addAndMakeVisible(outputGainLabel);
    
    outputLevelSlider.setSliderStyle(Slider::LinearHorizontal);
    outputLevelSlider.setRange(0.0, 2.0, 0.01);
    outputLevelSlider.setValue(1.0);
    outputLevelSlider.setTextBoxStyle(Slider::NoTextBox, false, 0, 0);
    outputLevelSlider.setColour(Slider::thumbColourId, Colour(0xff00cec9));
    outputLevelSlider.setColour(Slider::trackColourId, Colour(0xff444466));
    outputLevelSlider.onValueChange = [this]() {
        if (processor)
        {
            float val = (float)outputLevelSlider.getValue();
            auto* param = processor->getValueTreeState().getParameter("wet");
            if (param)
                param->setValueNotifyingHost(param->convertTo0to1(val));
        }
    };
    addAndMakeVisible(outputLevelSlider);
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
        
        // Sync all controls with processor state
        syncControlsWithProcessor();
    }
}

void ActiveSessionView::syncControlsWithProcessor()
{
    if (!processor)
        return;
    
    auto& vts = processor->getValueTreeState();
    
    // Sync monitor state (dry parameter)
    if (auto* p = vts.getParameter("dry"))
    {
        bool isMonitoring = p->getValue() > 0.01f;
        monitorButton.setToggleState(isMonitoring, dontSendNotification);
    }
    
    // Sync mute input state
    if (auto* p = vts.getParameter("mastinmute"))
    {
        muteInputButton.setToggleState(p->getValue() > 0.5f, dontSendNotification);
    }
    
    // Sync mute send state
    if (auto* p = vts.getParameter("mastsendmute"))
    {
        muteSendButton.setToggleState(p->getValue() > 0.5f, dontSendNotification);
    }
    
    // Sync mute receive state
    if (auto* p = vts.getParameter("mastrecvmute"))
    {
        muteReceiveButton.setToggleState(p->getValue() > 0.5f, dontSendNotification);
    }
    
    // Sync input gain slider
    if (auto* p = vts.getParameter("ingain"))
    {
        float val = p->convertFrom0to1(p->getValue());
        inputGainSlider.setValue(val, dontSendNotification);
    }
    
    // Sync output level slider
    if (auto* p = vts.getParameter("wet"))
    {
        float val = p->convertFrom0to1(p->getValue());
        outputLevelSlider.setValue(val, dontSendNotification);
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
    auto bounds = getLocalBounds().reduced(20);
    
    // Title and status
    titleLabel.setBounds(bounds.removeFromTop(35));
    bounds.removeFromTop(5);
    statusLabel.setBounds(bounds.removeFromTop(20));
    
    bounds.removeFromTop(15);
    
    // Participants
    participantsLabel.setBounds(bounds.removeFromTop(18));
    participantListLabel.setBounds(bounds.removeFromTop(22));
    
    bounds.removeFromTop(15);
    
    // Level meters section
    int meterWidth = 25;
    int meterHeight = 80;
    int meterSpacing = 50;
    
    auto metersArea = bounds.removeFromTop(meterHeight + 18);
    int metersStartX = (metersArea.getWidth() - (meterWidth * 2 + meterSpacing)) / 2;
    
    inputLevelLabel.setBounds(metersArea.getX() + metersStartX, metersArea.getY(), meterWidth, 14);
    inputMeter->setBounds(metersArea.getX() + metersStartX, metersArea.getY() + 16, meterWidth, meterHeight);
    
    outputLevelLabel.setBounds(metersArea.getX() + metersStartX + meterWidth + meterSpacing, metersArea.getY(), meterWidth, 14);
    outputMeter->setBounds(metersArea.getX() + metersStartX + meterWidth + meterSpacing, metersArea.getY() + 16, meterWidth, meterHeight);
    
    bounds.removeFromTop(10);
    
    // Gain sliders section
    int sliderWidth = 150;
    int sliderHeight = 20;
    int labelWidth = 50;
    
    auto sliderArea = bounds.removeFromTop(sliderHeight * 2 + 15);
    int sliderStartX = (sliderArea.getWidth() - (labelWidth + sliderWidth)) / 2;
    
    inputGainLabel.setBounds(sliderArea.getX() + sliderStartX, sliderArea.getY(), labelWidth, sliderHeight);
    inputGainSlider.setBounds(sliderArea.getX() + sliderStartX + labelWidth, sliderArea.getY(), sliderWidth, sliderHeight);
    
    outputGainLabel.setBounds(sliderArea.getX() + sliderStartX, sliderArea.getY() + sliderHeight + 5, labelWidth, sliderHeight);
    outputLevelSlider.setBounds(sliderArea.getX() + sliderStartX + labelWidth, sliderArea.getY() + sliderHeight + 5, sliderWidth, sliderHeight);
    
    bounds.removeFromTop(10);
    
    // Audio control toggles row
    int toggleWidth = 90;
    int toggleHeight = 24;
    int toggleSpacing = 8;
    
    auto toggleRow = bounds.removeFromTop(toggleHeight);
    int totalToggleWidth = toggleWidth * 4 + toggleSpacing * 3;
    int toggleStartX = (toggleRow.getWidth() - totalToggleWidth) / 2;
    
    monitorButton.setBounds(toggleRow.getX() + toggleStartX, toggleRow.getY(), toggleWidth, toggleHeight);
    muteInputButton.setBounds(toggleRow.getX() + toggleStartX + toggleWidth + toggleSpacing, toggleRow.getY(), toggleWidth, toggleHeight);
    muteSendButton.setBounds(toggleRow.getX() + toggleStartX + (toggleWidth + toggleSpacing) * 2, toggleRow.getY(), toggleWidth, toggleHeight);
    muteReceiveButton.setBounds(toggleRow.getX() + toggleStartX + (toggleWidth + toggleSpacing) * 3, toggleRow.getY(), toggleWidth, toggleHeight);
    
    bounds.removeFromTop(10);
    
    // Recording time label
    recordingTimeLabel.setBounds(bounds.removeFromTop(25));
    
    bounds.removeFromTop(10);
    
    // Main action buttons row
    int buttonWidth = 100;
    int buttonHeight = 36;
    int buttonSpacing = 12;
    
    auto buttonRow = bounds.removeFromTop(buttonHeight);
    int totalButtonWidth = buttonWidth * 3 + buttonSpacing * 2;
    int buttonStartX = (buttonRow.getWidth() - totalButtonWidth) / 2;
    
    recordButton.setBounds(buttonRow.getX() + buttonStartX, buttonRow.getY(), buttonWidth, buttonHeight);
    chatButton.setBounds(buttonRow.getX() + buttonStartX + buttonWidth + buttonSpacing, buttonRow.getY(), buttonWidth, buttonHeight);
    inviteButton.setBounds(buttonRow.getX() + buttonStartX + (buttonWidth + buttonSpacing) * 2, buttonRow.getY(), buttonWidth, buttonHeight);
    
    bounds.removeFromTop(20);
    
    // End session button
    endSessionButton.setBounds((getWidth() - 140) / 2, bounds.getY(), 140, buttonHeight);
}