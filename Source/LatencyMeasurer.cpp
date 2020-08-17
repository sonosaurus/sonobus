#include "LatencyMeasurer.h"
#include <cmath>
#include <cstdlib>
#include <cstring>


/**
 Cross-platform class measuring round-trip audio latency.
 How one measurement step works:
 - Listen and measure the average loudness of the environment for 1 second.
 - Create a threshold value 24 decibels higher than the average loudness.
 - Begin playing a 1000 Hz sine wave and start counting the samples elapsed.
 - Stop counting and playing if the input's loudness is higher than the threshold, as the output wave is coming back (probably).
 - Divide the the elapsed samples with the sample rate to get the round-trip audio latency value in seconds.
 - We expect the threshold exceeded within 1 second. If it did not, then fail with error. Usually happens when the environment is too noisy (loud).
 How the measurement process works:
MODIFIED, THE BELOW IS NO LONGER TRUE
 XXX - Perform 10 measurement steps.
 XXX - Repeat every step until it returns without an error.
 XXX - Store the results in an array of 10 floats.
 XXX - After each step, check the minimum and maximum values.
 XXX - If the maximum is higher than the minimum's double, stop the measurement process with an error. It indicates an unknown error, perhaps an unwanted noise happened. Double jitter (dispersion) is too high, an audio system can not be so bad.
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
    
    //readAudioFromBinaryData(pulseDataBuffer, BinaryData::urei_main_wav, BinaryData::urei_main_wavSize);
    readAudioFromBinaryData(pulseDataBuffer, BinaryData::lgc_bar_wav, BinaryData::lgc_bar_wavSize);
}

void LatencyMeasurer::readAudioFromBinaryData(AudioSampleBuffer& buffer, const void* data, size_t sizeBytes)
{
    WavAudioFormat wavFormat;
    std::unique_ptr<AudioFormatReader> reader (wavFormat.createReaderFor (new MemoryInputStream (data, sizeBytes, false), true));
    if (reader.get() != nullptr)
    {
        buffer.setSize((int)reader->numChannels, (int)reader->lengthInSamples);
        reader->read(&buffer, 0, (int)reader->lengthInSamples, 0, true, true);
        DBG("Read pulse of " << buffer.getNumSamples() << " samples");
    }
    // reader is automatically deleted by using unique_ptr
}

void LatencyMeasurer::toggle(bool forcestart) {
    if (!forcestart && ((state == -1) || ((state > 0) && (state < 11)))) { // stop
        state = 0;
        nextMeasurementState = idle;
    } else { // start
        state = 1;
        samplerate = latencyMs = buffersize = 0;
        skipMeasure = measureOnlyOnce;
        smoothedLatency.reset();
        playing = false;
        nextMeasurementState = measure_average_loudness;
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
    pulsePos = 0;
    playing = true;
}


void LatencyMeasurer::processInput(float *audio, int _samplerate, int numberOfSamples) {
    rampdec = -1.0f;
    samplerate = _samplerate;
    buffersize = numberOfSamples;

    if (nextMeasurementState != measurementState) {
        if (nextMeasurementState == measure_average_loudness) samplesElapsed = 0;
        measurementState = nextMeasurementState;
    };

    switch (measurementState) {
        // Measuring average loudness for 1 second.
        case measure_average_loudness:
            sum += sumAudio(audio, numberOfSamples);
            samplesElapsed += numberOfSamples;

            if (samplesElapsed >= (noiseMeasureTime * samplerate)) { // measurement time elapsed
                // Look for an audio energy rise of 24 decibel.
                // hacking this threshold
                if (overrideThreshold > 0.0f) {
                    float averageAudioValue = jmax(0.001f, (float(sum) / float(samplesElapsed >> 1)));
                    float referenceDecibel = 20.0f * log10f(averageAudioValue) + 24.0f;
                    threshold = (float)(powf(10.0f, referenceDecibel / 20.0f));
                } else {
                    threshold = overrideThreshold;
                }
                //DebugLogC("Audio thresh is %g dB  or %g", referenceDecibel, threshold);
                
                measurementState = nextMeasurementState = playing_and_listening;
                sineWave = 0;
                samplesElapsed = numberOfSamples; // seed it with this buffer, because in the same cycle the output will go out
                sum = 0;
                pulsePos = 0;
                playing = true;
            };
            break;

        // Playing sine wave and listening if it comes back.
        case playing_and_listening: {
            //float averageInputValue = sumAudio(audio, numberOfSamples) / numberOfSamples;
            auto peakRange = FloatVectorOperations::findMinAndMax(audio, numberOfSamples);
            auto abspeak = jmax(abs(peakRange.getStart()), abs(peakRange.getEnd()));
            
            if (!usePulseData) {
                if (playing) {
                    if ((samplesElapsed / (float) samplerate) > sinPulseTime) {
                        // stop playing
                        rampdec = (1.0f / float(numberOfSamples));
                        playing = false;
                    }
                    else {
                        rampdec = 0.0;
                    }
                }
                else {
                    rampdec = -1.0;
                }
            }

            if (abspeak > threshold) { // The signal is above the threshold, so our signal came back on the input.
                int n = 0;
                float *input = audio;
                while (n < numberOfSamples) { // Check the location when it became loud enough.
                    if (abs(*input++) > threshold) break;
                    //if (*input++ > threshold) break;
                    n++;
                };
                samplesElapsed += n; // Now we know the total round trip latency.

                if (samplesElapsed > numberOfSamples) { // Expect at least 1 buffer of round-trip latency.

                    float latms = float(samplesElapsed * 1000) / float(samplerate);

                    DBG("latms: " << latms);
                    
                    if (smoothedLatency.Z > 1) {
                        smoothedLatency.Z *= 0.75;
                    }
                    smoothedLatency.push(latms);
                    
                    if (state == measurementCount) { // Final result.
                        latencyMs = smoothedLatency.xbar;
                        measurementState = nextMeasurementState = idle;
                    } else { // Next step.
                        latencyMs = smoothedLatency.xbar;
                        measurementState = nextMeasurementState = waiting;
                    }

                    state++;
                } else measurementState = nextMeasurementState = waiting; // Happens when an early noise comes in.

                //rampdec = 1.0f / float(numberOfSamples);
            }
            else { // Still listening.
                samplesElapsed += numberOfSamples;

                // Do not listen to more than max seconds, let's start over. Maybe the environment's noise is too high.
                if (samplesElapsed > samplerate*maxSecsToWait) {
                    rampdec = 1.0f / float(numberOfSamples);
                    playing = false;
                    measurementState = nextMeasurementState = waiting;
                    latencyMs = -1;
                };
            };
        }; break;

        case passthrough:
        case idle: break;

        default: // Waiting minWaitTime seconds
            samplesElapsed += numberOfSamples;

            playing = false;
            
            double waittime = (double) std::fmax(minWaitTime, 1.5f * latencyMs*0.001f);
            
            if (samplesElapsed > samplerate * waittime) { // min wait time elapsed, start over.
                samplesElapsed = 0;
                if (skipMeasure) {
                    measurementState = nextMeasurementState = playing_and_listening;
                    samplesElapsed = numberOfSamples; // seed it with this buffer, because in the same cycle the output will go out
                    sineWave = 0;
                    sum = 0;
                    pulsePos = 0;
                    playing = true;
                }
                else {
                    measurementState = nextMeasurementState = measure_average_loudness;
                }
            };
    };
}

void LatencyMeasurer::processOutput(float *audio) {
    if (measurementState == passthrough) return;

    if (usePulseData) {
        if (playing && pulsePos < pulseDataBuffer.getNumSamples()) {
            auto toread = jmin(buffersize, pulseDataBuffer.getNumSamples()-pulsePos);
            auto * pbuf = pulseDataBuffer.getReadPointer(0, pulsePos);
            memcpy(audio, pbuf, toread*sizeof(float));
            pulsePos += toread;
            
            if (toread < buffersize) {
                memset(audio+toread, 0, (size_t)(buffersize - toread) * sizeof(float)); // Output silence remainder
            }
        }
        else {
            memset(audio, 0, (size_t)buffersize * sizeof(float)); // Output silence.
        }
    }
    else if (rampdec < 0.0f) {
        memset(audio, 0, (size_t)buffersize * sizeof(float)); // Output silence.
    }
    else { // Output sine wave.
        float ramp = 0.7f, mul = (2.0f * float(M_PI) * 2000.0f) / float(samplerate); // 2000 Hz
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
