
#pragma once

#include "JuceHeader.h"

class SonoCallOutBox
: public juce::CallOutBox
{
public:

    SonoCallOutBox (Component& contentComponent,
                    Rectangle<int> areaToPointTo,
                    Component* parentComponent,
                    std::function<bool(const Component*)> canPassthroughFunc = {});

    /** This will launch a callout box containing the given content, pointing to the
        specified target component.

        This method will create and display a callout, returning immediately, after which
        the box will continue to run modally until the user clicks on some other component, at
        which point it will be dismissed and deleted automatically.

        It returns a reference to the newly-created box so that you can customise it, but don't
        keep a pointer to it, as it'll be deleted at some point when it gets closed.

        @param contentComponent     the component to display inside the call-out. This should
                                    already have a size set (although the call-out will also
                                    update itself when the component's size is changed later).
        @param areaToPointTo        the area that the call-out's arrow should point towards. If
                                    a parentComponent is supplied, then this is relative to that
                                    parent; otherwise, it's a global screen coord.
        @param parentComponent      if not a nullptr, this is the component to add the call-out to.
                                    If this is a nullptr, the call-out will be added to the desktop.
        @param dismissIfBackgrounded  If this is true, the call-out will be dismissed if we are no
                                    longer the foreground app.
        @param canPassthroughFunc  A function that will be called to determine if another component can be
                                   clicked/touched without dismissing us. if not specified all other touches will dismiss
    */
    static SonoCallOutBox& launchAsynchronously (std::unique_ptr<Component> contentComponent,
                                                 Rectangle<int> areaToPointTo,
                                                 Component* parentComponent,
                                                 bool dismissIfBackgrounded=true,
                                                 std::function<bool(const Component*)> canPassthroughFunc = {} );


    bool canModalEventBeSentToComponent (const Component* targetComponent) override;


    void setVisible(bool shouldBeVisible) override;
    
    std::function<bool(const Component*)> canPassthrough;

    std::function<void()> wasHidden;

    
protected:


    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SonoCallOutBox)
};
