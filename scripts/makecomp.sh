#!/bin/bash

mkdir -p faustComp
#faust -a arch.cpp -i -O faustComp -o faustComp.h -scn faustdsp -cn faustComp  compressor.dsp


#faust -a arch.cpp -i -O ../Source -o faustCompressor.h -scn faustdsp -cn faustCompressor  compressjlc.dsp
#faust -a arch.cpp -i -O ../Source -o faustCompressor.h -scn faustdsp -cn faustCompressor  compressor.dsp
faust -a arch.cpp -i -O ../Source -o faustCompressor.h -scn faustdsp -cn faustCompressor  compressor2.dsp


#faust -a arch.cpp -i -O ../Source -o faustExpander.h -scn faustdsp -cn faustExpander  expander.dsp
faust -a arch.cpp -i -O ../Source -o faustExpander.h -scn faustdsp -cn faustExpander  expander2.dsp

