// SPDX-License-Identifier: GPLv3-or-later
// Copyright (C) 2020 Jesse Chappell


#include "SonoTextButton.h"
#include "SonoLookAndFeel.h"
#include <math.h>

SonoTextButton::SonoTextButton(const String & name)
  : TextButton(name), _buttonStyle(SonoButtonStyleNormal)
{
    
}

SonoTextButton::~SonoTextButton()
{
    
}

void SonoTextButton::setButtonStyle(SonoButtonStyle style)
{
    _buttonStyle = style;
}


void SonoTextButton::drawButtonBackground(Graphics& g, bool isMouseOverButton, bool isButtonDown)
{
    Colour bgcolor = findColour (getToggleState() ? buttonOnColourId : buttonColourId);
    Colour bordcolor =  isColourSpecified(outlineColourId) ? findColour (outlineColourId) : Colour::fromFloatRGBA(0.3, 0.3, 0.3, 0.5);

    if (isButtonDown) {
        bgcolor = bgcolor.withMultipliedBrightness(1.8);
    } else if (isMouseOverButton) {
        bgcolor = bgcolor.withMultipliedBrightness(1.3);
    }
    
    g.setColour(bgcolor);
    g.fillPath(fillPath);
    
    g.setColour(bordcolor);
    g.strokePath(borderPath, PathStrokeType(1.0));
    
}


void SonoTextButton::paintButton (Graphics& g, bool isMouseOverButton, bool isButtonDown)
{
    LookAndFeel& lf = getLookAndFeel();
    
    drawButtonBackground(g, isMouseOverButton, isButtonDown);

    if (SonoLookAndFeel * tetlf = dynamic_cast<SonoLookAndFeel*>(&lf)) {
        Justification just = textJustification;
        switch (_buttonStyle) {
            case SonoButtonStyleLowerLeftCorner: just = Justification::centredLeft; break;
            case SonoButtonStyleLowerRightCorner: just = Justification::centredRight; break;
            default: break;
        }
        
        tetlf->drawButtonTextWithAlignment (g, *this, isMouseOverButton, isButtonDown, just);
    }
    else {
        lf.drawButtonText (g, *this, isMouseOverButton, isButtonDown);
    }
}

bool SonoTextButton::hitTest (int x, int y)
{
    return fillPath.contains(x, y);
}


void SonoTextButton::resized()
{
    TextButton::resized();
    
    setupPath();
}

void SonoTextButton::setupPath() {

    float borderOffset = 0.5; // _borderWidth*1.5f;
    float width = getWidth();
    float height = getHeight();
    fillPath.clear();
    borderPath.clear();
    
 
    if (_buttonStyle == SonoButtonStyleTop) {
        fillPath.startNewSubPath(borderOffset, borderOffset);
        fillPath.quadraticTo(width/2.0, height-borderOffset, width - borderOffset, borderOffset);
        fillPath.lineTo(borderOffset, borderOffset);

        borderPath.startNewSubPath(borderOffset, borderOffset);
        borderPath.quadraticTo(width/2.0, height-borderOffset, width - borderOffset, borderOffset);

        //[fillPath moveToPoint:CGPointMake(borderOffset, borderOffset)];
        //[fillPath addQuadCurveToPoint:CGPointMake(width - borderOffset, borderOffset) controlPoint:CGPointMake(width/2.0, height-borderOffset) ];
        //[fillPath addLineToPoint:CGPointMake(borderOffset, borderOffset)];
        
        //[borderPath moveToPoint:CGPointMake(borderOffset, borderOffset)];
        //[borderPath addQuadCurveToPoint:CGPointMake(width - borderOffset, borderOffset) controlPoint:CGPointMake(width/2.0, height-borderOffset) ];


    }
    else if (_buttonStyle == SonoButtonStyleBottom) {
        // H /2 + W^2 / 8H
        float maxdim =  jmax(width, height);
        float radius = circleRadius > 0.0f ? circleRadius :  height * 0.5 + (width*width)/(8.0f * height);
        float startAng = atan2(height - radius, maxdim/2 - width);
        float endAng = atan2(height - radius, width - maxdim/2);

        startAng += M_PI_2;
        endAng += M_PI_2;
        
        DBG("Start ang " << radiansToDegrees(startAng) << " end " << radiansToDegrees(endAng) << " h: " << height << "  rad: " << radius);
        
        fillPath.startNewSubPath(borderOffset, height - borderOffset);
        fillPath.addCentredArc(maxdim/2, radius, radius, radius, 0.0f, startAng, endAng);
        fillPath.lineTo(borderOffset, height - borderOffset);

        borderPath.startNewSubPath(borderOffset, height - borderOffset);
        borderPath.addCentredArc(maxdim/2, radius, radius, radius, 0.0f, startAng, endAng);
        borderPath.lineTo(borderOffset, height - borderOffset);

        
        //[fillPath moveToPoint:CGPointMake(borderOffset, height - borderOffset)];
        //[fillPath addArcWithCenter:CGPointMake(maxdim/2, radius) radius:radius startAngle:startAng endAngle:endAng clockwise:YES];
        //[fillPath addLineToPoint:CGPointMake(borderOffset, height - borderOffset)];
        
        //borderPath = [UIBezierPath bezierPath];
        //[borderPath moveToPoint:CGPointMake(borderOffset, height -borderOffset)];
        //[borderPath addArcWithCenter:CGPointMake(maxdim/2, radius) radius:radius startAngle:startAng endAngle:endAng clockwise:YES];
        //[borderPath addLineToPoint:CGPointMake(borderOffset, height - borderOffset)];
    }
    else if (_buttonStyle == SonoButtonStyleLeft) {

        fillPath.startNewSubPath(borderOffset, borderOffset);
        fillPath.quadraticTo(width - borderOffset, borderOffset, width - borderOffset, height/2);
        fillPath.quadraticTo(width - borderOffset, height-borderOffset, borderOffset, height-borderOffset);
        fillPath.lineTo(borderOffset, borderOffset);

        borderPath.startNewSubPath(borderOffset, borderOffset);
        borderPath.quadraticTo(width - borderOffset, borderOffset, width - borderOffset, height/2);
        borderPath.quadraticTo(width - borderOffset, height-borderOffset, borderOffset, height-borderOffset);
        
        //fillPath = [UIBezierPath bezierPath];
        //[fillPath moveToPoint:CGPointMake(borderOffset, borderOffset)];
        //[fillPath addQuadCurveToPoint:CGPointMake(width - borderOffset, height/2) controlPoint:CGPointMake(width - borderOffset , borderOffset) ];
        //[fillPath addQuadCurveToPoint:CGPointMake(borderOffset, height-borderOffset) controlPoint:CGPointMake(width - borderOffset , height-borderOffset) ];
        //[fillPath addLineToPoint:CGPointMake(borderOffset, borderOffset)];
        
        //borderPath = [UIBezierPath bezierPath];
        //[borderPath moveToPoint:CGPointMake(borderOffset, borderOffset)];
        //[borderPath addQuadCurveToPoint:CGPointMake(width - borderOffset, height/2) controlPoint:CGPointMake(width - borderOffset , borderOffset) ];
        //[borderPath addQuadCurveToPoint:CGPointMake(borderOffset, height-borderOffset) controlPoint:CGPointMake(width - borderOffset , height-borderOffset) ];
    }
    else if (_buttonStyle == SonoButtonStyleRight) {

        fillPath.startNewSubPath(width - borderOffset, borderOffset);
        fillPath.quadraticTo(borderOffset, borderOffset, borderOffset, height/2);
        fillPath.quadraticTo(borderOffset, height-borderOffset, width - borderOffset, height-borderOffset);
        fillPath.lineTo(width - borderOffset, borderOffset);
        
        borderPath.startNewSubPath(width - borderOffset, borderOffset);
        borderPath.quadraticTo(borderOffset, borderOffset, borderOffset, height/2);
        borderPath.quadraticTo(borderOffset, height-borderOffset, width - borderOffset, height-borderOffset);

        
        //fillPath = [UIBezierPath bezierPath];
        //[fillPath moveToPoint:CGPointMake(width - borderOffset, borderOffset)];
        //[fillPath addQuadCurveToPoint:CGPointMake(borderOffset, height/2) controlPoint:CGPointMake(borderOffset, borderOffset) ];
        //[fillPath addQuadCurveToPoint:CGPointMake(width - borderOffset, height - borderOffset) controlPoint:CGPointMake(borderOffset, height - borderOffset)];
        //[fillPath addLineToPoint:CGPointMake(width - borderOffset, borderOffset)];
        
        //borderPath = [UIBezierPath bezierPath];
        //[borderPath moveToPoint:CGPointMake(width -borderOffset, borderOffset)];
        //[borderPath addQuadCurveToPoint:CGPointMake(borderOffset, height/2) controlPoint:CGPointMake(borderOffset, borderOffset) ];
        //[borderPath addQuadCurveToPoint:CGPointMake(width - borderOffset, height - borderOffset) controlPoint:CGPointMake(borderOffset, height - borderOffset)];
        
    }
    else if (_buttonStyle == SonoButtonStyleUpperLeftCorner) {

        fillPath.startNewSubPath(borderOffset, borderOffset);
        fillPath.lineTo(width - borderOffset, borderOffset);
        fillPath.quadraticTo(width - borderOffset, height - borderOffset, borderOffset, height - borderOffset);
        fillPath.lineTo(borderOffset, borderOffset);

        borderPath.startNewSubPath(width - borderOffset, borderOffset);
        borderPath.quadraticTo(width - borderOffset, height - borderOffset, borderOffset, height - borderOffset);

        
        //fillPath = [UIBezierPath bezierPath];
        //[fillPath moveToPoint:CGPointMake(borderOffset, borderOffset)];
        //[fillPath addLineToPoint:CGPointMake(width-borderOffset, borderOffset)];
        //[fillPath addQuadCurveToPoint:CGPointMake(borderOffset, height-borderOffset) controlPoint:CGPointMake(width-borderOffset, height-borderOffset) ];
        //[fillPath addLineToPoint:CGPointMake(borderOffset, borderOffset)];
        
        //borderPath = [UIBezierPath bezierPath];
        //[borderPath moveToPoint:CGPointMake(width-borderOffset, borderOffset)];
        //[borderPath addQuadCurveToPoint:CGPointMake(borderOffset, height-borderOffset) controlPoint:CGPointMake(width-borderOffset, height-borderOffset) ];
        
    }
    else if (_buttonStyle == SonoButtonStyleUpperRightCorner) {

        fillPath.startNewSubPath(width - borderOffset, borderOffset);
        fillPath.lineTo(width - borderOffset, height - borderOffset);
        fillPath.quadraticTo(borderOffset, height - borderOffset, borderOffset, borderOffset);
        fillPath.lineTo(width - borderOffset, borderOffset);
        
        borderPath.startNewSubPath(width - borderOffset, height - borderOffset);
        borderPath.quadraticTo(borderOffset, height - borderOffset, borderOffset, borderOffset);

        
        //fillPath = [UIBezierPath bezierPath];
        //[fillPath moveToPoint:CGPointMake(width - borderOffset, borderOffset)];
        //[fillPath addLineToPoint:CGPointMake(width - borderOffset, height - borderOffset)];
        //[fillPath addQuadCurveToPoint:CGPointMake(borderOffset, borderOffset) controlPoint:CGPointMake(borderOffset, height-borderOffset) ];
        //[fillPath addLineToPoint:CGPointMake(width - borderOffset, borderOffset)];
        
        //borderPath = [UIBezierPath bezierPath];
        //[borderPath moveToPoint:CGPointMake(width - borderOffset, height - borderOffset)];
        //[borderPath addQuadCurveToPoint:CGPointMake(borderOffset, borderOffset) controlPoint:CGPointMake(borderOffset, height-borderOffset) ];
        
    }
    else if (_buttonStyle == SonoButtonStyleLowerLeftCorner) {

        fillPath.startNewSubPath(borderOffset, height - borderOffset);
        fillPath.lineTo(borderOffset, borderOffset);
        fillPath.quadraticTo(width-borderOffset, borderOffset, width-borderOffset, height-borderOffset);
        fillPath.lineTo(borderOffset, height-borderOffset);

        borderPath.startNewSubPath(borderOffset, borderOffset);
        borderPath.quadraticTo(width-borderOffset, borderOffset, width-borderOffset, height-borderOffset);

        //fillPath = [UIBezierPath bezierPath];
        //[fillPath moveToPoint:CGPointMake(borderOffset, height-borderOffset)];
        //[fillPath addLineToPoint:CGPointMake(borderOffset, borderOffset)];
        //[fillPath addQuadCurveToPoint:CGPointMake(width-borderOffset, height-borderOffset) controlPoint:CGPointMake(width-borderOffset, borderOffset) ];
        //[fillPath addLineToPoint:CGPointMake(borderOffset, height-borderOffset)];
        
        //borderPath = [UIBezierPath bezierPath];
        //[borderPath moveToPoint:CGPointMake(borderOffset, borderOffset)];
        //[borderPath addQuadCurveToPoint:CGPointMake(width-borderOffset, height-borderOffset) controlPoint:CGPointMake(width-borderOffset, borderOffset) ];
    }
    else if (_buttonStyle == SonoButtonStyleLowerRightCorner) {

        fillPath.startNewSubPath(width-borderOffset, height - borderOffset);
        fillPath.lineTo(borderOffset, height-borderOffset);
        fillPath.quadraticTo(borderOffset, borderOffset, width-borderOffset, borderOffset);
        fillPath.lineTo(width - borderOffset, height-borderOffset);
        
        borderPath.startNewSubPath(borderOffset, height - borderOffset);
        borderPath.quadraticTo(borderOffset, borderOffset, width-borderOffset, borderOffset);

        
        //fillPath = [UIBezierPath bezierPath];
        //[fillPath moveToPoint:CGPointMake(width-borderOffset, height-borderOffset)];
        //[fillPath addLineToPoint:CGPointMake(borderOffset, height-borderOffset)];
        //[fillPath addQuadCurveToPoint:CGPointMake(width-borderOffset, borderOffset) controlPoint:CGPointMake(borderOffset, borderOffset) ];
        //[fillPath addLineToPoint:CGPointMake(width-borderOffset, height-borderOffset)];
        
        //borderPath = [UIBezierPath bezierPath];
        //[borderPath moveToPoint:CGPointMake(borderOffset, height-borderOffset)];
        //[borderPath addQuadCurveToPoint:CGPointMake(width-borderOffset, borderOffset) controlPoint:CGPointMake(borderOffset, borderOffset) ];
    }
    else if (_buttonStyle == SonoButtonStyleLowerRightCornerRound) {
        float maxdim =  jmax(width*2, height);
        // H /2 + W^2 / 8H (but double the width here)
        float radius = circleRadius > 0.0f ? circleRadius : height * 0.5 + (width*width*4.0)/(8.0f * height);
        float startAng = atan2(height - radius, maxdim/2 - width*2);
        float endAng = atan2(height - radius, width - maxdim/2) ;
        
        startAng += M_PI_2;
        endAng += M_PI_2;

        fillPath.startNewSubPath(borderOffset, height - borderOffset);
        fillPath.addCentredArc(maxdim/2, radius, radius, radius, 0.0f, startAng, endAng);
        fillPath.lineTo(width - borderOffset, height - borderOffset);
        fillPath.lineTo(borderOffset, height - borderOffset);

        borderPath.startNewSubPath(borderOffset, height - borderOffset);
        borderPath.addCentredArc(maxdim/2, radius, radius, radius, 0.0f, startAng, endAng);
        borderPath.lineTo(width - borderOffset, height - borderOffset);
        borderPath.lineTo(borderOffset, height - borderOffset);

        
        //fillPath = [UIBezierPath bezierPath];
        //[fillPath moveToPoint:CGPointMake(borderOffset, height - borderOffset)];
        //[fillPath addArcWithCenter:CGPointMake(maxdim/2, radius) radius:radius startAngle:startAng endAngle:endAng clockwise:YES];
        //[fillPath addLineToPoint:CGPointMake(width - borderOffset, height - borderOffset)];
        //[fillPath addLineToPoint:CGPointMake(borderOffset, height - borderOffset)];
        
        //borderPath = [UIBezierPath bezierPath];
        //[borderPath moveToPoint:CGPointMake(borderOffset, height -borderOffset)];
        //[borderPath addArcWithCenter:CGPointMake(maxdim/2, radius) radius:radius startAngle:startAng endAngle:endAng clockwise:YES];
        //[borderPath addLineToPoint:CGPointMake(width - borderOffset, height - borderOffset)];
        //[borderPath addLineToPoint:CGPointMake(borderOffset, height - borderOffset)];
    }
    else if (_buttonStyle == SonoButtonStyleLowerLeftCornerRound) {
        float maxdim =  jmax(width*2, height);
        // H /2 + W^2 / 8H (but double the width here)
        float radius = circleRadius > 0.0f ? circleRadius : height * 0.5 + (width*width*4.0)/(8.0f * height);
        float startAng = atan2(height - radius, width - maxdim/2) ;
        float endAng = atan2(height - radius, width*2 - maxdim/2);
        startAng += M_PI_2;
        endAng += M_PI_2;

        fillPath.startNewSubPath(borderOffset, height - borderOffset);
        fillPath.lineTo(borderOffset, borderOffset);
        fillPath.addCentredArc(maxdim/2, radius, radius, radius, 0.0f, startAng, endAng);
        fillPath.lineTo(borderOffset, height - borderOffset);

        borderPath.startNewSubPath(borderOffset, height - borderOffset);
        borderPath.lineTo(borderOffset, borderOffset);
        borderPath.addCentredArc(maxdim/2, radius, radius, radius, 0.0f, startAng, endAng);
        borderPath.lineTo(borderOffset, height - borderOffset);
        
        //fillPath = [UIBezierPath bezierPath];
        //[fillPath moveToPoint:CGPointMake(borderOffset, height - borderOffset)];
        //[fillPath addLineToPoint:CGPointMake(borderOffset, borderOffset)];
        //[fillPath addArcWithCenter:CGPointMake(maxdim/2 - width, radius) radius:radius startAngle:startAng endAngle:endAng clockwise:YES];
        //[fillPath addLineToPoint:CGPointMake(borderOffset, height - borderOffset)];
        
        //borderPath = [UIBezierPath bezierPath];
        //[borderPath moveToPoint:CGPointMake(borderOffset, height -borderOffset)];
        //[borderPath addLineToPoint:CGPointMake(borderOffset, borderOffset)];
        //[borderPath addArcWithCenter:CGPointMake(maxdim/2 - width, radius) radius:radius startAngle:startAng endAngle:endAng clockwise:YES];
        //[borderPath addLineToPoint:CGPointMake(borderOffset, height - borderOffset)];
    }
    else if (_buttonStyle == SonoButtonStyleUpperRightCornerRound) {
        float maxdim =  jmax(width*2, height);
        // H /2 + W^2 / 8H (but double the width here)
        float radius = circleRadius > 0.0f ? circleRadius : height * 0.5 + (width*width*4.0)/(8.0f * height);
        float startAng = atan2(radius - height, maxdim/2 - width*2);
        float endAng = atan2(radius - height , width - maxdim/2) ;
        startAng += M_PI_2;
        endAng += M_PI_2;

        DBG("URC Start ang " << radiansToDegrees(startAng) << " end " << radiansToDegrees(endAng) << " h: " << height << "  rad: " << radius);

        fillPath.startNewSubPath(borderOffset, borderOffset);
        fillPath.addCentredArc(maxdim/2, height-radius, radius, radius, 0.0f, startAng, endAng);
        fillPath.lineTo(width - borderOffset, borderOffset);
        fillPath.lineTo(borderOffset, borderOffset);

        borderPath.startNewSubPath(borderOffset, borderOffset);
        borderPath.addCentredArc(maxdim/2, height-radius, radius, radius, 0.0f, startAng, endAng);
        borderPath.startNewSubPath(width - borderOffset, borderOffset);
        borderPath.lineTo(borderOffset, borderOffset);

        //fillPath = [UIBezierPath bezierPath];
        //[fillPath moveToPoint:CGPointMake(borderOffset, borderOffset)];
        //[fillPath addArcWithCenter:CGPointMake(maxdim/2, height -radius) radius:radius startAngle:startAng endAngle:endAng clockwise:NO];
        //[fillPath addLineToPoint:CGPointMake(width - borderOffset, borderOffset)];
        //[fillPath addLineToPoint:CGPointMake(borderOffset, borderOffset)];
        
        //borderPath = [UIBezierPath bezierPath];
        //[borderPath moveToPoint:CGPointMake(borderOffset, borderOffset)];
        //[borderPath addArcWithCenter:CGPointMake(maxdim/2, height -radius) radius:radius startAngle:startAng endAngle:endAng clockwise:NO];
        //[borderPath addLineToPoint:CGPointMake(width - borderOffset, borderOffset)];
        //[borderPath moveToPoint:CGPointMake(width - borderOffset, borderOffset)];
        //[borderPath addLineToPoint:CGPointMake(borderOffset, borderOffset)];
    }
    else if (_buttonStyle == SonoButtonStyleUpperLeftCornerRound) {
        float maxdim =  jmax(width*2, height);
        // H /2 + W^2 / 8H (but double the width here)
        float radius = circleRadius > 0.0f ? circleRadius : height * 0.5 + (width*width*4.0)/(8.0f * height);
        float startAng = atan2(radius - height, width - maxdim/2) ;
        float endAng = atan2(radius-height, width*2 - maxdim/2);
        startAng += M_PI_2;
        endAng += M_PI_2;

        DBG("URC Start ang " << radiansToDegrees(startAng) << " end " << radiansToDegrees(endAng) << " h: " << height << "  rad: " << radius);

        
        fillPath.startNewSubPath(borderOffset, borderOffset);
        fillPath.lineTo(borderOffset, height - borderOffset);
        fillPath.addCentredArc(maxdim/2 - width, height-radius, radius, radius, 0.0f, startAng, endAng);
        fillPath.lineTo(borderOffset, borderOffset);

        borderPath.startNewSubPath(borderOffset, height - borderOffset);
        borderPath.lineTo(borderOffset, height - borderOffset);
        borderPath.addCentredArc(maxdim/2 - width, height-radius, radius, radius, 0.0f, startAng, endAng);
        borderPath.lineTo(borderOffset, borderOffset);

        //fillPath = [UIBezierPath bezierPath];
        //[fillPath moveToPoint:CGPointMake(borderOffset, borderOffset)];
        //[fillPath addLineToPoint:CGPointMake(borderOffset, height - borderOffset)];
        ////[fillPath moveToPoint:CGPointMake(borderOffset, height - borderOffset)];
        //[fillPath addArcWithCenter:CGPointMake(maxdim/2 - width, height -radius) radius:radius startAngle:startAng endAngle:endAng clockwise:NO];
        //[fillPath addLineToPoint:CGPointMake(borderOffset, borderOffset)];
        
        //borderPath = [UIBezierPath bezierPath];
        ////[borderPath moveToPoint:CGPointMake(borderOffset, borderOffset)];
        ////[borderPath addLineToPoint:CGPointMake(borderOffset, height - borderOffset)];
        //[borderPath moveToPoint:CGPointMake(borderOffset, height - borderOffset)];
        //[borderPath addArcWithCenter:CGPointMake(maxdim/2 - width, height -radius) radius:radius startAngle:startAng endAngle:endAng clockwise:NO];
        //[borderPath addLineToPoint:CGPointMake(borderOffset, borderOffset)];
    }
    else
    {
        auto bounds = getLocalBounds().toFloat().reduced(borderOffset);
        fillPath.addRoundedRectangle(bounds, cornerRadius);
        borderPath.addRoundedRectangle(bounds, cornerRadius);
    }

    
}
