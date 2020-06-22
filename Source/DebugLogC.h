/*
 *  DebugLogC.h
 *  ThumbJam
 *
 *  Created by Jesse Chappell on 4/8/10.
 *  Copyright 2010 Sonosaurus. All rights reserved.
 *
 */
#ifndef DEBUGLOGC_H
#define DEBUGLOGC_H


//#include <stdio.h>

#ifdef DEBUG 

#define DebugLogC(args...) { juce::String tempDbgBuf = String::formatted(args); juce::Logger::outputDebugString (tempDbgBuf); }

#else

#define DebugLogC(x...) ;

#endif

#define AlwaysLogC(args...) { juce::String tempDbgBuf = String::formatted(args); juce::Logger::outputDebugString (tempDbgBuf); }


//void _DebugLogC(const char *file, int lineNumber, const char *format,...);


#endif
