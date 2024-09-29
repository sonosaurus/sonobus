// SPDX-License-Identifier: GPLv3-or-later WITH Appstore-exception
// Copyright (C) 2020 Jesse Chappell



#include "CrossPlatformUtils.h"

#include "../JuceLibraryCode/AppConfig.h"

#include <juce_core/system/juce_TargetPlatform.h>

#if JUCE_IOS

#include <juce_opengl/juce_opengl.h>



#import <AVFoundation/AVFoundation.h>

#import <UIKit/UIView.h>

#include "../JuceLibraryCode/JuceHeader.h"




void getSafeAreaInsets(void * component, float & top, float & bottom, float & left, float & right)
{
    top = bottom = left = right = 0;

    if ([(id)component isKindOfClass:[UIView class]]) {
        UIView * view = (UIView *) component;

        if (@available(iOS 11, *)) {
            UIEdgeInsets insets = view.safeAreaInsets;
            top = insets.top;
            bottom = insets.bottom;
            left = insets.left;
            right = insets.right;
        }

        //DebugLogC("Safe area insets of UIView: t: %g  b: %g  l: %g  r:%g", top, bottom, left, right);
    }
    else {
        top = bottom = left = right = 0;
        //DebugLogC("NOT A UIVIEW");
    }
}


bool urlBookmarkToBinaryData(void * bookmark, const void * & retdata, size_t & retsize)
{
    NSData * data = (NSData*) bookmark;
    if (data && [data isKindOfClass:NSData.class]) {
        retdata = [data bytes];
        retsize = [data length];
        return true;
    }
    return false;
}

void * binaryDataToUrlBookmark(const void * data, size_t size)
{
    NSData * nsdata = [[NSData alloc] initWithBytes:data length:size];

    return nsdata;
}

juce::URL generateUpdatedURL (juce::URL& urlToUse)
{
    juce::URL returl = urlToUse;
    
    if (NSData* bookmark = (NSData*) getURLBookmark (urlToUse))
    {
        BOOL isBookmarkStale = false;
        NSError* error = nil;

        auto nsURL = [NSURL URLByResolvingBookmarkData: bookmark
                                               options: 0
                                         relativeToURL: nil
                                   bookmarkDataIsStale: &isBookmarkStale
                                                  error: &error];
        
        if (error == nil)
        {
            DBG("resolved bookmark for: " << urlToUse.toString(true) << "  resolved as: " << [[nsURL absoluteString] UTF8String] );
            // actually change the value of urlToUse to match the url just returned, because it could have moved or the app it came from updated
            returl = juce::URL(juce::String([[nsURL absoluteString] UTF8String]));
            
            //securityAccessSucceeded = [nsURL startAccessingSecurityScopedResource];

            if (isBookmarkStale) {
                NSData* bookmark = [nsURL bookmarkDataWithOptions: NSURLBookmarkCreationSuitableForBookmarkFile
                                   includingResourceValuesForKeys: nil
                                                    relativeToURL: nil
                                                            error: &error];
                DBG("Updated stale bookmark");
                if (error == nil)
                    setURLBookmark (returl, (void*) bookmark);
            }
            else {
                NSData * nbookmark = [[NSData alloc] initWithData:bookmark];
                setURLBookmark(returl, (void*)nbookmark);
            }
            return returl;
        }

    }

    return returl;
}


bool getIsInputGainSettable()
{
    auto* session = [AVAudioSession sharedInstance];
    return [session isInputGainSettable];
}

float getInputGain() {
    auto* session = [AVAudioSession sharedInstance];
    return [session inputGain];
}

bool setInputGain(float gain) {
    auto* session = [AVAudioSession sharedInstance];
    NSError* error;
    return [session setInputGain:gain error:&error];
}

#endif
