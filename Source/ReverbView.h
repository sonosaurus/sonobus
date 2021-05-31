// SPDX-License-Identifier: GPLv3-or-later WITH Appstore-exception
// Copyright (C) 2020 Jesse Chappell


#pragma once

#include <JuceHeader.h>

#include "SonoLookAndFeel.h"
#include "SonoDrawableButton.h"
#include "EffectsBaseView.h"
#include "EffectParams.h"
#include "SonoChoiceButton.h"

//==============================================================================
/*
*/
class ReverbView    : public EffectsBaseView, public Slider::Listener, public Button::Listener, public SonoChoiceButton::Listener
{
public:
    ReverbView(SonobusAudioProcessor & processor_, bool input) : processor(processor_), inputMode(input)
    {
        modelChoice.setTitle(TRANS("Reverb Style"));
        modelChoice.setColour(SonoTextButton::outlineColourId, Colour::fromFloatRGBA(0.6, 0.6, 0.6, 0.4));
        modelChoice.addChoiceListener(this);
        modelChoice.addItem(TRANS("Freeverb"), SonobusAudioProcessor::ReverbModelFreeverb);
        modelChoice.addItem(TRANS("MVerb"), SonobusAudioProcessor::ReverbModelMVerb);
        modelChoice.addItem(TRANS("Zita"), SonobusAudioProcessor::ReverbModelZita);

        auto sizename = TRANS("Size");
        sizeSlider.setName("revsize");
        sizeSlider.setTitle(sizename);
        sizeSlider.setSliderSnapsToMousePosition(false);
        sizeSlider.setScrollWheelEnabled(false);
        configKnobSlider(sizeSlider);
        sizeSlider.setTextBoxStyle(Slider::TextBoxAbove, true, 80, 18);
        sizeSlider.setTextBoxIsEditable(true);

        sizeLabel.setText(sizename, dontSendNotification);
        sizeLabel.setAccessible(false);
        configLabel(sizeLabel);

        mReverbSizeAttachment = std::make_unique<AudioProcessorValueTreeState::SliderAttachment> (processor.getValueTreeState(),
                                                                                                  input ? SonobusAudioProcessor::paramInputReverbSize : SonobusAudioProcessor::paramMainReverbSize,
                                                                                                  sizeSlider);
        auto levelname = TRANS("Level");
        levelSlider.setName("revlevel");
        levelSlider.setTitle(levelname);
        levelSlider.setSliderSnapsToMousePosition(false);
        levelSlider.setScrollWheelEnabled(false);
        configKnobSlider(levelSlider);
        levelSlider.setTextBoxStyle(Slider::TextBoxAbove, true, 80, 18);
        levelSlider.setTextBoxIsEditable(true);

        levelLabel.setText(levelname, dontSendNotification);
        levelLabel.setAccessible(false);
        configLabel(levelLabel);

        mReverbLevelAttachment = std::make_unique<AudioProcessorValueTreeState::SliderAttachment> (processor.getValueTreeState(),
                                                                                                   input ? SonobusAudioProcessor::paramInputReverbLevel : SonobusAudioProcessor::paramMainReverbLevel,
                                                                                                   levelSlider);
        auto dampname = TRANS("Damping");
        dampingSlider.setName("revdamp");
        dampingSlider.setTitle(dampname);
        dampingSlider.setSliderSnapsToMousePosition(false);
        dampingSlider.setScrollWheelEnabled(false);
        configKnobSlider(dampingSlider);
        dampingSlider.setTextBoxStyle(Slider::TextBoxAbove, true, 80, 18);
        dampingSlider.setTextBoxIsEditable(true);

        dampingLabel.setText(dampname, dontSendNotification);
        dampingLabel.setAccessible(false);
        configLabel(dampingLabel);

        mReverbDampingAttachment = std::make_unique<AudioProcessorValueTreeState::SliderAttachment> (processor.getValueTreeState(),
                                                                                                     input ? SonobusAudioProcessor::paramInputReverbDamping : SonobusAudioProcessor::paramMainReverbDamping, dampingSlider);
        auto predelname = TRANS("Pre-Delay");
        preDelaySlider.setName("revpredel");
        preDelaySlider.setTitle(predelname);
        preDelaySlider.setSliderSnapsToMousePosition(false);
        preDelaySlider.setScrollWheelEnabled(false);
        configKnobSlider(preDelaySlider);
        preDelaySlider.setTextBoxStyle(Slider::TextBoxAbove, true, 80, 18);
        preDelaySlider.setTextBoxIsEditable(true);

        preDelayLabel.setText(predelname, dontSendNotification);
        preDelayLabel.setAccessible(false);
        configLabel(preDelayLabel);

        mReverbPreDelayAttachment = std::make_unique<AudioProcessorValueTreeState::SliderAttachment> (processor.getValueTreeState(),
                                                                                                      input ? SonobusAudioProcessor::paramInputReverbPreDelay : SonobusAudioProcessor::paramMainReverbPreDelay, preDelaySlider);
           
        // these are in the header component
        auto mainname = input ? TRANS("Input Reverb") : TRANS("Reverb");
        enableButton.addListener(this);
        enableButton.setTitle(mainname);
        titleLabel.setText(mainname, dontSendNotification);
        titleLabel.setAccessible(false);

        if (!input) {
            mReverbEnableAttachment = std::make_unique<AudioProcessorValueTreeState::ButtonAttachment> (processor.getValueTreeState(), SonobusAudioProcessor::paramMainReverbEnabled, enableButton);
        } else {
            enableButton.setVisible(false);
            dragButton.setVisible(false);
        }


        if (!input) {
            addAndMakeVisible(modelChoice);
        }
        addAndMakeVisible(levelSlider);
        addAndMakeVisible(levelLabel);
        addAndMakeVisible(sizeSlider);
        addAndMakeVisible(sizeLabel);
        addAndMakeVisible(dampingSlider);
        addAndMakeVisible(dampingLabel);
        addAndMakeVisible(preDelaySlider);
        addAndMakeVisible(preDelayLabel);
        
        setupLayout();
        
        //updateParams(mParams);
    }

    ~ReverbView()
    {
    }

   
    
    class Listener {
    public:
        virtual ~Listener() {}
        //virtual void reverbParamsChanged(ReverbView *comp, SonoAudio::CompressorParams &params) {}
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

        sizeBox.items.clear();
        sizeBox.flexDirection = FlexBox::Direction::column;
        sizeBox.items.add(FlexItem(minKnobWidth, knoblabelheight, sizeLabel).withMargin(0).withFlex(0));
        sizeBox.items.add(FlexItem(minKnobWidth, minitemheight, sizeSlider).withMargin(0).withFlex(1));

        levelBox.items.clear();
        levelBox.flexDirection = FlexBox::Direction::column;
        levelBox.items.add(FlexItem(minKnobWidth, knoblabelheight, levelLabel).withMargin(0).withFlex(0));
        levelBox.items.add(FlexItem(minKnobWidth, minitemheight, levelSlider).withMargin(0).withFlex(1));

        dampBox.items.clear();
        dampBox.flexDirection = FlexBox::Direction::column;
        dampBox.items.add(FlexItem(minKnobWidth, knoblabelheight, dampingLabel).withMargin(0).withFlex(0));
        dampBox.items.add(FlexItem(minKnobWidth, minitemheight, dampingSlider).withMargin(0).withFlex(1));

        preDelayBox.items.clear();
        preDelayBox.flexDirection = FlexBox::Direction::column;
        preDelayBox.items.add(FlexItem(minKnobWidth, knoblabelheight, preDelayLabel).withMargin(0).withFlex(0));
        preDelayBox.items.add(FlexItem(minKnobWidth, minitemheight, preDelaySlider).withMargin(0).withFlex(1));


        knobBox.items.clear();
        knobBox.flexDirection = FlexBox::Direction::row;
        knobBox.items.add(FlexItem(5, 5).withMargin(0).withFlex(0));
        knobBox.items.add(FlexItem(minKnobWidth, minitemheight, preDelayBox).withMargin(0).withFlex(1));
        knobBox.items.add(FlexItem(minKnobWidth, minitemheight, levelBox).withMargin(0).withFlex(1));
        knobBox.items.add(FlexItem(minKnobWidth, minitemheight, sizeBox).withMargin(0).withFlex(1));
        knobBox.items.add(FlexItem(minKnobWidth, minitemheight, dampBox).withMargin(0).withFlex(1));
        knobBox.items.add(FlexItem(5, 5).withMargin(0).withFlex(0));


        
        checkBox.items.clear();
        checkBox.flexDirection = FlexBox::Direction::row;
        //checkBox.items.add(FlexItem(5, 5).withMargin(0).withFlex(0));
        checkBox.items.add(FlexItem(enablewidth, minitemheight, enableButton).withMargin(0).withFlex(0));
        checkBox.items.add(FlexItem(2, 5).withMargin(0).withFlex(0));
        checkBox.items.add(FlexItem(100, minitemheight, titleLabel).withMargin(0).withFlex(1).withMaxWidth(120));
        //checkBox.items.add(FlexItem(2, 5).withMargin(0).withFlex(0.1));
        checkBox.items.add(FlexItem(24, minitemheight, dragButton).withMargin(0).withFlex(0));
        if (!inputMode) {
            checkBox.items.add(FlexItem(minKnobWidth, minitemheight, modelChoice).withMargin(0).withFlex(1));
        }

        checkBox.items.add(FlexItem(2, 5).withMargin(0).withFlex(0.1));

        headerComponent.headerBox.items.clear();
        headerComponent.headerBox.flexDirection = FlexBox::Direction::column;
        headerComponent.headerBox.items.add(FlexItem(150, minitemheight, checkBox).withMargin(0).withFlex(1));

        
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
            //mParams.enabled = enableButton.getToggleState();
            if (!inputMode) {
                processor.setMainReverbEnabled(enableButton.getToggleState());
            }

            headerComponent.repaint();
        }
        //listeners.call (&ReverbView::Listener::reverbParamsChanged, this, mParams);
        
    }
    
    void sliderValueChanged (Slider* slider) override
    {

        //listeners.call (&ExpanderView::Listener::expanderParamsChanged, this, mParams);
        
    }

    void choiceButtonSelected(SonoChoiceButton *comp, int index, int ident) override {
        if (comp == &modelChoice) {
            processor.setMainReverbModel((SonobusAudioProcessor::ReverbModel) ident);
        }
    }

    void updateParams() {

        if (!inputMode) {
            modelChoice.setSelectedId(processor.getMainReverbModel(), dontSendNotification);
            enableButton.setToggleState(processor.getMainReverbEnabled(), dontSendNotification);
        }
    }

    /*
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
    */
   
    
private:

    SonobusAudioProcessor & processor;
    bool inputMode = false;

    ListenerList<Listener> listeners;
    
    Slider levelSlider;
    Slider sizeSlider;
    Slider dampingSlider;
    Slider preDelaySlider;

    Label levelLabel;
    Label sizeLabel;
    Label dampingLabel;
    Label preDelayLabel;

    SonoChoiceButton modelChoice;

    FlexBox mainBox;
    FlexBox checkBox;
    FlexBox knobBox;
    FlexBox levelBox;
    FlexBox sizeBox;
    FlexBox dampBox;
    FlexBox preDelayBox;

    std::unique_ptr<AudioProcessorValueTreeState::ButtonAttachment> mReverbEnableAttachment;
    std::unique_ptr<AudioProcessorValueTreeState::SliderAttachment> mReverbSizeAttachment;
    std::unique_ptr<AudioProcessorValueTreeState::SliderAttachment> mReverbLevelAttachment;
    std::unique_ptr<AudioProcessorValueTreeState::SliderAttachment> mReverbDampingAttachment;
    std::unique_ptr<AudioProcessorValueTreeState::SliderAttachment> mReverbPreDelayAttachment;

    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ReverbView)
};
