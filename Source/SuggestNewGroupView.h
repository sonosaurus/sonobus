// SPDX-License-Identifier: GPLv3-or-later WITH Appstore-exception
// Copyright (C) 2021 Jesse Chappell


#pragma once

#include <JuceHeader.h>

#include "SonobusPluginProcessor.h"
#include "SonoDrawableButton.h"
#include "SonoLookAndFeel.h"
#include "SonobusPluginEditor.h"

#include <set>

class SuggestNewGroupView : public Component, public MultiTimer
{
public:
    SuggestNewGroupView(SonobusAudioProcessor& proc);
    ~SuggestNewGroupView();

    void paint (Graphics&) override;
    void resized() override;


    void timerCallback(int timerid) override;

    void updatePeerRows(bool force=false);

    std::function<void(const String & group, const String & groupPass, bool isPublic)> connectToGroup;

protected:

    void dismissSelf();

    SonoBigTextLookAndFeel smallLNF;

    ToggleButton * createPeerToggle();
    Label * createPeerLabel();

    SonobusAudioProcessor& processor;


    std::unique_ptr<Viewport> mViewport;
    std::unique_ptr<Component> mRowsContainer;
    OwnedArray<ToggleButton> mPeerToggles;

    std::unique_ptr<Label> mGroupStaticLabel;
    std::unique_ptr<TextEditor> mGroupEditor;
    std::unique_ptr<Label> mGroupPassStaticLabel;
    std::unique_ptr<TextEditor> mGroupPassEditor;
    std::unique_ptr<ToggleButton> mPublicToggle;
    std::unique_ptr<TextButton> mRequestButton;
    std::unique_ptr<TextButton> mSelectAllButton;
    std::unique_ptr<TextButton> mSelectNoneButton;

    std::unique_ptr<Label> mTitleLabel;
    std::unique_ptr<SonoDrawableButton> mCloseButton;

    std::unique_ptr<DrawableRectangle> mPeerRect;

    FlexBox mainBox;
    FlexBox titleBox;
    FlexBox peerRowsBox;
    FlexBox matchButtBox;
    FlexBox groupBox;
    FlexBox groupPassBox;
    FlexBox publicBox;
    FlexBox selButtBox;

    std::set<String> selectedPeers;

    bool initialCheckDone = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SuggestNewGroupView)
};
