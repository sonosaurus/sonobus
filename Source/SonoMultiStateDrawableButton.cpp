// SPDX-License-Identifier: GPLv3-or-later WITH Appstore-exception
// Copyright (C) 2020 Jesse Chappell


#include "SonoMultiStateDrawableButton.h"

SonoMultiStateDrawableButton::SonoMultiStateDrawableButton(const String& buttonName,
                                                           std::vector<std::unique_ptr<Drawable>> stateImages_,
                                                           std::vector<String> stateLabels_) :
        SonoDrawableButton(buttonName, DrawableButton::ButtonStyle::ImageAboveTextLabel),
        stateImages(std::move(stateImages_)),
        stateLabels(std::move(stateLabels_)),
        numberOfStates(stateImages.size())
{
    if (stateImages.size() != stateLabels.size()) {
        throw std::invalid_argument("State image and state label vectors must have the same size.");
    }
}

void SonoMultiStateDrawableButton::internalClickCallback(const ModifierKeys& keys)
{
    setState(currentState + 1);

    setClickingTogglesState(false);
    Button::internalClickCallback(keys);
}

void SonoMultiStateDrawableButton::setState(int state)
{
    currentState = state % numberOfStates;
    setButtonText(stateLabels[currentState]);
    setImages(stateImages[currentState].get());

    repaint();
}

