
#include "CrossPlatformUtils.h"

#include "../JuceLibraryCode/JuceHeader.h"

#if JUCE_LINUX


void getSafeAreaInsets(void * component, float & top, float & bottom, float & left, float & right)
{
    top = bottom = left = right = 0;
}

#endif
