// Music 256a / CS 476a | fall 2016
// CCRMA, Stanford University
//
// Author: Romain Michon (rmichonATccrmaDOTstanfordDOTedu)
// Description: Simple Faust architecture file to easily integrate a Faust DSP module
// in a JUCE project

// needed by any Faust arch file
#include "faust/misc.h"

// allows to control a Faust DSP module in a simple manner by using parameter's path
#include "faust/gui/MapUI.h"

// needed by any Faust arch file
#include "faust/dsp/dsp.h"

// tags used by the Faust compiler to paste the generated c++ code
<<includeIntrinsic>>
<<includeclass>>