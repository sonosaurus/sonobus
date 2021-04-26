// SPDX-License-Identifier: GPLv3-or-later WITH Appstore-exception
// Copyright (C) 2020 Jesse Chappell


#include "SonoDrawableButton.h"

SonoDrawableButton::SonoDrawableButton (const String& buttonName, ButtonStyle buttonStyle)
: DrawableButton(buttonName, buttonStyle)
{
}


void SonoDrawableButton::setBackgroundImage(const Drawable * img)
{
    if (img) {
        bgImage = img->createCopy();
    } else {
        bgImage.reset();
    }
}


void SonoDrawableButton::paint (Graphics& g)
{
    if (bgImage) {     
        bgImage->drawWithin(g, getLocalBounds().toFloat(), rectPlacement, 1.0);
    }
    
    DrawableButton::paint(g);
}

void SonoDrawableButton::resized()
{
    DrawableButton::resized();
}


Rectangle<float> SonoDrawableButton::getImageBounds() const
{
    Rectangle<int> r (getLocalBounds());
    
    if (getStyle() != ImageStretched)
    {
        int indentX = jmin (getEdgeIndent(), proportionOfWidth  (0.3f));
        int indentY = jmin (getEdgeIndent(), proportionOfHeight (0.3f));
        
        if (getStyle() == ImageOnButtonBackground)
        {
            indentX = jmax (indentX, proportionOfWidth  ((1.0f - fgImageRatio) * 0.5f));
            indentY = jmax (indentY, proportionOfHeight ((1.0f - fgImageRatio) * 0.5f));
            // indentX = jmax (getWidth()  / 4, indentX);
            // indentY = jmax (getHeight() / 4, indentY);
        }
        else if (getStyle() == ImageAboveTextLabel)
        {
            r = r.withTrimmedBottom (jmin (14, proportionOfHeight (0.2f)));
        }
        else if (getStyle() == ImageBelowTextLabel)
        {
            r = r.withTrimmedTop (jmin (14, proportionOfHeight (0.25f)));
        }
        else if (getStyle() == ImageLeftOfTextLabel)
        {
            r = r.withTrimmedRight (proportionOfWidth (1.0f - fgImageRatio));
        }
        else if (getStyle() == ImageRightOfTextLabel)
        {
            r = r.withTrimmedLeft (proportionOfWidth (1.0f - fgImageRatio));
        }
        r = r.reduced (indentX, indentY);
    }
    
    return r.toFloat();
}
