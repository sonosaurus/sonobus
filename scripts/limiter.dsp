// Version added to JackTrip (standalone program test):

import("stdfaust.lib");

threshold       = nentry("threshold", -3, -24, 0, 0.01);
ratio           = nentry("ratio", 12, 1, 20, 0.1);
attack          = hslider("attack", 0.00001, 0, 1, 0.001) : max(1/ma.SR);
release         = hslider("release", 0.25, 0, 2, 0.01) : max(1/ma.SR);

softClipLevel =  threshold : ba.db2linear;

// lookahead(s), threshold, attack(s), hold(s), release(s)
//limiter = co.limiter_lad_stereo(0.0001, softClipLevel, attack, release/2, release); // GPLv3 license
// If you need a less restricted license, try co.limiter_1176_R4_mono (MIT style license)

limiter = co.compressor_stereo(ratio, threshold, attack, release);

process = limiter;
