// SPDX-License-Identifier: GPLv3-or-later WITH Appstore-exception
// Copyright (C) 2020 Jesse Chappell


#pragma once

#include "JuceHeader.h"

class SonoPlaybackProgressButton : public TextButton
{
public:
    SonoPlaybackProgressButton();
    explicit SonoPlaybackProgressButton(const String& buttonName);
    SonoPlaybackProgressButton(const String& buttonName, const String& toolTip);

    void setPlaybackPosition(double value);

    void paintButton(Graphics& graphics, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override;

private:
    double playbackPosition = 0.0;
};
