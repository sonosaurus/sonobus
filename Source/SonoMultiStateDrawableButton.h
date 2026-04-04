// SPDX-License-Identifier: GPLv3-or-later WITH Appstore-exception
// Copyright (C) 2020 Jesse Chappell


#pragma once

#include "SonoDrawableButton.h"

class SonoMultiStateDrawableButton : public SonoDrawableButton
{
public:
    SonoMultiStateDrawableButton(const String& buttonName, std::vector<std::unique_ptr<Drawable>> stateImages, std::vector<String> stateLabels);

    int getState() const { return currentState; }

    void setState(int state);

protected:
    void internalClickCallback(const ModifierKeys &keys) override;
    
private:
    std::vector<std::unique_ptr<Drawable>> stateImages;
    std::vector<String> stateLabels;

    int currentState;
    int numberOfStates;
};
