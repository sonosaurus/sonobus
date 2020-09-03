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

//==============================================================================
/*
*/
class EffectsBaseView    : public Component
{
public:
    EffectsBaseView() : sonoSliderLNF(14), smallLNF(12)
    {
        headerComponent.addAndMakeVisible(enableButton);
        headerComponent.addAndMakeVisible(titleLabel);

        headerComponent.addMouseListener(this, true);
        
        std::unique_ptr<Drawable> powerimg(Drawable::createFromImageData(BinaryData::power_svg, BinaryData::power_svgSize));
        std::unique_ptr<Drawable> powerselimg(Drawable::createFromImageData(BinaryData::power_sel_svg, BinaryData::power_sel_svgSize));
        enableButton.setImages(powerimg.get(), nullptr, nullptr, nullptr, powerselimg.get());
        enableButton.setClickingTogglesState(true);
        enableButton.setColour(TextButton::buttonColourId, Colours::transparentBlack);
        enableButton.setColour(TextButton::buttonOnColourId, Colours::transparentBlack);
        enableButton.setColour(DrawableButton::backgroundColourId, Colours::transparentBlack);
        enableButton.setColour(DrawableButton::backgroundOnColourId, Colours::transparentBlack);

    }

    virtual ~EffectsBaseView()
    {
    }

    class HeaderComponent : public Component
    {
    public:
        HeaderComponent(EffectsBaseView & parent_) : parent(parent_) {
            
        }
        ~HeaderComponent() {}
        
        void paint (Graphics& g) override
        {
            Colour usecolor = parent.enableButton.getToggleState() ? enabledColor : normalColor;

            if (mouseIsOver) {
                usecolor = usecolor.withMultipliedBrightness(1.3f);
            }
            
            g.setColour(usecolor);  

            auto bounds = getLocalBounds().withTrimmedTop(2).withTrimmedBottom(2);
            g.fillRoundedRectangle(bounds.toFloat(), 6.0);
        }
        
        void resized() override {
            auto bounds = getLocalBounds().withTrimmedTop(2).withTrimmedBottom(2);
            headerBox.performLayout(bounds);
        }
        
        void mouseEnter (const MouseEvent& event) override {
            mouseIsOver = true;
            repaint();
        }

        void mouseExit (const MouseEvent& event) override {
            mouseIsOver = false;            
            repaint();
        }
        
        FlexBox headerBox;
        EffectsBaseView & parent;
        Colour normalColor = { Colour(0xff2a2a2a) };
        Colour enabledColor = { Colour::fromFloatRGBA(0.2f, 0.5f, 0.7f, 0.5f) };
        bool mouseIsOver = false;
    };
    
    virtual Component * getHeaderComponent() {
        
        return &headerComponent;
    }
    
    void mouseUp (const MouseEvent& event) override {
        if (event.eventComponent == &headerComponent) {
            if (!event.mouseWasDraggedSinceMouseDown()) {                
                headerListeners.call (&EffectsBaseView::HeaderListener::effectsHeaderClicked, this, event);
            }
        }
    }

    

    
    class HeaderListener {
    public:
        virtual ~HeaderListener() {}
        virtual void effectsHeaderClicked(EffectsBaseView *comp, const MouseEvent & event) {}
    };
    
    void addHeaderListener(HeaderListener * listener) { headerListeners.add(listener); }
    void removeHeaderListener(HeaderListener * listener) { headerListeners.remove(listener); }
    
    

    virtual juce::Rectangle<int> getMinimumContentBounds() const {
        return minBounds;
    }

    virtual juce::Rectangle<int> getMinimumHeaderBounds() const {

        return minHeaderBounds;
    }

    
    
   
    
protected:
    

    ListenerList<HeaderListener> headerListeners;
    juce::Rectangle<int> minHeaderBounds;
    juce::Rectangle<int> minBounds;

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
        slider.setColour(Slider::textBoxTextColourId, Colour(0x77eeeeee));
        slider.setColour(TooltipWindow::textColourId, Colour(0xf0eeeeee));
        slider.setLookAndFeel(&sonoSliderLNF);
    }
    
    void configBarSlider(Slider & slider) 
    {
        slider.setSliderStyle(Slider::SliderStyle::LinearBar);
        slider.setTextBoxStyle(Slider::TextBoxRight, true, 60, 14);
        slider.setMouseDragSensitivity(128);
        slider.setScrollWheelEnabled(false);
        slider.setTextBoxIsEditable(false);
        slider.setSliderSnapsToMousePosition(false);
        //slider->setPopupDisplayEnabled(true, false, this);
        //slider.setColour(Slider::textBoxBackgroundColourId, Colours::transparentBlack);
        //slider.setColour(Slider::textBoxOutlineColourId, Colours::transparentBlack);
        //slider.setColour(Slider::textBoxTextColourId, Colour(0x90eeeeee));
        slider.setColour(TooltipWindow::textColourId, Colour(0xf0eeeeee));
        //slider.setLookAndFeel(&sonoSliderLNF);
    }
    
    void configLabel(Label & label, bool bright=false) 
    {
        if (bright) {
            label.setFont(13);
            label.setColour(Label::textColourId, Colour(0xffeeeeee));
        } else {
            label.setFont(12);
            label.setColour(Label::textColourId, Colour(0xc0eeeeee));            
        }
        label.setJustificationType(Justification::centred);
        label.setMinimumHorizontalScale(0.3);
    }
    
    SonoBigTextLookAndFeel sonoSliderLNF;
    SonoBigTextLookAndFeel smallLNF;

    SonoDrawableButton enableButton = { "enable", DrawableButton::ButtonStyle::ImageFitted };    
    HeaderComponent headerComponent = { *this };
    Label titleLabel;

        
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (EffectsBaseView)
};
