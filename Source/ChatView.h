// SPDX-License-Identifier: GPLv3-or-later
// Copyright (C) 2021 Jesse Chappell


#pragma once

#include <JuceHeader.h>

#include "SonobusPluginProcessor.h"
#include "SonoDrawableButton.h"
#include <map>

class FocusTextEditor : public TextEditor
{
public:
    
    FocusTextEditor(const String & name = {}) : TextEditor(name) {}

    void focusGained (FocusChangeType) override;

    std::function<void(FocusChangeType)> onFocusGained;
};

class ChatView : public Component
{
public:
    ChatView(SonobusAudioProcessor& proc, AooServerConnectionInfo & connectinfo);
    ~ChatView();

    void paint (Graphics&) override;
    void resized() override;

    void addNewChatMessage(const SBChatEvent & mesg, bool refresh=true);
    void addNewChatMessages(const Array<SBChatEvent> & mesgs, bool refresh=false);
    void refreshMessages(); // only new ones not yet rendered
    void refreshAllMessages(); // re-render all messages

    void clearAll();

    bool haveNewSinceLastView() const;
    void setFocusToChat();

    void setUseFixedWidthFont(bool flag);

    bool keyPressed (const KeyPress & key) override;

    void mouseDown (const MouseEvent& event)  override;
    void mouseUp (const MouseEvent& event)  override;
    void mouseDrag (const MouseEvent& event)  override;
    void mouseMove (const MouseEvent& event)  override;

    void visibilityChanged() override;

protected:

    void processNewChatMessages(int index, int count);
    void commitChatMessage();

    void showMenu(bool show);
    void showSaveChat();

    void updateFontSizes();

    bool parseStringForUrl(const String & str, Array<Range<int> > & retranges);

    bool findUrlAtPos(juce::Point<int>, String & retstr);

    void chatTextGainedFocus(FocusChangeType ctype);
    void chatTextLostFocus();

    SonobusAudioProcessor& processor;
    AooServerConnectionInfo & currConnectionInfo;

    double mLastChatMessageStamp = 0.0;
    double mLastChatShowTimeStamp = 0.0;
    double mLastChatUserMessageStamp = 0.0;

    double mLastChatViewStamp = 0.0;

    bool mUserScrolled = false;

    bool mKeyboardVisible = false;

    uint32 mLastUrlCheckStampMs = 0;
    bool mOverUrl = false;

    MouseCursor mHandCursor = { MouseCursor::PointingHandCursor };
    MouseCursor mTextCursor = { MouseCursor::IBeamCursor };

    int lastShownCount = 0;

    std::unique_ptr<Component> mChatContainer;
    std::unique_ptr<TextEditor> mChatTextEditor;
    std::unique_ptr<FocusTextEditor> mChatSendTextEditor;
    std::unique_ptr<SonoDrawableButton> mMenuButton;
    std::unique_ptr<SonoDrawableButton> mCloseButton;
    std::unique_ptr<Label> mTitleLabel;

    // URL position list
    struct cmpRange {
        bool operator()(const Range<int>& a, const Range<int>& b) const {
            return a.getStart() < b.getStart();
        }
    };

    typedef std::map<Range<int>, String, cmpRange> RangeStringMap;
    RangeStringMap mUrlRanges;

    //Array<Range<int> > mUrlRanges;

    std::unique_ptr<FileChooser> mFileChooser;


    Font mChatNameFont;
    Font mChatMesgFont;
    Font mChatMesgFixedFont;
    Font mChatEditFont;
    Font mChatEditFixedFont;
    Font mChatSpacerFont;
    String mLastChatEventFrom;
    std::map<String,Colour> mChatUserColors;

    FlexBox mainBox;
    FlexBox titleBox;
    FlexBox chatContainerBox;
    FlexBox chatSendBox;



    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ChatView)
};
