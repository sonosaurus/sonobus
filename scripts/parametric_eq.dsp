
import("stdfaust.lib");

parametric_eq = fi.low_shelf(LL,FL) : fi.peak_eq(LP1,FP1,BP1) : fi.peak_eq(LP2,FP2,BP2) :  fi.high_shelf(LH,FH)
with{
        eq_group(x) = hgroup("[0] parametric eq [tooltip: See Faust's filters.lib
                for info and pointers]",x);
        ls_group(x) = eq_group(vgroup("[1] low shelf",x));

        LL = ls_group(hslider("[0] gain [unit:dB] [style:knob]
                [tooltip: Amount of low-frequency boost or cut in decibels]",0,-40,40,0.1)) : si.smoo;
        FL = ls_group(hslider("[1] transition freq [unit:Hz] [style:knob] [scale:log]
                [tooltip: Transition-frequency from boost (cut) to unity gain]",200,1,5000,1)) : si.smooth(0.999);

        pq1_group(x) = eq_group(vgroup("[2] para1 [tooltip: Parametric Equalizer
                sections from filters.lib]",x));
        LP1 = pq1_group(hslider("[0] peak gain [unit:dB] [style:knob][tooltip: Amount of
                local boost or cut in decibels]",0,-40,40,0.1)) : si.smoo;
        FP1 = pq1_group(hslider("[1] peak frequency [unit:Hz] [style:knob] [tooltip: Peak
                Frequency in Hz]",400,40,10000,1)) : si.smooth(0.999);
        Q1 = pq1_group(hslider("[2] peak q [style:knob] [scale:log] [tooltip: Quality factor
                (Q) of the peak = center-frequency/bandwidth]",40,1,1000,0.1));

        BP1 = FP1/Q1;

        pq2_group(x) = eq_group(vgroup("[2] para2 [tooltip: Parametric Equalizer
                sections from filters.lib]",x));
        LP2 = pq2_group(hslider("[0] peak gain [unit:dB] [style:knob][tooltip: Amount of
                local boost or cut in decibels]",0,-40,40,0.1)) : si.smoo ;
        FP2 = pq2_group(hslider("[1] peak frequency [unit:Hz] [style:knob] [tooltip: Peak
                Frequency in Hz]",800,40,10000,1)) : si.smooth(0.999);
        Q2 = pq2_group(hslider("[2] peak q [style:knob] [scale:log] [tooltip: Quality factor
                (Q) of the peak = center-frequency/bandwidth]",40,1,1000,0.1)) : si.smoo ;

        BP2 = FP2/Q2;


        hs_group(x) = eq_group(vgroup("[3] high shelf [tooltip: A high shelf provides a boost
                or cut above some frequency]",x));
        LH = hs_group(hslider("[0] gain [unit:dB] [style:knob] [tooltip: Amount of
                high-frequency boost or cut in decibels]",0,-40,40,.1)) : si.smoo;
        FH = hs_group(hslider("[1] transition freq [unit:Hz] [style:knob] [scale:log]
        [tooltip: Transition-frequency from boost (cut) to unity gain]",8000,20,10000,1)) : si.smooth(0.999);
};

process = parametric_eq;
