/*
  ==============================================================================

    CompressorView.h
    Created: 29 Aug 2020 11:21:37pm
    Author:  Jesse Chappell

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

#include "SonobusPluginProcessor.h"
#include "SonoLookAndFeel.h"

//==============================================================================
/*
*/
class CompressorView    : public Component, public Slider::Listener, public Button::Listener
{
public:
    CompressorView() : sonoSliderLNF(14), smallLNF(12)
    {
        // In your constructor, you should add any child components, and
        // initialise any special settings that your component needs.

        thresholdSlider.setName("thresh");
        thresholdSlider.setRange(-60.0f, 0.0f, 1);
        thresholdSlider.setSkewFactor(1.5);
        thresholdSlider.setTextValueSuffix(" dB");
        thresholdSlider.setDoubleClickReturnValue(true, -20.0);
        configKnobSlider(thresholdSlider);
        thresholdSlider.addListener(this);

        thresholdLabel.setText(TRANS("Threshold"), dontSendNotification);
        configLabel(thresholdLabel);

           
        ratioSlider.setName("ratio");
        ratioSlider.setRange(2.0f, 20.0f, 1);
        ratioSlider.setSkewFactor(0.5);
        ratioSlider.setTextValueSuffix(" : 1");
        ratioSlider.setDoubleClickReturnValue(true, 4.0);
        configKnobSlider(ratioSlider);
        ratioSlider.addListener(this);
        
        ratioLabel.setText(TRANS("Ratio"), dontSendNotification);
        configLabel(ratioLabel);

           
        attackSlider.setName("attack");
        attackSlider.setRange(1.0f, 1000.0f, 1);
        attackSlider.setSkewFactor(0.5);
        attackSlider.setTextValueSuffix(" ms");
        attackSlider.setDoubleClickReturnValue(true, 10.0);
        configKnobSlider(attackSlider);
        attackSlider.addListener(this);
        
        attackLabel.setText(TRANS("Attack"), dontSendNotification);
        configLabel(attackLabel);

           
        releaseSlider.setName("release");
        releaseSlider.setRange(1.0f, 1000.0f, 1);
        releaseSlider.setSkewFactor(0.5);
        releaseSlider.setTextValueSuffix(" ms");
        releaseSlider.setDoubleClickReturnValue(true, 500.0);
        configKnobSlider(releaseSlider);
        releaseSlider.addListener(this);
        
        releaseLabel.setText(TRANS("Release"), dontSendNotification);
        configLabel(releaseLabel);

           
        makeupGainSlider.setName("makeup");
        makeupGainSlider.setRange(0.0f, 60.0f, 1);
        makeupGainSlider.setSkewFactor(0.75);
        makeupGainSlider.setTextValueSuffix(" dB");
        makeupGainSlider.setDoubleClickReturnValue(true, 0.0);
        configKnobSlider(makeupGainSlider);
        makeupGainSlider.addListener(this);

        makeupGainLabel.setText(TRANS("Makeup Gain"), dontSendNotification);
        configLabel(makeupGainLabel);
        
        enableButton.setButtonText(TRANS("Compressor"));
        enableButton.addListener(this);
        autoMakeupButton.setButtonText(TRANS("Auto"));
        autoMakeupButton.addListener(this);
        autoMakeupButton.setLookAndFeel(&smallLNF);
           

        addAndMakeVisible(thresholdSlider);
        addAndMakeVisible(thresholdLabel);
        addAndMakeVisible(ratioSlider);
        addAndMakeVisible(ratioLabel);
        addAndMakeVisible(attackSlider);
        addAndMakeVisible(attackLabel);
        addAndMakeVisible(releaseSlider);
        addAndMakeVisible(releaseLabel);
        addAndMakeVisible(makeupGainSlider);
        addAndMakeVisible(makeupGainLabel);
        addAndMakeVisible(enableButton);
        addAndMakeVisible(autoMakeupButton);

        setupLayout();
        
        updateParams(mParams);
    }

    ~CompressorView()
    {
    }

    void paint (Graphics& g) override
    {
       
    }
    
    class Listener {
    public:
        virtual ~Listener() {}
        virtual void compressorParamsChanged(CompressorView *comp, SonobusAudioProcessor::CompressorParams &params) {}
    };
    
    void addListener(Listener * listener) { listeners.add(listener); }
    void removeListener(Listener * listener) { listeners.remove(listener); }
    
    
    void setupLayout()
    {
        int minKnobWidth = 54;
        int minitemheight = 36;
        int knoblabelheight = 18;
        int knobitemheight = 62;

#if JUCE_IOS
        // make the button heights a bit more for touchscreen purposes
        minitemheight = 44;
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

        makeupBox.items.clear();
        makeupBox.flexDirection = FlexBox::Direction::column;
        makeupBox.items.add(FlexItem(minKnobWidth, knoblabelheight, makeupGainLabel).withMargin(0).withFlex(0));
        makeupBox.items.add(FlexItem(minKnobWidth, knobitemheight, makeupGainSlider).withMargin(0).withFlex(1));
        
        
        checkBox.items.clear();
        checkBox.flexDirection = FlexBox::Direction::row;
        checkBox.items.add(FlexItem(5, 5).withMargin(0).withFlex(0));
        checkBox.items.add(FlexItem(100, minitemheight, enableButton).withMargin(0).withFlex(1));
        //knobBox.items.add(FlexItem(6, 5).withMargin(0).withFlex(0.1));
        checkBox.items.add(FlexItem(66, minitemheight, autoMakeupButton).withMargin(0).withFlex(0));
        
        
        
        knobBox.items.clear();
        knobBox.flexDirection = FlexBox::Direction::row;
        knobBox.items.add(FlexItem(6, 5).withMargin(0).withFlex(0));
        knobBox.items.add(FlexItem(minKnobWidth, knobitemheight + knoblabelheight, threshBox).withMargin(0).withFlex(1));
        knobBox.items.add(FlexItem(minKnobWidth, knobitemheight + knoblabelheight, ratioBox).withMargin(0).withFlex(1));
        knobBox.items.add(FlexItem(minKnobWidth, knobitemheight + knoblabelheight, attackBox).withMargin(0).withFlex(1));
        knobBox.items.add(FlexItem(minKnobWidth, knobitemheight + knoblabelheight, releaseBox).withMargin(0).withFlex(1));
        knobBox.items.add(FlexItem(minKnobWidth, knobitemheight + knoblabelheight, makeupBox).withMargin(0).withFlex(1));
        knobBox.items.add(FlexItem(6, 5).withMargin(0).withFlex(0));
        
        mainBox.items.clear();
        mainBox.flexDirection = FlexBox::Direction::column;
        mainBox.items.add(FlexItem(150, minitemheight, checkBox).withMargin(0).withFlex(0));
        mainBox.items.add(FlexItem(6, 3).withMargin(0).withFlex(0));
        mainBox.items.add(FlexItem(100, knoblabelheight + knobitemheight, knobBox).withMargin(0).withFlex(1));
        
        minBounds.setSize(jmax(180, minKnobWidth * 5 + 16), minitemheight + knobitemheight + knoblabelheight);
    }

    Rectangle<int> getMinimumContentBounds() const {
        return minBounds;
    }

    void resized() override
    {
        mainBox.performLayout(getLocalBounds());
    }

    void buttonClicked (Button* buttonThatWasClicked) override
    {
        if (buttonThatWasClicked == &enableButton) {
            mParams.enabled = enableButton.getToggleState();
        }
        else if (buttonThatWasClicked == &autoMakeupButton) {
            mParams.automakeupGain = autoMakeupButton.getToggleState();
        }
        listeners.call (&CompressorView::Listener::compressorParamsChanged, this, mParams);
        
        // could have been changed
        if (mParams.automakeupGain) {
            makeupGainSlider.setValue(mParams.makeupGainDb, dontSendNotification);
        }
        makeupGainSlider.setEnabled(!mParams.automakeupGain);

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
        else if (slider == &makeupGainSlider) {
            mParams.makeupGainDb = slider->getValue();
        }
        listeners.call (&CompressorView::Listener::compressorParamsChanged, this, mParams);
        
        // could have been changed
        if (mParams.automakeupGain) {
            makeupGainSlider.setValue(mParams.makeupGainDb, dontSendNotification);
        }
    }

    void updateParams(const SonobusAudioProcessor::CompressorParams & params) {
        mParams = params;
        
        thresholdSlider.setValue(mParams.thresholdDb, dontSendNotification);
        ratioSlider.setValue(mParams.ratio, dontSendNotification);
        attackSlider.setValue(mParams.attackMs, dontSendNotification);
        releaseSlider.setValue(mParams.releaseMs, dontSendNotification);
        makeupGainSlider.setValue(mParams.makeupGainDb, dontSendNotification);
        
        autoMakeupButton.setToggleState(mParams.automakeupGain, dontSendNotification);
        enableButton.setToggleState(mParams.enabled, dontSendNotification);
        makeupGainSlider.setEnabled(!mParams.automakeupGain);
    }
    
    const SonobusAudioProcessor::CompressorParams & getParams() const { 
        return mParams;         
    }
    
   
    
private:
    
    SonoBigTextLookAndFeel sonoSliderLNF;
    SonoBigTextLookAndFeel smallLNF;

    ListenerList<Listener> listeners;
    Rectangle<int> minBounds;
    
    void configKnobSlider(Slider & slider) 
    {
        slider.setSliderStyle(Slider::SliderStyle::RotaryHorizontalVerticalDrag);
        slider.setTextBoxStyle(Slider::TextBoxAbove, true, 60, 14);
        slider.setMouseDragSensitivity(128);
        slider.setScrollWheelEnabled(false);
        slider.setTextBoxIsEditable(true);
        slider.setSliderSnapsToMousePosition(false);
        //slider->setPopupDisplayEnabled(true, false, this);
        slider.setColour(Slider::textBoxBackgroundColourId, Colours::transparentBlack);
        slider.setColour(Slider::textBoxOutlineColourId, Colours::transparentBlack);
        slider.setColour(Slider::textBoxTextColourId, Colour(0x90eeeeee));
        slider.setColour(TooltipWindow::textColourId, Colour(0xf0eeeeee));
        slider.setLookAndFeel(&sonoSliderLNF);
    }
    
    void configLabel(Label & label) 
    {
        label.setFont(12);
        label.setColour(Label::textColourId, Colour(0xa0eeeeee));
        label.setJustificationType(Justification::centred);
        label.setMinimumHorizontalScale(0.3);
    }
    
    ToggleButton enableButton;
    ToggleButton autoMakeupButton;
    
    Slider thresholdSlider;
    Slider ratioSlider;
    Slider attackSlider;
    Slider releaseSlider;
    Slider makeupGainSlider;

    Label thresholdLabel;
    Label ratioLabel;
    Label attackLabel;
    Label releaseLabel;
    Label makeupGainLabel;

    
    FlexBox mainBox;
    FlexBox checkBox;
    FlexBox knobBox;
    FlexBox threshBox;
    FlexBox ratioBox;
    FlexBox attackBox;
    FlexBox releaseBox;
    FlexBox makeupBox;


    SonobusAudioProcessor::CompressorParams mParams;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CompressorView)
};
