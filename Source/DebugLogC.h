// SPDX-License-Identifier: GPLv3-or-later
// Copyright (C) 2020 Jesse Chappell

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

#define DebugLogC(...) { juce::String tempDbgBuf = String::formatted(__VA_ARGS__); juce::Logger::outputDebugString (tempDbgBuf); }


#else

#define DebugLogC(...) ;

#endif

#define AlwaysLogC(...) { juce::String tempDbgBuf = String::formatted(__VA_ARGS__); juce::Logger::outputDebugString (tempDbgBuf); }


//void _DebugLogC(const char *file, int lineNumber, const char *format,...);


#endif
