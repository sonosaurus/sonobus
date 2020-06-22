
#include "CrossPlatformUtils.h"

#include "../JuceLibraryCode/JuceHeader.h"

#ifdef JUCE_IOS
#import <UIKit/UIKit.h>

#endif

#include "DebugLogC.h"

void getSafeAreaInsets(void * component, float & top, float & bottom, float & left, float & right)
{
    top = bottom = left = right = 0;

#ifdef JUCE_IOS
    if ([(id)component isKindOfClass:[UIView class]]) {
        UIView * view = (UIView *) component;

        if (@available(iOS 11, *)) {
            UIEdgeInsets insets = view.safeAreaInsets;
            top = insets.top;
            bottom = insets.bottom;
            left = insets.left;
            right = insets.right;
        }

        DebugLogC("Safe area insets of UIView: t: %g  b: %g  l: %g  r:%g", top, bottom, left, right);
    }
    else {
        top = bottom = left = right = 0;
        DebugLogC("NOT A UIVIEW");
    }
#endif
}
