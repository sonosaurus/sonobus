// SPDX-License-Identifier: GPLv3-or-later WITH Appstore-exception
// Copyright (C) 2024 SoundFlip

#include "SessionDetailView.h"

SessionDetailView::SessionDetailView()
{
    backButton = std::make_unique<TextButton>("< Back");
    backButton->addListener(this);
    addAndMakeVisible(backButton.get());

    sessionNameLabel = std::make_unique<Label>("name", "Session");
    sessionNameLabel->setFont(Font(24.0f, Font::bold));
    sessionNameLabel->setColour(Label::textColourId, Colours::white);
    addAndMakeVisible(sessionNameLabel.get());

    sessionInfoLabel = std::make_unique<Label>("info", "");
    sessionInfoLabel->setFont(Font(14.0f));
    sessionInfoLabel->setColour(Label::textColourId, Colours::grey);
    addAndMakeVisible(sessionInfoLabel.get());

    participantsLabel = std::make_unique<Label>("participants", "");
    participantsLabel->setFont(Font(14.0f));
    participantsLabel->setColour(Label::textColourId, Colours::grey);
    addAndMakeVisible(participantsLabel.get());

    stemsLabel = std::make_unique<Label>("stems", "Stems");
    stemsLabel->setFont(Font(18.0f, Font::bold));
    stemsLabel->setColour(Label::textColourId, Colours::white);
    addAndMakeVisible(stemsLabel.get());
    
    noStemsLabel = std::make_unique<Label>("nostems", "No stems uploaded yet");
    noStemsLabel->setFont(Font(14.0f));
    noStemsLabel->setColour(Label::textColourId, Colours::grey);
    noStemsLabel->setJustificationType(Justification::centred);
    addAndMakeVisible(noStemsLabel.get());

    openInSoundFlipButton = std::make_unique<TextButton>("Open in SoundFlip");
    openInSoundFlipButton->addListener(this);
    openInSoundFlipButton->setColour(TextButton::buttonColourId, Colour(0xff6c5ce7));
    addAndMakeVisible(openInSoundFlipButton.get());
}

SessionDetailView::~SessionDetailView()
{
    clearStemRows();
}

void SessionDetailView::setSession(const String& sessionId, SoundFlipAPI* apiRef)
{
    currentSessionId = sessionId;
    api = apiRef;
    
    if (api && currentSessionId.isNotEmpty())
    {
        auto session = api->getCollabSession(currentSessionId);
        if (api->getLastStatusCode() == 200)
        {
            sessionName = session.name;
            inviteCode = session.inviteCode;
            sessionNameLabel->setText(sessionName, dontSendNotification);
            
            StringArray participantNames;
            for (const auto& p : session.participants)
            {
                participantNames.add("@" + p.username);
            }
            sessionParticipants = participantNames;
            
            if (participantNames.size() > 0)
            {
                participantsLabel->setText("with " + participantNames.joinIntoString(", "), dontSendNotification);
            }
            else
            {
                participantsLabel->setText("", dontSendNotification);
            }
            
            sessionInfoLabel->setText(formatDate(String(session.createdAt)), dontSendNotification);
        }
        
        fetchStems();
    }
}

void SessionDetailView::setSessionInfo(const String& name, const String& date, const String& duration, const StringArray& participants)
{
    sessionName = name;
    sessionDate = date;
    sessionDuration = duration;
    sessionParticipants = participants;
    
    sessionNameLabel->setText(name, dontSendNotification);
    
    String infoText = date;
    if (duration.isNotEmpty())
        infoText += " - " + duration;
    sessionInfoLabel->setText(infoText, dontSendNotification);
    
    if (participants.size() > 0)
    {
        participantsLabel->setText("with " + participants.joinIntoString(", "), dontSendNotification);
    }
    else
    {
        participantsLabel->setText("", dontSendNotification);
    }
}

void SessionDetailView::fetchStems()
{
    if (!api || currentSessionId.isEmpty())
        return;
    
    DBG("SessionDetailView: Fetching stems for session " + currentSessionId);
    
    auto fetchedStems = api->listSessionStems(currentSessionId);
    
    if (api->getLastStatusCode() == 200)
    {
        stems.clear();
        for (const auto& stem : fetchedStems)
        {
            stems.add(stem);
        }
        
        DBG("SessionDetailView: Fetched " + String(stems.size()) + " stems");
        refreshStemList();
    }
    else
    {
        DBG("SessionDetailView: Failed to fetch stems - " + api->getLastError());
    }
}

void SessionDetailView::refreshStemList()
{
    clearStemRows();
    
    if (stems.isEmpty())
    {
        noStemsLabel->setVisible(true);
    }
    else
    {
        noStemsLabel->setVisible(false);
        
        for (int i = 0; i < stems.size(); ++i)
        {
            createStemRow(stems[i], i);
        }
    }
    
    resized();
}

void SessionDetailView::createStemRow(const SoundFlipAPI::Stem& stem, int index)
{
    auto row = new StemRow();
    
    // Use correct field names from SoundFlipAPI::Stem
    String infoText = stem.filename;
    infoText += "\n@" + stem.uploadedByUsername;
    if (stem.durationSeconds > 0)
    {
        infoText += " - " + formatDuration(stem.durationSeconds);
    }
    
    row->infoLabel = std::make_unique<Label>("info" + String(index), infoText);
    row->infoLabel->setFont(Font(13.0f));
    row->infoLabel->setColour(Label::textColourId, Colours::white);
    addAndMakeVisible(row->infoLabel.get());
    
    row->downloadButton = std::make_unique<TextButton>("Download");
    row->downloadButton->setColour(TextButton::buttonColourId, Colour(0xff00cec9));
    row->downloadButton->addListener(this);
    row->downloadButton->setComponentID("download_" + String(index));
    addAndMakeVisible(row->downloadButton.get());
    
    stemRows.add(row);
}

void SessionDetailView::clearStemRows()
{
    for (auto* row : stemRows)
    {
        if (row->infoLabel)
            removeChildComponent(row->infoLabel.get());
        if (row->downloadButton)
            removeChildComponent(row->downloadButton.get());
    }
    stemRows.clear();
}

String SessionDetailView::formatDuration(int seconds)
{
    int mins = seconds / 60;
    int secs = seconds % 60;
    return String::formatted("%d:%02d", mins, secs);
}

String SessionDetailView::formatDate(const String& isoDate)
{
    if (isoDate.length() >= 10)
    {
        String datePart = isoDate.substring(0, 10);
        StringArray parts = StringArray::fromTokens(datePart, "-", "");
        if (parts.size() == 3)
        {
            static const char* months[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun",
                                           "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
            int month = parts[1].getIntValue();
            if (month >= 1 && month <= 12)
            {
                return String(months[month-1]) + " " + String(parts[2].getIntValue()) + ", " + parts[0];
            }
        }
    }
    return isoDate;
}

void SessionDetailView::paint(Graphics& g)
{
    g.fillAll(Colour(0xff1a1a2e));

    auto bounds = getLocalBounds().reduced(20);

    g.setColour(Colour(0xff2d2d44));
    
    int stemStartY = 195;
    int stemRowHeight = 60;
    int stemSpacing = 5;
    
    for (int i = 0; i < stemRows.size(); ++i)
    {
        int y = stemStartY + i * (stemRowHeight + stemSpacing);
        g.fillRoundedRectangle(20.0f, (float)y, (float)bounds.getWidth(), (float)stemRowHeight, 8.0f);
    }
}

void SessionDetailView::resized()
{
    auto bounds = getLocalBounds().reduced(20);

    backButton->setBounds(20, 20, 80, 30);

    sessionNameLabel->setBounds(20, 60, bounds.getWidth(), 30);
    sessionInfoLabel->setBounds(20, 95, bounds.getWidth(), 20);
    participantsLabel->setBounds(20, 115, bounds.getWidth(), 20);

    stemsLabel->setBounds(20, 160, 100, 25);
    
    int stemStartY = 195;
    int stemRowHeight = 60;
    int stemSpacing = 5;
    int downloadButtonWidth = 80;
    
    if (stems.isEmpty())
    {
        noStemsLabel->setBounds(20, stemStartY, bounds.getWidth(), 40);
    }
    
    for (int i = 0; i < stemRows.size(); ++i)
    {
        int y = stemStartY + i * (stemRowHeight + stemSpacing);
        auto* row = stemRows[i];
        
        row->infoLabel->setBounds(30, y + 5, bounds.getWidth() - downloadButtonWidth - 40, stemRowHeight - 10);
        row->downloadButton->setBounds(bounds.getWidth() - downloadButtonWidth + 10, y + 15, downloadButtonWidth, 30);
    }

    openInSoundFlipButton->setBounds(20, getHeight() - 65, bounds.getWidth(), 45);
}

void SessionDetailView::buttonClicked(Button* buttonThatWasClicked)
{
    if (buttonThatWasClicked == backButton.get())
    {
        if (onBackClicked)
            onBackClicked();
    }
    else if (buttonThatWasClicked == openInSoundFlipButton.get())
    {
        if (inviteCode.isNotEmpty())
        {
            URL webUrl("https://soundflip.app/session/" + inviteCode);
            webUrl.launchInDefaultBrowser();
        }
        
        if (onOpenInSoundFlipClicked)
            onOpenInSoundFlipClicked();
    }
    else
    {
        String componentId = buttonThatWasClicked->getComponentID();
        if (componentId.startsWith("download_"))
        {
            int index = componentId.substring(9).getIntValue();
            
            if (index >= 0 && index < stems.size())
            {
                const auto& stem = stems[index];
                if (stem.downloadUrl.isNotEmpty())
                {
                    URL downloadUrl(stem.downloadUrl);
                    downloadUrl.launchInDefaultBrowser();
                }
                
                if (onDownloadStemClicked)
                    onDownloadStemClicked(index);
            }
        }
    }
}