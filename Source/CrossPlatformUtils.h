// SPDX-License-Identifier: GPLv3-or-later WITH Appstore-exception
// Copyright (C) 2020 Jesse Chappell


#pragma once

void getSafeAreaInsets(void * component, float & top, float & bottom, float & left, float & right);


#if JUCE_MAC

void disableAppNap();

#endif

#if JUCE_IOS
bool urlBookmarkToBinaryData(void * bookmark, const void * & retdata, size_t & retsize);

void *binaryDataToUrlBookmark(const void * data, size_t size);

juce::URL generateUpdatedURL (juce::URL& urlToUse);

bool getIsInputGainSettable();
float getInputGain();
bool setInputGain(float gain);
#endif
