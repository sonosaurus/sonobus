// SPDX-License-Identifier: GPLv3-or-later WITH Appstore-exception
// Copyright (C) 2020 Jesse Chappell

#pragma once


#include "JuceHeader.h"


class SonoLookAndFeel   : public LookAndFeel_V4, public foleys::LevelMeter::LookAndFeelMethods
{
public:
    SonoLookAndFeel(bool useUniversalFont=false);

    void setLanguageCode(const String & lang, bool useUniversalFont=false);

    //void fillWithBackgroundTexture (Graphics&);
    //static void fillWithBackgroundTexture (Component&, Graphics&);

    void setLabelCornerRadius(float val) { labelCornerRadius = val; }
    float getLabelCornerRadius() const { return labelCornerRadius; }
    
    void drawTabButton (TabBarButton& button, Graphics&, bool isMouseOver, bool isMouseDown) override;
    void drawTabbedButtonBarBackground (TabbedButtonBar&, Graphics&) override;
    void drawTabAreaBehindFrontButton (TabbedButtonBar&, Graphics&, int, int) override;
    void drawTabButtonText (TabBarButton& button, Graphics& g, bool isMouseOver, bool isMouseDown) override;
    int getTabButtonSpaceAroundImage() override;

    int getTabButtonBestWidth (TabBarButton&, int tabDepth) override;
    juce::Rectangle<int> getTabButtonExtraComponentBounds (const TabBarButton&, juce::Rectangle<int>& textArea, Component& extraComp) override;

    void createTabTextLayout (const TabBarButton& button, float length, float depth,
                              Colour colour, TextLayout& textLayout);

    Typeface::Ptr getTypefaceForFont (const Font& font) override;

    Font getMenuBarFont (MenuBarComponent& menuBar, int /*itemIndex*/, const String& /*itemText*/) override;

    
    Button* createSliderButton (Slider&, const bool isIncrement) override;
    Label* createSliderTextBox (Slider&) override;
    
    Font getTextButtonFont (TextButton&, int buttonHeight) override;
    void drawButtonText (Graphics& g, TextButton& button, bool /*isMouseOverButton*/, bool /*isButtonDown*/) override;

    void drawButtonTextWithAlignment (Graphics& g, TextButton& button, bool /*isMouseOverButton*/, bool /*isButtonDown*/, Justification textjust = Justification::centred) ;

    void drawBubble (Graphics&, BubbleComponent&, const Point<float>& tip, const juce::Rectangle<float>& body) override;


    void drawFileBrowserRow (Graphics&, int width, int height,
                             const File& file, const String& filename, Image* icon,
                             const String& fileSizeDescription, const String& fileTimeDescription,
                             bool isDirectory, bool isItemSelected, int itemIndex,
                             DirectoryContentsDisplayComponent&) override;
    
    Button* createFileBrowserGoUpButton() override;

    void layoutFileBrowserComponent (FileBrowserComponent&,
                                     DirectoryContentsDisplayComponent*,
                                     FilePreviewComponent*,
                                     ComboBox* currentPathBox,
                                     TextEditor* filenameBox,
                                     Button* goUpButton) override;


    void drawTreeviewPlusMinusBox (Graphics& g, const juce::Rectangle<float>& area,
                                   Colour backgroundColour, bool isOpen, bool isMouseOver) override;


    void drawToggleButton (Graphics& g, ToggleButton& button,
                           bool isMouseOverButton, bool isButtonDown) override;


    void drawTickBox (Graphics&, Component&,
                      float x, float y, float w, float h,
                      bool ticked, bool isEnabled, bool isMouseOverButton, bool isButtonDown) override;

    void drawRotarySlider (Graphics& g, int x, int y, int width, int height, float sliderPos,
                           const float rotaryStartAngle, const float rotaryEndAngle, Slider& slider) override;

    void drawLinearSlider (Graphics&, int x, int y, int width, int height,
                           float sliderPos, float minSliderPos, float maxSliderPos,
                           const Slider::SliderStyle, Slider&) override;
    
    Font getSliderPopupFont (Slider&) override;
    int getSliderPopupPlacement (Slider&) override;

    int getSliderThumbRadius (Slider& slider) override;

    Slider::SliderLayout getSliderLayout (Slider& slider) override;

    void drawDrawableButton (Graphics& g, DrawableButton& button,
                             bool /*isMouseOverButton*/, bool /*isButtonDown*/) override;

    
    void drawCallOutBoxBackground (CallOutBox& box, Graphics& g,
                                   const Path& path, Image& cachedImage) override;

    Font getLabelFont (Label& label) override;
    void drawLabel (Graphics& g, Label& label) override;
    
    PopupMenu::Options getOptionsForComboBoxPopupMenu (ComboBox&, Label&) override;
    Font getComboBoxFont (ComboBox&) override;
    Font getPopupMenuFont() override;
    void drawPopupMenuBackground (Graphics& g, [[maybe_unused]] int width, [[maybe_unused]] int height) override;
  
    void drawTooltip (Graphics& g, const String& text, int width, int height) override;


    Justification sliderTextJustification = Justification::centred;

    static float getFontScale() { return fontScale; }
    static void setFontScale(float scale) { fontScale = scale; }


protected:

    static float fontScale;

    Font myFont;

    float labelCornerRadius = 4.0f;
    String languageCode;

    bool mUseUniversalFont = false;

public:

  #include "LevelMeterLookAndFeelMethods.h"

    
};

class SonoBigTextLookAndFeel   : public SonoLookAndFeel
{
public:
    SonoBigTextLookAndFeel(float maxTextSize=32.0f);
    
    Font getTextButtonFont (TextButton&, int buttonHeight) override;
    Button* createSliderButton (Slider&, const bool isIncrement) override;
    Label* createSliderTextBox (Slider&) override;

    void drawToggleButton (Graphics& g, ToggleButton& button,
                           bool isMouseOverButton, bool isButtonDown) override;

    Justification textJustification = Justification::centred;
    
protected:
    float maxSize;
    
};

class SonoPanSliderLookAndFeel   : public SonoLookAndFeel
{
public:
    SonoPanSliderLookAndFeel(float maxTextSize=14.0f);
    
    Font getSliderPopupFont (Slider&) override;
    int getSliderPopupPlacement (Slider&) override;

    Button* createSliderButton (Slider&, const bool isIncrement) override;
    Label* createSliderTextBox (Slider&) override;
    int getSliderThumbRadius (Slider& slider) override;

    void drawLinearSlider (Graphics&, int x, int y, int width, int height,
                           float sliderPos, float minSliderPos, float maxSliderPos,
                           const Slider::SliderStyle, Slider&) override;

    Justification textJustification = Justification::centred;
    
protected:
    float maxSize;
    
};

class SonoDashedBorderButtonLookAndFeel : public SonoLookAndFeel
{
public:
    void drawButtonBackground(Graphics&, Button&, const Colour& backgroundColour,
                              bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override;
};

