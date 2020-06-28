/*
  ==============================================================================

    BeatToggleGrid.h
    Created: 18 May 2017 5:29:12pm
    Author:  Jesse Chappell

  ==============================================================================
*/

#pragma once

#include "JuceHeader.h"

class BeatToggleGridDelegate;


class BeatPad : public Component
{
public:
    BeatPad() : active(false), selected(false), accented(false) {}
    
    bool  active;
    bool selected;
    bool accented;

    ScopedPointer<Label> label;
    ScopedPointer<DrawableRectangle> rect;
    ScopedPointer<DrawableRectangle> bgrect;
};

class BeatToggleGrid  :  public Component
{
public:
    BeatToggleGrid();
    virtual ~BeatToggleGrid();
    
    void setLabel(const String & text, int item);
    
    void setAccented(bool flag, int item);
    void setSelected(bool flag, int item);
    void setActive(bool flag, int item);

    bool getAccented(int item) const;
    bool getSelected(int item) const;
    bool getActive(int item) const;

    
    void setPadSize(int size, int item);
    
    void setSegments(int numSegments);
    int getSegments() const { return segments; }    
    void setSegmentSize(int size, int seg);

    void setItems(int numItems);
    int getItems() const { return items; }
    
    void setLandscape(bool land);
    bool getLandscape() const { return landscape; }
    
    void setDelegate(BeatToggleGridDelegate * del) { delegate = del; }
    
    void setValue(int val);
    int getValue() const { return value; }
    
    void resized() override;
    
    void refreshGrid(bool reset);

    void mouseDown (const MouseEvent &event) override;
    void mouseDrag (const MouseEvent &event) override;
    void mouseUp (const MouseEvent &event) override;
    
protected:
    
    void setupStuff();
    void updatedPadItem(int item);
    void refreshSizes();
    int findTouchPad(Point<int>touchPoint);

    ScopedPointer<Label> mInfoText;
    
    OwnedArray<BeatPad> gridLabels;
    
    HashMap<int, int> touchIndexes;
    
        int  segments = 0;
        
        Array<int>  segmentSizes;
        
    Colour offColor;
    Colour onColor;
    Colour activeColor;
    Colour accentedColor;
    
    int  items;
    int  value;
    bool landscape;
    float bgAlpha;

    
    bool keyboardMode;
  
    WeakReference<BeatToggleGridDelegate> delegate;
};


/*
 Protocol to be adopted by the section header's delegate; the section header tells its delegate when the section should be opened and closed.
 */
class BeatToggleGridDelegate
{
public:
    virtual ~BeatToggleGridDelegate()
    {
        // This will zero all the references - you need to call this in your destructor.
        masterReference.clear();
    }
    virtual bool beatToggleGridPressed(BeatToggleGrid* grid, int index, const MouseEvent & event) { return false; }
    virtual bool beatToggleGridMoved(BeatToggleGrid* grid, int fromIndex, int toIndex, const MouseEvent & event) {return false; }
    virtual bool beatToggleGridReleased(BeatToggleGrid* grid, int index, const MouseEvent & event) {return false; }

    virtual bool beatToggleGridSegmentChanged(BeatToggleGrid* grid, int index) {return false; }

private:
    WeakReference<BeatToggleGridDelegate>::Master masterReference;
    friend class WeakReference<BeatToggleGridDelegate>;
};

