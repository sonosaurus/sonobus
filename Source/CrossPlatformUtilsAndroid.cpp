
#include "CrossPlatformUtils.h"

#include "../JuceLibraryCode/JuceHeader.h"

#ifdef JUCE_ANDROID


#include "DebugLogC.h"

void getSafeAreaInsets(void * component, float & top, float & bottom, float & left, float & right)
{
    top = bottom = left = right = 0;
}

#endif
