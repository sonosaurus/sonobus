#pragma once

#include "JuceHeader.h"

class SonoSlider : public Slider
{
public:
    SonoSlider(const String& name = String()) : Slider(name) {}
    SonoSlider(SliderStyle style, TextEntryBoxPosition textBoxPos) : Slider(style, textBoxPos) {}
    
    void mouseDown(const MouseEvent& event) override {
        if (event.mods.isRightButtonDown()) {
            // Ignore right-click to reserve it for MIDI Learn
        } else {
            Slider::mouseDown(event);
        }
    }
    
    void mouseDrag(const MouseEvent& event) override {
        if (event.mods.isRightButtonDown()) {
            // Ignore right-click drag
        } else {
            Slider::mouseDrag(event);
        }
    }

    void mouseUp(const MouseEvent& event) override {
        if (event.mods.isRightButtonDown()) {
            // Ignore right-click up
        } else {
            Slider::mouseUp(event);
        }
    }
};
