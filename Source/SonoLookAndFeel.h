//
//  ActionBridge.h
//  ThumbJamJuce
//
//  Created by Jesse Chappell on 6/5/14.
//
//

#ifndef __TonalEnergyTuner__TETLookAndFeel__
#define __TonalEnergyTuner__TETLookAndFeel__

#include "JuceHeader.h"

class SonoLookAndFeel   : public LookAndFeel_V4
{
public:
    SonoLookAndFeel();

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
    Rectangle<int> getTabButtonExtraComponentBounds (const TabBarButton&, Rectangle<int>& textArea, Component& extraComp) override;

    void createTabTextLayout (const TabBarButton& button, float length, float depth,
                              Colour colour, TextLayout& textLayout);

    Typeface::Ptr getTypefaceForFont (const Font& font) override;


    Button* createSliderButton (Slider&, const bool isIncrement) override;
    Label* createSliderTextBox (Slider&) override;
    
    Font getTextButtonFont (TextButton&, int buttonHeight) override;
    void drawButtonText (Graphics& g, TextButton& button, bool /*isMouseOverButton*/, bool /*isButtonDown*/) override;

    void drawButtonTextWithAlignment (Graphics& g, TextButton& button, bool /*isMouseOverButton*/, bool /*isButtonDown*/, Justification textjust = Justification::centred) ;

    void drawBubble (Graphics&, BubbleComponent&, const Point<float>& tip, const Rectangle<float>& body) override;


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


    void drawTreeviewPlusMinusBox (Graphics& g, const Rectangle<float>& area,
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
    
    
    void drawDrawableButton (Graphics& g, DrawableButton& button,
                             bool /*isMouseOverButton*/, bool /*isButtonDown*/) override;

    
    void drawCallOutBoxBackground (CallOutBox& box, Graphics& g,
                                   const Path& path, Image& cachedImage) override;

    Font getLabelFont (Label& label) override;
    void drawLabel (Graphics& g, Label& label) override;
    

protected:
    
    Font myFont;
    float fontScale;
    
    float labelCornerRadius = 6.0f;
};

class SonoBigTextLookAndFeel   : public SonoLookAndFeel
{
public:
    SonoBigTextLookAndFeel(float maxTextSize=32.0f);
    
    Font getTextButtonFont (TextButton&, int buttonHeight);
    Button* createSliderButton (Slider&, const bool isIncrement);
    Label* createSliderTextBox (Slider&);

protected:
    float maxSize;
    
};

#endif
