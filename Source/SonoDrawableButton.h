// SPDX-License-Identifier: GPLv3-or-later WITH Appstore-exception
// Copyright (C) 2020 Jesse Chappell


#pragma once

#include "JuceHeader.h"

class SonoDrawableButton : public DrawableButton
{
public:
    SonoDrawableButton (const String& buttonName,
                    ButtonStyle buttonStyle);
    
    
    juce::Rectangle<float> getImageBounds() const override;

    void paint (Graphics& g) override;    
    void resized() override;

    void setForegroundImageRatio(float ratio) { fgImageRatio = ratio; }
    float getForegroundImageRatio() const { return fgImageRatio; }
    
    void setBackgroundImagePlacement(RectanglePlacement plac) { rectPlacement = plac; }
    RectanglePlacement getBackgroundImagePlacement() const { return rectPlacement; }
    
    enum SonoColourIds
    {
        overOverlayColourId     = 0x1004015,
        downOverlayColourId     = 0x1004016,
    };

    void setBackgroundImage(const Drawable * img);
    
protected:
    
    std::unique_ptr<Drawable> bgImage;
    float fgImageRatio = 0.75f;
    RectanglePlacement rectPlacement = RectanglePlacement::stretchToFit;
};
