// SPDX-License-Identifier: GPLv3-or-later
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

class ChannelGroupEffectsView :
public Component,
public CompressorView::Listener,
public ExpanderView::Listener,
public ParametricEqView::Listener,
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

    void effectsHeaderClicked(EffectsBaseView *comp, const MouseEvent & ev) override;


    int  groupIndex = 0;
    bool peerMode = false;
    int  peerIndex = 0;
    bool firstShow = true;

protected:

    SonobusAudioProcessor& processor;

    ListenerList<Listener> listeners;

    std::unique_ptr<ConcertinaPanel> effectsConcertina;

    std::unique_ptr<CompressorView> compressorView;
    std::unique_ptr<DrawableRectangle> compressorBg;
    std::unique_ptr<DrawableRectangle> reverbHeaderBg;


    std::unique_ptr<ExpanderView> expanderView;
    std::unique_ptr<DrawableRectangle> expanderBg;

    std::unique_ptr<ParametricEqView> eqView;


    FlexBox effectsBox;

    juce::Rectangle<int> minBounds;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ChannelGroupEffectsView)

};


class ChannelGroupView : public Component {
public:
    ChannelGroupView();
    virtual ~ChannelGroupView();
    
    void paint(Graphics& g) override;
    void resized() override;
    
    //void updateLayout();

    bool linked = false;

    SonoBigTextLookAndFeel smallLnf;
    SonoBigTextLookAndFeel medLnf;
    SonoBigTextLookAndFeel sonoSliderLNF;
    SonoPanSliderLookAndFeel panSliderLNF;

    SonoLookAndFeel rmeterLnf;
    SonoLookAndFeel smeterLnf;
    
    std::unique_ptr<Label> nameLabel;
    std::unique_ptr<TextButton> muteButton;
    std::unique_ptr<TextButton> soloButton;
    std::unique_ptr<TextButton> fxButton;
    std::unique_ptr<Label>  chanLabel;
    std::unique_ptr<Label>  levelLabel;
    std::unique_ptr<Slider> levelSlider;
    std::unique_ptr<Slider> monitorSlider;
    std::unique_ptr<Label>  panLabel;
    std::unique_ptr<Slider> panSlider;
    std::unique_ptr<SonoDrawableButton> linkButton;



    std::unique_ptr<foleys::LevelMeter> meter;

    
    FlexBox mainbox;
    FlexBox maincontentbox;
    FlexBox inbox;
    FlexBox monbox;
    FlexBox namebox;
    FlexBox levelbox;
    FlexBox pannerbox;

    FlexBox linkedchannelsbox;

    

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
public Label::Listener,
public ChannelGroupEffectsView::Listener,
public MultiTimer
{
public:
    ChannelGroupsView(SonobusAudioProcessor&, bool peerMode, int peerIndex=0);
    virtual ~ChannelGroupsView();


    class Listener {
    public:
        virtual ~Listener() {}
        virtual void channelLayoutChanged(ChannelGroupsView *comp) {}
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

    void labelTextChanged (Label* labelThatHasChanged) override;

    void effectsEnableChanged(ChannelGroupEffectsView *comp) override;

    void mouseUp (const MouseEvent& event)  override;

    void timerCallback(int timerId) override;

    void visibilityChanged() override;

    void setMetersActive(bool flag);

    int getGroupViewsCount() const { return mChannelViews.size(); }

    void rebuildChannelViews();
    void updateChannelViews(int specific=-1);
    
    void setNarrowMode(bool flag) { if (isNarrow != flag) { isNarrow = flag; updateLayout(); resized(); } }
    bool setNarrowMode() const { return isNarrow; }
    
    void showPopupMenu(Component * source, int index);

    void genericItemChooserSelected(GenericItemChooser *comp, int index) override;
    

    void clearClipIndicators();

    void setEstimatedWidth(int estwidth) { mEstimatedWidth = estwidth; }
    int getEstimatedWidth() const { return mEstimatedWidth;}

    Rectangle<int> getMinimumContentBounds() const;

    void applyToAllSliders(std::function<void(Slider *)> & routine);

    void updateLayout(bool notify=true);
    
protected:

    void configLevelSlider(Slider * slider, bool monmode=false);    
    void configLabel(Label *label, int ltype);
    void configKnobSlider(Slider * slider);    

    void updateInputModeChannelViews(int specific=-1);
    void updatePeerModeChannelViews(int specific=-1);

    void linkButtonPressed(int index, bool newlinkstate);


    ChannelGroupView * createChannelGroupView(bool first=false);

    void showPopTip(const String & message, int timeoutMs, Component * target, int maxwidth);
    void showEffects(int index, bool flag, Component * fromView=nullptr);


    ListenerList<Listener> listeners;

    OwnedArray<ChannelGroupView> mChannelViews;
    SonobusAudioProcessor& processor;

    std::unique_ptr<ChannelGroupEffectsView> mEffectsView;


    std::unique_ptr<Slider> mInGainSlider;

    std::unique_ptr<BubbleMessageComponent> popTip;

    WeakReference<Component> effectsCalloutBox;

    FlexBox channelsBox;
    int channelMinHeight = 60;
    int channelMinWidth = 400;
    int mEstimatedWidth = 0;

    bool isNarrow = false;
    bool metersActive = false;

    bool mPeerMode = false;
    int mPeerIndex = 0;

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
