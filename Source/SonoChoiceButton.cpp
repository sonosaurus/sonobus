// SPDX-License-Identifier: GPLv3-or-later
// Copyright (C) 2020 Jesse Chappell

//
//  ScaleTableList.cpp
//  ThumbJamJuce
//
//  Created by Jesse Chappell on 6/4/14.
//
//

#include "SonoChoiceButton.h"
//#include "AppState.h"

//using namespace SonoAudio;



SonoChoiceButton::SonoChoiceButton()
{
    addAndMakeVisible(textLabel = new Label());
    textLabel->setJustificationType(Justification::centredLeft);
    
    selIndex = 0;
    addListener(this);
    
}

SonoChoiceButton::~SonoChoiceButton()
{
    
}

void SonoChoiceButton::clearItems()
{
    items.clear();
    idList.clear();
}

void SonoChoiceButton::addItem(const String & name, int ident)
{
    items.add(GenericItemChooserItem(name));
    idList.add(ident);
}

void SonoChoiceButton::addItem(const String & name, int ident, const Image & newItemImage)
{
    items.add(GenericItemChooserItem(name, newItemImage));
    idList.add(ident);
}


void SonoChoiceButton::setSelectedItemIndex(int index, NotificationType notification)
{
    selIndex = index;
    if (selIndex < items.size()) {
        textLabel->setText(items[selIndex].name, dontSendNotification);
    }

    if (notification != dontSendNotification) {
        int ident = index < idList.size() ? idList[index] : 0;
        listeners.call (&SonoChoiceButton::Listener::choiceButtonSelected, this, index, ident);
    }

    repaint();
}

void SonoChoiceButton::setSelectedId(int ident, NotificationType notification)
{
    for (int i=0; i < idList.size(); ++i) {
        if (idList[i] == ident) {
            setSelectedItemIndex(i, notification);
            break;
        }
    }
}


String SonoChoiceButton::getItemText(int index) const
{
    if (selIndex < items.size()) {
        return items[selIndex].name;
    }
    
    return "";
}


void SonoChoiceButton::genericItemChooserSelected(GenericItemChooser *comp, int index)
{
    setSelectedItemIndex(index);
    
    int ident = index < idList.size() ? idList[index] : 0;
    
    listeners.call (&SonoChoiceButton::Listener::choiceButtonSelected, this, index, ident);
    
    if (CallOutBox* const dw = comp->findParentComponentOfClass<CallOutBox>()) {
        dw->dismiss();
    }

}

void SonoChoiceButton::resized()
{
    SonoTextButton::resized();

    int xoff = 0;
    bool validImage = false;
    if (selIndex < items.size()) {
        if (items[selIndex].image.isValid()) {
            float imagesize = getHeight() - 8;
            xoff = imagesize;
            validImage = true;
        }
    }
    
    
    if (showArrow && getWidth()-(20 + xoff + 4) > 40) {
        textLabel->setBounds(4 + xoff, 2, getWidth() - 22, getHeight()-4 - xoff);
    }
    else if (!showArrow && getWidth()-(xoff + 4) > 40) {
        textLabel->setBounds(4 + xoff, 2, getWidth() - 8 - xoff, getHeight()-4 - xoff);        
    }
    else {
        textLabel->setSize(0, 0);
    }
}


void SonoChoiceButton::paint(Graphics & g)
{
    int width = getWidth();
    int height = getHeight();
    
    SonoTextButton::paint(g);
    
    if (showArrow) {
        Rectangle<int> arrowZone (width - 20, 0, 16, height);
        Path path;
        path.startNewSubPath (arrowZone.getX() + 3.0f, arrowZone.getCentreY() - 2.0f);
        path.lineTo (static_cast<float> (arrowZone.getCentreX()), arrowZone.getCentreY() + 3.0f);
        path.lineTo (arrowZone.getRight() - 3.0f, arrowZone.getCentreY() - 2.0f);
    
        g.setColour (findColour (ComboBox::arrowColourId).withAlpha ((isEnabled() ? 0.9f : 0.2f)));
        g.strokePath (path, PathStrokeType (2.0f));
    }
    
    if (selIndex < items.size()) {
        if (items[selIndex].image.isValid()) {
            float imagesize = height - 8;
            //g.drawImage(items[selIndex].image, Rectangle<float>(2, 4, imagesize, imagesize));
            g.drawImageWithin(items[selIndex].image, 2, 4, imagesize, imagesize, RectanglePlacement(RectanglePlacement::centred|RectanglePlacement::onlyReduceInSize));

        }
    }
}


void SonoChoiceButton::buttonClicked (Button* buttonThatWasClicked)
{
    GenericItemChooser * chooser = new GenericItemChooser(items);
    chooser->setRowHeight(std::min(getHeight(), 40));
    chooser->addListener(this);
    chooser->setCurrentRow(selIndex);
    //DocumentWindow* const dw = this->findParentComponentOfClass<DocumentWindow>();
    Component* dw = this->findParentComponentOfClass<DocumentWindow>();

    if (!dw) {
        dw = this->findParentComponentOfClass<AudioProcessorEditor>();        
    }
    if (!dw) {
        dw = this->findParentComponentOfClass<Component>();        
    }
    
    chooser->setSize(jmin(chooser->getWidth(), dw->getWidth() - 16), jmin(chooser->getHeight(), dw->getHeight() - 20));

    
    Rectangle<int> bounds =  dw->getLocalArea(nullptr, getScreenBounds());
    
    CallOutBox & box = CallOutBox::launchAsynchronously (chooser, bounds , dw);
    box.setDismissalMouseClicksAreAlwaysConsumed(true);
    //box.setArrowSize(0);
}
