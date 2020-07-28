
//
//  ActionBridge.cpp
//  ThumbJamJuce
//
//  Created by Jesse Chappell on 6/5/14.
//
//

#include "SonoLookAndFeel.h"
#include "DebugLogC.h"
#include "SonoDrawableButton.h"
//#include "ConfigurationRowView.h"
#include "SonoTextButton.h"
//#include "AppState.h"

//==============================================================================
SonoLookAndFeel::SonoLookAndFeel()
{
    // setColour (mainBackgroundColourId, Colour::greyLevel (0.8f));
    //DebugLogC("Sonolook and feel");
    
    setUsingNativeAlertWindows(true);

    fontScale = 1.0;
    
    String lang = SystemStats::getUserLanguage();
    if (lang == "zh") {
        fontScale = 1.0f;
    } else {
        // with open sans
        //fontScale = 1.3;
    }
        
    setColourScheme(getDarkColourScheme());
    
    getCurrentColourScheme().setUIColour(ColourScheme::UIColour::windowBackground, Colour::fromFloatRGBA(0.0, 0.0, 0.0, 1.0));
    getCurrentColourScheme().setUIColour(ColourScheme::UIColour::widgetBackground, Colour::fromFloatRGBA(0.1, 0.1, 0.1, 1.0));
    getCurrentColourScheme().setUIColour(ColourScheme::UIColour::outline, Colour::fromFloatRGBA(0.3, 0.3, 0.3, 0.5));

    setColour (Label::textColourId, Colour (0xffcccccc));
    setColour (Label::textWhenEditingColourId, Colour (0xffe9e9e9));
    
    setColour(ResizableWindow::backgroundColourId, Colour(0xff111111));
    
    //setColour (TextButton::buttonColourId, Colour (0xff363636));
    setColour (TextButton::buttonColourId, Colour::fromFloatRGBA(0.15, 0.15, 0.15, 0.7));
    //setColour (TextButton::buttonOnColourId, Colour (0xff3d70c8));
    setColour (TextButton::buttonOnColourId, Colour::fromFloatRGBA(0.5, 0.4, 0.6, 0.8));
    setColour (TextButton::textColourOnId, Colour (0xddcccccc));
    setColour (TextButton::textColourOffId, Colour (0xdde9e9e9));

    setColour (ToggleButton::textColourId, Colour (0xddcccccc));

    
    setColour (SonoTextButton::outlineColourId, Colour::fromFloatRGBA(0.3, 0.3, 0.3, 0.5));

    setColour (ScrollBar::ColourIds::thumbColourId, Colour::fromFloatRGBA(0.4, 0.4, 0.4, 0.6));
    
    //setColour (ComboBox::backgroundColourId, Colour (0xff161616));
    setColour (ComboBox::backgroundColourId, Colour::fromFloatRGBA(0.15, 0.15, 0.15, 0.7));
    setColour (ComboBox::textColourId, Colour (0xffe9e9e9));
    setColour (ComboBox::outlineColourId, Colour::fromFloatRGBA(0.3, 0.3, 0.3, 0.5));

    setColour (TextEditor::backgroundColourId, Colour (0xff050505));
    setColour (TextEditor::textColourId, Colour (0xffe9e9e9));
    setColour (TextEditor::highlightColourId, Colour (0xff5959f9));
    setColour (TextEditor::outlineColourId, Colour::fromFloatRGBA(0.3, 0.3, 0.3, 0.5));
    setColour (TextEditor::focusedOutlineColourId, Colour::fromFloatRGBA(0.3, 0.3, 0.3, 0.7));

    setColour (Slider::backgroundColourId, Colour::fromFloatRGBA(0.2, 0.2, 0.2, 1.0));
    setColour (Slider::rotarySliderOutlineColourId, Colour::fromFloatRGBA(0.2, 0.2, 0.2, 1.0));
    setColour (Slider::textBoxTextColourId, Colour(0xddcccccc));
    setColour (Slider::textBoxBackgroundColourId, Colour::fromFloatRGBA(0.1, 0.1, 0.1, 0.7));
    setColour (Slider::textBoxHighlightColourId, Colour (0xff050505));
    setColour (Slider::textBoxOutlineColourId, Colour::fromFloatRGBA(0.3, 0.3, 0.3, 0.5));
    
    setColour (Slider::trackColourId, Colour::fromFloatRGBA(0.1, 0.4, 0.6, 0.8));
    setColour (Slider::thumbColourId, Colour::fromFloatRGBA(0.1, 0.4, 0.6, 0.9));
    setColour (Slider::rotarySliderFillColourId, Colour::fromFloatRGBA(0.5, 0.4, 0.6, 0.9));
    
    setColour (TabbedButtonBar::tabOutlineColourId, Colour::fromFloatRGBA(0.3, 0.3, 0.3, 0.5));


    
    setColour (ListBox::backgroundColourId, Colour::fromFloatRGBA(0.15, 0.15, 0.15, 0.7));
    setColour (ListBox::outlineColourId, Colour::fromFloatRGBA(0.3, 0.3, 0.3, 0.5));

    setColour (BubbleComponent::backgroundColourId, Colour::fromFloatRGBA(0.25, 0.25, 0.25, 1.0));
    setColour (BubbleComponent::outlineColourId, Colour::fromFloatRGBA(0.4, 0.4, 0.4, 0.5));
    //setColour (TooltipWindow::textColourId, Colour(0xeecccccc));
    setColour (TooltipWindow::textColourId, Colour(0xee222222));
    setColour (TooltipWindow::backgroundColourId, Colour(0xeeffff99));

    //setColour (SonoDrawableButton::overOverlayColourId, Colour::fromFloatRGBA(0.8, 0.8, 0.8, 0.08));
    //setColour (SonoDrawableButton::downOverlayColourId, Colour::fromFloatRGBA(0.8, 0.8, 0.8, 0.3));
    setColour (SonoDrawableButton::overOverlayColourId, Colour::fromFloatRGBA(0.8, 0.8, 0.8, 0.0));
    setColour (SonoDrawableButton::downOverlayColourId, Colour::fromFloatRGBA(0.8, 0.8, 0.8, 0.0));
    
    setColour (DrawableButton::textColourId, Colour (0xffb9b9b9));
    setColour (DrawableButton::textColourOnId, Colour (0xffe9e9e9));

    //setColour (DrawableButton::backgroundColourId, Colour (0xffb9b9b9));
    setColour (DrawableButton::backgroundOnColourId, Colour::fromFloatRGBA(0.5, 0.4, 0.6, 0.8));

    //setColour (ConfigurationRowView::backgroundColourId, Colour::fromFloatRGBA(0.05, 0.05, 0.05, 1.0));
    //setColour (ConfigurationRowView::selectedBackgroundColourId, Colour::fromFloatRGBA(0.15, 0.15, 0.15, 1.0));

    setColour(ToggleButton::tickColourId, Colour::fromFloatRGBA(0.4, 0.8, 1.0, 1.0));
    
    setColour (DirectoryContentsDisplayComponent::highlightColourId, Colour::fromFloatRGBA(0.1, 0.4, 0.6, 0.9));
    setColour (DirectoryContentsDisplayComponent::textColourId, Colour (0xffe9e9e9));
    // setColour (Label::textColourId, Colour (0xffe9e9e9));

    //myFont = Typeface::createSystemTypefaceFor (BinaryData::DejaVuSans_ttf, BinaryData::DejaVuSans_ttfSize);
    //setDefaultSansSerifTypefaceName("Gill Sans");
    //setDefaultSansSerifTypefaceName("Arial Unicode MS");
    //setDefaultSansSerifTypefaceName(myFont.getTypefaceName());

    //myFont = Typeface::createSystemTypefaceFor (BinaryData::GillSans_ttc, BinaryData::GillSans_ttcSize);
    myFont = Font(16 * fontScale);
    
    setupDefaultMeterColours();
    
    //DebugLogC("Myfont name %s", myFont.getTypefaceName().toRawUTF8());
}


Typeface::Ptr SonoLookAndFeel::getTypefaceForFont (const Font& font)
{
    DebugLogC("get typeface for font %s", font.getTypefaceName().toRawUTF8());
    if (font.getTypefaceName() == Font::getDefaultSansSerifFontName())
    {
        // if on android and language is japanese/chinese/korean, use DroidSansFallback
        String lang = SystemStats::getUserLanguage();
        //if (!AppState::getInstance()->mainConfig.activeLanguageCode.empty()) {
        //    lang = AppState::getInstance()->mainConfig.activeLanguageCode;
        //}
        
        String slang = lang.initialSectionNotContaining("_").toLowerCase();
        
        if (slang == "ja" || slang == "ko") {
            DebugLogC("Using japanese/korean");
            Font jfont(font);
            //jfont.setTypefaceName("DroidSansFallback");
#if JUCE_MAC
            //jfont.setTypefaceName("Arial Unicode MS");
            jfont.setTypefaceName("Hiragino Sans W3");
#elif JUCE_ANDROID
            //jfont.setTypefaceName("Droid Sans Fallback");            
            return Typeface::createSystemTypefaceFor (BinaryData::DejaVuSans_ttf, BinaryData::DejaVuSans_ttfSize);
#endif
            return Typeface::createSystemTypefaceFor (jfont);            
        }
        else if (slang == "zh") {
            DebugLogC("Using chinese");
            Font jfont(font);
            //jfont.setTypefaceName("DroidSansFallback");
#if JUCE_MAC
            jfont.setTypefaceName("Arial Unicode MS");
            //jfont.setTypefaceName("Hiragino Sans W3");
#elif JUCE_ANDROID
            jfont.setTypefaceName("DroidSansFallback");            
            return Typeface::createSystemTypefaceFor (BinaryData::DejaVuSans_ttf, BinaryData::DejaVuSans_ttfSize);
#endif
            return Typeface::createSystemTypefaceFor (jfont);            
        }
        else {
            DebugLogC("Creating custom typeface!!");
            
            return Typeface::createSystemTypefaceFor (BinaryData::DejaVuSans_ttf, BinaryData::DejaVuSans_ttfSize);
            //return Typeface::createSystemTypefaceFor (BinaryData::OpenSansRegular_ttf, BinaryData::OpenSansRegular_ttfSize);
        }
    }
    return LookAndFeel_V4::getTypefaceForFont(font);
}

#if 1


//==============================================================================
void SonoLookAndFeel::drawCallOutBoxBackground (CallOutBox& box, Graphics& g,
                                               const Path& path, Image& cachedImage)
{
    if (cachedImage.isNull())
    {
        cachedImage = Image (Image::ARGB, box.getWidth(), box.getHeight(), true);
        Graphics g2 (cachedImage);
        
        DropShadow (Colours::black.withAlpha (0.7f), 8, Point<int> (0, 2)).drawForPath (g2, path);
    }
    
    g.setColour (Colours::black);
    g.drawImageAt (cachedImage, 0, 0);
    
    //g.setColour (getCurrentColourScheme().getUIColour (ColourScheme::UIColour::widgetBackground).withAlpha (0.8f));
    g.setColour (getCurrentColourScheme().getUIColour (ColourScheme::UIColour::widgetBackground));
    g.fillPath (path);
    
    g.setColour (getCurrentColourScheme().getUIColour (ColourScheme::UIColour::outline).withAlpha (0.8f));
    g.strokePath (path, PathStrokeType (1.0f));
}



int SonoLookAndFeel::getTabButtonBestWidth (TabBarButton& button, int depth)
{
    return 250; // 120;
}

int SonoLookAndFeel::getTabButtonSpaceAroundImage() {
    return 0;
}

static Colour getTabBackgroundColour (TabBarButton& button)
{
    
    const Colour bkg (button.findColour (TabbedComponent::backgroundColourId).contrasting (0.15f));

    if (button.isFrontTab())
        return bkg.overlaidWith (Colours::yellow.withAlpha (0.8f));

    return bkg;
}

Rectangle<int> SonoLookAndFeel::getTabButtonExtraComponentBounds (const TabBarButton& button, Rectangle<int>& textArea, Component& comp)
{
    Rectangle<int> extraComp;
    
    auto orientation = button.getTabbedButtonBar().getOrientation();
    
    if (button.getExtraComponentPlacement() == TabBarButton::beforeText)
    {
        switch (orientation)
        {
            case TabbedButtonBar::TabsAtBottom:
            case TabbedButtonBar::TabsAtTop:     extraComp = textArea.removeFromLeft   (comp.getWidth()); break;
            case TabbedButtonBar::TabsAtLeft:    extraComp = textArea.removeFromBottom (comp.getHeight()); break;
            case TabbedButtonBar::TabsAtRight:   extraComp = textArea.removeFromTop    (comp.getHeight()); break;
            default:                             jassertfalse; break;
        }
    }
    else if (button.getExtraComponentPlacement() == TabBarButton::afterText)
    {
        switch (orientation)
        {
            case TabbedButtonBar::TabsAtBottom:
            case TabbedButtonBar::TabsAtTop:     extraComp = textArea.removeFromRight  (comp.getWidth()); break;
            case TabbedButtonBar::TabsAtLeft:    extraComp = textArea.removeFromTop    (comp.getHeight()); break;
            case TabbedButtonBar::TabsAtRight:   extraComp = textArea.removeFromBottom (comp.getHeight()); break;
            default:                             jassertfalse; break;
        }
    }
    else if (button.getExtraComponentPlacement() == TabBarButton::aboveText)
    {
        switch (orientation)
        {
            case TabbedButtonBar::TabsAtBottom:
            case TabbedButtonBar::TabsAtTop:     extraComp = textArea.removeFromTop  (comp.getHeight()); break;
            case TabbedButtonBar::TabsAtLeft:    extraComp = textArea.removeFromTop    (comp.getHeight()); break;
            case TabbedButtonBar::TabsAtRight:   extraComp = textArea.removeFromTop (comp.getHeight()); break;
            default:                             jassertfalse; break;
        }

        // DebugLogC("Extra comp bounds: %s", extraComp.toString().toRawUTF8())
        extraComp.translate(0, 3);
        //DebugLogC("After Extra comp bounds: %s", extraComp.toString().toRawUTF8())

    }
    else if (button.getExtraComponentPlacement() == TabBarButton::belowText)
    {
        switch (orientation)
        {
            case TabbedButtonBar::TabsAtBottom:
            case TabbedButtonBar::TabsAtTop:     extraComp = textArea.removeFromBottom  (comp.getHeight()); break;
            case TabbedButtonBar::TabsAtLeft:    extraComp = textArea.removeFromBottom    (comp.getHeight()); break;
            case TabbedButtonBar::TabsAtRight:   extraComp = textArea.removeFromBottom (comp.getHeight()); break;
            default:                             jassertfalse; break;
        }
    }
    
    return extraComp;
}

void SonoLookAndFeel::createTabTextLayout (const TabBarButton& button, float length, float depth,
                                          Colour colour, TextLayout& textLayout)
{
    float fontsize = button.getExtraComponent() != nullptr ? jmin(depth, 32.0f) * 0.85f : jmin(depth, 32.0f) * 0.5f;
    Font font = myFont.withHeight(fontsize * fontScale);
    font.setUnderline (button.hasKeyboardFocus (false));

    AttributedString s;
    s.setWordWrap(AttributedString::byWord);
    s.setJustification (Justification::centred);
    s.append (button.getButtonText().trim(), font, colour);
    
    textLayout.createLayout (s, length);
}

void SonoLookAndFeel::drawTabButton (TabBarButton& button, Graphics& g, bool isMouseOver, bool isMouseDown)
{
    const Rectangle<int> activeArea (button.getActiveArea());
    
    const TabbedButtonBar::Orientation o = button.getTabbedButtonBar().getOrientation();
    
    const Colour bkg (button.getTabBackgroundColour());
    const Colour selcol = Colour::fromFloatRGBA(0.0f, 0.2f, 0.4f, 1.0f);

    // DebugLogC("Sono draw tab button");
    
    
    if (button.getToggleState() && bkg != Colours::black)
    {
        //g.setColour (bkg);
        g.setColour (selcol);
    }
    else
    {
        
        Point<int> p1, p2;
        
        switch (o)
        {
            case TabbedButtonBar::TabsAtBottom:   p1 = activeArea.getBottomLeft(); p2 = activeArea.getTopLeft();    break;
            case TabbedButtonBar::TabsAtTop:      p1 = activeArea.getTopLeft();    p2 = activeArea.getBottomLeft(); break;
            case TabbedButtonBar::TabsAtRight:    p1 = activeArea.getTopRight();   p2 = activeArea.getTopLeft();    break;
            case TabbedButtonBar::TabsAtLeft:     p1 = activeArea.getTopLeft();    p2 = activeArea.getTopRight();   break;
            default:                              jassertfalse; break;
        }

        g.setColour(isMouseDown ? bkg.brighter(0.1) : bkg);

        //g.setGradientFill (ColourGradient (bkg.darker (0.1f), (float) p1.x, (float) p1.y,
        //                                   bkg.darker (0.5f),   (float) p2.x, (float) p2.y, false));
    }

    Rectangle<int> p (activeArea.reduced(1));

    g.fillRect (p);

    //g.fillRect (activeArea);
   
#if 0
    g.setColour (button.findColour (TabbedButtonBar::tabOutlineColourId));
    
    Rectangle<int> r (activeArea);
    
    if (o != TabbedButtonBar::TabsAtBottom)   g.fillRect (r.removeFromTop (1));
    if (o != TabbedButtonBar::TabsAtTop)      g.fillRect (r.removeFromBottom (1));
    if (o != TabbedButtonBar::TabsAtRight)    g.fillRect (r.removeFromLeft (1));
    if (o != TabbedButtonBar::TabsAtLeft)     g.fillRect (r.removeFromRight (1));
#endif


    const float alpha = button.isEnabled() ? ((isMouseOver || isMouseDown) ? 1.0f : 0.8f) : 0.3f;
    
    Colour col (bkg.contrasting().withMultipliedAlpha (alpha));
    
    if (TabbedButtonBar* bar = button.findParentComponentOfClass<TabbedButtonBar>())
    {
        TabbedButtonBar::ColourIds colID = button.isFrontTab() ? TabbedButtonBar::frontTextColourId
        : TabbedButtonBar::tabTextColourId;
        
        if (bar->isColourSpecified (colID))
            col = bar->findColour (colID);
        else if (isColourSpecified (colID))
            col = findColour (colID);
    }
    
    const Rectangle<float> area (button.getTextArea().toFloat());
    
    float length = area.getWidth();
    float depth  = area.getHeight();
    
    if (button.getTabbedButtonBar().isVertical())
        std::swap (length, depth);
    
    TextLayout textLayout;
    createTabTextLayout (button, length, depth, col, textLayout);
    
    AffineTransform t;
    
    switch (o)
    {
        case TabbedButtonBar::TabsAtLeft:   t = t.rotated (float_Pi * -0.5f).translated (area.getX(), area.getBottom()); break;
        case TabbedButtonBar::TabsAtRight:  t = t.rotated (float_Pi *  0.5f).translated (area.getRight(), area.getY()); break;
        case TabbedButtonBar::TabsAtTop:
        case TabbedButtonBar::TabsAtBottom: t = t.translated (area.getX(), area.getY()); break;
        default:                            jassertfalse; break;
    }
    
    g.addTransform (t);
    textLayout.draw (g, Rectangle<float> (length, depth));
}

/*
void SonoLookAndFeel::drawTabButton (TabBarButton& button, Graphics& g, bool isMouseOver, bool isMouseDown)
{
    const Rectangle<int> activeArea (button.getActiveArea());

    const Colour bkg (getTabBackgroundColour (button));

    g.setGradientFill (ColourGradient (bkg.brighter (0.1f), 0, (float) activeArea.getY(),
                                       bkg.darker (0.1f), 0, (float) activeArea.getBottom(), false));
    g.fillRect (activeArea);

    g.setColour (button.findColour (TabbedComponent::backgroundColourId).darker (0.3f));
    g.drawRect (activeArea);

    const float alpha = button.isEnabled() ? ((isMouseOver || isMouseDown) ? 1.0f : 0.8f) : 0.3f;
    const Colour col (bkg.contrasting().withMultipliedAlpha (alpha));

    TextLayout textLayout;
    LookAndFeel_V3::createTabTextLayout (button, (float) activeArea.getWidth(), (float) activeArea.getHeight(), col, textLayout);

    textLayout.draw (g, button.getTextArea().toFloat());
}
*/

void SonoLookAndFeel::drawTabbedButtonBarBackground (TabbedButtonBar&, Graphics&) {}

void SonoLookAndFeel::drawTabAreaBehindFrontButton (TabbedButtonBar& bar, Graphics& g, const int w, const int h)
{
    const float shadowSize = 0.15f;
    
    Rectangle<int> shadowRect, line;
    ColourGradient gradient (Colours::black.withAlpha (bar.isEnabled() ? 0.08f : 0.04f), 0, 0,
                             Colours::transparentBlack, 0, 0, false);
    
    switch (bar.getOrientation())
    {
        case TabbedButtonBar::TabsAtLeft:
            gradient.point1.x = (float) w;
            gradient.point2.x = w * (1.0f - shadowSize);
            shadowRect.setBounds ((int) gradient.point2.x, 0, w - (int) gradient.point2.x, h);
            line.setBounds (w - 1, 0, 1, h);
            break;
            
        case TabbedButtonBar::TabsAtRight:
            gradient.point2.x = w * shadowSize;
            shadowRect.setBounds (0, 0, (int) gradient.point2.x, h);
            line.setBounds (0, 0, 1, h);
            break;
            
        case TabbedButtonBar::TabsAtTop:
            gradient.point1.y = (float) h;
            gradient.point2.y = h * (1.0f - shadowSize);
            shadowRect.setBounds (0, (int) gradient.point2.y, w, h - (int) gradient.point2.y);
            line.setBounds (0, h - 1, w, 1);
            break;
            
        case TabbedButtonBar::TabsAtBottom:
            gradient.point2.y = h * shadowSize;
            shadowRect.setBounds (0, 0, w, (int) gradient.point2.y);
            line.setBounds (0, 0, w, 1);
            break;
            
        default: break;
    }
    
    g.setGradientFill (gradient);
    g.fillRect (shadowRect.expanded (2, 2));
    
    g.setColour (bar.findColour (TabbedButtonBar::tabOutlineColourId));
    g.fillRect (line);
}

void SonoLookAndFeel::drawTabButtonText (TabBarButton& button, Graphics& g, bool isMouseOver, bool isMouseDown)
{
    const Rectangle<float> area (button.getTextArea().toFloat());
    
    DebugLogC("Sono look and feel drawtabbutton text: %s", button.getButtonText().toRawUTF8());
    
    float length = area.getWidth();
    float depth  = area.getHeight();
    
    if (button.getTabbedButtonBar().isVertical())
        std::swap (length, depth);
    
    Font font  = myFont.withHeight(jmin(depth,30.0f) * 0.6f * fontScale);
    font.setUnderline (button.hasKeyboardFocus (false));
    
    AffineTransform t;
    
    switch (button.getTabbedButtonBar().getOrientation())
    {
        case TabbedButtonBar::TabsAtLeft:   t = t.rotated (float_Pi * -0.5f).translated (area.getX(), area.getBottom()); break;
        case TabbedButtonBar::TabsAtRight:  t = t.rotated (float_Pi *  0.5f).translated (area.getRight(), area.getY()); break;
        case TabbedButtonBar::TabsAtTop:
        case TabbedButtonBar::TabsAtBottom: t = t.translated (area.getX(), area.getY()); break;
        default:                            jassertfalse; break;
    }
    
    Colour col;
    
    if (button.isFrontTab() && (button.isColourSpecified (TabbedButtonBar::frontTextColourId)
                                || isColourSpecified (TabbedButtonBar::frontTextColourId)))
        col = findColour (TabbedButtonBar::frontTextColourId);
    else if (button.isColourSpecified (TabbedButtonBar::tabTextColourId)
             || isColourSpecified (TabbedButtonBar::tabTextColourId))
        col = findColour (TabbedButtonBar::tabTextColourId);
    else
        col = button.getTabBackgroundColour().contrasting();
    
    const float alpha = button.isEnabled() ? ((isMouseOver || isMouseDown) ? 1.0f : 0.8f) : 0.3f;
    
    g.setColour (col.withMultipliedAlpha (alpha));
    g.setFont (font);
    g.addTransform (t);
    
    g.drawFittedText (button.getButtonText().trim(),
                      0, 0, (int) length, (int) depth,
                      Justification::centred,
                      //jmax (1, ((int) depth) / 12), 0.5f);
                      1, 0.5f);
}

static Range<float> getBrightnessRange (const Image& im)
{
    float minB = 1.0f, maxB = 0;
    const int w = im.getWidth();
    const int h = im.getHeight();

    for (int y = 0; y < h; ++y)
    {
        for (int x = 0; x < w; ++x)
        {
            const float b = im.getPixelAt (x, y).getBrightness();
            minB = jmin (minB, b);
            maxB = jmax (maxB, b);
        }
    }

    return Range<float> (minB, maxB);
}

Font SonoLookAndFeel::getLabelFont (Label& label)
{
    if (fontScale == 1.0f) {
        return label.getFont();
    }
    else {
        return label.getFont().withHeight(label.getFont().getHeight() * fontScale);        
    }
}

void SonoLookAndFeel::drawLabel (Graphics& g, Label& label)
{
    Colour olcolor = label.findColour (Label::backgroundColourId);

    g.setColour(olcolor);
    if (!olcolor.isTransparent()) {
        if (labelCornerRadius > 0.0f) {
            g.fillRoundedRectangle(label.getLocalBounds().toFloat(), labelCornerRadius);
        } else {
            g.fillAll (olcolor);
        }
    }
    
    
    if (! label.isBeingEdited())
    {
        auto alpha = label.isEnabled() ? 1.0f : 0.5f;
        const Font font (getLabelFont (label));
        
        g.setColour (label.findColour (Label::textColourId).withMultipliedAlpha (alpha));
        g.setFont (font);
        
        auto textArea = getLabelBorderSize (label).subtractedFrom (label.getLocalBounds());
        
        g.drawFittedText (label.getText(), textArea, label.getJustificationType(),
                          jmax (1, (int) (textArea.getHeight() / font.getHeight())),
                          label.getMinimumHorizontalScale());
        
        olcolor = label.findColour (Label::outlineColourId).withMultipliedAlpha (alpha);
    }
    else if (label.isEnabled())
    {
        olcolor = label.findColour (Label::outlineColourId);
    }
    
    if (!olcolor.isTransparent()) {
        g.setColour (olcolor);
        if (labelCornerRadius > 0.0f) {
            g.drawRoundedRectangle(label.getLocalBounds().toFloat(), labelCornerRadius, 1.0f);
        } else {
            g.drawRect (label.getLocalBounds());        
        }
    }
}

Font SonoLookAndFeel::getTextButtonFont (TextButton& button, int buttonHeight)
{
    // DebugLogC("GetTextButton font with height: %d", buttonHeight);
    float textRatio = 0.5f;
    if (SonoTextButton* const textbutt = dynamic_cast<SonoTextButton*> (&button)) {
        textRatio = textbutt->getTextHeightRatio();
    }
    
    return myFont.withHeight(jmin (16.0f, buttonHeight * textRatio) * fontScale);
}

Button* SonoLookAndFeel::createSliderButton (Slider&, const bool isIncrement)
{
    TextButton * butt = new TextButton (isIncrement ? "+" : "-", {});
    return butt;
}

Label* SonoLookAndFeel::createSliderTextBox (Slider& slider)
{
    Label * lab = LookAndFeel_V4::createSliderTextBox(slider);
    lab->setKeyboardType(TextInputTarget::decimalKeyboard);
    lab->setFont(myFont.withHeight(16.0* fontScale));
    lab->setMinimumHorizontalScale(0.5);
    lab->setJustificationType(Justification::centredRight);
    return lab;
}

Font SonoLookAndFeel::getSliderPopupFont (Slider&)
{
    return Font (18.0f, Font::bold);
}

int SonoLookAndFeel::getSliderPopupPlacement (Slider&)
{
    return BubbleComponent::above
    //| BubbleComponent::below
    | BubbleComponent::left
    | BubbleComponent::right
    ;
}

void SonoLookAndFeel::drawButtonTextWithAlignment (Graphics& g, TextButton& button, bool /*isMouseOverButton*/, bool /*isButtonDown*/, Justification textjust)
{
    Font font (getTextButtonFont (button, button.getHeight()));
    g.setFont (font);
    g.setColour (button.findColour (button.getToggleState() ? TextButton::textColourOnId
                                    : TextButton::textColourOffId)
                 .withMultipliedAlpha (button.isEnabled() ? 1.0f : 0.5f));
    
    float textRatio = 0.7f;
    if (SonoTextButton* const textbutt = dynamic_cast<SonoTextButton*> (&button)) {
        textRatio = textbutt->getTextHeightRatio();
    }
    
    const int yIndent = jmin (2, button.proportionOfHeight ((1.0 - textRatio) * 0.5));
    const int cornerSize = jmin (button.getHeight(), button.getWidth()) / 2;
    
    const int fontHeight = roundToInt (font.getHeight() * 0.3);
    const int leftIndent  = jmin (fontHeight, 2 + cornerSize / (button.isConnectedOnLeft() ? 4 : 2));
    const int rightIndent = jmin (fontHeight, 2 + cornerSize / (button.isConnectedOnRight() ? 4 : 2));
    
    g.drawFittedText (button.getButtonText(),
                      leftIndent,
                      yIndent,
                      button.getWidth() - leftIndent - rightIndent,
                      button.getHeight() - yIndent * 2,
                      textjust, 2, 0.7f);
}


void SonoLookAndFeel::drawButtonText (Graphics& g, TextButton& button, bool isMouseOverButton, bool isButtonDown)
{
    drawButtonTextWithAlignment(g, button, isMouseOverButton, isButtonDown);
}


void SonoLookAndFeel::drawFileBrowserRow (Graphics& g, int width, int height,
                                         const File& file, const String& filename, Image* icon,
                                         const String& fileSizeDescription,
                                         const String& fileTimeDescription,
                                         bool isDirectory, bool isItemSelected,
                                         int itemIndex, DirectoryContentsDisplayComponent& dcc)
{
    Component* const fileListComp = dynamic_cast<Component*> (&dcc);

    if (isItemSelected)
        g.fillAll (fileListComp != nullptr ? fileListComp->findColour (DirectoryContentsDisplayComponent::highlightColourId)
                   : findColour (DirectoryContentsDisplayComponent::highlightColourId));

    int x = 32;
    g.setColour (Colours::black);

    if (isDirectory) {
        if (icon != nullptr && icon->isValid())
        {
            g.drawImageWithin (*icon, 2, 2, x - 4, height - 4,
                               RectanglePlacement::centred | RectanglePlacement::onlyReduceInSize,
                               false);
        }
        else
        {
            if (const Drawable* d = isDirectory ? getDefaultFolderImage()
                : getDefaultDocumentFileImage())
                d->drawWithin (g, Rectangle<float> (2.0f, 2.0f, x - 4.0f, height - 4.0f),
                               RectanglePlacement::centred | RectanglePlacement::onlyReduceInSize, 1.0f);
        }
    }
    else {
        x = 4;
    }

    g.setColour (fileListComp != nullptr ? fileListComp->findColour (DirectoryContentsDisplayComponent::textColourId)
                 : findColour (DirectoryContentsDisplayComponent::textColourId));
    g.setFont (myFont.withHeight(height * 0.5f));

    if (width > 450 && ! isDirectory)
    {
        const int sizeX = roundToInt (width * 0.7f);
        const int dateX = roundToInt (width * 0.8f);

        g.drawFittedText (filename,
                          x, 0, sizeX - x, height,
                          Justification::centredLeft, 1);

        g.setFont (myFont.withHeight(height * 0.5f));
        g.setColour (Colours::darkgrey);

        if (! isDirectory)
        {
            g.drawFittedText (fileSizeDescription,
                              sizeX, 0, dateX - sizeX - 8, height,
                              Justification::centredRight, 1);

            g.drawFittedText (fileTimeDescription,
                              dateX, 0, width - 8 - dateX, height,
                              Justification::centredRight, 1);
        }
    }
    else
    {
        g.drawFittedText (filename,
                          x, 0, width - x, height,
                          Justification::centredLeft, 1);

    }
}

Button* SonoLookAndFeel::createFileBrowserGoUpButton()
{
    DrawableButton* goUpButton = new DrawableButton ("up", DrawableButton::ImageOnButtonBackground);

    Path arrowPath;
    arrowPath.addArrow (Line<float> (50.0f, 100.0f, 50.0f, 0.0f), 40.0f, 100.0f, 50.0f);

    DrawablePath arrowImage;
    arrowImage.setFill (Colours::white.withAlpha (0.4f));
    arrowImage.setPath (arrowPath);

    goUpButton->setImages (&arrowImage);

    return goUpButton;
}

void SonoLookAndFeel::layoutFileBrowserComponent (FileBrowserComponent& browserComp,
                                                 DirectoryContentsDisplayComponent* fileListComponent,
                                                 FilePreviewComponent* previewComp,
                                                 ComboBox* currentPathBox,
                                                 TextEditor* filenameBox,
                                                 Button* goUpButton)
{
    const int x = 8;
    int w = browserComp.getWidth() - x - x;

    if (previewComp != nullptr)
    {
        const int previewWidth = w / 3;
        previewComp->setBounds (x + w - previewWidth, 0, previewWidth, browserComp.getHeight());

        w -= previewWidth + 4;
    }

    int y = 4;

    const int controlsHeight = 22;
    const int bottomSectionHeight = controlsHeight + 8;
    const int upButtonWidth = 50;

    currentPathBox->setBounds (x, y, w - upButtonWidth - 6, controlsHeight);
    goUpButton->setBounds (x + w - upButtonWidth, y, upButtonWidth, controlsHeight);

    y += controlsHeight + 4;

    if (Component* const listAsComp = dynamic_cast <Component*> (fileListComponent))
    {
        listAsComp->setBounds (x, y, w, browserComp.getHeight() - y - bottomSectionHeight);
        y = listAsComp->getBottom() + 4;
    }

    filenameBox->setBounds (x + 50, y, w - 50, controlsHeight);
}


void SonoLookAndFeel::drawTreeviewPlusMinusBox (Graphics& g, const Rectangle<float>& area,
                                               Colour backgroundColour, bool isOpen, bool isMouseOver)
{
    Path p;
    p.addTriangle (0.0f, 0.0f, 1.0f, isOpen ? 0.0f : 0.5f, isOpen ? 0.5f : 0.0f, 1.0f);

    DebugLogC("draw plus minus ours");

    //g.setColour (backgroundColour.contrasting().withAlpha (isMouseOver ? 0.5f : 0.3f));
    g.setColour (Colours::white.withAlpha (isMouseOver ? 0.5f : 0.3f));
    g.fillPath (p, p.getTransformToScaleToFit (area.reduced (2, area.getHeight() / 4), true));
}


void SonoLookAndFeel::drawToggleButton (Graphics& g, ToggleButton& button,
                                       bool isMouseOverButton, bool isButtonDown)
{
    /*
    if (button.hasKeyboardFocus (true))
    {
        g.setColour (button.findColour (TextEditor::focusedOutlineColourId));
        g.drawRect (0, 0, button.getWidth(), button.getHeight());
    }
     */

    float fontSize = jmin (15.0f, button.getHeight() * 0.75f) * fontScale;
    const float tickWidth = fontSize * 1.1f;

    drawTickBox (g, button, 4.0f, (button.getHeight() - tickWidth) * 0.5f,
                 tickWidth, tickWidth,
                 button.getToggleState(),
                 button.isEnabled(),
                 isMouseOverButton,
                 isButtonDown);

    g.setColour (button.findColour (ToggleButton::textColourId));
    g.setFont (myFont.withHeight(fontSize));

    if (! button.isEnabled())
        g.setOpacity (0.5f);

    const int textX = (int) tickWidth + 10;

    g.drawFittedText (button.getButtonText(),
                      textX, 0,
                      button.getWidth() - textX - 2, button.getHeight(),
                      Justification::centredLeft, 10);
}

void SonoLookAndFeel::drawTickBox (Graphics& g, Component& component,
                                  float x, float y, float w, float h,
                                  const bool ticked,
                                  const bool isEnabled,
                                  const bool isMouseOverButton,
                                  const bool isButtonDown)
{
    const float boxSize = w * 1.0f;


    g.setColour (component.findColour (TextEditor::focusedOutlineColourId));
    g.drawRect (x, y + (h - boxSize) * 0.5f, boxSize, boxSize);

    if (ticked)
    {
        Path tick;
        tick.startNewSubPath (1.5f, 3.0f);
        tick.lineTo (3.0f, 6.0f);
        tick.lineTo (6.0f, 0.0f);

        
        g.setColour (isEnabled ? component.findColour(ToggleButton::tickColourId) : Colours::grey);

        const AffineTransform trans (AffineTransform::scale (w / 9.0f, h / 9.0f)
                                     .translated (x+2, y+1));

        g.strokePath (tick, PathStrokeType (2.5f), trans);
    }
}

void SonoLookAndFeel::drawRotarySlider (Graphics& g, int x, int y, int width, int height, float sliderPos,
                                       const float rotaryStartAngle, const float rotaryEndAngle, Slider& slider)
{
    const auto outline = findColour (Slider::rotarySliderOutlineColourId);
    const auto fill    = findColour (Slider::rotarySliderFillColourId);
    
    const auto bounds = Rectangle<int> (x, y, width, height).toFloat().reduced (3);
    
    
    auto radius = jmin (bounds.getWidth(), bounds.getHeight()) / 2.0f;
    const auto toAngle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);
    auto lineW = jmin (8.0f, radius * 0.3f);
    auto arcRadius = radius - lineW * 0.5f;
    
    Path backgroundArc;
    backgroundArc.addCentredArc (bounds.getCentreX(),
                                 bounds.getCentreY(),
                                 arcRadius,
                                 arcRadius,
                                 0.0f,
                                 rotaryStartAngle,
                                 rotaryEndAngle,
                                 true);
    
    g.setColour (outline);
    g.strokePath (backgroundArc, PathStrokeType (lineW, PathStrokeType::curved, PathStrokeType::rounded));
    
    auto rotStartAngle = rotaryStartAngle;
    
    if (slider.getProperties().contains ("fromCentre"))
    {
        rotStartAngle = (rotStartAngle + rotaryEndAngle) / 2;
    }

    
    if (slider.isEnabled())
    {
        Path valueArc;
        valueArc.addCentredArc (bounds.getCentreX(),
                                bounds.getCentreY(),
                                arcRadius,
                                arcRadius,
                                0.0f,
                                rotStartAngle,
                                toAngle,
                                true);
        
        g.setColour (fill);
        g.strokePath (valueArc, PathStrokeType (lineW, PathStrokeType::curved, PathStrokeType::rounded));
    }
    
    const auto thumbWidth = lineW * 1.5f;
    const Point<float> thumbPoint (bounds.getCentreX() + arcRadius * std::cos (toAngle - float_Pi * 0.5f),
                                   bounds.getCentreY() + arcRadius * std::sin (toAngle - float_Pi * 0.5f));
    
    g.setColour (findColour (Slider::thumbColourId));
    g.fillEllipse (Rectangle<float> (thumbWidth, thumbWidth).withCentre (thumbPoint));
}

void SonoLookAndFeel::drawLinearSlider (Graphics& g, int x, int y, int width, int height,
                                       float sliderPos,
                                       float minSliderPos,
                                       float maxSliderPos,
                                       const Slider::SliderStyle style, Slider& slider)
{
    if (slider.isBar())
    {
        g.setColour (slider.findColour (Slider::trackColourId));
        if (slider.getProperties().contains ("fromCentre")) {
            auto centrex = x + width*0.5f;
            auto centrey = y + height*0.5f;
            
            if (!slider.getProperties().contains ("noFill")) {
                g.fillRect (slider.isHorizontal() ? Rectangle<float> (sliderPos > centrex ? centrex : sliderPos, y + 0.5f, sliderPos > centrex ? sliderPos - centrex : centrex - sliderPos, height - 1.0f)
                            : Rectangle<float> (x + 0.5f, sliderPos < centrey ? sliderPos : centrey, width - 1.0f, sliderPos < centrey ?  centrey - sliderPos : sliderPos - centrey));
            }
            
            // draw line
            g.fillRect (slider.isHorizontal() ? Rectangle<float> (sliderPos - 1, y + 0.5f, 2, height - 1.0f)
                        : Rectangle<float> (x + 0.5f, sliderPos - 1, width - 1.0f, 2));
        }
        else {
            g.fillRect (slider.isHorizontal() ? Rectangle<float> (static_cast<float> (x), y + 0.5f, sliderPos - x, height - 1.0f)
                        : Rectangle<float> (x + 0.5f, sliderPos, width - 1.0f, y + (height - sliderPos)));            
        }
    }
    else
    {
        auto isTwoVal   = (style == Slider::SliderStyle::TwoValueVertical   || style == Slider::SliderStyle::TwoValueHorizontal);
        auto isThreeVal = (style == Slider::SliderStyle::ThreeValueVertical || style == Slider::SliderStyle::ThreeValueHorizontal);
        
        auto trackWidth = jmin (10.0f, slider.isHorizontal() ? height * 0.25f : width * 0.25f);
        
        Point<float> startPoint (slider.isHorizontal() ? x : x + width * 0.5f,
                                 slider.isHorizontal() ? y + height * 0.5f : height + y);
        
        Point<float> endPoint (slider.isHorizontal() ? width + x : startPoint.x,
                               slider.isHorizontal() ? startPoint.y : y);
        
        Path backgroundTrack;
        backgroundTrack.startNewSubPath (startPoint);
        backgroundTrack.lineTo (endPoint);
        g.setColour (slider.findColour (Slider::backgroundColourId));
        g.strokePath (backgroundTrack, { trackWidth, PathStrokeType::curved, PathStrokeType::rounded });
        
        Path valueTrack;
        Point<float> minPoint, maxPoint, thumbPoint;
        
        if (isTwoVal || isThreeVal)
        {
            minPoint = { slider.isHorizontal() ? minSliderPos : width * 0.5f,
                slider.isHorizontal() ? height * 0.5f : minSliderPos };
            
            if (isThreeVal)
                thumbPoint = { slider.isHorizontal() ? sliderPos : width * 0.5f,
                    slider.isHorizontal() ? height * 0.5f : sliderPos };
            
            maxPoint = { slider.isHorizontal() ? maxSliderPos : width * 0.5f,
                slider.isHorizontal() ? height * 0.5f : maxSliderPos };
        }
        else
        {
            auto kx = slider.isHorizontal() ? sliderPos : (x + width * 0.5f);
            auto ky = slider.isHorizontal() ? (y + height * 0.5f) : sliderPos;
            
            minPoint = startPoint;
            maxPoint = { kx, ky };
        }
        
        auto thumbWidth = getSliderThumbRadius (slider);
        
        valueTrack.startNewSubPath (minPoint);
        valueTrack.lineTo (isThreeVal ? thumbPoint : maxPoint);
        g.setColour (slider.findColour (Slider::trackColourId));
        g.strokePath (valueTrack, { trackWidth, PathStrokeType::curved, PathStrokeType::rounded });
        
        if (! isTwoVal)
        {
            g.setColour (slider.findColour (Slider::thumbColourId));
            g.fillEllipse (Rectangle<float> (static_cast<float> (thumbWidth), static_cast<float> (thumbWidth)).withCentre (isThreeVal ? thumbPoint : maxPoint));
        }
        
        if (isTwoVal || isThreeVal)
        {
            auto sr = jmin (trackWidth, (slider.isHorizontal() ? height : width) * 0.4f);
            auto pointerColour = slider.findColour (Slider::thumbColourId);
            
            auto wscale = 2.0f;
            
            if (slider.isHorizontal())
            {
                drawPointer (g, minSliderPos - sr,
                             height * 0.5f - trackWidth*wscale*0.5, //jmax (0.0f, y + height * 0.5f - trackWidth * 0.5f),
                             trackWidth * wscale, pointerColour, 1); // 2
                
                drawPointer (g, maxSliderPos - trackWidth,
                             height * 0.5f - trackWidth*wscale*0.5, //jmin (y + height - trackWidth * 2.0f, (float)y + height * 0.5f),
                             trackWidth * wscale, pointerColour, 3); // 4
            }
            else
            {
                drawPointer (g, jmax (0.0f, x + width * 0.5f - trackWidth * 2.0f),
                             minSliderPos - trackWidth,
                             trackWidth * wscale, pointerColour, 1);
                
                drawPointer (g, jmin (x + width - trackWidth * 2.0f, x + width * 0.5f), maxSliderPos - sr,
                             trackWidth * wscale, pointerColour, 3);
            }
        }
    }
}


void SonoLookAndFeel::drawDrawableButton (Graphics& g, DrawableButton& button,
                                         bool isMouseOverButton, bool isButtonDown)
{
    const auto cornerSize = 6.0f;
    bool toggleState = button.getToggleState();
    
    ;

    Rectangle<float> bounds = g.getClipBounds().toFloat();

    g.setColour(button.findColour (toggleState ? DrawableButton::backgroundOnColourId
                                   : DrawableButton::backgroundColourId));
    g.fillRoundedRectangle(bounds, cornerSize);
    
    if (isButtonDown) {
        g.setColour(findColour(SonoDrawableButton::downOverlayColourId));
        g.fillRoundedRectangle(bounds, cornerSize);
    }
    else if (isMouseOverButton) {
        g.setColour(findColour(SonoDrawableButton::overOverlayColourId));
        g.fillRoundedRectangle(bounds, cornerSize);
    }
    
    //g.fillAll (button.findColour (toggleState ? DrawableButton::backgroundOnColourId
    //                              : DrawableButton::backgroundColourId));
    
    const int textH = (button.getStyle() == DrawableButton::ImageAboveTextLabel)
    ? jmin (14, button.proportionOfHeight (0.2f))
    : 0;
    
    if (textH > 0)
    {
        g.setFont (myFont.withHeight((float) textH * fontScale));
        
        g.setColour (button.findColour (toggleState ? DrawableButton::textColourOnId
                                        : DrawableButton::textColourId)
                     .withMultipliedAlpha (button.isEnabled() ? 1.0f : 0.4f));
        
        g.drawFittedText (button.getButtonText(),
                          2, button.getHeight() - textH - 1,
                          button.getWidth() - 4, textH,
                          Justification::centred, 1);
    }
}


void SonoLookAndFeel::drawBubble (Graphics& g, BubbleComponent& comp,
                                 const Point<float>& tip, const Rectangle<float>& body)
{
    Path p;
    p.addBubble (body.reduced (0.5f), body.getUnion (Rectangle<float> (tip.x, tip.y, 1.0f, 1.0f)),
                 tip, 5.0f, jmin (10.0f, body.getWidth() * 0.2f, body.getHeight() * 0.2f));
    
    g.setColour (comp.findColour (BubbleComponent::backgroundColourId));
    g.fillPath (p);
    
    g.setColour (comp.findColour (BubbleComponent::outlineColourId));
    g.strokePath (p, PathStrokeType (1.0f));
}


SonoBigTextLookAndFeel::SonoBigTextLookAndFeel(float maxTextSize)
: maxSize(maxTextSize)
{
    
}

Font SonoBigTextLookAndFeel::getTextButtonFont (TextButton& button, int buttonHeight)
{
    // DebugLogC("GetTextButton font with height: %d  maxsize: %g", buttonHeight, maxSize);
    float textRatio = 0.8f;
       if (SonoTextButton* const textbutt = dynamic_cast<SonoTextButton*> (&button)) {
           textRatio = textbutt->getTextHeightRatio();
       }
    
    return myFont.withHeight(jmin (maxSize, buttonHeight * textRatio) * fontScale);
}

Label* SonoBigTextLookAndFeel::createSliderTextBox (Slider& slider)
{
    Label * lab = LookAndFeel_V4::createSliderTextBox(slider);
    lab->setFont(myFont.withHeight(maxSize * fontScale));
    lab->setJustificationType(Justification::centred);
    return lab;
}

Button* SonoBigTextLookAndFeel::createSliderButton (Slider&, const bool isIncrement)
{
    TextButton * butt = new TextButton (isIncrement ? "+" : "-", {});
    butt->setLookAndFeel(this);
    return butt;
}




#endif
