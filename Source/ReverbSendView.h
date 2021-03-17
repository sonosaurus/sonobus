// SPDX-License-Identifier: GPLv3-or-later
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
class ReverbSendView    : public EffectsBaseView,
   public Slider::Listener,
   public Button::Listener,
   public SonoChoiceButton::Listener,
   public EffectsBaseView::HeaderListener
{
public:
    ReverbSendView(SonobusAudioProcessor & processor_)  : sonoSliderLNF(14), processor(processor_)
    {
        sonoSliderLNF.textJustification = Justification::centredLeft;

        sendSlider.setName("revsend");
        //sendSlider.setRange(0.0f, 500.0f, 0.1);
        //sendSlider.setSkewFactor(0.4);
        //sendSlider.setTextValueSuffix(" dB");
        //timeSlider.setDoubleClickReturnValue(true, -60.0);
        configLevelSlider(sendSlider, true, TRANS("Send Level: "));
        sendSlider.addListener(this);
        //sendSlider.getProperties().set ("noFill", true);
        sendSlider.setTextBoxIsEditable(true);
        sendSlider.setLookAndFeel(&sonoSliderLNF);
        sendSlider.setTextBoxStyle(Slider::TextBoxAbove, true, 150, 14);

        sendLabel.setText(TRANS("Reverb Send"), dontSendNotification);
        configLabel(sendLabel, true);
        sendLabel.setJustificationType(Justification::centredLeft);

        infoLabel.setText(TRANS("Enable the main reverb at the bottom of the window to hear the effect"), dontSendNotification);
        configLabel(infoLabel);
        infoLabel.setJustificationType(Justification::centredLeft);
        infoLabel.setMinimumHorizontalScale(0.9);

        // these are in the header component
        enableButton.setVisible(false);
        enableButton.addListener(this);
        titleLabel.setText(TRANS("Main Reverb Send"), dontSendNotification);

        dragButton.setVisible(false);

        //addAndMakeVisible(sendLabel);
        addAndMakeVisible(sendSlider);
        addAndMakeVisible(infoLabel);

        //addAndMakeVisible(headerComponent);
        //addHeaderListener(this);

        setupLayout();
        
    }

    ~ReverbSendView()
    {
    }

   
    
    void paint (Graphics& g) override
    {
       
    }
    
    class Listener {
    public:
        virtual ~Listener() {}
        virtual void reverbSendLevelChanged(ReverbSendView *comp, float revlevel) {}
    };
    
    void addListener(Listener * listener) { listeners.add(listener); }
    void removeListener(Listener * listener) { listeners.remove(listener); }
    
    
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
        
        sendBox.items.clear();
        sendBox.flexDirection = FlexBox::Direction::row;
        //timeBox.items.add(FlexItem(minKnobWidth, knoblabelheight, timeLabel).withMargin(0).withFlex(0));
        sendBox.items.add(FlexItem(12, 4).withMargin(0));
        sendBox.items.add(FlexItem(minKnobWidth, minitemheight, sendSlider).withMargin(0).withFlex(1));

        infoBox.items.clear();
        infoBox.flexDirection = FlexBox::Direction::row;
        //timeBox.items.add(FlexItem(minKnobWidth, knoblabelheight, timeLabel).withMargin(0).withFlex(0));
        infoBox.items.add(FlexItem(12, 4).withMargin(0));
        infoBox.items.add(FlexItem(minKnobWidth, minitemheight, infoLabel).withMargin(0).withFlex(1));


        checkBox.items.clear();
        checkBox.flexDirection = FlexBox::Direction::row;
        //checkBox.items.add(FlexItem(5, 5).withMargin(0).withFlex(0));
        //checkBox.items.add(FlexItem(enablewidth, minitemheight, enableButton).withMargin(0).withFlex(0));
        checkBox.items.add(FlexItem(7, 5).withMargin(0).withFlex(0));
        checkBox.items.add(FlexItem(100, minitemheight, titleLabel).withMargin(0).withFlex(1));
        //checkBox.items.add(FlexItem(2, 5).withMargin(0).withFlex(0.1));
        //checkBox.items.add(FlexItem(24, minitemheight, dragButton).withMargin(0).withFlex(0));
        checkBox.items.add(FlexItem(2, 5).withMargin(0).withFlex(0.1));


        headerComponent.headerBox.items.clear();
        headerComponent.headerBox.flexDirection = FlexBox::Direction::column;
        headerComponent.headerBox.items.add(FlexItem(150, minitemheight, checkBox).withMargin(0).withFlex(1));

        int ipw = 0;
        for (auto & item : sendBox.items) {
            ipw += item.minWidth + item.margin.left + item.margin.right;
        }

        mainBox.items.clear();
        mainBox.flexDirection = FlexBox::Direction::column;
        //mainBox.items.add(FlexItem(ipw, headerheight, headerComponent).withMargin(0).withFlex(0));
        mainBox.items.add(FlexItem(6, 5).withMargin(0).withFlex(0));
        mainBox.items.add(FlexItem(100, minitemheight, sendBox).withMargin(0).withFlex(1));
        mainBox.items.add(FlexItem(6, 4).withMargin(0).withFlex(0));
        mainBox.items.add(FlexItem(100, minitemheight, infoBox).withMargin(0).withFlex(1));
        mainBox.items.add(FlexItem(6, 4).withMargin(0).withFlex(0));

        int iph = 0;
        for (auto & item : mainBox.items) {
            iph += item.minHeight + item.margin.top + item.margin.bottom;
        }



        minBounds.setSize(jmax(180, ipw + 10), iph + 4);
        minHeaderBounds.setSize(jmax(180, ipw),  minitemheight + 8);

    }

   
    void resized() override
    {
        mainBox.performLayout(getLocalBounds().reduced(2));

        //sendLabel.setBounds(sendSlider.getBounds().removeFromLeft(sendSlider.getWidth()*0.75));

        sendSlider.setMouseDragSensitivity(jmax(128, sendSlider.getWidth()));
    }

    void buttonClicked (Button* buttonThatWasClicked) override
    {
        /*
        if (buttonThatWasClicked == &enableButton) {
            mParams.enabled = enableButton.getToggleState();
            headerComponent.repaint();
        }
         */
        //listeners.call (&ReverbSendView::Listener::reverbSendLevelChanged, this, mParams);
    }

    void effectsHeaderClicked(EffectsBaseView *comp, const MouseEvent & event) override
    {
        //mParams.enabled = !enableButton.getToggleState();
        //processor.setMonitoringDelayActive(!enableButton.getToggleState());
        //updateParams(mParams);

        //listeners.call (&ReverbSendView::Listener::reverbSendLevelChanged, this, mParams);
    }

    
    void sliderValueChanged (Slider* slider) override
    {
        if (slider == &sendSlider) {
            //processor.setMonitoringDelayTimeMs(slider->getValue());
            //auto deltimems = processor.getMonitoringDelayTimeMs();

            reverbSendLevel = slider->getValue();

            //if (deltimems != slider->getValue()) {
            //    slider->setValue(deltimems, dontSendNotification);
            //}

            listeners.call (&ReverbSendView::Listener::reverbSendLevelChanged, this, reverbSendLevel);
        }
    }

    void choiceButtonSelected(SonoChoiceButton *comp, int index, int ident) override
    {

    }


    void updateParams(float revSendLevel) {
        reverbSendLevel = revSendLevel;

        sendSlider.setValue(reverbSendLevel, dontSendNotification);

        sendSlider.setSliderSnapsToMousePosition(processor.getSlidersSnapToMousePosition());

        //auto active = mParams.enabled;
        //enableButton.setAlpha(active ? 1.0 : 0.5);
        //enableButton.setToggleState(active, dontSendNotification);
        //headerComponent.repaint();
    }

    
private:
    
    SonoBigTextLookAndFeel sonoSliderLNF;

    ListenerList<Listener> listeners;

    SonobusAudioProcessor & processor;

    Slider       sendSlider;

    Label sendLabel;
    Label infoLabel;

    
    FlexBox mainBox;
    FlexBox sendBox;
    FlexBox infoBox;
    FlexBox checkBox;

    float reverbSendLevel = 0.0f;


    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ReverbSendView)
};
