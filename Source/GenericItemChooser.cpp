// SPDX-License-Identifier: GPLv3-or-later WITH Appstore-exception
// Copyright (C) 2020 Jesse Chappell


#include "GenericItemChooser.h"

enum {
    nameTextColourId = 0x1002830,
    currentNameTextColourId = 0x1002850,
    selectedColourId = 0x1002840,
    separatorColourId = 0x1002860,
    disabledColourId = 0x1002870
};


CallOutBox& GenericItemChooser::launchPopupChooser(const Array<GenericItemChooserItem> & items, Rectangle<int> targetBounds, Component * targetComponent, GenericItemChooser::Listener * listener, int tag, int selectedIndex, int maxheight, bool dismissSel)
{
    
    auto chooser = std::make_unique<GenericItemChooser>(items, tag);
    chooser->dismissOnSelected = dismissSel;
    if (selectedIndex >= 0) {
        chooser->setCurrentRow(selectedIndex);
    }

    if (listener) {
        chooser->addListener(listener);
    }
    if (maxheight > 0) {
        chooser->setMaxHeight(maxheight);        
    }
    
    
    CallOutBox & box = CallOutBox::launchAsynchronously (std::move(chooser), targetBounds, targetComponent);
    box.setDismissalMouseClicksAreAlwaysConsumed(true);
    // box.setArrowSize(0);
    box.grabKeyboardFocus();

    return box;
}

CallOutBox& GenericItemChooser::launchPopupChooser(const Array<GenericItemChooserItem> & items, juce::Rectangle<int> targetBounds, Component * targetComponent, std::function<void (GenericItemChooser* chooser,int index)> onSelectedFunction, int selectedIndex, int maxheight, bool dismissSel)
{
    auto chooser = std::make_unique<GenericItemChooser>(items, 0);
    chooser->dismissOnSelected = dismissSel;

    if (selectedIndex >= 0) {
        chooser->setCurrentRow(selectedIndex);
    }

    chooser->onSelected = onSelectedFunction;

    if (maxheight > 0) {
        chooser->setMaxHeight(maxheight);
    }

    CallOutBox & box = CallOutBox::launchAsynchronously (std::move(chooser), targetBounds, targetComponent);
    box.setDismissalMouseClicksAreAlwaysConsumed(true);
    // box.setArrowSize(0);
    box.grabKeyboardFocus();

    return box;
}


GenericItemChooser::GenericItemChooser(const Array<GenericItemChooserItem> & items_, int tag_)   : font (16.0, Font::plain), catFont(15.0, Font::plain), items(items_), tag(tag_)

{
    currentIndex = -1;
#if JUCE_IOS || JUCE_ANDROID
    rowHeight = 42;
#else
    rowHeight = 32;
#endif
    numRows = items.size();
    
    // Create our table component and add it to this component..
    addAndMakeVisible (table);
    table.setModel (this);

    // give it a border

    table.setColour (ListBox::outlineColourId, Colour::fromFloatRGBA(0.0, 0.0, 0.0, 0.0));
    table.setColour (ListBox::backgroundColourId, Colour::fromFloatRGBA(0.1, 0.1, 0.1, 1.0f));
    table.setColour (ListBox::textColourId, Colours::whitesmoke.withAlpha(0.8f));
    
    setColour (nameTextColourId, Colour::fromFloatRGBA(1.0f, 1.0f, 1.0f, 0.8f));
    setColour (currentNameTextColourId, Colour::fromFloatRGBA(0.4f, 0.8f, 1.0f, 0.9f));
    setColour (selectedColourId, Colour (0xff3d70c8).withAlpha(0.5f));
    setColour (separatorColourId, Colour::fromFloatRGBA(1.0f, 1.0f, 1.0f, 0.5f));
    setColour (disabledColourId, Colour::fromFloatRGBA(0.7f, 0.7f, 0.7f, 0.7f));

    
    table.setOutlineThickness (0);
    
    
    
    //table.getViewport()->setScrollOnDragEnabled(true);
    table.getViewport()->setScrollBarsShown(true, false);
    table.getViewport()->setScrollOnDragMode(Viewport::ScrollOnDragMode::nonHover);
    table.setRowSelectedOnMouseDown(true);
    table.setRowClickedOnMouseDown(false);
    table.setMultipleSelectionEnabled (false);
    table.setRowHeight(rowHeight);
   
    int newh = (rowHeight) * numRows + 4;
    setSize(getAutoWidth(), newh);

}

GenericItemChooser::~GenericItemChooser()
{
}

void GenericItemChooser::setCurrentRow(int index)
{
    currentIndex = index;
    SparseSet<int> selrows;
    selrows.addRange(Range<int>(index,index+1));
    table.setSelectedRows(selrows);
    table.updateContent();
}


void GenericItemChooser::setItems(const Array<GenericItemChooserItem> & items_)
{
    items = items_;
    numRows = items.size();
    table.updateContent();
    
    int newh = (rowHeight) * numRows;

    setSize(getAutoWidth(), newh);

}

int GenericItemChooser::getAutoWidth()
{
    int targw = 60;
    
    for (int i=0; i < items.size(); ++i) {
        int tsize = font.getStringWidth(items[i].name);
        if (items[i].image.isValid()) {
            tsize += rowHeight - 8;
        }
        targw = jmax(targw, tsize);
    }
    
    return targw + 30;
}


void GenericItemChooser::setRowHeight(int ht)
{
    rowHeight = ht;
    table.setRowHeight(rowHeight);
    int newh = (rowHeight+2) * numRows;
    if (maxHeight > 0) {
        newh = jmin(newh, maxHeight);
    }
    setSize(getAutoWidth(), newh);
}

void GenericItemChooser::setMaxHeight(int ht)
{
    maxHeight = ht;

    int newh = (rowHeight+2) * numRows;
    if (maxHeight > 0) {
        newh = jmin(newh, maxHeight);
    }
    setSize(getAutoWidth(), newh);
}


    // This is overloaded from TableListBoxModel, and must return the total number of rows in our table
int GenericItemChooser::getNumRows()
{
    return numRows;
}

String GenericItemChooser::getNameForRow (int rowNumber)
{
    if (rowNumber< items.size()) {
        return items[rowNumber].name;
    }
    return ListBoxModel::getNameForRow(rowNumber);
}


void GenericItemChooser::listBoxItemClicked (int rowNumber, const MouseEvent& e)
{

    DBG("listbox clicked");

    if (items[rowNumber].disabled) {
        // not selectable
        return;
    }

    listeners.call (&GenericItemChooser::Listener::genericItemChooserSelected, this, rowNumber);

    if (onSelected) {
        onSelected(this, rowNumber);
    }

    if (dismissOnSelected) {
        if (CallOutBox* const cb = findParentComponentOfClass<CallOutBox>()) {
            cb->dismiss();
        }
    } else {
        setCurrentRow(rowNumber);
        repaint();
    }
}

void GenericItemChooser::selectedRowsChanged(int lastRowSelected)
{
    // notify listeners
    DBG("Selected rows changed");
}

void GenericItemChooser::deleteKeyPressed (int)
{
    DBG("delete key pressed");

}

void GenericItemChooser::returnKeyPressed (int rowNumber)
{
    DBG("return key pressed: " << rowNumber);

    listeners.call (&GenericItemChooser::Listener::genericItemChooserSelected, this, rowNumber);

    if (rowNumber < items.size() && items[rowNumber].disabled) {
        // not selectable
        return;
    }

    if (onSelected) {
        onSelected(this, rowNumber);
    }

    if (dismissOnSelected) {

        if (CallOutBox* const cb = findParentComponentOfClass<CallOutBox>()) {
            cb->giveAwayKeyboardFocus();
            cb->dismiss();
        }
    } else {
        setCurrentRow(rowNumber);
        repaint();
    }
}


void GenericItemChooser::paintListBoxItem (int rowNumber, Graphics &g, int width, int height, bool rowIsSelected)
{

    if (items[rowNumber].separator) {
        g.setColour (findColour(separatorColourId));
        g.drawLine(0, 0, width, 0);
    }

    if (rowIsSelected && !items[rowNumber].disabled) {
        g.setColour (findColour(selectedColourId));
        g.fillRect(Rectangle<int>(0,0,width,height));
    }
    
    if (items[rowNumber].disabled) {
        g.setColour (findColour(disabledColourId));
    }
    else if (rowNumber == currentIndex) {
        g.setColour (findColour(currentNameTextColourId));
    }
    else {
        g.setColour (findColour(nameTextColourId));
    }

    g.setFont (font);

    int imagewidth = 0;



    if (rowNumber < items.size()) {
        if (items[rowNumber].image.isValid()) {
            imagewidth = height-8;
            //g.drawImage(items[rowNumber].image, Rectangle<float>(2, 2, imagewidth, height - 4));
            g.drawImageWithin(items[rowNumber].image, 2, 4, imagewidth, imagewidth, RectanglePlacement(RectanglePlacement::centred|RectanglePlacement::onlyReduceInSize));

            
        }
    }

    
    /*

    int imagewidth = height * 0.75;
    
    if (rowNumber == 0) {
        g.drawImageWithin(wheelImage, 2, 2, imagewidth, height - 4, RectanglePlacement::centred|RectanglePlacement::onlyReduceInSize);
    }
    else if (rowNumber == 1) {
        g.drawImageWithin(stringImage, 2, 2, imagewidth, height - 4, RectanglePlacement::centred|RectanglePlacement::onlyReduceInSize);
    }
    else {
        g.drawImageWithin(keyboardImage, 2, 2, imagewidth, height - 4, RectanglePlacement::centred|RectanglePlacement::onlyReduceInSize);
    }
    */
        
    String text = items[rowNumber].name;
    //DBG("Paint " << text);

    
    //g.drawFittedText (text, imagewidth + 10, 0, width - (imagewidth+8), height, Justification::centredLeft, 1, 0.5);
    g.drawFittedText (text, imagewidth + 8, 0, width - (imagewidth+8), height, Justification::centredLeft, 1, 0.5);
}



void GenericItemChooser::buttonClicked (Button* buttonThatWasClicked)
{
    //AppState * app = AppState::getInstance();
    
    
}


void GenericItemChooser::paint (Graphics& g)
{
    //g.fillAll (Colour (0xff000000));
}
 

//==============================================================================
void GenericItemChooser::resized()
{
    // position our table with a gap around its edge
    int keywidth = 50;
    int bottomButtHeight = 40;
    
    table.setBoundsInset (BorderSize<int>(2,2,2,2));
    
//    table.setTouchScrollScale(1.0f / AppState::getInstance()->scaleFactor);
    
    //int bottbuttwidth = (int) (0.3333f * (getWidth()-keywidth));
    //int addbuttwidth = (getWidth()-keywidth) - 2*bottbuttwidth - 8;
    
    //allButton->setBounds(keywidth+2, getHeight()-bottomButtHeight-4, bottbuttwidth, bottomButtHeight);
    //favsButton->setBounds(keywidth+2+bottbuttwidth, getHeight()-bottomButtHeight-4, bottbuttwidth, bottomButtHeight);
    //addFavButton->setBounds(getWidth()-addbuttwidth-4, getHeight()-bottomButtHeight-4, addbuttwidth, bottomButtHeight);
    
}

