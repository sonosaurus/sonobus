//----------------------------`(dm.)compressor_demo`-------------------------
// Compressor demo application.
//
// #### Usage
//
// ```
// _,_ : compressor_demo : _,_;
// ```
//------------------------------------------------------------

declare name "compressor";
declare version "0.0";
declare author "JOS, revised by RM";
declare description "Compressor demo application";

import("stdfaust.lib");


compressorjlc_demo = ba.bypass2(cbp,compressor_stereo_demo)
with{
        main_group(x) = vgroup("compressor [tooltip: Reference:
                http://en.wikipedia.org/wiki/Dynamic_range_compression]", x);

       comp_group(x)  = main_group(hgroup("[0] comp", x));
       env_group(x)  = main_group(hgroup("[1] env", x));
       gain_group(x)  = main_group(hgroup("[2] gain", x));

        cbp = main_group(checkbox("[0] Bypass  [tooltip: When this is checked, the compressor
                has no effect]"));
	//gainview = co.compression_gain_mono(ratio,threshold,attack,release) :
	gainview = co.compression_gain_mono(ratio,threshold,attack,release) : ba.linear2db :
        gain_group(hbargraph("[1] outgain [unit:db] [tooltip: Current gain of
        the compressor in linear gain]",0,2.0)) ;

        displaygain = _,_ <: _,_,(abs,abs:+) : _,_,gainview : _,attach;

        compressor_stereo_demo =
        displaygain(co.compressor_stereo(ratio,threshold,attack,release)) :
        *(makeupgain) , *(makeupgain);

        //ctl_group(x)  = knob_group(hgroup("[3] Compression Control", x));

        ratio = comp_group(hslider("[1] ratio [style:knob]
        [tooltip: A compression Ratio of N means that for each N dB increase in input
        signal level above Threshold, the output level goes up 1 dB]",
        5, 1, 20, 0.1));

        threshold = comp_group(hslider("[0] threshold [unit:dB] [style:knob]
        [tooltip: When the signal level exceeds the Threshold (in dB), its level
        is compressed according to the Ratio]",
        -30, -100, 10, 0.1));

        // env_group(x)  = knob_group(hgroup("[4] Compression Response", x));

        attack = env_group(hslider("[0] attack [unit:sec] [style:knob] [scale:log]
        [tooltip: Time constant in ms (1/e smoothing time) for the compression gain
        to approach (exponentially) a new lower target level (the compression
        `kicking in')]", 50, 1, 1000, 0.1))  : max(1/ma.SR);

        release = env_group(hslider("[1] release [unit:sec] [style: knob] [scale:log]
        [tooltip: Time constant in ms (1/e smoothing time) for the compression gain
        to approach (exponentially) a new higher target level (the compression
        'releasing')]", 500, 1, 1000, 0.1))  : max(1/ma.SR);

        makeupgain = gain_group(hslider("[0] makeup gain [unit:dB]
        [tooltip: The compressed-signal output level is increased by this amount
        (in dB) to make up for the level lost due to compression]",
        40, -96, 96, 0.1)) : ba.db2linear : si.smoo;
};


process = compressorjlc_demo;