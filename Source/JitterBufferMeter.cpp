

#include "JitterBufferMeter.h"

JitterBufferMeter::JitterBufferMeter() {
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
    
    g.fillAll(Colours::black);

    float fwidth = (float)width * _ratio;
    float edgewidth = jmax(2.0f, width * 2.0f * _stdev);
    Rectangle<float> fillbox(0.0f, 0.0f, fwidth, height);
    // edge whose thickness uses the std deviation
    Rectangle<float> edgebox(fwidth - 0.5f*edgewidth, 0.0f, edgewidth, height);

    float goodness = _recvmode ? _ratio : (1.0f - _ratio);

    Colour fillColor = Colour::fromHSV(0.33f*goodness, 1.0f, 0.5f, 1.0f);
    
    g.setColour(fillColor.darker());
    g.fillRect(fillbox);

    g.setColour(fillColor);
    g.fillRect(edgebox);
    
}

void JitterBufferMeter::resized()
{

}
