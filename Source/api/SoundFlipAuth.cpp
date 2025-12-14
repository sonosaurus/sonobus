// SPDX-License-Identifier: GPLv3-or-later WITH Appstore-exception
// Copyright (C) 2024 SoundFlip

#include "SoundFlipAuth.h"

#if JUCE_MAC
// Declarations for functions in SoundFlipAuth_mac.mm
extern "C" {
    void soundflip_saveToKeychain(const char* key, const char* value);
    char* soundflip_loadFromKeychain(const char* key);
    void soundflip_freeKeychainResult(char* ptr);
    void soundflip_deleteFromKeychain(const char* key);
}
#endif

#if JUCE_WINDOWS
#include <windows.h>
#include <wincred.h>
#pragma comment(lib, "Advapi32.lib")
#endif

//==============================================================================
SoundFlipAuth::SoundFlipAuth()
{
    tryRestoreSession();
}

SoundFlipAuth::~SoundFlipAuth()
{
    stopTimer();
}

//==============================================================================
void SoundFlipAuth::startOAuthFlow()
{
    URL authUrl(webAuthUrl);
    
    // Print URL for manual testing with different browsers
    std::cout << "\n=== OAUTH URL (copy to incognito for different user) ===" << std::endl;
    std::cout << authUrl.toString(true).toStdString() << std::endl;
    std::cout << "=========================================================\n" << std::endl;
    
    authUrl.launchInDefaultBrowser();
}

void SoundFlipAuth::handleDeepLink(const String& url)
{
    if (!url.startsWith("soundflipconnect://callback"))
    {
        listeners.call([](Listener& l) { 
            l.authenticationFailed("Invalid callback URL"); 
        });
        return;
    }
    
    URL parsedUrl(url);
    String code = parsedUrl.getParameterValues()[parsedUrl.getParameterNames().indexOf("code")];
    
    if (code.isEmpty())
    {
        String error = parsedUrl.getParameterValues()[parsedUrl.getParameterNames().indexOf("error")];
        listeners.call([error](Listener& l) { 
            l.authenticationFailed(error.isEmpty() ? "No auth code received" : error); 
        });
        return;
    }
    
    exchangeCodeForTokens(code);
}

void SoundFlipAuth::exchangeCodeForTokens(const String& code)
{
    DynamicObject::Ptr bodyObj = new DynamicObject();
    bodyObj->setProperty("code", code);
    bodyObj->setProperty("clientType", "desktop");
    
    String jsonString = JSON::toString(var(bodyObj.get()));
    URL url = URL(apiBaseUrl + "/auth/desktop/token").withPOSTData(jsonString);
    
    int statusCode = 0;
    auto stream = url.createInputStream(
        URL::InputStreamOptions(URL::ParameterHandling::inPostData)
            .withExtraHeaders("Content-Type: application/json")
            .withConnectionTimeoutMs(30000)
            .withStatusCode(&statusCode)
    );
    
    if (stream == nullptr)
    {
        listeners.call([](Listener& l) { 
            l.authenticationFailed("Network error - could not connect"); 
        });
        return;
    }
    
    String response = stream->readEntireStreamAsString();
    var jsonResponse = JSON::parse(response);
    
    if (jsonResponse.isVoid())
    {
        listeners.call([](Listener& l) { 
            l.authenticationFailed("Invalid response from server"); 
        });
        return;
    }
    
    if (jsonResponse.hasProperty("error"))
    {
        String error = jsonResponse["error"].toString();
        listeners.call([error](Listener& l) { 
            l.authenticationFailed(error); 
        });
        return;
    }
    
    {
        const ScopedLock sl(tokenLock);
        
        accessToken = jsonResponse["accessToken"].toString();
        refreshToken = jsonResponse["refreshToken"].toString();
        
        int expiresIn = jsonResponse["expiresIn"];
        expiresAt = Time::currentTimeMillis() + (expiresIn * 1000);
        
        var user = jsonResponse["user"];
        if (user.isObject())
        {
            std::cout << "=== USER JSON ===" << std::endl << std::flush;
            std::cout << JSON::toString(user).toStdString() << std::endl << std::flush;
            userId = user["id"].toString();
            userEmail = user["email"].toString();
            displayName = user["displayName"].toString();
            teamId = user["teamId"].toString();
            workspaceId = user["workspaceId"].toString();
            defaultProjectId = user["defaultProjectId"].toString();
            std::cout << "=== PARSED displayName: " << displayName.toStdString() << std::endl << std::flush;
        }
    }
    
    saveTokensToSecureStorage();
    startTimer(60000);

    std::cout << "=== SoundFlipAuth: About to notify " << listeners.size() << " listeners ===" << std::endl << std::flush;
    
    listeners.call([](Listener& l) { 
        std::cout << "=== Calling listener ===" << std::endl << std::flush;
        l.authenticationSucceeded(); 
    });
    
    std::cout << "=== SoundFlipAuth: Done notifying listeners ===" << std::endl << std::flush;
}

bool SoundFlipAuth::refreshAccessToken()
{
    String currentRefreshToken;
    {
        const ScopedLock sl(tokenLock);
        currentRefreshToken = refreshToken;
    }
    
    if (currentRefreshToken.isEmpty())
        return false;
    
    DynamicObject::Ptr bodyObj = new DynamicObject();
    bodyObj->setProperty("refreshToken", currentRefreshToken);
    bodyObj->setProperty("clientType", "desktop");
    
    String jsonString = JSON::toString(var(bodyObj.get()));
    URL url = URL(apiBaseUrl + "/auth/desktop/refresh").withPOSTData(jsonString);
    
    int statusCode = 0;
    auto stream = url.createInputStream(
        URL::InputStreamOptions(URL::ParameterHandling::inPostData)
            .withExtraHeaders("Content-Type: application/json")
            .withConnectionTimeoutMs(30000)
            .withStatusCode(&statusCode)
    );
    
    if (stream == nullptr)
        return false;
    
    String response = stream->readEntireStreamAsString();
    var jsonResponse = JSON::parse(response);
    
    if (jsonResponse.isVoid() || jsonResponse.hasProperty("error"))
        return false;
    
    {
        const ScopedLock sl(tokenLock);
        
        accessToken = jsonResponse["accessToken"].toString();
        
        if (jsonResponse.hasProperty("refreshToken"))
            refreshToken = jsonResponse["refreshToken"].toString();
        
        int expiresIn = jsonResponse["expiresIn"];
        expiresAt = Time::currentTimeMillis() + (expiresIn * 1000);
    }
    
    saveTokensToSecureStorage();
    return true;
}

bool SoundFlipAuth::tryRestoreSession()
{
    if (!loadTokensFromSecureStorage())
        return false;
    
    {
        const ScopedLock sl(tokenLock);
        if (accessToken.isEmpty() || refreshToken.isEmpty())
            return false;
    }
    
    if (isTokenExpired())
    {
        if (!refreshAccessToken())
        {
            logout();
            return false;
        }
    }
    
    startTimer(60000);
    return true;
}

void SoundFlipAuth::logout()
{
    {
        const ScopedLock sl(tokenLock);
        accessToken.clear();
        refreshToken.clear();
        expiresAt = 0;
        userId.clear();
        userEmail.clear();
        displayName.clear();
        teamId.clear();
        workspaceId.clear();
        defaultProjectId.clear();
    }
    
    clearSecureStorage();
    stopTimer();
    
    listeners.call([](Listener& l) { 
        l.authenticationLoggedOut(); 
    });
}

//==============================================================================
String SoundFlipAuth::getAccessToken() const
{
    const ScopedLock sl(tokenLock);
    return accessToken;
}

String SoundFlipAuth::getRefreshToken() const
{
    const ScopedLock sl(tokenLock);
    return refreshToken;
}

bool SoundFlipAuth::isAuthenticated() const
{
    const ScopedLock sl(tokenLock);
    return accessToken.isNotEmpty() && !isTokenExpired();
}

bool SoundFlipAuth::isTokenExpired() const
{
    const ScopedLock sl(tokenLock);
    return Time::currentTimeMillis() >= (expiresAt - tokenRefreshMarginSeconds * 1000);
}

//==============================================================================
void SoundFlipAuth::addListener(Listener* listener)
{
    listeners.add(listener);
}

void SoundFlipAuth::removeListener(Listener* listener)
{
    listeners.remove(listener);
}

//==============================================================================
void SoundFlipAuth::timerCallback()
{
    if (isTokenExpired() && refreshToken.isNotEmpty())
        refreshAccessToken();
}

//==============================================================================
void SoundFlipAuth::saveTokensToSecureStorage()
{
    DynamicObject::Ptr obj = new DynamicObject();
    
    {
        const ScopedLock sl(tokenLock);
        obj->setProperty("accessToken", accessToken);
        obj->setProperty("refreshToken", refreshToken);
        obj->setProperty("expiresAt", expiresAt);
        obj->setProperty("userId", userId);
        obj->setProperty("userEmail", userEmail);
        obj->setProperty("displayName", displayName);
        obj->setProperty("teamId", teamId);
        obj->setProperty("workspaceId", workspaceId);
        obj->setProperty("defaultProjectId", defaultProjectId);
    }
    
    String jsonString = JSON::toString(var(obj.get()));
    
#if JUCE_MAC
    soundflip_saveToKeychain("tokens", jsonString.toRawUTF8());
#elif JUCE_WINDOWS
    saveToCredentialManager("tokens", jsonString);
#else
    saveToFile("tokens", jsonString);
#endif
}

bool SoundFlipAuth::loadTokensFromSecureStorage()
{
    String jsonString;
    
#if JUCE_MAC
    char* result = soundflip_loadFromKeychain("tokens");
    if (result != nullptr)
    {
        jsonString = String::fromUTF8(result);
        soundflip_freeKeychainResult(result);
    }
#elif JUCE_WINDOWS
    jsonString = loadFromCredentialManager("tokens");
#else
    jsonString = loadFromFile("tokens");
#endif
    
    if (jsonString.isEmpty())
        return false;
    
    var jsonData = JSON::parse(jsonString);
    if (jsonData.isVoid())
        return false;
    
    {
        const ScopedLock sl(tokenLock);
        accessToken = jsonData["accessToken"].toString();
        refreshToken = jsonData["refreshToken"].toString();
        expiresAt = (int64)jsonData["expiresAt"];
        userId = jsonData["userId"].toString();
        userEmail = jsonData["userEmail"].toString();
        displayName = jsonData["displayName"].toString();
        teamId = jsonData["teamId"].toString();
        workspaceId = jsonData["workspaceId"].toString();
        defaultProjectId = jsonData["defaultProjectId"].toString();
    }
    
    return true;
}

void SoundFlipAuth::clearSecureStorage()
{
#if JUCE_MAC
    soundflip_deleteFromKeychain("tokens");
#elif JUCE_WINDOWS
    deleteFromCredentialManager("tokens");
#else
    deleteFile("tokens");
#endif
}

//==============================================================================
// Windows implementation

#if JUCE_WINDOWS

void SoundFlipAuth::saveToCredentialManager(const String& key, const String& value)
{
    String targetName = "SoundFlipConnect_" + key;
    
    CREDENTIALW cred = { 0 };
    cred.Type = CRED_TYPE_GENERIC;
    cred.TargetName = (LPWSTR)targetName.toWideCharPointer();
    cred.CredentialBlobSize = (DWORD)(value.getNumBytesAsUTF8() + 1);
    cred.CredentialBlob = (LPBYTE)value.toRawUTF8();
    cred.Persist = CRED_PERSIST_LOCAL_MACHINE;
    cred.UserName = (LPWSTR)L"SoundFlipUser";
    
    CredWriteW(&cred, 0);
}

String SoundFlipAuth::loadFromCredentialManager(const String& key)
{
    String targetName = "SoundFlipConnect_" + key;
    
    PCREDENTIALW pcred = nullptr;
    if (CredReadW((LPCWSTR)targetName.toWideCharPointer(), CRED_TYPE_GENERIC, 0, &pcred))
    {
        String result = String::fromUTF8((const char*)pcred->CredentialBlob, 
                                          (int)pcred->CredentialBlobSize);
        CredFree(pcred);
        return result;
    }
    
    return {};
}

void SoundFlipAuth::deleteFromCredentialManager(const String& key)
{
    String targetName = "SoundFlipConnect_" + key;
    CredDeleteW((LPCWSTR)targetName.toWideCharPointer(), CRED_TYPE_GENERIC, 0);
}

#endif

//==============================================================================
// Linux fallback

#if JUCE_LINUX

File SoundFlipAuth::getSecureStorageFile()
{
    File configDir = File::getSpecialLocation(File::userApplicationDataDirectory)
                         .getChildFile("SoundFlipConnect");
    configDir.createDirectory();
    return configDir.getChildFile("auth.json");
}

void SoundFlipAuth::saveToFile(const String& key, const String& value)
{
    ignoreUnused(key);
    File f = getSecureStorageFile();
    f.replaceWithText(value);
    chmod(f.getFullPathName().toRawUTF8(), S_IRUSR | S_IWUSR);
}

String SoundFlipAuth::loadFromFile(const String& key)
{
    ignoreUnused(key);
    File f = getSecureStorageFile();
    if (f.existsAsFile())
        return f.loadFileAsString();
    return {};
}

void SoundFlipAuth::deleteFile(const String& key)
{
    ignoreUnused(key);
    getSecureStorageFile().deleteFile();
}

#endif