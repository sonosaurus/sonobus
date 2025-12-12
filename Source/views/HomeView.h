// SPDX-License-Identifier: GPLv3-or-later WITH Appstore-exception
// Copyright (C) 2024 SoundFlip

#pragma once

#include <JuceHeader.h>

class HomeView : public Component,
                 public Button::Listener
{
public:
    HomeView();
    ~HomeView() override;

    void paint(Graphics&) override;
    void resized() override;
    void buttonClicked(Button* buttonThatWasClicked) override;

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
    // Callbacks
    
    std::function<void()> onStartSessionClicked;
    std::function<void()> onJoinSessionClicked;
    std::function<void()> onSettingsClicked;
    std::function<void(int)> onRecentSessionClicked;

private:
    //==============================================================================
    // UI Components
    
    std::unique_ptr<Label> usernameLabel;
    std::unique_ptr<Label> emailLabel;
    std::unique_ptr<TextButton> settingsButton;
    std::unique_ptr<TextButton> startSessionButton;
    std::unique_ptr<TextButton> joinSessionButton;
    std::unique_ptr<Label> recentSessionsLabel;
    std::unique_ptr<TextButton> recentSession1Button;
    std::unique_ptr<TextButton> recentSession2Button;

    //==============================================================================
    // User Data
    
    String currentUsername;
    String currentEmail;
    
    //==============================================================================
    // Helper Methods
    
    /** Draw the user avatar circle with initials */
    void drawAvatar(Graphics& g, int x, int y, int size);
    
    /** Extract initials from display name (e.g., "John Doe" -> "JD") */
    String getInitials() const;
    
    /** Generate a consistent color based on the username */
    Colour getAvatarColour() const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(HomeView)
};