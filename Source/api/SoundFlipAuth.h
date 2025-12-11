// SPDX-License-Identifier: GPLv3-or-later WITH Appstore-exception
// Copyright (C) 2024 SoundFlip

#pragma once

#include "JuceHeader.h"

class SoundFlipAuth : public Timer
{
public:
    SoundFlipAuth();
    ~SoundFlipAuth() override;

    //==============================================================================
    // Authentication Flow
    
    /** Starts the OAuth flow by opening the browser */
    void startOAuthFlow();
    
    /** Handle the deep link callback from browser */
    void handleDeepLink(const String& url);
    
    /** Exchange auth code for tokens */
    void exchangeCodeForTokens(const String& code);
    
    /** Refresh the access token using refresh token */
    bool refreshAccessToken();
    
    /** Try to restore session from stored tokens */
    bool tryRestoreSession();
    
    /** Logout and clear all tokens */
    void logout();

    //==============================================================================
    // Token Access
    
    String getAccessToken() const;
    String getRefreshToken() const;
    bool isAuthenticated() const;
    bool isTokenExpired() const;
    
    //==============================================================================
    // User Info
    
    String getUserId() const { return userId; }
    String getUserEmail() const { return userEmail; }
    String getDisplayName() const { return displayName; }
    String getTeamId() const { return teamId; }
    String getWorkspaceId() const { return workspaceId; }
    String getDefaultProjectId() const { return defaultProjectId; }

    //==============================================================================
    // Listener
    
    class Listener
    {
    public:
        virtual ~Listener() = default;
        virtual void authenticationSucceeded() {}
        virtual void authenticationFailed(const String& error) {}
        virtual void authenticationLoggedOut() {}
    };
    
    void addListener(Listener* listener);
    void removeListener(Listener* listener);

private:
    //==============================================================================
    void timerCallback() override;
    
    // Token storage (platform-specific)
    void saveTokensToSecureStorage();
    bool loadTokensFromSecureStorage();
    void clearSecureStorage();
    
    // Platform-specific secure storage
#if JUCE_MAC
    void saveToKeychain(const String& key, const String& value);
    String loadFromKeychain(const String& key);
    void deleteFromKeychain(const String& key);
#elif JUCE_WINDOWS
    void saveToCredentialManager(const String& key, const String& value);
    String loadFromCredentialManager(const String& key);
    void deleteFromCredentialManager(const String& key);
#else
    // Linux fallback - file-based (less secure)
    void saveToFile(const String& key, const String& value);
    String loadFromFile(const String& key);
    void deleteFile(const String& key);
    File getSecureStorageFile();
#endif

    //==============================================================================
    String accessToken;
    String refreshToken;
    int64 expiresAt = 0;
    
    String userId;
    String userEmail;
    String displayName;
    String teamId;
    String workspaceId;
    String defaultProjectId;
    
    ListenerList<Listener> listeners;
    mutable CriticalSection tokenLock;
    
    //==============================================================================
    // URLs - Development vs Production
    
    // Development URLs (local testing)
    const String webAuthUrl = "http://localhost:3000/auth/desktop";  // Next.js web app (browser opens this)
    const String apiBaseUrl = "http://localhost:4400/api";           // NestJS API (desktop app calls this)
    
    // Production URLs (uncomment these and comment above for production)
    // const String webAuthUrl = "https://soundflip.xyz/auth/desktop";  // Next.js web app
    // const String apiBaseUrl = "https://api.soundflip.xyz";           // NestJS API
    
    static constexpr int tokenRefreshMarginSeconds = 300; // Refresh 5 min before expiry

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SoundFlipAuth)
};