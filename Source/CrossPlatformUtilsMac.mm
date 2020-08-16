//
//  CrossPlatformUtilsMac.m
//  SonoBus
//
//  Created by Jesse Chappell on 8/11/20.
//  Copyright © 2020 Sonosaurus. All rights reserved.
//



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
