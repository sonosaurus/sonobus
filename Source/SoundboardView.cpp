// SPDX-License-Identifier: GPLv3-or-later WITH Appstore-exception
// Copyright (C) 2020 Jesse Chappell



#include "SoundboardView.h"

SoundboardView::SoundboardView()
{
    setOpaque(true);

    mTitleLabel = std::make_unique<Label>("title", TRANS("Soundboard"));
    mTitleLabel->setJustificationType(Justification::centred);
    mTitleLabel->setFont(Font(18, Font::bold));
    mTitleLabel->setColour(Label::textColourId, Colour(0xeeffffff));

    addAndMakeVisible(mTitleLabel.get());

    mainBox.items.clear();
    mainBox.flexDirection = FlexBox::Direction::row;
    mainBox.items.add(FlexItem(90, 4, *mTitleLabel).withMargin(0).withFlex(1));
}

void SoundboardView::paint(Graphics& g)
{
    g.fillAll(Colour(0xff272727));
}

void SoundboardView::resized()
{
    mainBox.performLayout(getLocalBounds().reduced(2));
}