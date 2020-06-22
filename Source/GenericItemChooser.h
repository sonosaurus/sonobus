
#pragma once

#include "JuceHeader.h"

#include "SonoTextButton.h"

class KeyListBox;

struct GenericItemChooserItem
{
    GenericItemChooserItem() : image(Image()) {}
    GenericItemChooserItem(const String & name_, const Image & image_=Image()) : name(name_), image(image_) {}
    String name;
    Image image;
};

class GenericItemChooser : public Component, public ListBoxModel, public Button::Listener
{
public:
    GenericItemChooser(const Array<GenericItemChooserItem> & items_, int tag=0);
    virtual ~GenericItemChooser();
    
    class Listener {
    public:
        virtual ~Listener() {}
        virtual void genericItemChooserSelected(GenericItemChooser *comp, int index) {}
    };
    
    void setItems(const Array<GenericItemChooserItem> & items);
    const Array<GenericItemChooserItem> & getItems() const { return items; }
    
    // This is overloaded from TableListBoxModel, and must return the total number of rows in our table
    int getNumRows() override;

    void 	paintListBoxItem (int rowNumber, Graphics &g, int width, int height, bool rowIsSelected) override;
    

    // This is overloaded from TableListBoxModel, and must update any custom components that we're using
    //Component* refreshComponentForCell (int rowNumber, int columnId, bool /*isRowSelected*/,
    //                                    Component* existingComponentToUpdate) override;

    
    void setCurrentRow(int index);
    int getCurrentRow() const { return currentIndex; }
    
    //==============================================================================
    void resized() override;

    void listBoxItemClicked (int rowNumber, const MouseEvent& e) override;
    void selectedRowsChanged(int lastRowSelected) override;

    void buttonClicked (Button* buttonThatWasClicked) override;
    void paint (Graphics& g) override;
    
    void addListener(Listener * listener) { listeners.add(listener); }
    void removeListener(Listener * listener) { listeners.remove(listener); }

    void setRowHeight(int ht);
    int getRowHeight() const { return rowHeight; }
    
    void setMaxHeight(int ht);
    int getMaxHeight() const { return maxHeight; }
    
    void setTag(int tag_) { tag = tag_;}
    int getTag() const { return tag; }
    
    static CallOutBox& launchPopupChooser(const Array<GenericItemChooserItem> & items, Rectangle<int> targetBounds, Component * targetComponent, GenericItemChooser::Listener * listener, int tag = 0, int selectedIndex=-1, int maxheight=0);
    
private:
    
    int  getAutoWidth();
    
    ListenerList<Listener> listeners;

    ListBox table;     // the table component itself
    Font font;
    Font catFont;

    int numRows;            // The number of rows of data we've got
    bool sortDirection;
    int selectedRow;
    int rowHeight;
    int maxHeight = 0;
    
    //StringArray items;
    Array<GenericItemChooserItem> items;

    int currentIndex;
    int tag;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GenericItemChooser)
};



