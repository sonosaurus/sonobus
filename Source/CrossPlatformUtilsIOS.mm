// SPDX-License-Identifier: GPLv3-or-later WITH Appstore-exception
// Copyright (C) 2020 Jesse Chappell



#include "CrossPlatformUtils.h"

#include "../JuceLibraryCode/AppConfig.h"

#include <juce_core/system/juce_TargetPlatform.h>

#if JUCE_IOS





#import <UIKit/UIView.h>



//#include "../JuceLibraryCode/JuceHeader.h"



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

#endif
