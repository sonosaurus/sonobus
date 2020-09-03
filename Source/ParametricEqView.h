/*
  ==============================================================================

    CompressorView.h
    Created: 29 Aug 2020 11:21:37pm
    Author:  Jesse Chappell

  ==============================================================================
*/

#pragma once

#include "JuceHeader.h"

#include "SonobusPluginProcessor.h"
#include "SonoLookAndFeel.h"
#include "SonoDrawableButton.h"
#include "EffectsBaseView.h"

//==============================================================================
/*
*/
class ParametricEqView    : public EffectsBaseView, public Slider::Listener, public Button::Listener
{
public:
    ParametricEqView() 
    {
        // In your constructor, you should add any child components, and
        // initialise any special settings that your component needs.

        highShelfGainSlider.setName("hsgain");
        highShelfGainSlider.setRange(-24.0f, 24.0f, 0.1);
        highShelfGainSlider.setSkewFactor(1.0);
        highShelfGainSlider.setTextValueSuffix(" dB");
        highShelfGainSlider.setDoubleClickReturnValue(true, 0.0);
        configKnobSlider(highShelfGainSlider);
        //configBarSlider(highShelfGainSlider);
        highShelfGainSlider.addListener(this);
        highShelfGainSlider.getProperties().set ("fromCentre", true);

        highShelfGainLabel.setText(TRANS("Gain"), dontSendNotification);
        configLabel(highShelfGainLabel);

           
        highShelfFreqSlider.setName("hsfreq");
        highShelfFreqSlider.setRange(20.0f, 16000.0f, 1);
        highShelfFreqSlider.setSkewFactor(0.5);
        highShelfFreqSlider.setTextValueSuffix(" Hz");
        highShelfFreqSlider.setDoubleClickReturnValue(true, 10000.0);
        configKnobSlider(highShelfFreqSlider);
        //configBarSlider(highShelfFreqSlider);
        highShelfFreqSlider.addListener(this);

        highShelfFreqLabel.setText(TRANS("High Shelf"), dontSendNotification);
        configLabel(highShelfFreqLabel, true);

        
        lowShelfGainSlider.setName("lsgain");
        lowShelfGainSlider.setRange(-24.0f, 24.0f, 0.1);
        lowShelfGainSlider.setSkewFactor(1.0);
        lowShelfGainSlider.setTextValueSuffix(" dB");
        lowShelfGainSlider.setDoubleClickReturnValue(true, 0.0);
        configKnobSlider(lowShelfGainSlider);
        //configBarSlider(lowShelfGainSlider);
        lowShelfGainSlider.addListener(this);
        lowShelfGainSlider.getProperties().set ("fromCentre", true);

        lowShelfGainLabel.setText(TRANS("Gain"), dontSendNotification);
        configLabel(lowShelfGainLabel);

           
        lowShelfFreqSlider.setName("lsfreq");
        lowShelfFreqSlider.setRange(10.0f, 5000.0f, 1);
        lowShelfFreqSlider.setSkewFactor(0.5);
        lowShelfFreqSlider.setTextValueSuffix(" Hz");
        lowShelfFreqSlider.setDoubleClickReturnValue(true, 60.0);
        configKnobSlider(lowShelfFreqSlider);
        //configBarSlider(lowShelfFreqSlider);        
        lowShelfFreqSlider.addListener(this);

        lowShelfFreqLabel.setText(TRANS("Low Shelf"), dontSendNotification);
        configLabel(lowShelfFreqLabel, true);

        
        para1GainSlider.setName("para1gain");
        para1GainSlider.setRange(-24.0f, 24.0f, 0.1);
        para1GainSlider.setSkewFactor(1.0);
        para1GainSlider.setTextValueSuffix(" dB");
        para1GainSlider.setDoubleClickReturnValue(true, 0.0);
        configKnobSlider(para1GainSlider);
        //configBarSlider(para1GainSlider);
        para1GainSlider.addListener(this);
        para1GainSlider.getProperties().set ("fromCentre", true);

        para1GainLabel.setText(TRANS("Gain"), dontSendNotification);
        configLabel(para1GainLabel);

           
        para1FreqSlider.setName("para1freq");
        para1FreqSlider.setRange(20.0f, 10000.0f, 1);
        para1FreqSlider.setSkewFactor(0.5);
        para1FreqSlider.setTextValueSuffix(" Hz");
        para1FreqSlider.setDoubleClickReturnValue(true, 90.0);
        configKnobSlider(para1FreqSlider);
        //configBarSlider(para1FreqSlider);
        para1FreqSlider.addListener(this);

        para1FreqLabel.setText(TRANS("Freq 1"), dontSendNotification);
        configLabel(para1FreqLabel, true);

        
        para1QSlider.setName("para1q");
        para1QSlider.setRange(0.4f, 100.0f, 0.1f);
        para1QSlider.setSkewFactor(0.5);
        para1QSlider.setTextValueSuffix("");
        para1QSlider.setDoubleClickReturnValue(true, 1.5);
        //configBarSlider(para1QSlider);
        configKnobSlider(para1QSlider);
        para1QSlider.addListener(this);

        para1QLabel.setText(TRANS("Q"), dontSendNotification);
        configLabel(para1QLabel);

        para2GainSlider.setName("para1gain");
        para2GainSlider.setRange(-24.0f, 24.0f, 0.1);
        para2GainSlider.setSkewFactor(1.0);
        para2GainSlider.setTextValueSuffix(" dB");
        para2GainSlider.setDoubleClickReturnValue(true, 0.0);
        configKnobSlider(para2GainSlider);
        //configBarSlider(para2GainSlider);
        para2GainSlider.addListener(this);
        para2GainSlider.getProperties().set ("fromCentre", true);

        para2GainLabel.setText(TRANS("Gain"), dontSendNotification);
        configLabel(para2GainLabel);
        
        
        para2FreqSlider.setName("para2freq");
        para2FreqSlider.setRange(20.0f, 10000.0f, 1);
        para2FreqSlider.setSkewFactor(0.5);
        para2FreqSlider.setTextValueSuffix(" Hz");
        para2FreqSlider.setDoubleClickReturnValue(true, 360.0);
        configKnobSlider(para2FreqSlider);
        //configBarSlider(para2FreqSlider);
        para2FreqSlider.addListener(this);
        
        para2FreqLabel.setText(TRANS("Freq 2"), dontSendNotification);
        configLabel(para2FreqLabel, true);
        
        
        para2QSlider.setName("para2q");
        para2QSlider.setRange(0.4f, 100.0f, 0.1);
        para2QSlider.setSkewFactor(0.5);
        para2QSlider.setTextValueSuffix("");
        para2QSlider.setDoubleClickReturnValue(true, 4.0);
        configKnobSlider(para2QSlider);
        //configBarSlider(para2QSlider);
        para2QSlider.addListener(this);
        
        para2QLabel.setText(TRANS("Q"), dontSendNotification);
        configLabel(para2QLabel);
        
        
        
        enableButton.addListener(this);
        
        titleLabel.setText(TRANS("Parametric EQ"), dontSendNotification);

           
        //Colour bgfillcol = Colour::fromFloatRGBA(0.07, 0.07, 0.07, 1.0);
        Colour bgfillcol = Colour::fromFloatRGBA(0.08, 0.08, 0.08, 1.0);
        Colour bgstrokecol = Colour::fromFloatRGBA(0.5, 0.5, 0.5, 0.25);
        
        lowShelfBg.setCornerSize(Point<float>(6,6));
        lowShelfBg.setFill (bgfillcol);
        lowShelfBg.setStrokeFill (bgstrokecol);
        lowShelfBg.setStrokeThickness(0.5);

        highShelfBg.setCornerSize(Point<float>(6,6));
        highShelfBg.setFill (bgfillcol);
        highShelfBg.setStrokeFill (bgstrokecol);
        highShelfBg.setStrokeThickness(0.5);

        para1Bg.setCornerSize(Point<float>(6,6));
        para1Bg.setFill (bgfillcol);
        para1Bg.setStrokeFill (bgstrokecol);
        para1Bg.setStrokeThickness(0.5);

        para2Bg.setCornerSize(Point<float>(6,6));
        para2Bg.setFill (bgfillcol);
        para2Bg.setStrokeFill (bgstrokecol);
        para2Bg.setStrokeThickness(0.5);
        
        
        addAndMakeVisible(lowShelfBg);
        addAndMakeVisible(highShelfBg);
        addAndMakeVisible(para1Bg);
        addAndMakeVisible(para2Bg);

        addAndMakeVisible(highShelfGainSlider);
        addAndMakeVisible(highShelfGainLabel);
        addAndMakeVisible(highShelfFreqSlider);
        addAndMakeVisible(highShelfFreqLabel);
        addAndMakeVisible(lowShelfGainSlider);
        addAndMakeVisible(lowShelfGainLabel);
        addAndMakeVisible(lowShelfFreqSlider);
        addAndMakeVisible(lowShelfFreqLabel);
        addAndMakeVisible(para1GainSlider);
        addAndMakeVisible(para1GainLabel);
        addAndMakeVisible(para1FreqSlider);
        addAndMakeVisible(para1FreqLabel);
        addAndMakeVisible(para1QSlider);
        addAndMakeVisible(para1QLabel);
        addAndMakeVisible(para2GainSlider);
        addAndMakeVisible(para2GainLabel);
        addAndMakeVisible(para2FreqSlider);
        addAndMakeVisible(para2FreqLabel);
        addAndMakeVisible(para2QSlider);
        addAndMakeVisible(para2QLabel);

        
        setupLayout();
        
        updateParams(mParams);
    }

    ~ParametricEqView()
    {
    }


    class Listener {
    public:
        virtual ~Listener() {}
        virtual void parametricEqParamsChanged(ParametricEqView *comp, SonobusAudioProcessor::ParametricEqParams &params) {}
    };
    
    void addListener(Listener * listener) { listeners.add(listener); }
    void removeListener(Listener * listener) { listeners.remove(listener); }
    
    
    void setupLayout()
    {
#if 1

        int minKnobWidth = 54;
        int minitemheight = 32;
        int knoblabelheight = 18;
        int knobitemheight = 62;
        int enablewidth = 44;

#if JUCE_IOS
        // make the button heights a bit more for touchscreen purposes
        minitemheight = 40;
        knobitemheight = 80;
#endif
        
        
        highShelfGainBox.items.clear();
        highShelfGainBox.flexDirection = FlexBox::Direction::column;
        highShelfGainBox.items.add(FlexItem(minKnobWidth, knoblabelheight, highShelfGainLabel).withMargin(0).withFlex(0));
        highShelfGainBox.items.add(FlexItem(minKnobWidth, knobitemheight, highShelfGainSlider).withMargin(0).withFlex(1));

        highShelfFreqBox.items.clear();
        highShelfFreqBox.flexDirection = FlexBox::Direction::column;
        highShelfFreqBox.items.add(FlexItem(minKnobWidth, knoblabelheight, highShelfFreqLabel).withMargin(0).withFlex(0));
        highShelfFreqBox.items.add(FlexItem(minKnobWidth, knobitemheight, highShelfFreqSlider).withMargin(0).withFlex(1));

        highShelfBox.items.clear();
        highShelfBox.flexDirection = FlexBox::Direction::column;
        highShelfBox.items.add(FlexItem(2, 4).withMargin(0).withFlex(0));
        highShelfBox.items.add(FlexItem(minKnobWidth, knobitemheight + knoblabelheight, highShelfFreqBox).withMargin(0).withFlex(1));
        highShelfBox.items.add(FlexItem(2, 6).withMargin(0).withFlex(0));
        highShelfBox.items.add(FlexItem(minKnobWidth, knobitemheight + knoblabelheight, highShelfGainBox).withMargin(0).withFlex(1));
        highShelfBox.items.add(FlexItem(2, 4).withMargin(0).withFlex(0));

        
        lowShelfGainBox.items.clear();
        lowShelfGainBox.flexDirection = FlexBox::Direction::column;
        lowShelfGainBox.items.add(FlexItem(minKnobWidth, knoblabelheight, lowShelfGainLabel).withMargin(0).withFlex(0));
        lowShelfGainBox.items.add(FlexItem(minKnobWidth, knobitemheight, lowShelfGainSlider).withMargin(0).withFlex(1));

        lowShelfFreqBox.items.clear();
        lowShelfFreqBox.flexDirection = FlexBox::Direction::column;
        lowShelfFreqBox.items.add(FlexItem(minKnobWidth, knoblabelheight, lowShelfFreqLabel).withMargin(0).withFlex(0));
        lowShelfFreqBox.items.add(FlexItem(minKnobWidth, knobitemheight, lowShelfFreqSlider).withMargin(0).withFlex(1));

        lowShelfBox.items.clear();
        lowShelfBox.flexDirection = FlexBox::Direction::column;
        lowShelfBox.items.add(FlexItem(2, 4).withMargin(0).withFlex(0));
        lowShelfBox.items.add(FlexItem(minKnobWidth, knobitemheight + knoblabelheight, lowShelfFreqBox).withMargin(0).withFlex(1));
        lowShelfBox.items.add(FlexItem(2, 6).withMargin(0).withFlex(0));
        lowShelfBox.items.add(FlexItem(minKnobWidth, knobitemheight + knoblabelheight, lowShelfGainBox).withMargin(0).withFlex(1));
        lowShelfBox.items.add(FlexItem(2, 4).withMargin(0).withFlex(0));

        
        para1GainBox.items.clear();
        para1GainBox.flexDirection = FlexBox::Direction::column;
        para1GainBox.items.add(FlexItem(minKnobWidth, knoblabelheight, para1GainLabel).withMargin(0).withFlex(0));
        para1GainBox.items.add(FlexItem(minKnobWidth, knobitemheight, para1GainSlider).withMargin(0).withFlex(1));

        para1FreqBox.items.clear();
        para1FreqBox.flexDirection = FlexBox::Direction::column;
        para1FreqBox.items.add(FlexItem(minKnobWidth, knoblabelheight, para1FreqLabel).withMargin(0).withFlex(0));
        para1FreqBox.items.add(FlexItem(minKnobWidth, knobitemheight, para1FreqSlider).withMargin(0).withFlex(1));

        para1QBox.items.clear();
        para1QBox.flexDirection = FlexBox::Direction::column;
        para1QBox.items.add(FlexItem(minKnobWidth, knoblabelheight, para1QLabel).withMargin(0).withFlex(0));
        para1QBox.items.add(FlexItem(minKnobWidth, knobitemheight, para1QSlider).withMargin(0).withFlex(1));

        para1Box.items.clear();
        para1Box.flexDirection = FlexBox::Direction::row;
        para1Box.items.add(FlexItem(minKnobWidth, knobitemheight + knoblabelheight, para1FreqBox).withMargin(0).withFlex(1));
        para1Box.items.add(FlexItem(minKnobWidth, knobitemheight + knoblabelheight, para1GainBox).withMargin(0).withFlex(1));
        para1Box.items.add(FlexItem(minKnobWidth, knobitemheight + knoblabelheight, para1QBox).withMargin(0).withFlex(1));

        
        para2GainBox.items.clear();
        para2GainBox.flexDirection = FlexBox::Direction::column;
        para2GainBox.items.add(FlexItem(minKnobWidth, knoblabelheight, para2GainLabel).withMargin(0).withFlex(0));
        para2GainBox.items.add(FlexItem(minKnobWidth, knobitemheight, para2GainSlider).withMargin(0).withFlex(1));

        para2FreqBox.items.clear();
        para2FreqBox.flexDirection = FlexBox::Direction::column;
        para2FreqBox.items.add(FlexItem(minKnobWidth, knoblabelheight, para2FreqLabel).withMargin(0).withFlex(0));
        para2FreqBox.items.add(FlexItem(minKnobWidth, knobitemheight, para2FreqSlider).withMargin(0).withFlex(1));

        para2QBox.items.clear();
        para2QBox.flexDirection = FlexBox::Direction::column;
        para2QBox.items.add(FlexItem(minKnobWidth, knoblabelheight, para2QLabel).withMargin(0).withFlex(0));
        para2QBox.items.add(FlexItem(minKnobWidth, knobitemheight, para2QSlider).withMargin(0).withFlex(1));
        
        para2Box.items.clear();
        para2Box.flexDirection = FlexBox::Direction::row;
        para2Box.items.add(FlexItem(minKnobWidth, knobitemheight + knoblabelheight, para2FreqBox).withMargin(0).withFlex(1));
        para2Box.items.add(FlexItem(minKnobWidth, knobitemheight + knoblabelheight, para2GainBox).withMargin(0).withFlex(1));
        para2Box.items.add(FlexItem(minKnobWidth, knobitemheight + knoblabelheight, para2QBox).withMargin(0).withFlex(1));


        paraBox.items.clear();
        paraBox.flexDirection = FlexBox::Direction::column;
        paraBox.items.add(FlexItem(2, 4).withMargin(0).withFlex(0));
        paraBox.items.add(FlexItem(3*minKnobWidth, knobitemheight + knoblabelheight, para1Box).withMargin(0).withFlex(1));
        paraBox.items.add(FlexItem(2, 6).withMargin(0).withFlex(0));
        paraBox.items.add(FlexItem(3*minKnobWidth, knobitemheight + knoblabelheight, para2Box).withMargin(0).withFlex(1));
        paraBox.items.add(FlexItem(2, 4).withMargin(0).withFlex(0));

        
        checkBox.items.clear();
        checkBox.flexDirection = FlexBox::Direction::row;
        //checkBox.items.add(FlexItem(5, 5).withMargin(0).withFlex(0));
        checkBox.items.add(FlexItem(enablewidth, minitemheight, enableButton).withMargin(0).withFlex(0));
        checkBox.items.add(FlexItem(2, 5).withMargin(0).withFlex(0));
        checkBox.items.add(FlexItem(100, minitemheight, titleLabel).withMargin(0).withFlex(1));

        headerComponent.headerBox.items.clear();
        headerComponent.headerBox.flexDirection = FlexBox::Direction::column;
        headerComponent.headerBox.items.add(FlexItem(150, minitemheight, checkBox).withMargin(0).withFlex(1));

        
        knobBox.items.clear();
        knobBox.flexDirection = FlexBox::Direction::row;
        knobBox.items.add(FlexItem(3, 5).withMargin(0).withFlex(0));
        knobBox.items.add(FlexItem(minKnobWidth, 2*(knobitemheight + knoblabelheight) + 14, lowShelfBox).withMargin(0).withFlex(1));
        knobBox.items.add(FlexItem(6, 5).withMargin(0).withFlex(0));
        knobBox.items.add(FlexItem(3*minKnobWidth, 2 * (knobitemheight + knoblabelheight) + 14, paraBox).withMargin(0).withFlex(1));
        knobBox.items.add(FlexItem(6, 5).withMargin(0).withFlex(0));
        knobBox.items.add(FlexItem(minKnobWidth, 2*(knobitemheight + knoblabelheight) + 14, highShelfBox).withMargin(0).withFlex(1));
        knobBox.items.add(FlexItem(3, 5).withMargin(0).withFlex(0));
        
        mainBox.items.clear();
        mainBox.flexDirection = FlexBox::Direction::column;
        //mainBox.items.add(FlexItem(150, minitemheight, checkBox).withMargin(0).withFlex(0));
        //mainBox.items.add(FlexItem(6, 2).withMargin(0).withFlex(0));
        mainBox.items.add(FlexItem(5*minKnobWidth+18, 2 * (knoblabelheight + knobitemheight) + 14, knobBox).withMargin(0).withFlex(1));
        //mainBox.items.add(FlexItem(6, 2).withMargin(0).withFlex(0));
        
        minBounds.setSize(jmax(180, 5*minKnobWidth + 18), 2 * (knobitemheight + knoblabelheight) + 14);
        minHeaderBounds.setSize(jmax(180, 5*minKnobWidth * 5 + 18),  minitemheight + 8);
#else
        
        int minKnobWidth = 70;
        int minitemheight = 32;
        int knoblabelheight = 18;
        int knobitemheight = 32;
        
#if JUCE_IOS
        // make the button heights a bit more for touchscreen purposes
        minitemheight = 40;
        knobitemheight = 40;
#endif

        highShelfGainBox.items.clear();
        highShelfGainBox.flexDirection = FlexBox::Direction::column;
        //highShelfGainBox.items.add(FlexItem(minKnobWidth, knoblabelheight, highShelfGainLabel).withMargin(0).withFlex(0));
        highShelfGainBox.items.add(FlexItem(minKnobWidth, knobitemheight, highShelfGainSlider).withMargin(0).withFlex(1));

        highShelfFreqBox.items.clear();
        highShelfFreqBox.flexDirection = FlexBox::Direction::column;
        //highShelfFreqBox.items.add(FlexItem(minKnobWidth, knoblabelheight, highShelfFreqLabel).withMargin(0).withFlex(0));
        highShelfFreqBox.items.add(FlexItem(minKnobWidth, knobitemheight, highShelfFreqSlider).withMargin(0).withFlex(1));

        highShelfBox.items.clear();
        highShelfBox.flexDirection = FlexBox::Direction::column;
        highShelfBox.items.add(FlexItem(minKnobWidth, knobitemheight , highShelfGainBox).withMargin(0).withFlex(1));
        highShelfBox.items.add(FlexItem(minKnobWidth, knobitemheight , highShelfFreqBox).withMargin(0).withFlex(1));

        
        lowShelfGainBox.items.clear();
        lowShelfGainBox.flexDirection = FlexBox::Direction::column;
        //lowShelfGainBox.items.add(FlexItem(minKnobWidth, knoblabelheight, lowShelfGainLabel).withMargin(0).withFlex(0));
        lowShelfGainBox.items.add(FlexItem(minKnobWidth, knobitemheight, lowShelfGainSlider).withMargin(0).withFlex(1));

        lowShelfFreqBox.items.clear();
        lowShelfFreqBox.flexDirection = FlexBox::Direction::column;
        //lowShelfFreqBox.items.add(FlexItem(minKnobWidth, knoblabelheight, lowShelfFreqLabel).withMargin(0).withFlex(0));
        lowShelfFreqBox.items.add(FlexItem(minKnobWidth, knobitemheight, lowShelfFreqSlider).withMargin(0).withFlex(1));

        lowShelfBox.items.clear();
        lowShelfBox.flexDirection = FlexBox::Direction::column;
        lowShelfBox.items.add(FlexItem(minKnobWidth, knobitemheight , lowShelfGainBox).withMargin(0).withFlex(1));
        lowShelfBox.items.add(FlexItem(minKnobWidth, knobitemheight , lowShelfFreqBox).withMargin(0).withFlex(1));

        
        para1GainBox.items.clear();
        para1GainBox.flexDirection = FlexBox::Direction::column;
        //para1GainBox.items.add(FlexItem(minKnobWidth, knoblabelheight, para1GainLabel).withMargin(0).withFlex(0));
        para1GainBox.items.add(FlexItem(minKnobWidth, knobitemheight, para1GainSlider).withMargin(0).withFlex(1));

        para1FreqBox.items.clear();
        para1FreqBox.flexDirection = FlexBox::Direction::column;
        //para1FreqBox.items.add(FlexItem(minKnobWidth, knoblabelheight, para1FreqLabel).withMargin(0).withFlex(0));
        para1FreqBox.items.add(FlexItem(minKnobWidth, knobitemheight, para1FreqSlider).withMargin(0).withFlex(1));

        para1QBox.items.clear();
        para1QBox.flexDirection = FlexBox::Direction::column;
        //para1QBox.items.add(FlexItem(minKnobWidth, knoblabelheight, para1QLabel).withMargin(0).withFlex(0));
        para1QBox.items.add(FlexItem(minKnobWidth, knobitemheight, para1QSlider).withMargin(0).withFlex(1));

        para1Box.items.clear();
        para1Box.flexDirection = FlexBox::Direction::column;
        para1Box.items.add(FlexItem(minKnobWidth, knobitemheight , para1GainBox).withMargin(0).withFlex(1));
        para1Box.items.add(FlexItem(minKnobWidth, knobitemheight , para1FreqBox).withMargin(0).withFlex(1));
        para1Box.items.add(FlexItem(minKnobWidth, knobitemheight , para1QBox).withMargin(0).withFlex(1));

        
        para2GainBox.items.clear();
        para2GainBox.flexDirection = FlexBox::Direction::column;
        //para2GainBox.items.add(FlexItem(minKnobWidth, knoblabelheight, para2GainLabel).withMargin(0).withFlex(0));
        para2GainBox.items.add(FlexItem(minKnobWidth, knobitemheight, para2GainSlider).withMargin(0).withFlex(1));

        para2FreqBox.items.clear();
        para2FreqBox.flexDirection = FlexBox::Direction::column;
        //para2FreqBox.items.add(FlexItem(minKnobWidth, knoblabelheight, para2FreqLabel).withMargin(0).withFlex(0));
        para2FreqBox.items.add(FlexItem(minKnobWidth, knobitemheight, para2FreqSlider).withMargin(0).withFlex(1));

        para2QBox.items.clear();
        para2QBox.flexDirection = FlexBox::Direction::column;
        //para2QBox.items.add(FlexItem(minKnobWidth, knoblabelheight, para2QLabel).withMargin(0).withFlex(0));
        para2QBox.items.add(FlexItem(minKnobWidth, knobitemheight, para2QSlider).withMargin(0).withFlex(1));
        
        para2Box.items.clear();
        para2Box.flexDirection = FlexBox::Direction::column;
        para2Box.items.add(FlexItem(minKnobWidth, knobitemheight , para2GainBox).withMargin(0).withFlex(1));
        para2Box.items.add(FlexItem(minKnobWidth, knobitemheight , para2FreqBox).withMargin(0).withFlex(1));
        para2Box.items.add(FlexItem(minKnobWidth, knobitemheight , para2QBox).withMargin(0).withFlex(1));


        paraBox.items.clear();
        paraBox.flexDirection = FlexBox::Direction::row;
        paraBox.items.add(FlexItem(minKnobWidth, knobitemheight , para1Box).withMargin(0).withFlex(1));
        paraBox.items.add(FlexItem(3, 5).withMargin(0).withFlex(0));
        paraBox.items.add(FlexItem(minKnobWidth, knobitemheight , para2Box).withMargin(0).withFlex(1));

        
        checkBox.items.clear();
        checkBox.flexDirection = FlexBox::Direction::row;
        checkBox.items.add(FlexItem(5, 5).withMargin(0).withFlex(0));
        checkBox.items.add(FlexItem(minitemheight, minitemheight, enableButton).withMargin(0).withFlex(0));
        checkBox.items.add(FlexItem(2, 5).withMargin(0).withFlex(0));
        checkBox.items.add(FlexItem(100, minitemheight, titleLabel).withMargin(0).withFlex(1));

        headerComponent.headerBox.items.clear();
        headerComponent.headerBox.flexDirection = FlexBox::Direction::column;
        headerComponent.headerBox.items.add(FlexItem(150, minitemheight, checkBox).withMargin(0).withFlex(1));

        
        knobBox.items.clear();
        knobBox.flexDirection = FlexBox::Direction::row;
        knobBox.items.add(FlexItem(4, 5).withMargin(0).withFlex(0));
        knobBox.items.add(FlexItem(minKnobWidth, knobitemheight , lowShelfBox).withMargin(0).withFlex(1));
        knobBox.items.add(FlexItem(3, 5).withMargin(0).withFlex(0));
        knobBox.items.add(FlexItem(2*minKnobWidth, knobitemheight , paraBox).withMargin(0).withFlex(1));
        knobBox.items.add(FlexItem(3, 5).withMargin(0).withFlex(0));
        knobBox.items.add(FlexItem(minKnobWidth, knobitemheight , highShelfBox).withMargin(0).withFlex(1));
        knobBox.items.add(FlexItem(4, 5).withMargin(0).withFlex(0));
        
        mainBox.items.clear();
        mainBox.flexDirection = FlexBox::Direction::column;
        //mainBox.items.add(FlexItem(150, minitemheight, checkBox).withMargin(0).withFlex(0));
        //mainBox.items.add(FlexItem(6, 2).withMargin(0).withFlex(0));
        mainBox.items.add(FlexItem(4*minKnobWidth + 14, 2 * (knoblabelheight), knobBox).withMargin(0).withFlex(1));
        mainBox.items.add(FlexItem(6, 2).withMargin(0).withFlex(0));
        
        minBounds.setSize(jmax(180, 4*minKnobWidth + 16), 3 * (knobitemheight) + 2);
        minHeaderBounds.setSize(jmax(180, minKnobWidth * 4 + 16),  minitemheight + 8);
#endif
        
    }
    
    
    void resized() override
    {
        mainBox.performLayout(getLocalBounds());
        
        lowShelfBg.setRectangle (lowShelfFreqLabel.getBounds().withBottom(lowShelfGainSlider.getBottom()).expanded(2).toFloat());
        highShelfBg.setRectangle (highShelfFreqLabel.getBounds().withBottom(highShelfGainSlider.getBottom()).expanded(2).toFloat());
        para1Bg.setRectangle (para1FreqLabel.getBounds().withBottom(para1GainSlider.getBottom()).withRight(para1QSlider.getRight()).expanded(2).toFloat());
        para2Bg.setRectangle (para2FreqLabel.getBounds().withBottom(para2GainSlider.getBottom()).withRight(para2QSlider.getRight()).expanded(2).toFloat());

#if 0
        lowShelfGainLabel.setBounds(lowShelfGainSlider.getBounds().removeFromLeft(lowShelfGainSlider.getWidth() / 2));
        lowShelfFreqLabel.setBounds(lowShelfFreqSlider.getBounds().removeFromLeft(lowShelfFreqSlider.getWidth() / 2));
        highShelfGainLabel.setBounds(highShelfGainSlider.getBounds().removeFromLeft(highShelfGainSlider.getWidth() / 2));
        highShelfFreqLabel.setBounds(highShelfFreqSlider.getBounds().removeFromLeft(highShelfFreqSlider.getWidth() / 2));
        para1GainLabel.setBounds(para1GainSlider.getBounds().removeFromLeft(para1GainSlider.getWidth() / 2));
        para1FreqLabel.setBounds(para1FreqSlider.getBounds().removeFromLeft(para1FreqSlider.getWidth() / 2));
        para1QLabel.setBounds(para1QSlider.getBounds().removeFromLeft(para1QSlider.getWidth() / 2));
        para2GainLabel.setBounds(para2GainSlider.getBounds().removeFromLeft(para2GainSlider.getWidth() / 2));
        para2FreqLabel.setBounds(para2FreqSlider.getBounds().removeFromLeft(para2FreqSlider.getWidth() / 2));
        para2QLabel.setBounds(para2QSlider.getBounds().removeFromLeft(para2QSlider.getWidth() / 2));
#endif
    }

    void buttonClicked (Button* buttonThatWasClicked) override
    {
        if (buttonThatWasClicked == &enableButton) {
            mParams.enabled = enableButton.getToggleState();
            headerComponent.repaint();
        }
        listeners.call (&ParametricEqView::Listener::parametricEqParamsChanged, this, mParams);
        
    }
    
    void sliderValueChanged (Slider* slider) override
    {
        if (slider == &lowShelfGainSlider) {
            mParams.lowShelfGain = slider->getValue();
        }
        else if (slider == &lowShelfFreqSlider) {
            mParams.lowShelfFreq = slider->getValue();
        }
        else if (slider == &highShelfGainSlider) {
            mParams.highShelfGain = slider->getValue();
        }
        else if (slider == &highShelfFreqSlider) {
            mParams.highShelfFreq = slider->getValue();
        }
        else if (slider == &para1GainSlider) {
            mParams.para1Gain = slider->getValue();
        }
        else if (slider == &para1FreqSlider) {
            mParams.para1Freq = slider->getValue();
        }
        else if (slider == &para1QSlider) {
            mParams.para1Q = slider->getValue();
        }
        else if (slider == &para2GainSlider) {
            mParams.para2Gain = slider->getValue();
        }
        else if (slider == &para2FreqSlider) {
            mParams.para2Freq = slider->getValue();
        }
        else if (slider == &para2QSlider) {
            mParams.para2Q = slider->getValue();
        }

        listeners.call (&ParametricEqView::Listener::parametricEqParamsChanged, this, mParams);
        
    }

    void updateParams(const SonobusAudioProcessor::ParametricEqParams & params) {
        mParams = params;
        
        lowShelfGainSlider.setValue(mParams.lowShelfGain, dontSendNotification);
        lowShelfFreqSlider.setValue(mParams.lowShelfFreq, dontSendNotification);
        highShelfGainSlider.setValue(mParams.highShelfGain, dontSendNotification);
        highShelfFreqSlider.setValue(mParams.highShelfFreq, dontSendNotification);
        para1GainSlider.setValue(mParams.para1Gain, dontSendNotification);
        para1FreqSlider.setValue(mParams.para1Freq, dontSendNotification);
        para1QSlider.setValue(mParams.para1Q, dontSendNotification);
        para2GainSlider.setValue(mParams.para2Gain, dontSendNotification);
        para2FreqSlider.setValue(mParams.para2Freq, dontSendNotification);
        para2QSlider.setValue(mParams.para2Q, dontSendNotification);
        
        enableButton.setToggleState(mParams.enabled, dontSendNotification);
    }
    
    const SonobusAudioProcessor::ParametricEqParams & getParams() const { 
        return mParams;         
    }
    
   
    
private:
    

    ListenerList<Listener> listeners;
    
    
    
    Slider lowShelfGainSlider;
    Slider lowShelfFreqSlider;
    Slider highShelfGainSlider;
    Slider highShelfFreqSlider;
    Slider para1GainSlider;
    Slider para1FreqSlider;
    Slider para1QSlider;
    Slider para2GainSlider;
    Slider para2FreqSlider;
    Slider para2QSlider;

    Label lowShelfGainLabel;
    Label lowShelfFreqLabel;
    Label highShelfGainLabel;
    Label highShelfFreqLabel;
    Label para1GainLabel;
    Label para1FreqLabel;
    Label para1QLabel;
    Label para2GainLabel;
    Label para2FreqLabel;
    Label para2QLabel;

    DrawableRectangle lowShelfBg;
    DrawableRectangle highShelfBg;
    DrawableRectangle para1Bg;
    DrawableRectangle para2Bg;

    
    FlexBox mainBox;
    FlexBox checkBox;
    FlexBox knobBox;
    
    FlexBox lowShelfBox;
    FlexBox lowShelfGainBox;
    FlexBox lowShelfFreqBox;

    FlexBox highShelfBox;
    FlexBox highShelfGainBox;
    FlexBox highShelfFreqBox;

    FlexBox paraBox;

    FlexBox para1Box;
    FlexBox para1GainBox;
    FlexBox para1FreqBox;
    FlexBox para1QBox;

    FlexBox para2Box;
    FlexBox para2GainBox;
    FlexBox para2FreqBox;
    FlexBox para2QBox;


    SonobusAudioProcessor::ParametricEqParams mParams;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ParametricEqView)
};
