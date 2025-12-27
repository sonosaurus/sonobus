// SPDX-License-Identifier: GPLv3-or-later WITH Appstore-exception
// Copyright (C) 2024 SoundFlip

#pragma once

#include <JuceHeader.h>

// Forward declaration
class SessionManager;

class HomeView : public Component,
                 public Button::Listener,
                 public ChangeListener
{
public:
    HomeView(SessionManager* sessionManager = nullptr);
    ~HomeView() override;

    void paint(Graphics&) override;
    void resized() override;
    void buttonClicked(Button* buttonThatWasClicked) override;
    void changeListenerCallback(ChangeBroadcaster* source) override;

    //==============================================================================
    // User Info Setters
    
    /** Set the display name shown in the header */
    void setUsername(const String& name);
    
    /** Set the user's email (shown as subtitle under name) */
    void setUserEmail(const String& email);
    
    /** Set all user info at once */
    void setUserInfo(const String& name, const String& email);
    
    /** Clear user info (for logout) */
    void clearUserInfo();
    
    //==============================================================================
    // Session Management
    
    /** Refresh recent sessions from API */
    void refreshRecentSessions();
    
    /** Set the session manager (can be set after construction) */
    void setSessionManager(SessionManager* sm);

    //==============================================================================
    // Callbacks
    
    std::function<void()> onStartSessionClicked;
    std::function<void()> onJoinSessionClicked;
    std::function<void()> onSettingsClicked;
    std::function<void(int)> onRecentSessionClicked;  // Legacy: index-based
    std::function<void(const String&)> onRecentSessionClickedById;  // New: ID-based

private:
    //==============================================================================
    // Helper Methods
    
    void setupUI();
    
    /** Draw the user avatar circle with initials */
    void drawAvatar(Graphics& g, int x, int y, int size);
    
    /** Extract initials from display name (e.g., "John Doe" -> "JD") */
    String getInitials() const;
    
    /** Generate a consistent color based on the username */
    Colour getAvatarColour() const;
    
    /** Update UI with sessions from SessionManager */
    void updateRecentSessionsUI();
    
    /** Format timestamp to readable date */
    String formatSessionDate(int64 timestamp) const;

    //==============================================================================
    // Session Manager
    
    SessionManager* sessionManager = nullptr;
    
    //==============================================================================
    // UI Components
    
    std::unique_ptr<Label> usernameLabel;
    std::unique_ptr<Label> emailLabel;
    std::unique_ptr<TextButton> settingsButton;
    std::unique_ptr<TextButton> startSessionButton;
    std::unique_ptr<TextButton> joinSessionButton;
    std::unique_ptr<Label> recentSessionsLabel;
    
    // Static placeholder buttons (used when no SessionManager)
    std::unique_ptr<TextButton> recentSession1Button;
    std::unique_ptr<TextButton> recentSession2Button;
    
    // Dynamic session buttons (used with SessionManager)
    OwnedArray<TextButton> dynamicSessionButtons;
    StringArray recentSessionIds;
    bool usingDynamicSessions = false;

    //==============================================================================
    // User Data
    
    String currentUsername;
    String currentEmail;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(HomeView)
};