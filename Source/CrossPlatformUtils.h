// SPDX-License-Identifier: GPLv3-or-later WITH Appstore-exception
// Copyright (C) 2020 Jesse Chappell


#pragma once

void getSafeAreaInsets(void * component, float & top, float & bottom, float & left, float & right);


#if JUCE_MAC

void disableAppNap();

#endif
