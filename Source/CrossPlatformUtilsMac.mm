// SPDX-License-Identifier: GPLv3-or-later
// Copyright (C) 2020 Jesse Chappell


#include "CrossPlatformUtils.h"

#include "../JuceLibraryCode/AppConfig.h"

#include <juce_core/system/juce_TargetPlatform.h>

#if JUCE_MAC


//#import <Foundation/Foundation.h>


void getSafeAreaInsets(void * component, float & top, float & bottom, float & left, float & right)
{
    top = bottom = left = right = 0;
}


#endif
