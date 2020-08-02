
#pragma once

#include <JuceHeader.h>

class JitterBufferMeter : public Component
{
public:
    JitterBufferMeter();
    ~JitterBufferMeter();

    void paint (Graphics&) override;
    void resized() override;

    void setRecvMode(bool recvmode);
    void setFillRatio (float ratio, float stdev);
    
private:

    bool  _recvmode = true;
    float _ratio = 0.0f;
    float _stdev = 0.0f;
    
    Colour  jitterColor;
    Colour  barColor;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (JitterBufferMeter)
};
