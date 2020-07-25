#include "LatencyMeasurer.h"
#include <cmath>
#include <cstdlib>
#include <string>

/*
 Cross-platform class measuring round-trip audio latency.
 How one measurement step works:
 - Listen and measure the average loudness of the environment for 1 second.
 - Create a threshold value 24 decibels higher than the average loudness.
 - Begin playing a 1000 Hz sine wave and start counting the samples elapsed.
 - Stop counting and playing if the input's loudness is higher than the threshold, as the output wave is coming back (probably).
 - Divide the the elapsed samples with the sample rate to get the round-trip audio latency value in seconds.
 - We expect the threshold exceeded within 1 second. If it did not, then fail with error. Usually happens when the environment is too noisy (loud).
 How the measurement process works:
 - Perform 10 measurement steps.
 - Repeat every step until it returns without an error.
 - Store the results in an array of 10 floats.
 - After each step, check the minimum and maximum values.
 - If the maximum is higher than the minimum's double, stop the measurement process with an error. It indicates an unknown error, perhaps an unwanted noise happened. Double jitter (dispersion) is too high, an audio system can not be so bad.
*/

// Returns with the absolute sum of the audio.
static float sumAudio(float *audio, int numberOfSamples) {
    float sum = 0;
    while (numberOfSamples) {
        numberOfSamples--;
        sum += abs(audio[0]);
        audio += 1;
    };
    return sum;
}

LatencyMeasurer::LatencyMeasurer() : state(0), samplerate(0), latencyMs(0), buffersize(0), measurementState(idle), nextMeasurementState(idle),  sineWave(0), sum(0) ,samplesElapsed(0),threshold(0)  {
}

void LatencyMeasurer::toggle() {
    if ((state == -1) || ((state > 0) && (state < 11))) { // stop
        state = 0;
        nextMeasurementState = idle;
    } else { // start
        state = 1;
        samplerate = latencyMs = buffersize = 0;
        skipMeasure = false;
        smoothedLatency.reset();
        nextMeasurementState = measure_average_loudness_for_1_sec;
    };
}

void LatencyMeasurer::togglePassThrough() {
    if (state != -1) {
        state = -1;
        nextMeasurementState = passthrough;
    } else {
        state = 0;
        nextMeasurementState = idle;
    };
}

void LatencyMeasurer::initializeWithThreshold(float threshDb)
{
    state = 1;
    samplerate = latencyMs = buffersize = 0;

    skipMeasure = true;

    // Look for an audio energy rise of 24 decibel.
    float referenceDecibel = threshDb + 24.0f;

    threshold = (powf(10.0f, referenceDecibel / 20.0f));

    smoothedLatency.reset();
    
    measurementState = nextMeasurementState = playing_and_listening;
    sineWave = 0;
    samplesElapsed = 0;
    sum = 0;
}


void LatencyMeasurer::processInput(float *audio, int _samplerate, int numberOfSamples) {
    rampdec = -1.0f;
    samplerate = _samplerate;
    buffersize = numberOfSamples;

    if (nextMeasurementState != measurementState) {
        if (nextMeasurementState == measure_average_loudness_for_1_sec) samplesElapsed = 0;
        measurementState = nextMeasurementState;
    };

    switch (measurementState) {
        // Measuring average loudness for 1 second.
        case measure_average_loudness_for_1_sec:
            sum += sumAudio(audio, numberOfSamples);
            samplesElapsed += numberOfSamples;

            if (samplesElapsed >= samplerate) { // 1 second elapsed, set up the next step.
                // Look for an audio energy rise of 24 decibel.
                float averageAudioValue = (float(sum) / float(samplesElapsed >> 1));
                float referenceDecibel = 20.0f * log10f(averageAudioValue) + 24.0f;
                threshold = (float)(powf(10.0f, referenceDecibel / 20.0f));

                measurementState = nextMeasurementState = playing_and_listening;
                sineWave = 0;
                samplesElapsed = 0;
                sum = 0;
            };
            break;

        // Playing sine wave and listening if it comes back.
        case playing_and_listening: {
            float averageInputValue = sumAudio(audio, numberOfSamples) / numberOfSamples;
            rampdec = 0.0f;

            if (averageInputValue > threshold) { // The signal is above the threshold, so our sine wave comes back on the input.
                int n = 0;
                float *input = audio;
                while (n < numberOfSamples) { // Check the location when it became loud enough.
                    if (*input++ > threshold) break;
                    //if (*input++ > threshold) break;
                    n++;
                };
                samplesElapsed += n; // Now we know the total round trip latency.

                if (samplesElapsed > numberOfSamples) { // Expect at least 1 buffer of round-trip latency.

                    float latms = float(samplesElapsed * 1000) / float(samplerate);

                    smoothedLatency.Z *= 0.75;
                    smoothedLatency.push(latms);
                    
                    /*
                    roundTripLatencyMs[state - 1] = float(samplesElapsed * 1000) / float(samplerate);

                    float sum = 0, max = 0, min = 100000.0f;
                    for (n = 0; n < state; n++) {
                        if (roundTripLatencyMs[n] > max) max = roundTripLatencyMs[n];
                        if (roundTripLatencyMs[n] < min) min = roundTripLatencyMs[n];
                        sum += roundTripLatencyMs[n];
                    };
                     */
                     
                    //if (max / min > 2.0f) { // Dispersion error.
                    //    latencyMs = 0;
                    //    state = measurementCount;
                    //    measurementState = nextMeasurementState = idle;
                    //}
                    if (state == measurementCount) { // Final result.
                        //latencyMs = (sum / measurementCount);
                        latencyMs = smoothedLatency.xbar;
                        measurementState = nextMeasurementState = idle;
                    } else { // Next step.
                        latencyMs = smoothedLatency.xbar;
                        // latencyMs = roundTripLatencyMs[state - 1];
                        measurementState = nextMeasurementState = waiting;
                    }

                    state++;
                } else measurementState = nextMeasurementState = waiting; // Happens when an early noise comes in.

                rampdec = 1.0f / float(numberOfSamples);
            } else { // Still listening.
                samplesElapsed += numberOfSamples;

                // Do not listen to more than max seconds, let's start over. Maybe the environment's noise is too high.
                if (samplesElapsed > samplerate*maxSecsToWait) {
                    rampdec = 1.0f / float(numberOfSamples);
                    measurementState = nextMeasurementState = waiting;
                    latencyMs = -1;
                };
            };
        }; break;

        case passthrough:
        case idle: break;

        default: // Waiting minWaitTime seconds
            samplesElapsed += numberOfSamples;

            double waittime = std::max(minWaitTime, 1.25f * latencyMs*0.001f);
            
            if (samplesElapsed > samplerate * waittime) { // min wait time elapsed, start over.
                samplesElapsed = 0;
                if (skipMeasure) {
                    measurementState = nextMeasurementState = playing_and_listening;
                    sineWave = 0;
                    sum = 0;
                }
                else {
                    measurementState = nextMeasurementState = measure_average_loudness_for_1_sec;
                }
            };
    };
}

void LatencyMeasurer::processOutput(float *audio) {
    if (measurementState == passthrough) return;

    if (rampdec < 0.0f) memset(audio, 0, (size_t)buffersize * sizeof(float)); // Output silence.
    else { // Output sine wave.
        float ramp = 1.0f, mul = (2.0f * float(M_PI) * 1000.0f) / float(samplerate); // 1000 Hz
        int n = buffersize;
        while (n) {
            n--;
            audio[0] = (sinf(mul * sineWave) * ramp);
            ramp -= rampdec;
            sineWave += 1.0f;
            audio += 1;
        };
    };
}
