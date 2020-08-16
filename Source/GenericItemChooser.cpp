//
//  ScaleTableList.cpp
//  ThumbJamJuce
//
//  Created by Jesse Chappell on 6/4/14.
//
//

#include "GenericItemChooser.h"
//#include "AppState.h"
#include "DebugLogC.h"

//using namespace SonoAudio;

enum {
    nameTextColourId = 0x1002830,
    currentNameTextColourId = 0x1002850,
    selectedColourId = 0x1002840
};


CallOutBox& GenericItemChooser::launchPopupChooser(const Array<GenericItemChooserItem> & items, Rectangle<int> targetBounds, Component * targetComponent, GenericItemChooser::Listener * listener, int tag, int selectedIndex, int maxheight)
{
    GenericItemChooser * chooser = new GenericItemChooser(items, tag);
    if (selectedIndex >= 0) {
        chooser->setCurrentRow(selectedIndex);
    }
    if (listener) {
        chooser->addListener(listener);
    }
    if (maxheight > 0) {
        chooser->setMaxHeight(maxheight);        
    }
    
    
    CallOutBox & box = CallOutBox::launchAsynchronously (chooser, targetBounds, targetComponent);
    box.setDismissalMouseClicksAreAlwaysConsumed(true);
    // box.setArrowSize(0);
    return box;
}



GenericItemChooser::GenericItemChooser(const Array<GenericItemChooserItem> & items_, int tag_)   : font (16.0, Font::plain), catFont(15.0, Font::plain), items(items_), tag(tag_)

{
    currentIndex = -1;
    rowHeight = 42;
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

    
    table.setOutlineThickness (0);
    
    
    
    //table.getViewport()->setScrollOnDragEnabled(true);
    table.getViewport()->setScrollBarsShown(true, false);
    table.getViewport()->setScrollOnDragEnabled(true);
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
    
    return targw + 20;
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

void GenericItemChooser::listBoxItemClicked (int rowNumber, const MouseEvent& e)
{
    listeners.call (&GenericItemChooser::Listener::genericItemChooserSelected, this, rowNumber);
    
}

void GenericItemChooser::selectedRowsChanged(int lastRowSelected)
{
    // notify listeners

}

void GenericItemChooser::paintListBoxItem (int rowNumber, Graphics &g, int width, int height, bool rowIsSelected)
{
    
    if (rowIsSelected) {
        g.setColour (findColour(selectedColourId));
        g.fillRect(Rectangle<int>(0,0,width,height));
    }
    
    if (rowNumber == currentIndex) {
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
    //DebugLogC("Paint %s", text.toRawUTF8());

    
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

