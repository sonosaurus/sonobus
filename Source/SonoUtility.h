// SPDX-License-Identifier: GPLv3-or-later
// Copyright (C) 2020 Jesse Chappell


#pragma once

#include "JuceHeader.h"

class SonoUtility
{
public:
    // should put this in some utility class
    static String durationToString(double pos, bool withColon=false, bool fractional=false)
    {
        int hours = (int) (pos/3600.0);
        int minutes = (int) (pos/60.0) % 60;
        float secs =  fmodf(pos, 60.0);
        if (hours > 0) {
            if (withColon) {
                return String::formatted("%d:%02d:%02d", hours, minutes, (int) secs);
            } else {
                return String::formatted("%dh%dm%ds", hours, minutes, (int) secs);
            }
        }
        else if (minutes > 0 || withColon) {
            if (withColon) {
                return String::formatted("%02d:%02d", minutes, (int) (secs));
            } else {
                return String::formatted("%dm%ds", minutes, (int) (secs));
            }
        }
        else if (!fractional || secs > 3.0f){
            return String::formatted("%ds", (int) (secs));
        }
        else {
            return String::formatted("%.1fs", secs);
        }
    }

};
