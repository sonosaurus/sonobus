// SPDX-License-Identifier: GPLv3-or-later
// Copyright (C) 2021 Jesse Chappell


#pragma once

#include <JuceHeader.h>

#include "SonobusPluginProcessor.h"
#include "SonoDrawableButton.h"

class ChatView : public Component
{
public:
    ChatView(SonobusAudioProcessor& proc, AooServerConnectionInfo & connectinfo);
    ~ChatView();

    void paint (Graphics&) override;
    void resized() override;

    void addNewChatMessages(const Array<SBChatEvent> & mesgs, bool refresh=false);
    void refreshMessages();

    bool keyPressed (const KeyPress & key) override;

protected:

    void processNewChatMessages(int index, int count, bool isSelf=false);
    void commitChatMessage();

    SonobusAudioProcessor& processor;
    AooServerConnectionInfo & currConnectionInfo;

    
    double mLastChatMessageStamp = 0.0;

    Array<SBChatEvent> allChatEvents;
    int lastShownCount = 0;

    std::unique_ptr<Component> mChatContainer;
    std::unique_ptr<TextEditor> mChatTextEditor;
    std::unique_ptr<TextEditor> mChatSendTextEditor;
    //std::unique_ptr<TextButton> mChatSendButton;
    std::unique_ptr<SonoDrawableButton> mCloseButton;
    std::unique_ptr<Label> mTitleLabel;


    Font mChatNameFont;
    Font mChatMesgFont;
    Font mChatSpacerFont;
    String mLastChatEventFrom;
    std::map<String,Colour> mChatUserColors;

    FlexBox mainBox;
    FlexBox titleBox;
    FlexBox chatContainerBox;
    FlexBox chatSendBox;



    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ChatView)
};
