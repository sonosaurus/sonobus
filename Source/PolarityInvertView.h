// SPDX-License-Identifier: GPLv3-or-later WITH Appstore-exception
// Copyright (C) 2021 Jesse Chappell


#pragma once

#include <JuceHeader.h>

#include "SonoLookAndFeel.h"
#include "SonoDrawableButton.h"
#include "EffectsBaseView.h"
#include "EffectParams.h"
#include "SonoChoiceButton.h"

#include "SonobusPluginProcessor.h"

//==============================================================================
/*
*/
class PolarityInvertView    : public EffectsBaseView,
   public Slider::Listener,
   public Button::Listener,
   public SonoChoiceButton::Listener,
   public EffectsBaseView::HeaderListener
{
public:
    PolarityInvertView(SonobusAudioProcessor & processor_, bool showdrag=true, bool input=false)  : processor(processor_), showDragIcon(showdrag), inputMode(input)
    {
        sonoSliderLNF.textJustification = Justification::centredLeft;

        // these are in the header component
        enableButton.setVisible(true);
        enableButton.addListener(this);
        titleLabel.setText(TRANS("Polarity Invert"), dontSendNotification);

        dragButton.setVisible(showDragIcon);

        setupLayout();
    }

    ~PolarityInvertView()
    {
    }

   
    class Listener {
    public:
        virtual ~Listener() {}
        virtual void polarityInvertChanged(PolarityInvertView *comp, bool polinvert) {}
    };
    
    void addListener(Listener * listener) { listeners.add(listener); }
    void removeListener(Listener * listener) { listeners.remove(listener); }

    void setShowDragIcon(bool flag) { showDragIcon = flag; }
    bool getShowDragIcon() const { return showDragIcon; }

    void setupLayout()
    {
        int minKnobWidth = 54;
        int minitemheight = 32;
        int knoblabelheight = 18;
        int knobitemheight = 62;
        int enablewidth = 44;
        int headerheight = 44;
        int buttwidth = 120;
        int autobuttwidth = 150;

#if JUCE_IOS || JUCE_ANDROID
        // make the button heights a bit more for touchscreen purposes
        minitemheight = 40;
        knobitemheight = 80;
        headerheight = 50;
#endif


        checkBox.items.clear();
        checkBox.flexDirection = FlexBox::Direction::row;
        //checkBox.items.add(FlexItem(5, 5).withMargin(0).withFlex(0));
        checkBox.items.add(FlexItem(enablewidth, minitemheight, enableButton).withMargin(0).withFlex(0));
        checkBox.items.add(FlexItem(2, 5).withMargin(0).withFlex(0));
        checkBox.items.add(FlexItem(100, minitemheight, titleLabel).withMargin(0).withFlex(1).withMaxWidth(120));
        //checkBox.items.add(FlexItem(2, 5).withMargin(0).withFlex(0.1));
        if (showDragIcon) {
            checkBox.items.add(FlexItem(24, minitemheight, dragButton).withMargin(0).withFlex(0));
        }
        checkBox.items.add(FlexItem(2, 5).withMargin(0).withFlex(0.1));


        dragButton.setVisible(showDragIcon);



        headerComponent.headerBox.items.clear();
        headerComponent.headerBox.flexDirection = FlexBox::Direction::column;
        headerComponent.headerBox.items.add(FlexItem(150, minitemheight, checkBox).withMargin(0).withFlex(1));

        mainBox.items.clear();
        mainBox.flexDirection = FlexBox::Direction::column;

        minBounds.setSize(120, 0);
        minHeaderBounds.setSize(120,  minitemheight + 8);

    }

   
    void resized() override
    {
        //mainBox.performLayout(getLocalBounds().reduced(2));

    }

    void buttonClicked (Button* buttonThatWasClicked) override
    {

        if (buttonThatWasClicked == &enableButton) {
            polarityInvert = enableButton.getToggleState();
            headerComponent.repaint();

            listeners.call (&PolarityInvertView::Listener::polarityInvertChanged, this, polarityInvert);
        }
    }

    void effectsHeaderClicked(EffectsBaseView *comp) override
    {
        //mParams.enabled = !enableButton.getToggleState();
        //processor.setMonitoringDelayActive(!enableButton.getToggleState());
        //updateParams(mParams);

        //listeners.call (&ReverbSendView::Listener::reverbSendLevelChanged, this, mParams);
    }

    
    void sliderValueChanged (Slider* slider) override
    {

    }

    void choiceButtonSelected(SonoChoiceButton *comp, int index, int ident) override
    {

    }


    void updateParams(bool polInvert) {
        polarityInvert = polInvert;
        enableButton.setToggleState(polInvert, dontSendNotification);
        headerComponent.repaint();
    }

    
private:
    
    SonoBigTextLookAndFeel sonoSliderLNF;

    ListenerList<Listener> listeners;

    SonobusAudioProcessor & processor;

    bool showDragIcon = false;
    bool inputMode = false;

    
    FlexBox mainBox;
    FlexBox checkBox;

    bool polarityInvert = false;


    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PolarityInvertView)
};
