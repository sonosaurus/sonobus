// SPDX-License-Identifier: GPLv3-or-later
// Copyright (C) 2020 Jesse Chappell



#if JUCE_ANDROID

#include "CrossPlatformUtils.h"

//#include "../JuceLibraryCode/JuceHeader.h"

//#include "DebugLogC.h"

void getSafeAreaInsets(void * component, float & top, float & bottom, float & left, float & right)
{
    top = bottom = left = right = 0;
}

#endif
