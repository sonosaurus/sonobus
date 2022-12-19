//
//  MacSpecific.m
//  AudioPluginHost - App
//
//  Created by Jesse Chappell on 1/6/22.
//  Copyright © 2022 Raw Material Software Limited. All rights reserved.
//

#include <juce_core/system/juce_TargetPlatform.h>

#if JUCE_MAC

#import <Cocoa/Cocoa.h>

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
