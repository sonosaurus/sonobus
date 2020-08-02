

#include "JitterBufferMeter.h"

JitterBufferMeter::JitterBufferMeter()
{
    jitterColor = Colour::fromHSV(0.9f, 0.3f, 0.4f, 1.0f);
    barColor = jitterColor.withAlpha(0.7f);
    //fixedColor = Colour::fromFloatRGBA(0.6f, 0.2f, 0.6f, 1.0f);
}

JitterBufferMeter::~JitterBufferMeter()
{
}

void JitterBufferMeter::setRecvMode(bool recvmode)
{
    if (_recvmode != recvmode) {
        _recvmode = recvmode;
        repaint();
    }
}

void JitterBufferMeter::setFillRatio (float ratio, float stdev)
{
    if (fabsf(ratio - _ratio) > 0.005f || fabsf(stdev - _stdev) > 0.001f) {
        _ratio = ratio;
        _stdev = stdev;
        repaint();
    }
}


void JitterBufferMeter::paint (Graphics& g)
{
    int width = getWidth();
    int height = getHeight();
    if (width <= 0 || height <= 0) return;
    
    int radius = 2;
    
    g.setColour(Colours::black);
    g.fillRoundedRectangle(0, 0, width, height, radius);
    
    //g.fillAll(Colours::black);

    float fwidth = (float)width * _ratio;
    float edgewidth = jmax(2.0f, width * 2.0f * _stdev);
    Rectangle<float> fillbox(0.0f, 0.0f, fwidth, height);
    fillbox.reduce(1.0f, 1.0f);
    // edge whose thickness uses the std deviation
    Rectangle<float> edgebox(fwidth - edgewidth, 0.0f, edgewidth, height);
    edgebox.reduce(0.0f, 1.0f);
    
    if (edgebox.getRight() >= width) {
        edgebox.translate(width - edgebox.getRight(), 0.0f);
    } else if (edgebox.getX() <= 0) {
        edgebox.translate(-edgebox.getX(), 0.0f);
    }
    
    float goodness = _recvmode ? _ratio : (1.0f - _ratio);
    
    //Colour fillColor = fixedColor; //  Colour::fromHSV(0.33f*goodness, 1.0f, 0.5f, 1.0f);
    
    g.setColour(barColor);
    g.fillRoundedRectangle(fillbox, radius);
    //g.fillRect(fillbox);

    g.setColour(jitterColor);
    g.fillRoundedRectangle(edgebox, radius);
    //g.fillRect(edgebox);
    
}

void JitterBufferMeter::resized()
{

}
