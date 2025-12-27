// SPDX-License-Identifier: GPLv3-or-later WITH Appstore-exception
// Copyright (C) 2024 SoundFlip

#include "HomeView.h"
#include "../managers/SessionManager.h"

HomeView::HomeView(SessionManager* sm)
    : sessionManager(sm)
{
    if (sessionManager)
        sessionManager->addChangeListener(this);
    
    setupUI();
    
    // If we have a session manager, fetch real sessions
    if (sessionManager)
    {
        // Hide placeholder buttons - we'll use dynamic ones
        recentSession1Button->setVisible(false);
        recentSession2Button->setVisible(false);
        
        // Fetch sessions async to not block UI
        MessageManager::callAsync([this]() {
            refreshRecentSessions();
        });
    }
}

HomeView::~HomeView()
{
    if (sessionManager)
        sessionManager->removeChangeListener(this);
}

void HomeView::setupUI()
{
    // Username label - will be updated with real name after login
    usernameLabel = std::make_unique<Label>("username", "Welcome");
    usernameLabel->setFont(Font(18.0f, Font::bold));
    usernameLabel->setColour(Label::textColourId, Colours::white);
    addAndMakeVisible(usernameLabel.get());

    // Email label - subtle subtitle under username
    emailLabel = std::make_unique<Label>("email", "");
    emailLabel->setFont(Font(12.0f));
    emailLabel->setColour(Label::textColourId, Colour(0xffaaaaaa));
    addAndMakeVisible(emailLabel.get());

    // Settings button (gear icon would be nice, but text works for now)
    settingsButton = std::make_unique<TextButton>("Settings");
    settingsButton->addListener(this);
    addAndMakeVisible(settingsButton.get());

    // Main action buttons
    startSessionButton = std::make_unique<TextButton>("Start Session");
    startSessionButton->setColour(TextButton::buttonColourId, Colour(0xff6c5ce7));
    startSessionButton->setColour(TextButton::textColourOnId, Colours::white);
    startSessionButton->addListener(this);
    addAndMakeVisible(startSessionButton.get());

    joinSessionButton = std::make_unique<TextButton>("Join Session");
    joinSessionButton->setColour(TextButton::buttonColourId, Colour(0xff00cec9));
    joinSessionButton->setColour(TextButton::textColourOnId, Colours::white);
    joinSessionButton->addListener(this);
    addAndMakeVisible(joinSessionButton.get());

    // Recent sessions section
    recentSessionsLabel = std::make_unique<Label>("recent", "Recent Sessions");
    recentSessionsLabel->setFont(Font(16.0f, Font::bold));
    recentSessionsLabel->setColour(Label::textColourId, Colours::white);
    addAndMakeVisible(recentSessionsLabel.get());

    // Static placeholder buttons (shown when no SessionManager or no sessions)
    recentSession1Button = std::make_unique<TextButton>("Friday cookup with @mike\nDec 8 - 3 stems");
    recentSession1Button->addListener(this);
    addAndMakeVisible(recentSession1Button.get());

    recentSession2Button = std::make_unique<TextButton>("Beat review with @sarah\nDec 5 - 2 stems");
    recentSession2Button->addListener(this);
    addAndMakeVisible(recentSession2Button.get());
}

//==============================================================================
// Session Manager

void HomeView::setSessionManager(SessionManager* sm)
{
    if (sessionManager)
        sessionManager->removeChangeListener(this);
    
    sessionManager = sm;
    
    if (sessionManager)
    {
        sessionManager->addChangeListener(this);
        
        // Hide placeholders, use dynamic buttons
        recentSession1Button->setVisible(false);
        recentSession2Button->setVisible(false);
        
        refreshRecentSessions();
    }
    else
    {
        // Show placeholders again
        recentSession1Button->setVisible(true);
        recentSession2Button->setVisible(true);
        
        // Clear dynamic buttons
        dynamicSessionButtons.clear();
        recentSessionIds.clear();
        usingDynamicSessions = false;
    }
    
    resized();
}

void HomeView::refreshRecentSessions()
{
    if (sessionManager)
    {
        sessionManager->fetchRecentSessions(5);
        // updateRecentSessionsUI() will be called via changeListenerCallback
    }
}

void HomeView::changeListenerCallback(ChangeBroadcaster* source)
{
    if (source == sessionManager)
    {
        updateRecentSessionsUI();
    }
}

void HomeView::updateRecentSessionsUI()
{
    if (!sessionManager)
        return;
    
    const auto& sessions = sessionManager->getRecentSessions();
    
    // Clear existing dynamic buttons
    dynamicSessionButtons.clear();
    recentSessionIds.clear();
    
    if (sessions.isEmpty())
    {
        // Show placeholder buttons if no real sessions
        recentSession1Button->setVisible(true);
        recentSession2Button->setVisible(true);
        usingDynamicSessions = false;
    }
    else
    {
        // Hide placeholders
        recentSession1Button->setVisible(false);
        recentSession2Button->setVisible(false);
        usingDynamicSessions = true;
        
        // Create buttons for each session
        for (int i = 0; i < jmin(5, sessions.size()); ++i)
        {
            const auto& session = sessions[i];
            
            // Format button text
            String buttonText = session.name.isNotEmpty() ? session.name : "Untitled Session";
            buttonText += "\n" + formatSessionDate(session.createdAt);
            
            if (session.stemCount > 0)
                buttonText += " - " + String(session.stemCount) + " stem" + (session.stemCount > 1 ? "s" : "");
            
            // Add participants info (excluding current user)
            if (session.participants.size() > 1)
            {
                buttonText += " with ";
                int othersCount = 0;
                for (const auto& p : session.participants)
                {
                    // Skip if this is likely the current user (we'd need user ID to be sure)
                    if (othersCount < 2)
                    {
                        if (othersCount > 0) buttonText += ", ";
                        buttonText += "@" + p.username;
                        othersCount++;
                    }
                }
                if (session.participants.size() > 3)
                    buttonText += " +" + String(session.participants.size() - 3) + " more";
            }
            
            auto* button = new TextButton(buttonText);
            button->addListener(this);
            addAndMakeVisible(button);
            dynamicSessionButtons.add(button);
            recentSessionIds.add(session.id);
        }
    }
    
    resized();
    repaint();
}

String HomeView::formatSessionDate(int64 timestamp) const
{
    if (timestamp == 0)
        return "";
    
    Time t(timestamp);
    Time now = Time::getCurrentTime();
    
    RelativeTime diff = now - t;
    
    if (diff.inDays() < 1)
        return "Today";
    else if (diff.inDays() < 2)
        return "Yesterday";
    else if (diff.inDays() < 7)
        return String((int)diff.inDays()) + " days ago";
    else
        return t.formatted("%b %d");
}

//==============================================================================
// User Info Methods

void HomeView::setUsername(const String& name)
{
    currentUsername = name.trim();
    
    if (currentUsername.isEmpty())
    {
        usernameLabel->setText("Welcome", dontSendNotification);
    }
    else
    {
        usernameLabel->setText(currentUsername, dontSendNotification);
    }
    
    // Trigger repaint to update avatar with new initials/color
    repaint();
}

void HomeView::setUserEmail(const String& email)
{
    currentEmail = email.trim();
    emailLabel->setText(currentEmail, dontSendNotification);
}

void HomeView::setUserInfo(const String& name, const String& email)
{
    setUsername(name);
    setUserEmail(email);
}

void HomeView::clearUserInfo()
{
    currentUsername.clear();
    currentEmail.clear();
    usernameLabel->setText("Welcome", dontSendNotification);
    emailLabel->setText("", dontSendNotification);
    repaint();
}

//==============================================================================
// Drawing

void HomeView::paint(Graphics& g)
{
    // Dark background
    g.fillAll(Colour(0xff1a1a2e));
    
    // Draw avatar
    drawAvatar(g, 20, 15, 40);
}

void HomeView::drawAvatar(Graphics& g, int x, int y, int size)
{
    // Draw colored circle - color is based on username for consistency
    g.setColour(getAvatarColour());
    g.fillEllipse((float)x, (float)y, (float)size, (float)size);
    
    // Draw initials
    g.setColour(Colours::white);
    g.setFont(Font((float)size * 0.4f, Font::bold));
    g.drawText(getInitials(), x, y, size, size, Justification::centred);
}

String HomeView::getInitials() const
{
    if (currentUsername.isEmpty())
        return "?";
    
    StringArray words = StringArray::fromTokens(currentUsername, " ", "");
    
    if (words.isEmpty())
        return "?";
    
    String initials;
    
    // First word's first letter
    if (words[0].isNotEmpty())
        initials += words[0].substring(0, 1).toUpperCase();
    
    // Last word's first letter (if different from first)
    if (words.size() > 1 && words[words.size() - 1].isNotEmpty())
        initials += words[words.size() - 1].substring(0, 1).toUpperCase();
    
    return initials.isEmpty() ? "?" : initials;
}

Colour HomeView::getAvatarColour() const
{
    if (currentUsername.isEmpty())
        return Colour(0xff6c5ce7); // Default purple
    
    // Generate a consistent color based on username hash
    // This ensures the same user always gets the same avatar color
    uint32 hash = (uint32)currentUsername.toLowerCase().hashCode();
    
    // Array of nice avatar colors (similar to Google/Slack style)
    const Colour avatarColours[] = {
        Colour(0xff6c5ce7), // Purple
        Colour(0xff00cec9), // Teal
        Colour(0xfffd79a8), // Pink
        Colour(0xfff39c12), // Orange
        Colour(0xff27ae60), // Green
        Colour(0xff3498db), // Blue
        Colour(0xffe74c3c), // Red
        Colour(0xff9b59b6), // Violet
        Colour(0xff1abc9c), // Turquoise
        Colour(0xffe67e22), // Dark Orange
    };
    
    const int numColours = sizeof(avatarColours) / sizeof(avatarColours[0]);
    return avatarColours[hash % numColours];
}

//==============================================================================
// Layout

void HomeView::resized()
{
    auto bounds = getLocalBounds().reduced(20);

    // Header section - avatar is drawn in paint(), labels positioned here
    usernameLabel->setBounds(70, 15, 200, 24);
    emailLabel->setBounds(70, 37, 200, 18);
    settingsButton->setBounds(bounds.getWidth() - 60, 20, 80, 30);

    // Main action buttons - side by side
    int buttonY = 80;
    int buttonHeight = 60;
    int buttonSpacing = 20;
    int buttonWidth = (bounds.getWidth() - buttonSpacing) / 2;
    
    startSessionButton->setBounds(20, buttonY, buttonWidth, buttonHeight);
    joinSessionButton->setBounds(20 + buttonWidth + buttonSpacing, buttonY, buttonWidth, buttonHeight);

    // Recent sessions section
    int recentY = buttonY + buttonHeight + 30;
    recentSessionsLabel->setBounds(20, recentY, 200, 25);
    
    int sessionButtonHeight = 50;
    int sessionSpacing = 10;
    
    if (usingDynamicSessions && !dynamicSessionButtons.isEmpty())
    {
        // Position dynamic session buttons
        int yPos = recentY + 35;
        for (auto* button : dynamicSessionButtons)
        {
            button->setBounds(20, yPos, bounds.getWidth(), sessionButtonHeight);
            yPos += sessionButtonHeight + sessionSpacing;
        }
    }
    else
    {
        // Position static placeholder buttons
        recentSession1Button->setBounds(20, recentY + 35, bounds.getWidth(), sessionButtonHeight);
        recentSession2Button->setBounds(20, recentY + 35 + sessionButtonHeight + sessionSpacing, 
                                        bounds.getWidth(), sessionButtonHeight);
    }
}

//==============================================================================
// Button Handling

void HomeView::buttonClicked(Button* buttonThatWasClicked)
{
    if (buttonThatWasClicked == startSessionButton.get())
    {
        if (onStartSessionClicked)
            onStartSessionClicked();
    }
    else if (buttonThatWasClicked == joinSessionButton.get())
    {
        if (onJoinSessionClicked)
            onJoinSessionClicked();
    }
    else if (buttonThatWasClicked == settingsButton.get())
    {
        if (onSettingsClicked)
            onSettingsClicked();
    }
    else if (buttonThatWasClicked == recentSession1Button.get())
    {
        if (onRecentSessionClicked)
            onRecentSessionClicked(0);
    }
    else if (buttonThatWasClicked == recentSession2Button.get())
    {
        if (onRecentSessionClicked)
            onRecentSessionClicked(1);
    }
    else
    {
        // Check if it's a dynamic session button
        for (int i = 0; i < dynamicSessionButtons.size(); ++i)
        {
            if (buttonThatWasClicked == dynamicSessionButtons[i])
            {
                // Call new ID-based callback if available
                if (onRecentSessionClickedById && i < recentSessionIds.size())
                {
                    onRecentSessionClickedById(recentSessionIds[i]);
                }
                // Fall back to index-based callback for compatibility
                else if (onRecentSessionClicked)
                {
                    onRecentSessionClicked(i);
                }
                break;
            }
        }
    }
}