#pragma once

#include "RunCumulantor.h"

#include "JuceHeader.h"

typedef enum measurementStates {
    measure_average_loudness,
    playing_and_listening,
    waiting,
    passthrough,
    idle
} measurementStates;

class LatencyMeasurer {
public:
    int state; // -1: passthrough, 0: idle, 1..10 taking measurement steps, 11 finished
    int samplerate;
    float latencyMs;
    int buffersize;

    float noiseMeasureTime = 0.2f;
    float maxSecsToWait = 3.0f;
    float minWaitTime = 0.6f;
    float overrideThreshold = 0.0f; // no override at 0
    
    int measurementCount = 10;
    bool measureOnlyOnce = true;
    
    bool usePulseData = true;
    
    float sinPulseTime = 0.1f;

    LatencyMeasurer();
    
    // skip listening phase if we know about the noise floor (or lack thereof) already
    void initializeWithThreshold(float threshDb);
    
    void processInput(float *audio, int samplerate, int numberOfSamples);
    void processOutput(float *audio);
    void toggle(bool forcestart=false);
    void togglePassThrough();

    
    
private:
    
    void readAudioFromBinaryData(AudioSampleBuffer& buffer, const void* data, size_t sizeBytes);

    
    stats::RunCumulantor1D  smoothedLatency; // ms

    AudioSampleBuffer pulseDataBuffer;
    int pulsePos = 0;
    
    measurementStates measurementState, nextMeasurementState;
    //float roundTripLatencyMs[10],
    float sineWave, rampdec = -1.0f;
    float sum;
    int samplesElapsed;
    float threshold;
    bool skipMeasure = false;
    bool playing = false;
};

