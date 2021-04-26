// SPDX-License-Identifier: GPLv3-or-later WITH Appstore-exception
// Copyright (C) 2020 Jesse Chappell


#pragma once

#include "JuceHeader.h"

#include "SonoTextButton.h"
#include "GenericItemChooser.h"
#include "SonoLookAndFeel.h"

class SonoChoiceLookAndFeel : public SonoLookAndFeel
{
public:
    int getCallOutBoxBorderSize (const CallOutBox&) override {
        return 40;
    }
    
};



class SonoChoiceButton : public SonoTextButton, public GenericItemChooser::Listener, public Button::Listener
{
public:
    SonoChoiceButton();
    virtual ~SonoChoiceButton();
    
    class Listener {
    public:
        virtual ~Listener() {}
        virtual void choiceButtonSelected(SonoChoiceButton *comp, int index, int ident) {}
    };

    void paint(Graphics & g) override;
    void resized() override;

    void genericItemChooserSelected(GenericItemChooser *comp, int index) override;
    
    void clearItems();
    void addItem(const String & name, int ident);
    void addItem(const String & name, int ident, const Image & newItemImage);
    
    //void setItems(const StringArray & items);
    //const StringArray & getItems() const { return items; }

    void setSelectedItemIndex(int index, NotificationType notification = dontSendNotification);
    int getSelectedItemIndex() const { return selIndex; }

    void setSelectedId(int ident, NotificationType notification = dontSendNotification);

    void setShowArrow(bool flag) { showArrow = flag; }
    bool getShowArrow() const { return showArrow; }
    
    String getItemText(int index) const;
    
    void buttonClicked (Button* buttonThatWasClicked) override;

    void addChoiceListener(Listener * listener) { listeners.add(listener); }
    void removeChoiceListener(Listener * listener) { listeners.remove(listener); }

protected:
    ListenerList<Listener> listeners;

    std::unique_ptr<Label> textLabel;

    Array<GenericItemChooserItem> items;
    Array<int> idList;
    
    int selIndex;
    bool showArrow = true;
    
    SonoChoiceLookAndFeel lnf;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SonoChoiceButton)

};


