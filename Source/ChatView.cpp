// SPDX-License-Identifier: GPLv3-or-later
// Copyright (C) 2020 Jesse Chappell



#include "ChatView.h"

ChatView::ChatView(SonobusAudioProcessor& proc, AooServerConnectionInfo & connectinfo) : processor(proc), currConnectionInfo(connectinfo)
{
    mChatNameFont = Font(13);
    mChatMesgFont = Font(16);
    mChatSpacerFont = Font(6);

    setOpaque(true);

    mTitleLabel = std::make_unique<Label>("title", TRANS("Chat"));
    mTitleLabel->setJustificationType(Justification::centred);
    mTitleLabel->setFont(Font(20, Font::bold));
    mTitleLabel->setColour(Label::textColourId, Colour(0xeeffffff));

    mChatTextEditor = std::make_unique<TextEditor>();
    mChatTextEditor->setReadOnly(true);
    mChatTextEditor->setMultiLine(true);
    mChatTextEditor->setScrollbarsShown(true);

    mChatSendTextEditor = std::make_unique<TextEditor>();
    mChatSendTextEditor->setMultiLine(true);
    mChatSendTextEditor->setScrollbarsShown(true);
    mChatSendTextEditor->setTextToShowWhenEmpty(TRANS("Enter message here..."), Colour(0x88bbbbbb));
    mChatSendTextEditor->onReturnKey = [this]() {
        commitChatMessage();
    };
    mChatSendTextEditor->setFont(Font(14));

    mCloseButton = std::make_unique<SonoDrawableButton>("x", DrawableButton::ButtonStyle::ImageFitted);
    std::unique_ptr<Drawable> ximg(Drawable::createFromImageData(BinaryData::x_icon_svg, BinaryData::x_icon_svgSize));
    mCloseButton->setImages(ximg.get());
    mCloseButton->setColour(DrawableButton::backgroundColourId, Colours::transparentBlack);
    mCloseButton->onClick = [this]() {
        setVisible(false);
    };


    //mChatSendButton = std::make_unique<TextButton>();
    //mChatSendButton->setButtonText(TRANS("Send"));
    //mChatSendButton->onClick = [this]() { commitChatMessage(); };


    addAndMakeVisible(mChatTextEditor.get());
    addAndMakeVisible(mChatSendTextEditor.get());
    addAndMakeVisible(mCloseButton.get());
    addAndMakeVisible(mTitleLabel.get());
    //addAndMakeVisible(mChatSendButton.get());

    int minitemheight = 36;
    int minButtonWidth = 90;
#if JUCE_IOS || JUCE_ANDROID
    // make the button heights a bit more for touchscreen purposes
    minitemheight = 44;
#endif
    int chatsendh = minitemheight + 16;

    titleBox.items.clear();
    titleBox.flexDirection = FlexBox::Direction::row;
    titleBox.items.add(FlexItem(minitemheight, 4, *mCloseButton).withMargin(0).withFlex(0));
    titleBox.items.add(FlexItem(minButtonWidth, 4, *mTitleLabel).withMargin(0).withFlex(1));


    chatSendBox.items.clear();
    chatSendBox.flexDirection = FlexBox::Direction::row;
    chatSendBox.items.add(FlexItem(4, 4).withMargin(0));
    chatSendBox.items.add(FlexItem(minButtonWidth, minitemheight, *mChatSendTextEditor).withMargin(0).withFlex(1));
    //chatSendBox.items.add(FlexItem(4, 4).withMargin(0));
    //chatSendBox.items.add(FlexItem(minKnobWidth, minitemheight, *mChatSendButton).withMargin(0));
    chatSendBox.items.add(FlexItem(4, 4).withMargin(0));


    chatContainerBox.items.clear();
    chatContainerBox.flexDirection = FlexBox::Direction::column;
    chatContainerBox.items.add(FlexItem(minButtonWidth, minitemheight, titleBox).withMargin(2).withFlex(0));
    //chatContainerBox.items.add(FlexItem(2, 2).withMargin(0));
    chatContainerBox.items.add(FlexItem(minButtonWidth, minitemheight, *mChatTextEditor).withMargin(4).withFlex(1));
    chatContainerBox.items.add(FlexItem(2, 2).withMargin(0));
    chatContainerBox.items.add(FlexItem(minButtonWidth, chatsendh, chatSendBox).withMargin(0).withFlex(0));
    chatContainerBox.items.add(FlexItem(2, 4).withMargin(0));

    mainBox.items.clear();
    mainBox.flexDirection = FlexBox::Direction::row;
    mainBox.items.add(FlexItem(minButtonWidth, 4, chatContainerBox).withMargin(0).withFlex(1));

}

ChatView::~ChatView()
{
}

void ChatView::paint (Graphics& g)
{
    g.fillAll (Colour(0xff272727));
}

void ChatView::resized()
{
    mainBox.performLayout(getLocalBounds().reduced(2));
}

void ChatView::addNewChatMessages(const Array<SBChatEvent> & mesgs, bool refresh)
{
    allChatEvents.addArray(mesgs);

    if (refresh) {
        refreshMessages();
    }
}

void ChatView::refreshMessages()
{
    // only new ones since last refresh
    int count = jmin(allChatEvents.size(), jmax(0, allChatEvents.size() - lastShownCount));

    if (count > 0) {
        processNewChatMessages(allChatEvents.size() - count, count);
    }
}

bool ChatView::keyPressed (const KeyPress & key)
{
    DBG("Chat: Got key: " << key.getTextCharacter() << "  isdown: " << (key.isCurrentlyDown() ? 1 : 0) << " keycode: " << key.getKeyCode() << " pcode: " << (int)'p');

    if (mChatSendTextEditor->hasKeyboardFocus(true)) {
        if (key.isKeyCode(KeyPress::returnKey) && (ModifierKeys::currentModifiers.isCtrlDown() || ModifierKeys::currentModifiers.isShiftDown())) {
            // insert line break
            mChatSendTextEditor->insertTextAtCaret("\n");
            return true;
        }
    }
    return false;
}

void ChatView::processNewChatMessages(int index, int count, bool isSelf)
{
    if (index < 0) return;

    // for now just add their text to the big texteditor showing all chats
    const Colour selfNameColour(0xaaeeeeff);
    const Colour selfBgColour(0x88222244);
    const Colour otherNameColour(0xaaffffaa);
    const Colour selfTextColour(0xffeeeeee);
    const Colour otherTextColour(0xffffffaa);

    mChatTextEditor->moveCaretToEnd();

    double nowtime = Time::getMillisecondCounterHiRes() * 1e-3;
    bool showtime = nowtime > mLastChatMessageStamp + 60.0;

    for (int i=index; i < index+count && i < allChatEvents.size(); ++i) {
        auto & event = allChatEvents.getReference(i);

        Colour usercolor;

        if (isSelf) {
            usercolor = selfTextColour;
        }
        else {
            if (mChatUserColors.find(event.from) == mChatUserColors.end()) {
                // create a new color for them
                mChatUserColors[event.from] = usercolor = Colour::fromHSL(Random::getSystemRandom().nextFloat(), 0.5f, 0.8f, 1.0f);
            } else {
                usercolor = mChatUserColors[event.from];
            }
        }

        Colour namecolor = usercolor.withAlpha(0.7f);

        mChatTextEditor->setFont(mChatSpacerFont);
        mChatTextEditor->insertTextAtCaret("\n");

        //mChatTextEditor->moveCaretDown(false);
        //mChatTextEditor->moveCaretToStartOfLine(false);

        if (event.from != mLastChatEventFrom || showtime) {
            mChatTextEditor->setColour(TextEditor::textColourId, namecolor);
            mChatTextEditor->setFont(mChatNameFont);
            mChatTextEditor->insertTextAtCaret(event.from);

            mChatTextEditor->insertTextAtCaret("      ");
            mChatTextEditor->insertTextAtCaret(Time::getCurrentTime().toString(false, true, false));

            mChatTextEditor->insertTextAtCaret("\n");
        }

        mChatTextEditor->setColour(TextEditor::textColourId, usercolor);
        mChatTextEditor->setFont(mChatMesgFont);
        mChatTextEditor->insertTextAtCaret(event.message);
        mChatTextEditor->insertTextAtCaret("\n");

        mLastChatEventFrom = event.from;
    }

    mLastChatMessageStamp = nowtime;

    lastShownCount = index + count;
}

void ChatView::commitChatMessage()
{
    // send it
    auto text = mChatSendTextEditor->getText();
    if (text.isEmpty()) return;

    SBChatEvent event;

    event.from = currConnectionInfo.userName;
    event.group = currConnectionInfo.groupName;
    event.message = text;
    // todo target(s), tags

    processor.sendChatEvent(event);

    // process self
    allChatEvents.add(event);
    processNewChatMessages(allChatEvents.size()-1, 1, true);

    mChatSendTextEditor->clear();
    mChatSendTextEditor->repaint();
}

