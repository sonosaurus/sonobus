// SPDX-License-Identifier: GPLv3-or-later WITH Appstore-exception
// Copyright (C) 2020 Jesse Chappell


#pragma once

#include <JuceHeader.h>

#include "SonoLookAndFeel.h"
#include "SonoDrawableButton.h"
#include "EffectsBaseView.h"
#include "EffectParams.h"

//==============================================================================
/*
*/
class ExpanderView    : public EffectsBaseView, public Slider::Listener, public Button::Listener
{
public:
    ExpanderView() 
    {
        // In your constructor, you should add any child components, and
        // initialise any special settings that your component needs.

        thresholdSlider.setName("thresh");
        thresholdSlider.setRange(-96.0f, 0.0f, 1);
        thresholdSlider.setSkewFactor(1.5);
        thresholdSlider.setTextValueSuffix(" dB");
        thresholdSlider.setDoubleClickReturnValue(true, -60.0);
        configKnobSlider(thresholdSlider);
        thresholdSlider.addListener(this);

        thresholdLabel.setText(TRANS("Noise Floor"), dontSendNotification);
        configLabel(thresholdLabel);

           
        ratioSlider.setName("ratio");
        ratioSlider.setRange(1.0f, 20.0f, 0.1);
        ratioSlider.setSkewFactor(0.5);
        ratioSlider.setTextValueSuffix(" : 1");
        ratioSlider.setDoubleClickReturnValue(true, 2.0);
        configKnobSlider(ratioSlider);
        ratioSlider.addListener(this);
        
        ratioLabel.setText(TRANS("Ratio"), dontSendNotification);
        configLabel(ratioLabel);

           
        attackSlider.setName("attack");
        attackSlider.setRange(1.0f, 1000.0f, 1);
        attackSlider.setSkewFactor(0.5);
        attackSlider.setTextValueSuffix(" ms");
        attackSlider.setDoubleClickReturnValue(true, 1.0);
        configKnobSlider(attackSlider);
        attackSlider.addListener(this);
        
        attackLabel.setText(TRANS("Attack"), dontSendNotification);
        configLabel(attackLabel);

           
        releaseSlider.setName("release");
        releaseSlider.setRange(1.0f, 1000.0f, 1);
        releaseSlider.setSkewFactor(0.5);
        releaseSlider.setTextValueSuffix(" ms");
        releaseSlider.setDoubleClickReturnValue(true, 200.0);
        configKnobSlider(releaseSlider);
        releaseSlider.addListener(this);
        
        releaseLabel.setText(TRANS("Release"), dontSendNotification);
        configLabel(releaseLabel);

           
        // these are in the header component
        enableButton.addListener(this);        
        titleLabel.setText(TRANS("Noise Gate"), dontSendNotification);

           

        addAndMakeVisible(thresholdSlider);
        addAndMakeVisible(thresholdLabel);
        addAndMakeVisible(ratioSlider);
        addAndMakeVisible(ratioLabel);
        addAndMakeVisible(attackSlider);
        addAndMakeVisible(attackLabel);
        addAndMakeVisible(releaseSlider);
        addAndMakeVisible(releaseLabel);
        
        setupLayout();
        
        updateParams(mParams);
    }

    ~ExpanderView()
    {
    }

   
    
    class Listener {
    public:
        virtual ~Listener() {}
        virtual void expanderParamsChanged(ExpanderView *comp, SonoAudio::CompressorParams &params) {}
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

#if JUCE_IOS || JUCE_ANDROID
        // make the button heights a bit more for touchscreen purposes
        minitemheight = 40;
        knobitemheight = 80;
#endif
        
        threshBox.items.clear();
        threshBox.flexDirection = FlexBox::Direction::column;
        threshBox.items.add(FlexItem(minKnobWidth, knoblabelheight, thresholdLabel).withMargin(0).withFlex(0));
        threshBox.items.add(FlexItem(minKnobWidth, knobitemheight, thresholdSlider).withMargin(0).withFlex(1));
        
        ratioBox.items.clear();
        ratioBox.flexDirection = FlexBox::Direction::column;
        ratioBox.items.add(FlexItem(minKnobWidth, knoblabelheight, ratioLabel).withMargin(0).withFlex(0));
        ratioBox.items.add(FlexItem(minKnobWidth, knobitemheight, ratioSlider).withMargin(0).withFlex(1));
        
        attackBox.items.clear();
        attackBox.flexDirection = FlexBox::Direction::column;
        attackBox.items.add(FlexItem(minKnobWidth, knoblabelheight, attackLabel).withMargin(0).withFlex(0));
        attackBox.items.add(FlexItem(minKnobWidth, knobitemheight, attackSlider).withMargin(0).withFlex(1));
        
        releaseBox.items.clear();
        releaseBox.flexDirection = FlexBox::Direction::column;
        releaseBox.items.add(FlexItem(minKnobWidth, knoblabelheight, releaseLabel).withMargin(0).withFlex(0));
        releaseBox.items.add(FlexItem(minKnobWidth, knobitemheight, releaseSlider).withMargin(0).withFlex(1));

        
        
        checkBox.items.clear();
        checkBox.flexDirection = FlexBox::Direction::row;
        //checkBox.items.add(FlexItem(5, 5).withMargin(0).withFlex(0));
        checkBox.items.add(FlexItem(enablewidth, minitemheight, enableButton).withMargin(0).withFlex(0));
        checkBox.items.add(FlexItem(2, 5).withMargin(0).withFlex(0));
        checkBox.items.add(FlexItem(100, minitemheight, titleLabel).withMargin(0).withFlex(1).withMaxWidth(120));
        //checkBox.items.add(FlexItem(2, 5).withMargin(0).withFlex(0.1));
        checkBox.items.add(FlexItem(24, minitemheight, dragButton).withMargin(0).withFlex(0));
        checkBox.items.add(FlexItem(2, 5).withMargin(0).withFlex(0.1));

        headerComponent.headerBox.items.clear();
        headerComponent.headerBox.flexDirection = FlexBox::Direction::column;
        headerComponent.headerBox.items.add(FlexItem(150, minitemheight, checkBox).withMargin(0).withFlex(1));

        
        knobBox.items.clear();
        knobBox.flexDirection = FlexBox::Direction::row;
        knobBox.items.add(FlexItem(6, 5).withMargin(0).withFlex(0));
        knobBox.items.add(FlexItem(minKnobWidth, knobitemheight + knoblabelheight, threshBox).withMargin(0).withFlex(1));
        knobBox.items.add(FlexItem(minKnobWidth, knobitemheight + knoblabelheight, ratioBox).withMargin(0).withFlex(1));
        knobBox.items.add(FlexItem(minKnobWidth, knobitemheight + knoblabelheight, attackBox).withMargin(0).withFlex(1));
        knobBox.items.add(FlexItem(minKnobWidth, knobitemheight + knoblabelheight, releaseBox).withMargin(0).withFlex(1));
        knobBox.items.add(FlexItem(6, 5).withMargin(0).withFlex(0));
        
        mainBox.items.clear();
        mainBox.flexDirection = FlexBox::Direction::column;
        //mainBox.items.add(FlexItem(150, minitemheight, checkBox).withMargin(0).withFlex(0));
        //mainBox.items.add(FlexItem(6, 2).withMargin(0).withFlex(0));
        mainBox.items.add(FlexItem(100, knoblabelheight + knobitemheight, knobBox).withMargin(0).withFlex(1));
        mainBox.items.add(FlexItem(6, 2).withMargin(0).withFlex(0));
        
        minBounds.setSize(jmax(180, minKnobWidth * 4 + 16), knobitemheight + knoblabelheight + 2);
        minHeaderBounds.setSize(jmax(180, minKnobWidth * 5 + 16),  minitemheight + 8);

    }

   
    void resized() override
    {
        mainBox.performLayout(getLocalBounds());
    }

    void buttonClicked (Button* buttonThatWasClicked) override
    {
        if (buttonThatWasClicked == &enableButton) {
            mParams.enabled = enableButton.getToggleState();
            headerComponent.repaint();
        }
        listeners.call (&ExpanderView::Listener::expanderParamsChanged, this, mParams);
        
    }
    
    void sliderValueChanged (Slider* slider) override
    {
        if (slider == &thresholdSlider) {
            mParams.thresholdDb = slider->getValue();
        }
        else if (slider == &ratioSlider) {
            mParams.ratio = slider->getValue();
        }
        else if (slider == &attackSlider) {
            mParams.attackMs = slider->getValue();
        }
        else if (slider == &releaseSlider) {
            mParams.releaseMs = slider->getValue();
        }
        listeners.call (&ExpanderView::Listener::expanderParamsChanged, this, mParams);
        
    }

    void updateParams(const SonoAudio::CompressorParams & params) {
        mParams = params;
        
        thresholdSlider.setValue(mParams.thresholdDb, dontSendNotification);
        ratioSlider.setValue(mParams.ratio, dontSendNotification);
        attackSlider.setValue(mParams.attackMs, dontSendNotification);
        releaseSlider.setValue(mParams.releaseMs, dontSendNotification);
        
        enableButton.setAlpha(mParams.enabled ? 1.0 : 0.5);
        enableButton.setToggleState(mParams.enabled, dontSendNotification);
        headerComponent.repaint();
    }
    
    const SonoAudio::CompressorParams & getParams() const {
        return mParams;         
    }
    
   
    
private:
    

    ListenerList<Listener> listeners;
    
    Slider thresholdSlider;
    Slider ratioSlider;
    Slider attackSlider;
    Slider releaseSlider;

    Label thresholdLabel;
    Label ratioLabel;
    Label attackLabel;
    Label releaseLabel;

    
    FlexBox mainBox;
    FlexBox checkBox;
    FlexBox knobBox;
    FlexBox threshBox;
    FlexBox ratioBox;
    FlexBox attackBox;
    FlexBox releaseBox;


    SonoAudio::CompressorParams mParams;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ExpanderView)
};
