// SPDX-License-Identifier: GPLv3-or-later WITH Appstore-exception
// Copyright (C) 2020 Jesse Chappell


#pragma once

#include "JuceHeader.h"

enum SonoButtonStyle {
    SonoButtonStyleNormal=0,
    SonoButtonStyleUpperLeftCorner,
    SonoButtonStyleUpperRightCorner,
    SonoButtonStyleLowerLeftCorner,
    SonoButtonStyleLowerRightCorner,
    SonoButtonStyleUpperLeftCornerRound,
    SonoButtonStyleUpperRightCornerRound,
    SonoButtonStyleLowerLeftCornerRound,
    SonoButtonStyleLowerRightCornerRound,
    SonoButtonStyleLeft,
    SonoButtonStyleRight,
    SonoButtonStyleTop,
    SonoButtonStyleBottom
};



class SonoTextButton : public TextButton
{
public:
    SonoTextButton(const String & name="");
    virtual ~SonoTextButton();
    
    enum SonoColourIds
    {
        outlineColourId     = 0x1008015,
    };

    
    void paintButton (Graphics&, bool isMouseOverButton, bool isButtonDown) override;
    void resized() override;
    
    void setCornerRadius(float rad) { cornerRadius = rad; }
    float getCornerRadius() const { return cornerRadius; }
    
    void setButtonStyle(SonoButtonStyle style);
    SonoButtonStyle getButtonStyle() const { return _buttonStyle; }
    
    void setCircleRadius(float rad) { circleRadius = rad; }
    float getCircleRadius() const { return circleRadius; }
    
    float getTextHeightRatio() const { return textHeightRatio; }
    void setTextHeightRatio(float ratio) { textHeightRatio = ratio; }

    void setTextJustification(Justification just) { textJustification = just; }
    Justification getTextJustification() const { return textJustification; }
    

protected:
    
    bool hitTest (int x, int y) override;

    void setupPath();
    
    void drawButtonBackground(Graphics& g, bool isMouseOverButton, bool isButtonDown);
    
    SonoButtonStyle _buttonStyle;
    float cornerRadius = 6.0f;
    Path borderPath;
    Path fillPath;
    
    float circleRadius = 0.0f;
    float textHeightRatio = 0.8f;
    Justification textJustification = Justification::centred;
};
