#!/bin/bash

OPTS=-inpl

faust $OPTS -a arch.cpp -i -O ../Source -o faustParametricEQ.h -scn faustdsp -cn faustParametricEQ  parametric_eq.dsp

