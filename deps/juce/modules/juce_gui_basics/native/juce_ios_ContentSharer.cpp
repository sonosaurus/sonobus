/*
  ==============================================================================

   This file is part of the JUCE library.
   Copyright (c) 2022 - Raw Material Software Limited

   JUCE is an open source library subject to commercial or open-source
   licensing.

   By using JUCE, you agree to the terms of both the JUCE 7 End-User License
   Agreement and JUCE Privacy Policy.

   End User License Agreement: www.juce.com/juce-7-licence
   Privacy Policy: www.juce.com/juce-privacy-policy

   Or: You may also use this code under the terms of the GPL v3 (see
   www.gnu.org/licenses).

   JUCE IS PROVIDED "AS IS" WITHOUT ANY WARRANTY, AND ALL WARRANTIES, WHETHER
   EXPRESSED OR IMPLIED, INCLUDING MERCHANTABILITY AND FITNESS FOR PURPOSE, ARE
   DISCLAIMED.

  ==============================================================================
*/

namespace juce
{

class ContentSharer::ContentSharerNativeImpl    : public ContentSharer::Pimpl,
                                                  private Component
{
public:
    ContentSharerNativeImpl (ContentSharer& cs)
        : owner (cs)
    {
        static PopoverDelegateClass cls;
        JUCE_BEGIN_IGNORE_WARNINGS_GCC_LIKE ("-Wobjc-method-access")
        popoverDelegate.reset ([cls.createInstance() initWithContentSharerNativeImpl:this]);
        JUCE_END_IGNORE_WARNINGS_GCC_LIKE
    }

    ~ContentSharerNativeImpl() override
    {
        exitModalState (0);
    }

    void shareFiles (const Array<URL>& files) override
    {
        auto urls = [NSMutableArray arrayWithCapacity: (NSUInteger) files.size()];

        for (const auto& f : files)
        {
            NSString* nativeFilePath = nil;

            if (f.isLocalFile())
            {
                nativeFilePath = juceStringToNS (f.getLocalFile().getFullPathName());
            }
            else
            {
                auto filePath = f.toString (false);

                auto* fileDirectory = filePath.contains ("/")
                                    ? juceStringToNS (filePath.upToLastOccurrenceOf ("/", false, false))
                                    : [NSString string];

                auto fileName = juceStringToNS (filePath.fromLastOccurrenceOf ("/", false, false)
                                                        .upToLastOccurrenceOf (".", false, false));

                auto fileExt = juceStringToNS (filePath.fromLastOccurrenceOf (".", false, false));

                if ([fileDirectory length] == NSUInteger (0))
                    nativeFilePath = [[NSBundle mainBundle] pathForResource: fileName
                                                                     ofType: fileExt];
                else
                    nativeFilePath = [[NSBundle mainBundle] pathForResource: fileName
                                                                     ofType: fileExt
                                                                inDirectory: fileDirectory];
            }

            if (nativeFilePath != nil) {
                [urls addObject: [NSURL fileURLWithPath: nativeFilePath]];
            }
            else {
                // just add the URL as-is (eg, NON file URLS like links, etc)
                [urls addObject: [NSURL URLWithString:juceStringToNS(f.toString(true))]]; 
            }
        }

        share (urls);
    }

    void shareText (const String& text) override
    {
        auto array = [NSArray arrayWithObject: juceStringToNS (text)];
        share (array);
    }

private:
    void share (NSArray* items)
    {
        if ([items count] == 0)
        {
            jassertfalse;
            owner.sharingFinished (false, "No valid items found for sharing.");
            return;
        }

        controller.reset ([[UIActivityViewController alloc] initWithActivityItems: items
                                                            applicationActivities: nil]);

        controller.get().excludedActivityTypes = nil;

        controller.get().completionWithItemsHandler = ^([[maybe_unused]] UIActivityType type, BOOL completed,
                                                        [[maybe_unused]] NSArray* returnedItems, NSError* error)
        {       
            // In some odd cases (e.g. with 'Save to Files' activity, but only if it's dismissed
            // by tapping Cancel and not outside the modal window), completionWithItemsHandler
            // may be called twice: when an activity is cancelled and when the
            // UIActivityViewController itself is dismissed. We must make sure exitModalState()
            // doesn't get called twice - checking presentingViewController seems to do the trick.
            if (controller.get().presentingViewController == nil) {
                succeeded = completed;
                
                if (error != nil)
                    errorDescription = nsStringToJuce ([error localizedDescription]);
                
                exitModalState (0);
            }
        };

        controller.get().modalTransitionStyle = UIModalTransitionStyleCoverVertical;

        auto bounds = Desktop::getInstance().getDisplays().getPrimaryDisplay()->userArea;
        setBounds (bounds);

        setAlwaysOnTop (true);
        if (owner.parentComponent != nullptr) {
            owner.parentComponent->addAndMakeVisible (this);
        } else {
            setVisible (true);
            addToDesktop (0);
        }

        enterModalState (true,
                         ModalCallbackFunction::create ([this] (int)
                         {
                             owner.sharingFinished (succeeded, errorDescription);
                         }),
                         false);
    }

    static bool isIPad()
    {
        return [UIDevice currentDevice].userInterfaceIdiom == UIUserInterfaceIdiomPad;
    }

    //==============================================================================
    void parentHierarchyChanged() override
    {
        auto* newPeer = dynamic_cast<UIViewComponentPeer*> (getPeer());

        if (peer != newPeer)
        {
            peer = newPeer;

            if (isIPad())
            {
                auto* popoverController = controller.get().popoverPresentationController;
                popoverController.sourceView = peer->view;
                popoverController.sourceRect = getPopoverSourceRect();
                popoverController.canOverlapSourceViewRect = YES;
                popoverController.delegate = popoverDelegate.get();
            }

            if (auto* parentController = peer->controller)
                [parentController showViewController: controller.get() sender: parentController];
        }
    }

    CGRect getPopoverSourceRect() {
        auto bounds = peer->view.bounds;
        
        return owner.sourceComponent == nullptr
        ? CGRectMake (0.f, bounds.size.height - 10.f, bounds.size.width, 10.f)
        : makeCGRect (peer->getAreaCoveredBy (*owner.sourceComponent.getComponent()));
    }
    
    //==============================================================================
    struct PopoverDelegateClass    : public ObjCClass<NSObject<UIPopoverPresentationControllerDelegate>>
    {
        PopoverDelegateClass()  : ObjCClass<NSObject<UIPopoverPresentationControllerDelegate>> ("PopoverDelegateClass_")
        {
            addIvar<ContentSharer::ContentSharerNativeImpl*>("nativeSharer");
            
            JUCE_BEGIN_IGNORE_WARNINGS_GCC_LIKE ("-Wundeclared-selector")
            addMethod (@selector (initWithContentSharerNativeImpl:), initWithContentSharerNativeImpl);
            JUCE_END_IGNORE_WARNINGS_GCC_LIKE

            addMethod (@selector (popoverPresentationController:willRepositionPopoverToRect:inView:), willRepositionPopover);

            registerClass();
        }

        static id initWithContentSharerNativeImpl (id _self, SEL, ContentSharer::ContentSharerNativeImpl* nativeSharer)
        {
            NSObject* self = sendSuperclassMessage<NSObject*> (_self, @selector (init));
            object_setInstanceVariable (self, "nativeSharer", nativeSharer);
            return self;
        }
        
        //==============================================================================
        static void willRepositionPopover (id self, SEL, UIPopoverPresentationController*, CGRect* rect, UIView*)
        {
            *rect = getIvar<ContentSharer::ContentSharerNativeImpl*> (self, "nativeSharer")->getPopoverSourceRect();
        }
    };

    ContentSharer& owner;
    UIViewComponentPeer* peer = nullptr;
    NSUniquePtr<UIActivityViewController> controller;
    NSUniquePtr<NSObject<UIPopoverPresentationControllerDelegate>> popoverDelegate;

    bool succeeded = false;
    String errorDescription;
};

//==============================================================================
ContentSharer::Pimpl* ContentSharer::createPimpl()
{
    return new ContentSharerNativeImpl (*this);
}

} // namespace juce
