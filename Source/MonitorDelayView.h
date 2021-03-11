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
class MonitorDelayView    : public EffectsBaseView,
   public Slider::Listener,
   public Button::Listener,
   public SonoChoiceButton::Listener,
   public EffectsBaseView::HeaderListener
{
public:
    MonitorDelayView(SonobusAudioProcessor & processor_)  : processor(processor_)
    {
        timeSlider.setName("time");
        timeSlider.setRange(0.0f, 500.0f, 0.1);
        timeSlider.setSkewFactor(0.4);
        timeSlider.setTextValueSuffix(" ms");
        //timeSlider.setDoubleClickReturnValue(true, -60.0);
        configBarSlider(timeSlider);
        timeSlider.addListener(this);
        timeSlider.getProperties().set ("noFill", true);

        timeLabel.setText(TRANS("Delay Time"), dontSendNotification);
        configLabel(timeLabel);
        timeLabel.setJustificationType(Justification::centredLeft);

        autoModeChoice.addChoiceListener(this);
        autoModeChoice.addItem(TRANS("One-way"), 0);
        autoModeChoice.addItem(TRANS("Round-trip"), 1);
        autoModeChoice.setSelectedItemIndex(0);

        autoButton.setButtonText(TRANS("Set From Peers"));
        autoButton.addListener(this);
        autoButton.setTooltip(TRANS("Pressing this will calculate an average latency for all connected peers and set the monitoring delay time accordingly, based on one-way or round-trip choice selection"));

        onlyPlaybackToggle.setButtonText(TRANS("Delay File/Met Playback Only"));
        onlyPlaybackToggle.addListener(this);

        // these are in the header component
        enableButton.addListener(this);        
        titleLabel.setText(TRANS("Additional Monitoring Delay"), dontSendNotification);

        dragButton.setVisible(false);

        addAndMakeVisible(timeSlider);
        addAndMakeVisible(timeLabel);
        addAndMakeVisible(autoModeChoice);
        addAndMakeVisible(autoButton);
        addAndMakeVisible(onlyPlaybackToggle);

        addAndMakeVisible(headerComponent);

        addHeaderListener(this);

        setupLayout();
        
        updateParams();
    }

    ~MonitorDelayView()
    {
    }

   
    
    void paint (Graphics& g) override
    {
       
    }
    
    class Listener {
    public:
        virtual ~Listener() {}
        //virtual void expanderParamsChanged(ExpanderView *comp, SonoAudio::CompressorParams &params) {}
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
        
        timeBox.items.clear();
        timeBox.flexDirection = FlexBox::Direction::row;
        //timeBox.items.add(FlexItem(minKnobWidth, knoblabelheight, timeLabel).withMargin(0).withFlex(0));
        timeBox.items.add(FlexItem(minKnobWidth, minitemheight, timeSlider).withMargin(0).withFlex(1));

        autoBox.items.clear();
        autoBox.flexDirection = FlexBox::Direction::row;
        autoBox.items.add(FlexItem(autobuttwidth, minitemheight, autoButton).withMargin(0).withFlex(1));
        autoBox.items.add(FlexItem(8, 4).withMargin(0));
        autoBox.items.add(FlexItem(buttwidth, minitemheight, autoModeChoice).withMargin(0).withFlex(0.5));

        optionBox.items.clear();
        optionBox.flexDirection = FlexBox::Direction::row;
        optionBox.items.add(FlexItem(16, 4).withMargin(0));
        optionBox.items.add(FlexItem(100, minitemheight, onlyPlaybackToggle).withMargin(0).withFlex(1));


        
        checkBox.items.clear();
        checkBox.flexDirection = FlexBox::Direction::row;
        //checkBox.items.add(FlexItem(5, 5).withMargin(0).withFlex(0));
        checkBox.items.add(FlexItem(enablewidth, minitemheight, enableButton).withMargin(0).withFlex(0));
        checkBox.items.add(FlexItem(2, 5).withMargin(0).withFlex(0));
        checkBox.items.add(FlexItem(100, minitemheight, titleLabel).withMargin(0).withFlex(1));
        //checkBox.items.add(FlexItem(2, 5).withMargin(0).withFlex(0.1));
        //checkBox.items.add(FlexItem(24, minitemheight, dragButton).withMargin(0).withFlex(0));
        checkBox.items.add(FlexItem(2, 5).withMargin(0).withFlex(0.1));

        headerComponent.headerBox.items.clear();
        headerComponent.headerBox.flexDirection = FlexBox::Direction::column;
        headerComponent.headerBox.items.add(FlexItem(150, minitemheight, checkBox).withMargin(0).withFlex(1));

        int ipw = 0;
        for (auto & item : autoBox.items) {
            ipw += item.minWidth;
        }

        mainBox.items.clear();
        mainBox.flexDirection = FlexBox::Direction::column;
        mainBox.items.add(FlexItem(ipw, headerheight, headerComponent).withMargin(0).withFlex(0));
        mainBox.items.add(FlexItem(6, 5).withMargin(0).withFlex(0));
        mainBox.items.add(FlexItem(100, minitemheight, timeBox).withMargin(0).withFlex(1));
        mainBox.items.add(FlexItem(6, 4).withMargin(0).withFlex(0));
        mainBox.items.add(FlexItem(100, minitemheight, autoBox).withMargin(0).withFlex(1));
        mainBox.items.add(FlexItem(6, 4).withMargin(0).withFlex(0));
        mainBox.items.add(FlexItem(100, minitemheight, optionBox).withMargin(0).withFlex(1));
        mainBox.items.add(FlexItem(6, 4).withMargin(0).withFlex(0));

        int iph = 0;
        for (auto & item : mainBox.items) {
            iph += item.minHeight;
        }



        minBounds.setSize(jmax(180, ipw + 10), iph + 10);
        minHeaderBounds.setSize(jmax(180, ipw),  minitemheight + 8);

    }

   
    void resized() override
    {
        mainBox.performLayout(getLocalBounds().reduced(2));

        timeLabel.setBounds(timeSlider.getBounds().removeFromLeft(timeSlider.getWidth()*0.75));

    }

    void buttonClicked (Button* buttonThatWasClicked) override
    {
        if (buttonThatWasClicked == &enableButton) {
            processor.setMonitoringDelayActive(enableButton.getToggleState());
            headerComponent.repaint();
        }
        else if (buttonThatWasClicked == &autoButton) {
            auto autoScalar = autoModeChoice.getSelectedItemIndex() == 1 ? 2.0f : 1.0f;

            processor.setMonitoringDelayTimeFromAvgPeerLatency(autoScalar);
            updateParams();
        }
        else if (buttonThatWasClicked == &onlyPlaybackToggle) {
            processor.setMonitoringDelayPlaybackOnly(onlyPlaybackToggle.getToggleState());
        }
    }

    void effectsHeaderClicked(EffectsBaseView *comp, const MouseEvent & event) override
    {
        processor.setMonitoringDelayActive(!enableButton.getToggleState());
        updateParams();
    }

    
    void sliderValueChanged (Slider* slider) override
    {
        if (slider == &timeSlider) {
            processor.setMonitoringDelayTimeMs(slider->getValue());
            auto deltimems = processor.getMonitoringDelayTimeMs();
            if (deltimems != slider->getValue()) {
                slider->setValue(deltimems, dontSendNotification);
            }
        }
    }

    void choiceButtonSelected(SonoChoiceButton *comp, int index, int ident) override
    {
        if (comp == &autoModeChoice) {
        }

    }


    void updateParams() {

        auto deltimems = processor.getMonitoringDelayTimeMs();
        timeSlider.setValue(deltimems, dontSendNotification);

        onlyPlaybackToggle.setToggleState(processor.getMonitoringDelayPlaybackOnly(), dontSendNotification);

        auto active = processor.getMonitoringDelayActive();
        enableButton.setAlpha(active ? 1.0 : 0.5);
        enableButton.setToggleState(active, dontSendNotification);
        headerComponent.repaint();
    }

    
private:
    

    ListenerList<Listener> listeners;

    SonobusAudioProcessor & processor;

    Slider       timeSlider;
    TextButton   autoButton;
    ToggleButton onlyPlaybackToggle;
    SonoChoiceButton autoModeChoice;

    Label timeLabel;
    Label infoLabel;

    
    FlexBox mainBox;
    FlexBox checkBox;
    FlexBox timeBox;
    FlexBox autoBox;
    FlexBox optionBox;
    FlexBox infoBox;


    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MonitorDelayView)
};
