#pragma once

#include "RunCumulantor.h"

typedef enum measurementStates {
    measure_average_loudness_for_1_sec,
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

    float maxSecsToWait = 3.0f;
    float minWaitTime = 0.4f;
    int measurementCount = 10;

    LatencyMeasurer();
    
    // skip listening phase if we know about the noise floor (or lack thereof) already
    void initializeWithThreshold(float threshDb);
    
    void processInput(float *audio, int samplerate, int numberOfSamples);
    void processOutput(float *audio);
    void toggle();
    void togglePassThrough();

    
    
private:
    
    stats::RunCumulantor1D  smoothedLatency; // ms

    measurementStates measurementState, nextMeasurementState;
    //float roundTripLatencyMs[10],
    float sineWave, rampdec;
    float sum;
    int samplesElapsed;
    float threshold;
    bool skipMeasure = false;
};

