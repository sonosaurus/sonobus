

#include "SonoCallOutBox.h"


SonoCallOutBox::SonoCallOutBox (Component& contentComponent,
                                Rectangle<int> areaToPointTo,
                                Component* parentComponent,
                                std::function<bool(const Component*)> canPassthroughFunc)
: CallOutBox(contentComponent, areaToPointTo, parentComponent) , canPassthrough(canPassthroughFunc)
{

}


//==============================================================================
class SonoCallOutBoxCallback  : public ModalComponentManager::Callback,
                            private Timer
{
public:
    SonoCallOutBoxCallback (std::unique_ptr<Component> c, const Rectangle<int>& area, Component* parent, bool dismissIfBg, std::function<bool(const Component*)>passthroughFunc)
        : content (std::move (c)),
          callout (*content, area, parent), dismissIfBackgrounded(dismissIfBg)
    {
        callout.canPassthrough = passthroughFunc;
        callout.setVisible (true);
        callout.enterModalState (true, this);
        if (dismissIfBackgrounded) {
            startTimer (200);
        }
    }

    void modalStateFinished (int) override {}

    void timerCallback() override
    {
        if (! Process::isForegroundProcess())
            callout.dismiss();
    }

    std::unique_ptr<Component> content;
    SonoCallOutBox callout;
    bool dismissIfBackgrounded = true;

    JUCE_DECLARE_NON_COPYABLE (SonoCallOutBoxCallback)
};

SonoCallOutBox& SonoCallOutBox::launchAsynchronously (std::unique_ptr<Component> content, Rectangle<int> area, Component* parent, bool dismissIfBackgrounded, std::function<bool(const Component*)> canPassthroughFunc)
{
    jassert (content != nullptr); // must be a valid content component!

    return (new SonoCallOutBoxCallback (std::move (content), area, parent, dismissIfBackgrounded, canPassthroughFunc))->callout;
}

bool SonoCallOutBox::canModalEventBeSentToComponent (const Component* targetComponent)
{
    if (canPassthrough) {
        return canPassthrough(targetComponent);
    }
    return false;
}

void SonoCallOutBox::setVisible(bool shouldBeVisible)
{
    CallOutBox::setVisible(shouldBeVisible);
    
    if (wasHidden && !shouldBeVisible) {
        wasHidden();
    }
}
