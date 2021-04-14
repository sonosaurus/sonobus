// SPDX-License-Identifier: GPLv3-or-later
// Copyright (C) 2020 Jesse Chappell


#include "CrossPlatformUtils.h"

#include <juce_core/system/juce_TargetPlatform.h>

#if JUCE_MAC


//#import <Foundation/Foundation.h>

#import <Cocoa/Cocoa.h>


void getSafeAreaInsets(void * component, float & top, float & bottom, float & left, float & right)
{
    top = bottom = left = right = 0;
}


void disableAppNap() {
    // Does the App Nap API even exist on this Mac?
    if ([[NSProcessInfo processInfo] respondsToSelector:@selector(beginActivityWithOptions:reason:)]) {
        // If the API exists, then disable App Nap...

        // From NSProcessInfo.h:
        // NSActivityIdleSystemSleepDisabled = (1ULL << 20),
        // NSActivityUserInitiated = (0x00FFFFFFULL | NSActivityIdleSystemSleepDisabled),
        // NSActivityLatencyCritical = 0xFF00000000ULL

        uint64_t options = (0x00FFFFFFULL | (1ULL << 20)) | 0xFF00000000ULL;

        // NSActivityLatencyCritical | NSActivityUserInitiated
        [[NSProcessInfo processInfo] beginActivityWithOptions:options
                                                       reason:@"avoiding audio hiccups and reducing latency"];
    }
}

#endif
