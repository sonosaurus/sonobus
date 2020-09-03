#!/bin/bash

OPTS=-inpl

faust $OPTS -a arch.cpp -i -O ../Source -o zitaRev.h -scn faustdsp -cn zitaRev  zitaRev.dsp 
