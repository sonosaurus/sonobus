// SPDX-License-Identifier: GPLv3-or-later WITH Appstore-exception
// Copyright (C) 2024 SoundFlip

#if __APPLE__

#include <CoreFoundation/CoreFoundation.h>
#include <Security/Security.h>
#include <string>

// C-linkage functions to avoid namespace issues
extern "C" {

void soundflip_saveToKeychain(const char* key, const char* value)
{
    const char* serviceName = "SoundFlipConnect";
    
    // Delete existing item first
    SecKeychainItemRef itemRef = nullptr;
    OSStatus status = SecKeychainFindGenericPassword(
        nullptr,
        (UInt32)strlen(serviceName), serviceName,
        (UInt32)strlen(key), key,
        nullptr, nullptr,
        &itemRef
    );
    
    if (status == errSecSuccess && itemRef != nullptr)
    {
        SecKeychainItemDelete(itemRef);
        CFRelease(itemRef);
    }
    
    // Add new item
    SecKeychainAddGenericPassword(
        nullptr,
        (UInt32)strlen(serviceName), serviceName,
        (UInt32)strlen(key), key,
        (UInt32)strlen(value), value,
        nullptr
    );
}

char* soundflip_loadFromKeychain(const char* key)
{
    const char* serviceName = "SoundFlipConnect";
    
    void* passwordData = nullptr;
    UInt32 passwordLength = 0;
    
    OSStatus status = SecKeychainFindGenericPassword(
        nullptr,
        (UInt32)strlen(serviceName), serviceName,
        (UInt32)strlen(key), key,
        &passwordLength, &passwordData,
        nullptr
    );
    
    if (status == errSecSuccess && passwordData != nullptr)
    {
        char* result = (char*)malloc(passwordLength + 1);
        memcpy(result, passwordData, passwordLength);
        result[passwordLength] = '\0';
        SecKeychainItemFreeContent(nullptr, passwordData);
        return result;
    }
    
    return nullptr;
}

void soundflip_freeKeychainResult(char* ptr)
{
    if (ptr) free(ptr);
}

void soundflip_deleteFromKeychain(const char* key)
{
    const char* serviceName = "SoundFlipConnect";
    
    SecKeychainItemRef itemRef = nullptr;
    OSStatus status = SecKeychainFindGenericPassword(
        nullptr,
        (UInt32)strlen(serviceName), serviceName,
        (UInt32)strlen(key), key,
        nullptr, nullptr,
        &itemRef
    );
    
    if (status == errSecSuccess && itemRef != nullptr)
    {
        SecKeychainItemDelete(itemRef);
        CFRelease(itemRef);
    }
}

} // extern "C"

#endif // __APPLE__