// SPDX-License-Identifier: GPLv3-or-later WITH Appstore-exception
// Copyright (C) 2021 Jesse Chappell


#pragma once

#include <JuceHeader.h>

#include "SonoLookAndFeel.h"
#include "SonoDrawableButton.h"
#include "EffectsBaseView.h"
#include "EffectParams.h"
#include "SonoChoiceButton.h"

#include "SonobusPluginProcessor.h"

//==============================================================================
/*
*/
class VDONinjaView    : public EffectsBaseView
{
public:
    VDONinjaView(SonobusAudioProcessor & processor_)  : processor(processor_)
    {
        modeInfoLabel.setText(TRANS("Mode:"), dontSendNotification);
        modeInfoLabel.setJustificationType(Justification::centredLeft);

        roomModeButton.setButtonText(TRANS("Room"));
        roomModeButton.setTooltip(TRANS("Room mode is simpler and better for large groups or groups with people entering and leaving often, but video quality may be reduced"));
        roomModeButton.setConnectedEdges(Button::ConnectedOnRight);
        roomModeButton.setRadioGroupId(1);
        roomModeButton.onClick = [this]() {
            processor.getVideoLinkInfo().roomMode = true;
            updateState();
        };
        
        pushViewButton.setButtonText(TRANS("Push/View"));
        pushViewButton.setTooltip(TRANS("Push/View is the highest quality and most flexible option, but requires regenerating the link when more people join"));
        pushViewButton.setConnectedEdges(Button::ConnectedOnLeft);
        pushViewButton.setRadioGroupId(1);
        pushViewButton.onClick = [this]() {
            processor.getVideoLinkInfo().roomMode = false;
            updateState();
        };
        
        directorButton.setButtonText(TRANS("Be Director"));
        directorButton.setTooltip(TRANS("The room mode director can get direct feeds and control various options, can be used for setting up streaming"));
        directorButton.onClick = [this]() {
            processor.getVideoLinkInfo().beDirector = directorButton.getToggleState();
            refreshURL();
        };
        
        copyLinkButton.setButtonText(TRANS("Copy"));
        copyLinkButton.setTooltip(TRANS("Copies URL to clipboard"));
        copyLinkButton.onClick = [this]() {
            SystemClipboard::copyTextToClipboard(urlEditor.getText());
        };

        showNamesButton.setButtonText(TRANS("Show Names"));
        showNamesButton.onClick = [this]() {
            processor.getVideoLinkInfo().showNames = showNamesButton.getToggleState();
            refreshURL();
        };

        openLinkButton.setButtonText(TRANS("Open"));
        openLinkButton.setTooltip(TRANS("Open URL in browser"));
        openLinkButton.onClick = [this]() {
            URL url(urlEditor.getText());
            url.launchInDefaultBrowser();
        };

        moreInfoButton.setButtonText(TRANS("More Info..."));
        moreInfoButton.setTooltip(TRANS("Open VDO.Ninja documentation in browser"));
        moreInfoButton.onClick = [this]() {
            URL url("https://docs.vdo.ninja");
            url.launchInDefaultBrowser();
        };

        
        linkLabel.setText(TRANS("Link:"), dontSendNotification);
        linkLabel.setJustificationType(Justification::centredRight);

        customInfoLabel.setText(TRANS("Extra Parameters:"), dontSendNotification);
        customInfoLabel.setJustificationType(Justification::centredLeft);

        customFieldEditor.setTooltip(TRANS("Enter extra URL parameters here (separated with &), for more details see Advanced Options in the VDO.Ninja documentation"));
        customFieldEditor.setTextToShowWhenEmpty(TRANS("optional"), Colour(0x44ffffff));
        customFieldEditor.onTextChange = [this] () {
            processor.getVideoLinkInfo().extraParams = customFieldEditor.getText();
            refreshURL();
        };
        
        urlEditor.setReadOnly(true);
        urlEditor.setCaretVisible(false);
        
#if JUCE_IOS
        infoLabel.setText(TRANS("VDO.Ninja is a high-quality web-based video streaming system."), dontSendNotification);
#else
        infoLabel.setText(TRANS("VDO.Ninja is a high-quality web-based video streaming system. Using with Chrome is highly recommended."), dontSendNotification);
#endif
        infoLabel.setJustificationType(Justification::centredLeft);
        infoLabel.setFont(13);
        
        //modeInfoLabel.setText(TRANS("VDO Ninja Link"), dontSendNotification);

        
        // these are in the header component
        enableButton.setVisible(false);

        titleLabel.setFont(16);
        titleLabel.setText(TRANS("VDO.Ninja Link Generator"), dontSendNotification);
        titleLabel.setJustificationType(Justification::centred);
        titleLabel.setAccessible(false);
        
        dragButton.setVisible(false);

        addAndMakeVisible(roomModeButton);
        addAndMakeVisible(pushViewButton);
        addAndMakeVisible(modeInfoLabel);
        addAndMakeVisible(customFieldEditor);
        addAndMakeVisible(customInfoLabel);
        addAndMakeVisible(urlEditor);
        //addAndMakeVisible(linkLabel);
        addAndMakeVisible(copyLinkButton);
        addAndMakeVisible(openLinkButton);
        addAndMakeVisible(infoLabel);
        addAndMakeVisible(moreInfoButton);
        addAndMakeVisible(showNamesButton);
        addChildComponent(directorButton);

        addAndMakeVisible(titleLabel);

        //addAndMakeVisible(headerComponent);
        //addHeaderListener(this);

        updateState();
    }

    ~VDONinjaView()
    {
    }

    void resized() override
    {
        int minKnobWidth = 54;
        int minitemheight = 32;
        int knoblabelheight = 18;
        int knobitemheight = 62;
        int enablewidth = 44;
        int headerheight = 44;
        int buttwidth = 100;
        int smallbuttwidth = 80;
        int autobuttwidth = 150;

#if JUCE_IOS || JUCE_ANDROID
        // make the button heights a bit more for touchscreen purposes
        minitemheight = 40;
        knobitemheight = 80;
        headerheight = 50;
#endif
        FlexBox modeBox;
        modeBox.flexDirection = FlexBox::Direction::row;
        modeBox.items.add(FlexItem(4, 4).withMargin(0).withFlex(0));
        modeBox.items.add(FlexItem(smallbuttwidth, minitemheight, modeInfoLabel).withMargin(0).withFlex(0));
        modeBox.items.add(FlexItem(5, 4).withMargin(0));
        modeBox.items.add(FlexItem(smallbuttwidth, minitemheight, roomModeButton).withMargin(0).withFlex(1).withMaxWidth(130));
        modeBox.items.add(FlexItem(smallbuttwidth, minitemheight, pushViewButton).withMargin(0).withFlex(1).withMaxWidth(130));
        modeBox.items.add(FlexItem(6, 4).withMargin(0).withFlex(0));
        modeBox.items.add(FlexItem(buttwidth, minitemheight, directorButton).withMargin(0).withFlex(0));

        FlexBox customBox;
        customBox.flexDirection = FlexBox::Direction::row;
        customBox.items.add(FlexItem(4, 4).withMargin(0));
        customBox.items.add(FlexItem(smallbuttwidth, minitemheight, customInfoLabel).withMargin(0).withFlex(0));
        customBox.items.add(FlexItem(12, 4).withMargin(0));
        customBox.items.add(FlexItem(minKnobWidth, minitemheight, customFieldEditor).withMargin(0).withFlex(1));
        customBox.items.add(FlexItem(4, 4).withMargin(0));
        customBox.items.add(FlexItem(buttwidth, minitemheight, showNamesButton).withMargin(0).withFlex(0));

        
        FlexBox editorBox;
        editorBox.flexDirection = FlexBox::Direction::row;
        //editorBox.items.add(FlexItem(12, 4).withMargin(0));
        //editorBox.items.add(FlexItem(minKnobWidth, minitemheight, linkLabel).withMargin(0).withFlex(0));
        editorBox.items.add(FlexItem(8, 4).withMargin(0));
        editorBox.items.add(FlexItem(minKnobWidth, minitemheight, urlEditor).withMargin(0).withFlex(1));

        FlexBox buttonBox;
        buttonBox.flexDirection = FlexBox::Direction::row;
        buttonBox.items.add(FlexItem(12, 4).withMargin(1).withFlex(1));
        buttonBox.items.add(FlexItem(buttwidth, minitemheight, copyLinkButton).withMargin(0).withFlex(0));
        buttonBox.items.add(FlexItem(12, 4).withMargin(0));
        buttonBox.items.add(FlexItem(buttwidth, minitemheight, openLinkButton).withMargin(0).withFlex(0));
        buttonBox.items.add(FlexItem(12, 4).withMargin(1).withFlex(1));

        FlexBox infoBox;
        infoBox.flexDirection = FlexBox::Direction::row;
        infoBox.items.add(FlexItem(6, 4).withMargin(1));
        infoBox.items.add(FlexItem(buttwidth, minitemheight, infoLabel).withMargin(0).withFlex(1));
        infoBox.items.add(FlexItem(8, 4).withMargin(0));
        infoBox.items.add(FlexItem(smallbuttwidth, minitemheight, moreInfoButton).withMargin(1).withFlex(0));

        
        FlexBox mainBox;
        mainBox.items.clear();
        mainBox.flexDirection = FlexBox::Direction::column;
        mainBox.items.add(FlexItem(100, minitemheight, titleLabel).withMargin(0).withFlex(0));
        mainBox.items.add(FlexItem(6, 8).withMargin(0).withFlex(0));
        mainBox.items.add(FlexItem(100, minitemheight, modeBox).withMargin(0).withFlex(0));
        mainBox.items.add(FlexItem(6, 8).withMargin(0).withFlex(0));
        mainBox.items.add(FlexItem(100, minitemheight, customBox).withMargin(0).withFlex(0));
        mainBox.items.add(FlexItem(6, 14).withMargin(0).withFlex(0));
        mainBox.items.add(FlexItem(100, minitemheight, editorBox).withMargin(0).withFlex(0));
        mainBox.items.add(FlexItem(6, 4).withMargin(0).withFlex(0));
        mainBox.items.add(FlexItem(100, minitemheight, buttonBox).withMargin(0).withFlex(0));
        mainBox.items.add(FlexItem(6, 8).withMargin(0).withFlex(0));
        mainBox.items.add(FlexItem(100, minitemheight, infoBox).withMargin(0).withFlex(1));
        mainBox.items.add(FlexItem(6, 4).withMargin(0).withFlex(0));
        
        mainBox.performLayout(getLocalBounds().reduced(2));
        
        int iph = 0;
        for (auto & item : mainBox.items) {
            iph += item.minHeight + item.margin.top + item.margin.bottom;
        }

        minBounds.setSize(280, iph + 10);
    }

    
    void updateState() {
        
        auto & state = processor.getVideoLinkInfo();
        
        pushViewButton.setToggleState(!state.roomMode, dontSendNotification);
        roomModeButton.setToggleState(state.roomMode, dontSendNotification);
        showNamesButton.setToggleState(state.showNames, dontSendNotification);
        directorButton.setToggleState(state.beDirector, dontSendNotification);
        customFieldEditor.setText(state.extraParams);
        
        directorButton.setVisible(state.roomMode);
        
        refreshURL();
    }

    void refreshURL()
    {
        auto url = generateURL();
        
        urlEditor.setText(url.toString(true));
        urlEditor.setCaretPosition(0);
    }
    
    
private:
    
    URL generateURL() {
        String baseurl = "https://vdo.ninja/";
        StringPairArray params;
        auto & state = processor.getVideoLinkInfo();

        auto makeId = [this](const String & name) {
            // Combine the group and the username, and take the first half of the MD5 hash
            // to compute a unique-ish, not-too-long, but reproducible by others ID for us
            auto pushsource = processor.getCurrentJoinedGroup() + name;
            auto pushid = MD5(pushsource.toUTF8());
            auto rawdata = pushid.getRawChecksumData();
            auto base64 = Base64::toBase64(rawdata.getData(), rawdata.getSize());
            base64 = base64.substring(0, base64.length()/2); // just make it 12 chars long
            // since this is a one-way thing, just replace any + / or = with X Y Z
            return std::move(base64.replaceCharacters("+/=", "XYZ"));
        };
        

        params.set("label", processor.getCurrentUsername());

        if (state.showNames) {
            params.set("showlabels", "");
            params.set("fontsize", "40");
        }
        
        // make sure no audio is possible, because we are doing the audio
        params.set("adevice", "0");
        params.set("nmb", ""); // no mic button
        params.set("nsb", ""); // no speaker button
        params.set("noaudio", "");
        params.set("deaf", "");
        params.set("noap", ""); // no audio processing

        
        // parse more params out of custom text
        StringArray custrawparams;
        custrawparams.addTokens(state.extraParams, "&", "");
        if (custrawparams.size() > 0) {
            for (auto & par : custrawparams) {
                auto trimmed = par.trim();
                auto key = trimmed.upToFirstOccurrenceOf("=", false, false);
                String val;
                if (key.length() + 1 < trimmed.length()) {
                    val = trimmed.substring(key.length() + 1);
                }
                params.set(key.trim(), val.trim());
            }
        }
        
        params.set("push", makeId(processor.getCurrentUsername()));
        
        
        if (state.roomMode) {
            auto roomName = "SB_" + processor.getCurrentJoinedGroup();
            if (state.beDirector) {
                params.set("dir", roomName);
                params.set("sd", "");
            }
            else {
                params.set("room", roomName);
                params.set("wc", ""); // go to webcam selection immediately
                params.set("ssb", ""); // allow screenshare later
            }
        }
        else {
            StringArray others;
            for (int i=0; i < processor.getNumberRemotePeers(); ++i) {
                auto name = processor.getRemotePeerUserName(i);
                others.add(makeId(name));
            }

            params.set("wc", ""); // go to webcam selection immediately
            params.set("ssb", ""); // allow screenshare later

            
            if (others.size() > 0) {
                params.set("view", others.joinIntoString(","));
            }
        }
        
        
        
        auto url = URL(baseurl).withParameters(params);

        return url;
    }
    
    SonobusAudioProcessor & processor;

    TextButton   roomModeButton;
    TextButton   pushViewButton;
    ToggleButton directorButton;
    Label modeInfoLabel;

    TextEditor   customFieldEditor;
    Label customInfoLabel;
    ToggleButton showNamesButton;
    
    TextEditor   urlEditor;
    Label linkLabel;
    
    TextButton   copyLinkButton;
    TextButton   openLinkButton;
    TextButton   moreInfoButton;
    Label infoLabel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VDONinjaView)
};
