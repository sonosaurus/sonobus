/* Copyright (c) 2010-Now Christof Ressi, Winfried Ritsch and others. 
 * For information on usage and redistribution, and for a DISCLAIMER OF ALL
 * WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */

#pragma once

#include <limits>
#include <stdio.h>

// Delay-Locked-Loop as described by Fons Adriaensen in
// "Using a DLL to filter time"
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace aoo {

class time_dll {
public:
    void setup(double sr, int nper, double bandwidth, double t){
        double tper = nper / sr;
        nper_ = nper;
        // compute coefficients
        double omega = 2 * M_PI * bandwidth * (nper / sr);
        b_ = omega * 1.4142135623731; // omega * sqrt(2)
        c_ = omega * omega;
        // initialize filter
        e2_ = tper;
        t0_ = t;
        t1_ = t + tper;
    }
    void update(double t){
        // calculate loop error
        double e = e_ = t - t1_;
        // update loop
        t0_ = t1_;
        t1_ += b_ * e + e2_;
        e2_ += c_ * e;
        // flush denormals
        if (std::numeric_limits<double>::has_denorm != std::denorm_absent){
            if (e2_ <= std::numeric_limits<double>::min()){
                e2_ = 0;
            #if 0
                fprintf(stderr, "flushed denormals\n!");
                fflush(stderr);
            #endif
            }
        }
    }
    double period() const {
        return t1_ - t0_;
    }
    double samplerate() const {
        return nper_ / period();
    }
private:
    double b_ = 0;
    double c_ = 0;
    double t0_ = 0;
    double t1_ = 0;
    double e_ = 0;
    double e2_ = 0;
    int nper_ = 0;
};

} // aoo
