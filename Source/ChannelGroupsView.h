// SPDX-License-Identifier: GPLv3-or-later WITH Appstore-exception
// Copyright (C) 2020 Jesse Chappell


#pragma once

#include "JuceHeader.h"

#include "SonobusPluginProcessor.h"
#include "SonoLookAndFeel.h"
#include "SonoChoiceButton.h"
#include "SonoDrawableButton.h"
#include "GenericItemChooser.h"
#include "CompressorView.h"
#include "ExpanderView.h"
#include "ParametricEqView.h"
#include "MonitorDelayView.h"
#include "ReverbSendView.h"
#include "PolarityInvertView.h"
#include "ReverbView.h"

class ChannelGroupEffectsView :
public Component,
public CompressorView::Listener,
public ExpanderView::Listener,
public ParametricEqView::Listener,
public ReverbSendView::Listener,
public PolarityInvertView::Listener,
public EffectsBaseView::HeaderListener
{
public:
    ChannelGroupEffectsView(SonobusAudioProcessor& proc, bool peermode=false);
    virtual ~ChannelGroupEffectsView();


    class Listener {
    public:
        virtual ~Listener() {}
        virtual void effectsEnableChanged(ChannelGroupEffectsView *comp) {}
    };

    void addListener(Listener * listener) { listeners.add(listener); }
    void removeListener(Listener * listener) { listeners.remove(listener); }


    juce::Rectangle<int> getMinimumContentBounds() const;


    void updateState();

    void updateStateForRemotePeer();

    void updateStateForInput();
    void updateLayout();

    void resized() override;


    void compressorParamsChanged(CompressorView *comp, SonoAudio::CompressorParams & params) override;

    void expanderParamsChanged(ExpanderView *comp, SonoAudio::CompressorParams & params) override;
    void parametricEqParamsChanged(ParametricEqView *comp, SonoAudio::ParametricEqParams & params) override;

    void reverbSendLevelChanged(ReverbSendView *comp, float revlevel) override;

    void polarityInvertChanged(PolarityInvertView *comp, bool polinv) override;

    void effectsHeaderClicked(EffectsBaseView *comp) override;


    int  groupIndex = 0;
    bool peerMode = false;
    int  peerIndex = 0;
    bool firstShow = true;

protected:

    SonobusAudioProcessor& processor;

    ListenerList<Listener> listeners;

    std::unique_ptr<ConcertinaPanel> effectsConcertina;

    std::unique_ptr<CompressorView> compressorView;


    std::unique_ptr<ExpanderView> expanderView;

    std::unique_ptr<ParametricEqView> eqView;

    std::unique_ptr<ReverbSendView> reverbSendView;

    std::unique_ptr<PolarityInvertView> polarityInvertView;

    FlexBox effectsBox;

    juce::Rectangle<int> minBounds;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ChannelGroupEffectsView)

};

class ChannelGroupMonitorEffectsView :
public Component,
public ParametricEqView::Listener,
public MonitorDelayView::Listener,
public ReverbSendView::Listener,
public EffectsBaseView::HeaderListener
{
public:
    ChannelGroupMonitorEffectsView(SonobusAudioProcessor& proc, bool peermode=false);
    virtual ~ChannelGroupMonitorEffectsView();


    class Listener {
    public:
        virtual ~Listener() {}
        virtual void monitorEffectsEnableChanged(ChannelGroupMonitorEffectsView *comp) {}
    };

    void addListener(Listener * listener) { listeners.add(listener); }
    void removeListener(Listener * listener) { listeners.remove(listener); }


    juce::Rectangle<int> getMinimumContentBounds() const;


    void updateState();

    void updateStateForRemotePeer();
    void updateStateForInput();

    void updateLayout();

    void updateLayoutForRemotePeer();
    void updateLayoutForInput();

    void resized() override;

    void monitorDelayParamsChanged(MonitorDelayView *comp, SonoAudio::DelayParams &params) override;

    void reverbSendLevelChanged(ReverbSendView *comp, float revlevel) override;

    void effectsHeaderClicked(EffectsBaseView *comp) override;

    // Public accessors for OSC control
    MonitorDelayView* getDelayView() { return delayView.get(); }
    ReverbSendView* getReverbSendView() { return reverbSendView.get(); }


    int  groupIndex = 0;
    bool peerMode = false;
    int  peerIndex = 0;
    bool firstShow = true;

protected:

    SonobusAudioProcessor& processor;

    ListenerList<Listener> listeners;

    std::unique_ptr<ConcertinaPanel> effectsConcertina;

    std::unique_ptr<MonitorDelayView> delayView;

    std::unique_ptr<ReverbSendView> reverbSendView;


    FlexBox effectsBox;

    juce::Rectangle<int> minBounds;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ChannelGroupMonitorEffectsView)

};

class ChannelGroupReverbEffectsView :
public Component,
public ReverbView::Listener,
public EffectsBaseView::HeaderListener
{
public:
    ChannelGroupReverbEffectsView(SonobusAudioProcessor& proc);
    virtual ~ChannelGroupReverbEffectsView();


    class Listener {
    public:
        virtual ~Listener() {}
        virtual void reverbEffectsEnableChanged(ChannelGroupReverbEffectsView *comp) {}
    };

    void addListener(Listener * listener) { listeners.add(listener); }
    void removeListener(Listener * listener) { listeners.remove(listener); }


    juce::Rectangle<int> getMinimumContentBounds() const;


    void updateState();

    void updateLayout();

    void resized() override;

    //void reverbSendLevelChanged(ReverbSendView *comp, float revlevel) override;

    void effectsHeaderClicked(EffectsBaseView *comp) override;

    bool firstShow = true;

protected:

    SonobusAudioProcessor& processor;

    ListenerList<Listener> listeners;

    std::unique_ptr<ConcertinaPanel> effectsConcertina;

    std::unique_ptr<ReverbView> reverbView;


    FlexBox effectsBox;

    juce::Rectangle<int> minBounds;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ChannelGroupReverbEffectsView)

};


class ChannelGroupView : public Component {
public:
    ChannelGroupView();
    virtual ~ChannelGroupView();
    
    void paint(Graphics& g) override;
    void resized() override;
    
    //void updateLayout();

    int group = 0;
    int chanIndex = 0;
    int groupChanCount = 0;

    bool showDivider = false;

    SonoBigTextLookAndFeel smallLnf;
    SonoBigTextLookAndFeel medLnf;
    SonoBigTextLookAndFeel sonoSliderLNF;
    SonoPanSliderLookAndFeel panSliderLNF;

    SonoLookAndFeel rmeterLnf;
    SonoLookAndFeel smeterLnf;
    
    std::unique_ptr<Label> nameLabel;
    std::unique_ptr<TextEditor> nameEditor;
    std::unique_ptr<TextButton> muteButton;
    std::unique_ptr<TextButton> soloButton;
    std::unique_ptr<TextButton> fxButton;
    std::unique_ptr<TextButton> monfxButton;
    std::unique_ptr<Label>  chanLabel;
    std::unique_ptr<Label>  levelLabel;
    std::unique_ptr<Slider> levelSlider;
    std::unique_ptr<Slider> monitorSlider;
    std::unique_ptr<Label>  panLabel;
    std::unique_ptr<Slider> panSlider;
    std::unique_ptr<SonoDrawableButton> linkButton;
    std::unique_ptr<SonoDrawableButton> monoButton;
    std::unique_ptr<SonoDrawableButton> destButton;


    bool singlePanner = true;

    std::unique_ptr<foleys::LevelMeter> meter;
    std::unique_ptr<foleys::LevelMeter> premeter;

    
    FlexBox mainbox;
    FlexBox maincontentbox;
    FlexBox inbox;
    FlexBox monbox;
    FlexBox namebox;
    FlexBox levelbox;
    FlexBox pannerbox;

    FlexBox linkedchannelsbox;

    
    bool  useBgColor = false;
    Colour bgColor;
    Colour borderColor;
    Colour itemColor;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ChannelGroupView)
};



class ChannelGroupsView : public Component,
public Button::Listener,
public Slider::Listener,
public SonoChoiceButton::Listener,
public GenericItemChooser::Listener,
public ChannelGroupEffectsView::Listener,
public ChannelGroupMonitorEffectsView::Listener,
public MultiTimer
{
public:
    ChannelGroupsView(SonobusAudioProcessor&, bool peerMode, int peerIndex=0);
    virtual ~ChannelGroupsView();


    class Listener {
    public:
        virtual ~Listener() {}
        virtual void channelLayoutChanged(ChannelGroupsView *comp) {}
        virtual void nameLabelClicked(ChannelGroupsView *comp) {}
    };

    void addListener(Listener * listener) { listeners.add(listener); }
    void removeListener(Listener * listener) { listeners.remove(listener); }



    void paint(Graphics & g) override;
    
    void resized() override;

    void setPeerMode(bool peermode, int index=0);
    bool getPeerMode() const { return mPeerMode; }
    int getPeerIndex() const { return mPeerIndex; }

    void buttonClicked (Button* buttonThatWasClicked) override;        
    void sliderValueChanged (Slider* slider) override;
    void choiceButtonSelected(SonoChoiceButton *comp, int index, int ident) override;

    void effectsEnableChanged(ChannelGroupEffectsView *comp) override;

    void monitorEffectsEnableChanged(ChannelGroupMonitorEffectsView *comp) override;


    void mouseUp (const MouseEvent& event)  override;
    void mouseDown (const MouseEvent& event)  override;
    void mouseDrag (const MouseEvent& event)  override;


    void timerCallback(int timerId) override;

    void visibilityChanged() override;

    void setMetersActive(bool flag);

    int getGroupViewsCount() const { return mChannelViews.size(); }

    void rebuildChannelViews(bool notify=false);
    void updateChannelViews(int specific=-1);
    
    void setNarrowMode(bool flag, bool update=false) { if (isNarrow != flag) { isNarrow = flag; if (update) { updateLayout(); resized(); }} }
    bool setNarrowMode() const { return isNarrow; }
    
    void showDestSelectionMenu(Component * source, int index);

    void genericItemChooserSelected(GenericItemChooser *comp, int index) override;

    bool isDraggable(Component * comp) const;

    void clearClipIndicators();

    void toggleAllMonitorDelay();

    void setEstimatedWidth(int estwidth) { mEstimatedWidth = estwidth; }
    int getEstimatedWidth() const { return mEstimatedWidth;}

    juce::Rectangle<int> getMinimumContentBounds() const;

    void applyToAllSliders(std::function<void(Slider *)> & routine);

    void updateLayout(bool notify=true);
    
    // Getters for OSC access to nested controls
    ChannelGroupView* getFileChannelView() { return mFileChannelView.get(); }
    ChannelGroupView* getSoundboardChannelView() { return mSoundboardChannelView.get(); }
    ChannelGroupView* getMetChannelView() { return mMetChannelView.get(); }
    TextButton* getInReverbButton() { return mInReverbButton.get(); }
    TextButton* getMonDelayButton() { return mMonDelayButton.get(); }


    std::function<AudioDeviceManager*()> getAudioDeviceManager; // = []() { return 0; };


protected:

    void updateLayoutForRemotePeer(bool notify=true);
    void updateLayoutForInput(bool notify=true);

    void updateMonDelayButton();

    void configLevelSlider(Slider * slider, bool monmode=false);    
    void configLabel(Label *label, int ltype);
    void configKnobSlider(Slider * slider);    

    void updateInputModeChannelViews(int specific=-1);
    void updatePeerModeChannelViews(int specific=-1);

    void nameLabelChanged(int changroup, const String & name);

    void inputButtonPressed(Component *src, int index, bool newlinkstate);
    void peerChanButtonPressed(Component *src, int index, bool newlinkstate);

    void addGroupPressed();
    void clearGroupsPressed();
    void showChangeGroupChannels(int changroup, Component * showfrom);
    void showChangePeerChannelsLayout(int changroup, Component * showfrom);


    ChannelGroupView * createChannelGroupView(bool first=false);
    void setupChildren(ChannelGroupView * pvf);

    void showPopTip(const String & message, int timeoutMs, Component * target, int maxwidth);
    void showEffects(int index, bool flag, Component * fromView=nullptr);
    void showMonitorEffects(int index, bool flag, Component * fromView=nullptr);
    void showInputReverbView(bool flag, Component * fromView=nullptr);

    int getChanGroupFromIndex(int index);
    juce::Rectangle<int> getBoundsForChanGroup(int chgroup);
    int getChanGroupForPoint(Point<int> pos, bool inbetween);

    SonoBigTextLookAndFeel addLnf;

    SonobusAudioProcessor& processor;

    ListenerList<Listener> listeners;

    OwnedArray<ChannelGroupView> mChannelViews;
    std::unique_ptr<ChannelGroupView> mMainChannelView; // used for peers

    std::unique_ptr<ChannelGroupView> mFileChannelView; // used for input
    std::unique_ptr<ChannelGroupView> mMetChannelView; // used for input
    std::unique_ptr<ChannelGroupView> mSoundboardChannelView; // used for init


    std::unique_ptr<ChannelGroupEffectsView> mEffectsView;
    std::unique_ptr<ChannelGroupMonitorEffectsView> mMonEffectsView;
    std::unique_ptr<ChannelGroupReverbEffectsView> mInputReverbView;



    std::unique_ptr<Slider> mInGainSlider;
    std::unique_ptr<TextButton> mAddButton;
    std::unique_ptr<TextButton> mClearButton;
    std::unique_ptr<TextButton> mInReverbButton;
    std::unique_ptr<TextButton> mMonDelayButton;

    std::unique_ptr<DrawableRectangle> mInsertLine;
    std::unique_ptr<DrawableImage> mDragDrawable;
    std::unique_ptr<DrawableRectangle> mMetFileBg;


    std::unique_ptr<BubbleMessageComponent> popTip;

    std::unique_ptr<AudioProcessorValueTreeState::ButtonAttachment> mMetSendAttachment;
    std::unique_ptr<AudioProcessorValueTreeState::ButtonAttachment> mFileSendAttachment;
    std::unique_ptr<AudioProcessorValueTreeState::ButtonAttachment> mSoundboardSendAttachment;
    std::unique_ptr<AudioProcessorValueTreeState::SliderAttachment> mMetLevelAttachment;


    WeakReference<Component> effectsCalloutBox;
    WeakReference<Component> monEffectsCalloutBox;
    WeakReference<Component> inReverbCalloutBox;

    FlexBox channelsBox;
    FlexBox addrowBox;
    int channelMinHeight = 60;
    int channelMinWidth = 400;
    int mEstimatedWidth = 0;

    bool isNarrow = false;
    bool metersActive = false;

    bool mPeerMode = false;
    int mPeerIndex = 0;

    // dragging state
    bool mDraggingActive = false;
    int mDraggingSourceGroup = -1;
    int mDraggingGroupPos = -1;
    Array< juce::Rectangle<int> > mChanGroupBounds;
    Image  mDragImage;
    bool mAutoscrolling = false;

    uint32 lastUpdateTimestampMs = 0;
    
    Colour mutedBySoloColor;
    Colour mutedTextColor;
    Colour mutedColor;
    Colour soloColor;
    Colour regularTextColor;
    Colour droppedTextColor;
    Colour dimTextColor;
    Colour outlineColor;
    Colour bgColor;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ChannelGroupsView)

};
