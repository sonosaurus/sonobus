/*
  ==============================================================================

    BeatToggleGrid.cpp
    Created: 18 May 2017 5:29:12pm
    Author:  Jesse Chappell

  ==============================================================================
*/

#include "BeatToggleGrid.h"

#include "DebugLogC.h"

BeatToggleGrid::BeatToggleGrid()
{
    setupStuff();
}

BeatToggleGrid::~BeatToggleGrid()
{

}


void BeatToggleGrid::setupStuff()
{
    // Initialization code here.
    //int width = frame.size.width;
    //int height = frame.size.height;
    
    /*
     CGRect labelFrame = CGRectMake(10, 1, width-10, height-5);
     mInfoText = [[UILabel alloc] initWithFrame:labelFrame];
     mInfoText.text = @"Instrument";
     mInfoText.textColor = [UIColor lightTextColor];
     mInfoText.backgroundColor = [UIColor clearColor];
     mInfoText.textAlignment = UITextAlignmentCenter;
     mInfoText.font = [UIFont systemFontOfSize:12.0];
     
     [self addSubview:mInfoText];
     */
    
    
    //offColor = [[UIColor colorWithRed:0.3 green:0.4 blue:0.5 alpha:1.0 ] retain];
    //offColor = [[UIColor colorWithRed:0.2 green:0.27 blue:0.333 alpha:1.0 ] retain];
    offColor = Colour::fromFloatRGBA(0.3, 0.3, 0.3, 0.6);
    
    
    //onColor = [[UIColor colorWithRed:0.2 green:0.3 blue:0.4 alpha:1.0 ] retain];
    //onColor = [[UIColor colorWithRed:0.247 green:0.368 blue:0.329 alpha:1.0 ] retain];
    onColor = Colour::fromFloatRGBA(0.1, 0.4, 0.6, 0.9);
    
    activeColor = Colour::fromFloatRGBA(0.1, 0.7, 0.5, 1.0);
    
    accentedColor = Colour::fromFloatRGBA(0.5, 0.4, 0.6, 0.9);
    
    
    //self.backgroundColor = [UIColor colorWithRed:0.4 green:0.5 blue:0.6 alpha:1.0 ];
    
    //self.backgroundColor = [UIColor colorWithRed:0.15 green:0.17 blue:0.27 alpha:balpha];
    
    //self.backgroundColor = [UIColor colorWithRed:0.3 green:0.4 blue:0.5 alpha:0.4 ];
    //    self.layer.cornerRadius = 6;
    //    self.layer.masksToBounds = YES;
    //self.layer.borderColor = [UIColor colorWithRed:0.05 green:0.15 blue:0.2 alpha:0.4].CGColor;
    
    //self.backgroundColor = [UIColor redColor];
    
    
    //self.layer.borderColor = [[UIColor colorWithRed:0.1 green:0.1 blue:0.1 alpha:0.5] CGColor];
    //self.layer.borderWidth = 1.0;
    
        
    segments = 1;
    segmentSizes.add(0);

    value = -1;
}


void BeatToggleGrid::resized()
{
    refreshSizes();
}

int BeatToggleGrid::findTouchPad(Point<int>touchPoint)
{
    for (int i=0; i < items; ++i) {
        Point<int> localPoint = gridLabels[i]->getLocalPoint(this, touchPoint);
        // DebugLogC("find %d  localpoint: %d %d  touchpoint: %d %d", i, localPoint.getX(), localPoint.getY(), touchPoint.getX(), touchPoint.getY());
        if (gridLabels[i]->getBounds().contains(touchPoint) && gridLabels[i]->isVisible()) {
            return i;
        }
    }
    
    return -1;
}


void BeatToggleGrid::mouseDown (const MouseEvent &event)
{
    int i = event.source.getIndex();
    
    Point<int> touchPoint = event.getPosition();;
        
    DebugLogC("Touch began: x=%d  y=%d", touchPoint.x, touchPoint.y);
    
    int index = findTouchPad(touchPoint);
    if (index >= 0) {
        DebugLogC("Hit on %d", index);
        
        setValue(index);
        
        touchIndexes.set(i, index);
        
        if (delegate.get()) {
            delegate.get()->beatToggleGridPressed(this, index, event);
        }
    }
}


void BeatToggleGrid::mouseDrag (const MouseEvent &event)
{
    
    int i = event.source.getIndex();
    
    Point<int> touchPoint = event.getPosition();;

    // DebugLogC("Touch moved: %d  x=%d  y=%d", i, touchPoint.x, touchPoint.y);
    
    int index = findTouchPad(touchPoint);
    if (index >= 0 && touchIndexes.contains(i)) {
        int lastindex = touchIndexes[i];
        
        if (lastindex != index) {
            // DebugLogC("Move Hit on %d", index);
            
            setValue(index);

            touchIndexes.set(i, index);

            if (delegate.get()) {
                delegate.get()->beatToggleGridMoved(this, lastindex, index, event);
            }
        }
    }
    
}


void BeatToggleGrid::mouseUp (const MouseEvent &event)
{
    DebugLogC("TOUCH END");

    int i = event.source.getIndex();
    
    Point<int> touchPoint = event.getPosition();;

    if (touchIndexes.contains(i)) {
        int lastindex = touchIndexes[i];
        
        if (lastindex >= 0) {
            DebugLogC("end Hit on %d", lastindex);
            

            if (delegate.get()) {
                delegate.get()->beatToggleGridReleased(this, lastindex, event);
            }
        }

        touchIndexes.remove(i);
    }
    
}

void BeatToggleGrid::setItems(int numItems)
{
    items = numItems;
    refreshGrid(false);
}

void BeatToggleGrid::setLandscape(bool land)
{
    landscape = land;
    refreshGrid(true);
}


void BeatToggleGrid::setValue(int val)
{
    if (val == value) return;
    
    value = val;
    DebugLogC("Setvalue to %u", value);
    
    //for (size_t i=0; i < [gridLabels count]; ++i)
    //{
    //    [self setSelected:(i==value) forRow:i];
    //}
    //[self refreshGrid];

    repaint();
}


void BeatToggleGrid::setLabel(const String & text, int item)
{
    if (item < items && item >=0 && item < gridLabels.size()) {
        BeatPad * pad = (BeatPad*) gridLabels[item];
        if (pad) {
            pad->label->setText(text, dontSendNotification);
        }
    }
    repaint();
}

void BeatToggleGrid::updatedPadItem(int item)
{
    if (item >=0 && item < gridLabels.size()) {
        BeatPad * pad = (BeatPad*) gridLabels[item];
        if (pad) {
            //DebugLogC("Setting bgcolor for %d  act: %d  sel: %d", item, pad->active, pad->selected);
            if (pad->active) {
                pad->rect->setFill(FillType(activeColor));
                pad->rect->setVisible(true);
            } else {
                pad->rect->setVisible(false);
            }
            pad->bgrect->setFill( pad->accented ? FillType(accentedColor) : (pad->selected ? FillType(onColor) : FillType(offColor)));
        }
    }
}

void BeatToggleGrid::setSelected(bool flag, int item)
{
    if (item < items && item >=0 && item < gridLabels.size()) {
        BeatPad * pad = (BeatPad*) gridLabels[item];
        if (pad && pad->selected != flag) {
            pad->selected = flag;
            updatedPadItem(item);
        }
    }
}

void BeatToggleGrid::setAccented(bool flag, int item)
{
    if (item < items && item >=0 && item < gridLabels.size()) {
        BeatPad * pad = (BeatPad*) gridLabels[item];
        if (pad && pad->accented != flag) {
            pad->accented = flag;
            updatedPadItem(item);
        }
    }
}


void BeatToggleGrid::setActive(bool flag, int item)
{
    if (item < items && item >=0 && item < gridLabels.size()) {
        BeatPad * pad = (BeatPad*) gridLabels[item];
        if (pad && pad->active != flag) {
            pad->active = flag;
            updatedPadItem(item);
        }
    }
}

bool BeatToggleGrid::getAccented(int item) const
{
    if (item < items && item >=0 && item < gridLabels.size()) {
        BeatPad * pad = (BeatPad*) gridLabels[item];
        if (pad) {
            return pad->accented;
        }
    }
    return false;
}

bool BeatToggleGrid::getSelected(int item) const
{
    if (item < items && item >=0 && item < gridLabels.size()) {
        BeatPad * pad = (BeatPad*) gridLabels[item];
        if (pad) {
            return pad->selected;
        }
    }
    return false;
}

bool BeatToggleGrid::getActive(int item) const
{
    if (item < items && item >=0 && item < gridLabels.size()) {
        BeatPad * pad = (BeatPad*) gridLabels[item];
        if (pad) {
            return pad->active;
        }
    }
    return false;
}


void BeatToggleGrid::setPadSize(int size, int item)
{
}


void BeatToggleGrid::setSegments(int numSegments)
{
    if (segments != numSegments) {
        segments = numSegments;
        
        segmentSizes.clear();
        
        for (int i=0; i < segments; ++i) {
            segmentSizes.add(0);
        }
        
        DebugLogC("Now segments is %u  segsize: %u", segments, segmentSizes.size());
        
    }
}


void BeatToggleGrid::setSegmentSize(int size, int seg)
{
    DebugLogC("Segment sizes is now: %u   want: %d", segmentSizes.size(), seg);
    if (seg < segments && seg < segmentSizes.size()) {
        segmentSizes.set(seg, size);
    }
}



void BeatToggleGrid::refreshSizes()
{
    // if we have multiple segments (>1), size based on selected
    
    int firstItem = 0;
    int itemCount = gridLabels.size();
    
    int width = getWidth();
    int height = getHeight();
    int yoffset = 4;
    int xoffset = 4;
    
    int rowgap = 2;
    int rowCount = segmentSizes.size();
    
    int rowheight = rowCount > 0 ? (height - 2*yoffset -rowgap) / rowCount : 0;
    int minrowwidth = 1000;
    
    for (int segi=0; segi < segmentSizes.size(); ++segi) {
        int colCount = segmentSizes[segi];
        int rowwidth = colCount > 0 ? (width - rowgap*(colCount+1) - xoffset) / colCount : 0;
        if (rowwidth < minrowwidth) {
            minrowwidth = rowwidth;
        }
    }
    
    for (int segi=0; segi < segmentSizes.size(); ++segi) {
        int colCount = segmentSizes[segi];
        
        int rowwidth = colCount > 0 ? (width - rowgap*(colCount+1) - xoffset) / colCount : 0;
        int xstart = (int) (width - (minrowwidth*colCount + rowgap*(colCount+1)) - 2*xoffset)/2 + xoffset;
        
        for (int i=firstItem; i < firstItem+colCount && i < gridLabels.size(); ++i) {
            BeatPad * pad = (BeatPad*) gridLabels.getUnchecked(i);
            
            // set the frames
            Rectangle<int> padFrame = Rectangle<int>((i-firstItem)*(minrowwidth+rowgap) + rowgap + xstart , yoffset, minrowwidth, rowheight);
            pad->setBounds(padFrame);
            pad->rect->setRectangle(Rectangle<float>(5, 5, padFrame.getWidth() - 10, padFrame.getHeight()-10));
            pad->label->setBounds(Rectangle<int>(0, 0, padFrame.getWidth(), padFrame.getHeight()));
            pad->bgrect->setRectangle(Rectangle<float>(0, 0, padFrame.getWidth(), padFrame.getHeight()));
            pad->setVisible(true);
            
            float fontsize = rintf(std::min(std::max(18.0f, (float)padFrame.getHeight() * 0.8f), 52.0f));
            pad->label->setFont(Font(fontsize));
            
            
            //DebugLogC("Setting pad           frame %d  to: %d  %d  %d %d", i, padFrame.getX(), padFrame.getY(), padFrame.getWidth(), padFrame.getHeight());
            //DebugLogC("Setting pad iamgeview frame %d  to: %d  %d  %d %d", i, pad->rect->getX(), pad->rect->getY(), pad->rect->getWidth(), pad->rect->getHeight());
        }
        
        firstItem += segmentSizes[segi];
        yoffset += rowheight + 2;
    }
    
    // hide the remainder
    for (int i=firstItem; i < gridLabels.size(); ++i) {
        BeatPad * pad = (BeatPad*) gridLabels.getUnchecked(i);
        pad->setVisible(false);
    }
    
}


void BeatToggleGrid::refreshGrid(bool reset)
{
    if (reset) {
        while (gridLabels.size() > 0) {
            BeatPad * label = (BeatPad*) gridLabels.getLast();
            if (label) {
                DebugLogC("removing last item");
                removeChildComponent(label);
            }
            gridLabels.removeLast();
        }
    }
    
    int width = getWidth();
    int height = getHeight();
    
    DebugLogC("Request items %d  landscape: %d  width: %d  h: %d", items, landscape, width, height);
    
    // if we have multiple segments (>1), size based on selected
    
    int yoffset = 4;
    int xoffset = 4;
    int rowgap = 1;
    int rowwidth = items > 0 ? (width - rowgap*(items+1) - xoffset) / items : 0;
    int rowheight = height - 10;
    
    if (gridLabels.size() < items)
    {
        int i = (int) gridLabels.size();
        while (gridLabels.size() < items) {
            
            //CGRect labelFrame = CGRectMake(5, i*(rowheight+rowgap) + rowgap + yoffset , width-10, rowheight);
            Rectangle<int> labelFrame = Rectangle<int>(i*(rowwidth+rowgap) + rowgap + xoffset , 5, rowwidth, rowheight);
            if (landscape) {
                //labelFrame = CGRectMake(i*(rowheight+rowgap) + rowgap + yoffset , 5, rowheight, height-10);
                //labelFrame = CGRectMake(i*(rowheight+rowgap) - (int)(rowheight/2) + yoffset, (int)(rowheight/2) + 5, height - 10, rowheight);
            }
            
            BeatPad * pad = new BeatPad();
            pad->setBounds(labelFrame);

            DrawableRectangle * bgiv = new DrawableRectangle();
            bgiv->setRectangle(Rectangle<float>(0, 0, pad->getWidth(), pad->getHeight()));
            bgiv->setCornerSize(Point<float>(12.0f, 12.0f));
            bgiv->setFill(FillType(offColor));
            pad->addAndMakeVisible(bgiv);
            pad->bgrect = bgiv;
            bgiv->setInterceptsMouseClicks(false, false);

            DrawableRectangle * iv = new DrawableRectangle();
            iv->setRectangle(Rectangle<float>(5, 0, pad->getWidth() - 10, pad->getHeight()-20));
            iv->setCornerSize(Point<float>(12.0f, 12.0f));
            pad->addAndMakeVisible(iv);
            pad->rect = iv;
            pad->rect->setVisible(false);
            iv->setInterceptsMouseClicks(false, false);

            
            float fontsize = rintf(std::min(std::max(18.0f, (float)labelFrame.getHeight() * 0.8f), 52.0f));
            
            Label *label = new Label();
            label->setBounds(Rectangle<int>(0, 0, labelFrame.getWidth(), pad->getHeight()));
            label->setText(String::formatted("%d", i+1), dontSendNotification);
            label->setColour(Label::textColourId, Colours::white);
            label->setJustificationType(Justification::centred);
            label->setFont(Font(fontsize));
            label->setInterceptsMouseClicks(false, false);
            label->setMinimumHorizontalScale(0.3);
            
            pad->addAndMakeVisible(label);
            pad->label = label;
            pad->setInterceptsMouseClicks(false, false);

            addAndMakeVisible(pad);
            gridLabels.add(pad);
            
            //DebugLogC("Added label for row");
            ++i;
        }
    }
    else if (gridLabels.size() > items) {
        // hide them
        while (gridLabels.size() > items) {
            BeatPad * label = (BeatPad*) gridLabels.getLast();
            if (label) {
               // DebugLogC("removing last item");
                removeChildComponent(label);
            }
            gridLabels.removeLast();
        }
        
    }
    
    refreshSizes();

    repaint();
}

