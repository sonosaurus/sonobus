// SPDX-License-Identifier: GPLv3-or-later WITH Appstore-exception
// Copyright (C) 2020 Jesse Chappell



#include "ChatView.h"
#include "GenericItemChooser.h"

void FocusTextEditor::focusGained (FocusChangeType gtype)
{
    TextEditor::focusGained(gtype);

    if (onFocusGained != nullptr) {
        onFocusGained(gtype);
    }
}


ChatView::ChatView(SonobusAudioProcessor& proc, AooServerConnectionInfo & connectinfo) : processor(proc), currConnectionInfo(connectinfo)
{
    updateFontSizes();

    setOpaque(true);

    mTitleLabel = std::make_unique<Label>("title", TRANS("Chat"));
    mTitleLabel->setJustificationType(Justification::centred);
    mTitleLabel->setFont(Font(18, Font::bold));
    mTitleLabel->setColour(Label::textColourId, Colour(0xeeffffff));

    mChatTextEditor = std::make_unique<TextEditor>();
    mChatTextEditor->setReadOnly(true);
    mChatTextEditor->setMultiLine(true);
    mChatTextEditor->setScrollbarsShown(true);
    mChatTextEditor->setScrollToShowCursor(false);
    mChatTextEditor->addMouseListener(this, false);
    mChatTextEditor->setTitle(TRANS("Chat Text"));

    mChatSendTextEditor = std::make_unique<FocusTextEditor>();
    mChatSendTextEditor->setMultiLine(true);
    mChatSendTextEditor->setScrollbarsShown(true);
    mChatSendTextEditor->setTextToShowWhenEmpty(TRANS("Enter message here..."), Colour(0x88bbbbbb));
    mChatSendTextEditor->setTitle(TRANS("Message Text"));
    mChatSendTextEditor->onReturnKey = [this]() {
        commitChatMessage();
    };
    mChatSendTextEditor->onFocusGained = [this](FocusChangeType gtype) {
        chatTextGainedFocus(gtype);
    };
    mChatSendTextEditor->onFocusLost = [this]() {
        chatTextLostFocus();
    };


    mChatSendTextEditor->setFont(processor.getChatUseFixedWidthFont() ? mChatEditFixedFont : mChatEditFont);

    mCloseButton = std::make_unique<SonoDrawableButton>("x", DrawableButton::ButtonStyle::ImageFitted);
    std::unique_ptr<Drawable> ximg(Drawable::createFromImageData(BinaryData::x_icon_svg, BinaryData::x_icon_svgSize));
    mCloseButton->setImages(ximg.get());
    mCloseButton->setTitle(TRANS("Close Chat"));
    mCloseButton->setColour(DrawableButton::backgroundColourId, Colours::transparentBlack);
    mCloseButton->onClick = [this]() {
        setVisible(false);
    };

    mMenuButton = std::make_unique<SonoDrawableButton>("menu", DrawableButton::ButtonStyle::ImageFitted);
    std::unique_ptr<Drawable> dotsimg(Drawable::createFromImageData(BinaryData::dots_svg, BinaryData::dots_svgSize));
    mMenuButton->setTitle(TRANS("Chat Menu"));
    mMenuButton->setImages(dotsimg.get());
    mMenuButton->setColour(DrawableButton::backgroundColourId, Colours::transparentBlack);
    mMenuButton->onClick = [this]() {
        showMenu(true);
    };

    //mChatSendButton = std::make_unique<TextButton>();
    //mChatSendButton->setButtonText(TRANS("Send"));
    //mChatSendButton->onClick = [this]() { commitChatMessage(); };


    addAndMakeVisible(mChatTextEditor.get());
    addAndMakeVisible(mChatSendTextEditor.get());
    addAndMakeVisible(mCloseButton.get());
    addAndMakeVisible(mTitleLabel.get());
    addAndMakeVisible(mMenuButton.get());

    int menubuttw = 36;
    int minitemheight = 36;
    int titleheight = 32;
    int minButtonWidth = 90;
#if JUCE_IOS || JUCE_ANDROID
    // make the button heights a bit more for touchscreen purposes
    minitemheight = 44;
    menubuttw = 40;
    titleheight = 34;
#endif
    int chatsendh = minitemheight + 16;

    titleBox.items.clear();
    titleBox.flexDirection = FlexBox::Direction::row;
    titleBox.items.add(FlexItem(4, 4).withMargin(0).withFlex(0));
    titleBox.items.add(FlexItem(menubuttw, 4, *mCloseButton).withMargin(0).withFlex(0));
    titleBox.items.add(FlexItem(minButtonWidth, 4, *mTitleLabel).withMargin(0).withFlex(1));
    titleBox.items.add(FlexItem(menubuttw, 4, *mMenuButton).withMargin(0).withFlex(0));
    titleBox.items.add(FlexItem(4, 4).withMargin(0).withFlex(0));


    chatSendBox.items.clear();
    chatSendBox.flexDirection = FlexBox::Direction::row;
    chatSendBox.items.add(FlexItem(4, 4).withMargin(0));
    chatSendBox.items.add(FlexItem(minButtonWidth, minitemheight, *mChatSendTextEditor).withMargin(0).withFlex(1));
    //chatSendBox.items.add(FlexItem(4, 4).withMargin(0));
    //chatSendBox.items.add(FlexItem(minKnobWidth, minitemheight, *mChatSendButton).withMargin(0));
    chatSendBox.items.add(FlexItem(4, 4).withMargin(0));


    chatContainerBox.items.clear();
    chatContainerBox.flexDirection = FlexBox::Direction::column;
    chatContainerBox.items.add(FlexItem(minButtonWidth, titleheight, titleBox).withMargin(0).withFlex(0));
    //chatContainerBox.items.add(FlexItem(2, 2).withMargin(0));
    chatContainerBox.items.add(FlexItem(minButtonWidth, minitemheight, *mChatTextEditor).withMargin(4).withFlex(1));
    chatContainerBox.items.add(FlexItem(2, 2).withMargin(0));
    chatContainerBox.items.add(FlexItem(minButtonWidth, chatsendh, chatSendBox).withMargin(0).withFlex(0));
    chatContainerBox.items.add(FlexItem(2, 4).withMargin(0));

    mainBox.items.clear();
    mainBox.flexDirection = FlexBox::Direction::row;
    mainBox.items.add(FlexItem(minButtonWidth, 4, chatContainerBox).withMargin(0).withFlex(1));

    setFocusContainerType(FocusContainerType::keyboardFocusContainer);

    refreshAllMessages();
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
    if (mKeyboardVisible) {
        int keybh = 0;
        // silly heuristics
#if JUCE_IOS || JUCE_ANDROID
        auto * display = Desktop::getInstance().getDisplays().getPrimaryDisplay();
        int maxheight = display ? display->userArea.getHeight() : 500;
        int maxwidth = display ? display->userArea.getWidth() : 300;
        keybh = maxwidth > 700 ? (maxwidth / 1.8f) : (maxwidth / 1.5f);
        keybh = jmin(maxheight > 700 ?  350 : 174, keybh);
        //keybh = (int) (maxwidth / 1.5f);
#endif
        mainBox.performLayout(getLocalBounds().withTrimmedBottom(keybh).reduced(2));
    }
    else {
        mainBox.performLayout(getLocalBounds().reduced(2));
    }
}

void ChatView::chatTextGainedFocus(FocusChangeType ctype)
{
#if JUCE_IOS || JUCE_ANDROID
    mKeyboardVisible = true;
    resized();
#endif
}

void ChatView::chatTextLostFocus()
{
#if JUCE_IOS || JUCE_ANDROID
    mKeyboardVisible = false;
    resized();
#endif
}


void ChatView::setUseFixedWidthFont(bool flag) {
    processor.setChatUseFixedWidthFont(flag);
    mChatSendTextEditor->setFont(flag ? mChatEditFixedFont : mChatEditFont);
}

void ChatView::updateFontSizes()
{
    auto offset = processor.getChatFontSizeOffset();

    mChatNameFont = Font(13 + offset);
    mChatMesgFont = Font(16 + offset);
    mChatMesgFixedFont = Font(Font::getDefaultMonospacedFontName(), 15+offset, Font::plain);
    mChatEditFont = Font(14 + offset);
    mChatEditFixedFont = Font(Font::getDefaultMonospacedFontName(), 15+offset, Font::plain);
    mChatSpacerFont = Font(6+offset);

    if (mChatSendTextEditor) {
        mChatSendTextEditor->setFont(processor.getChatUseFixedWidthFont() ? mChatEditFixedFont : mChatEditFont);
    }

}


void ChatView::setFocusToChat()
{
    mChatSendTextEditor->grabKeyboardFocus();
}


void ChatView::mouseDown (const MouseEvent& event)
{

}


bool ChatView::findUrlAtPos(Point<int> pos, String & retstr)
{
    // check for URL at this position
    auto index = mChatTextEditor->getTextIndexAt(pos.x, pos.y);
    DBG("Looking for URL at: index " << index);

    auto foundnext = mUrlRanges.upper_bound(Range<int>(index,index));

    if (!mUrlRanges.empty() && foundnext != mUrlRanges.begin()) {
        // check the one prior
        --foundnext;

        if (foundnext->first.contains(index)) {
            retstr = foundnext->second;
            return true;
        }
    }
    return false;
}


void ChatView::mouseUp (const MouseEvent& event)
{
    if (event.eventComponent == mChatTextEditor.get()) {
        if (event.getDistanceFromDragStart() < 4) {
            auto pos = event.getPosition();

            // check for URL at this position
            String urlstr;
            if (findUrlAtPos(pos, urlstr)) {
                URL url(urlstr);
                if (url.isWellFormed()) {
                    url.launchInDefaultBrowser();
                }
            }
        }
    }
}

void ChatView::mouseMove (const MouseEvent& event)
{
    if (event.eventComponent == mChatTextEditor.get()) {
        auto nowtimems = Time::getApproximateMillisecondCounter();
        if (nowtimems > mLastUrlCheckStampMs + 200) {
            auto pos = event.getPosition();
            String urlstr;

            auto over = findUrlAtPos(pos, urlstr);
            if (over != mOverUrl) {
                // set cursor
                if (over) {
                    mChatTextEditor->setMouseCursor(mHandCursor);
                } else {
                    mChatTextEditor->setMouseCursor(mTextCursor);
                }
                mOverUrl = over;
            }

            mLastUrlCheckStampMs = nowtimems;
        }

    }
}

void ChatView::mouseDrag (const MouseEvent& event)
{

}


void ChatView::addNewChatMessage(const SBChatEvent & mesg, bool refresh)
{
    processor.getAllChatEvents().add(mesg);

    if (refresh) {
        refreshMessages();
    }
}

void ChatView::addNewChatMessages(const Array<SBChatEvent> & mesgs, bool refresh)
{
    processor.getAllChatEvents().addArray(mesgs);

    if (refresh) {
        refreshMessages();
    }
}

void ChatView::refreshMessages()
{
    // only new ones since last refresh
    int count = jmin(processor.getAllChatEvents().size(), jmax(0, processor.getAllChatEvents().size() - lastShownCount));

    if (count > 0) {
        processNewChatMessages(processor.getAllChatEvents().size() - count, count);
    }
}

void ChatView::refreshAllMessages()
{
    // re-render all messages
    lastShownCount = 0;
    mLastChatMessageStamp = 0;
    mLastChatViewStamp = 0;

    mChatTextEditor->clear();
    processNewChatMessages(0, processor.getAllChatEvents().size());
}

struct FontSizeItemData : public GenericItemChooserItem::UserData
{
public:
    FontSizeItemData(const FontSizeItemData & other) : offset(other.offset) {}
    FontSizeItemData(int offs) : offset(offs) {}

    int offset;
};

void ChatView::showMenu(bool show)
{
    Array<GenericItemChooserItem> items;

    items.add(GenericItemChooserItem(TRANS("Save Chat..."), {}, nullptr, false));
    items.add(GenericItemChooserItem(TRANS("Clear Chat"), {}, nullptr, false));

    if (processor.getChatUseFixedWidthFont()) {
        items.add(GenericItemChooserItem(TRANS("Use Variable Width Font"), {}, nullptr, true));
    } else {
        items.add(GenericItemChooserItem(TRANS("Use Fixed Width Font"), {}, nullptr, true));
    }
    items.add(GenericItemChooserItem(TRANS("Font Size..."), {}, nullptr, false));

    Component* dw = mMenuButton->findParentComponentOfClass<AudioProcessorEditor>();
    if (!dw) dw = mMenuButton->findParentComponentOfClass<Component>();
    Rectangle<int> bounds =  dw->getLocalArea(nullptr, mMenuButton->getScreenBounds());

    SafePointer<ChatView> safeThis(this);

    auto callback = [safeThis,dw,bounds](GenericItemChooser* chooser,int index) mutable {
        if (!safeThis) return;

        if (index == 0) {
            // save
            safeThis->showSaveChat();
        } else if (index == 1) {

            Array<GenericItemChooserItem> citems;
            citems.add(GenericItemChooserItem(TRANS("Confirm Clear Chat")));

            auto callback = [safeThis](GenericItemChooser* chooser,int index) mutable {
                if (!safeThis) return;

                safeThis->clearAll();
            };

            GenericItemChooser::launchPopupChooser(citems, bounds, dw, callback, -1, dw ? dw->getHeight()-30 : 0);
        }
        else if (index == 2) {
            // fixed/variable width
            safeThis->setUseFixedWidthFont(!safeThis->processor.getChatUseFixedWidthFont());
            safeThis->refreshAllMessages();
        } else if (index == 3) {
            // font size popup
            Array<GenericItemChooserItem> citems;
            citems.add(GenericItemChooserItem(TRANS("Tiny"), {}, std::make_shared<FontSizeItemData>(-3)));
            citems.add(GenericItemChooserItem(TRANS("Small"), {}, std::make_shared<FontSizeItemData>(-1)));
            citems.add(GenericItemChooserItem(TRANS("Normal"), {}, std::make_shared<FontSizeItemData>(0)));
            citems.add(GenericItemChooserItem(TRANS("Large"), {}, std::make_shared<FontSizeItemData>(2)));
            citems.add(GenericItemChooserItem(TRANS("Huge"), {}, std::make_shared<FontSizeItemData>(4)));

            int selindex = safeThis->processor.getChatFontSizeOffset() + 2;

            auto callback = [safeThis](GenericItemChooser* chooser,int index) mutable {
                if (!safeThis) return;
                auto & selitem = chooser->getItems().getReference(index);
                int offset = 0;
                auto dclitem = std::dynamic_pointer_cast<FontSizeItemData>(selitem.userdata);
                if (dclitem) {
                    offset = dclitem->offset;
                }
                safeThis->processor.setChatFontSizeOffset(offset);
                safeThis->updateFontSizes();
                safeThis->refreshAllMessages();
            };

            //Rectangle<int> bounds =  dw->getLocalArea(nullptr, chooser->getScreenBounds());

            GenericItemChooser::launchPopupChooser(citems, bounds, dw, callback, selindex, dw ? dw->getHeight()-30 : 0, true);
        }
    };

    GenericItemChooser::launchPopupChooser(items, bounds, dw, callback, -1, dw ? dw->getHeight()-30 : 0);
}

void ChatView::clearAll()
{
    mChatTextEditor->clear();
    processor.getAllChatEvents().clearQuick();
    mLastChatMessageStamp = 0;
    mLastChatViewStamp = 0;
    mLastChatUserMessageStamp = 0;
    mLastChatEventFrom.clear();
    lastShownCount = 0;
    mUrlRanges.clear();
}


void ChatView::showSaveChat()
{
    SafePointer<ChatView> safeThis (this);

    if (FileChooser::isPlatformDialogAvailable())
    {
        File recdir; // = File::getSpecialLocation(File::userDocumentsDirectory).getChildFile("SonoBus Setups");

        // TODO - on iOS we need to give it a name first
//#if (JUCE_IOS || JUCE_ANDROID)
        String filename = String("SonoBusChat_") + Time::getCurrentTime().formatted("%Y-%m-%d_%H.%M.%S");
        recdir = File::getSpecialLocation(File::userDocumentsDirectory).getNonexistentChildFile (filename, ".txt");
//#endif

        mFileChooser.reset(new FileChooser(TRANS("Choose a location and name to store the setup"),
                                           recdir,
                                           "*.txt",
                                           true, false, getTopLevelComponent()));



        mFileChooser->launchAsync (FileBrowserComponent::saveMode | FileBrowserComponent::doNotClearFileNameOnRootChange,
                                   [safeThis] (const FileChooser& chooser) mutable
                                   {
            auto results = chooser.getURLResults();
            if (safeThis != nullptr && results.size() > 0)
            {
                auto url = results.getReference (0);

                DBG("Chose file to save chat: " <<  url.toString(false));

                if (url.isLocalFile()) {
                    File lfile = url.getLocalFile();

                    lfile.replaceWithText(safeThis->mChatTextEditor->getText());
                }
            }

            if (safeThis) {
                safeThis->mFileChooser.reset();
            }

        }, nullptr);

    }
    else {
        DBG("Need to enable code signing");
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

void ChatView::processNewChatMessages(int index, int count)
{
    if (index < 0) return;

    // for now just add their text to the big texteditor showing all chats
    const Colour selfNameColour(0xaaeeeeff);
    const Colour selfBgColour(0x88222244);
    const Colour otherNameColour(0xaaffffaa);
    const Colour selfTextColour(0xffeeeeee);
    const Colour otherTextColour(0xffffffaa);
    const Colour sysTextColour(0xffffaaaa);
    const Colour urlTextColour(0xff44aaff);

    mChatTextEditor->moveCaretToEnd();

    double nowtime = Time::getMillisecondCounterHiRes() * 1e-3;
    bool gotuser = false;
    Array<Range<int> >  urlranges;

    // calculate if we should auto-scroll
    auto startCaretInd = mChatTextEditor->getCaretPosition();
    auto botind =  getWidth() > 0 ? mChatTextEditor->getTextIndexAt(mChatTextEditor->getLocalBounds().getRight(), mChatTextEditor->getLocalBounds().getBottom()) : 0;
    bool doscroll = abs(botind - startCaretInd) < 10;
    //DBG("index: " << startCaretInd << "  botind: " << botind  << "  locbounds: " << mChatTextEditor->getLocalBounds().toString() << "  doscroll: " << (int)doscroll);

    bool fixedwidth = processor.getChatUseFixedWidthFont();

    auto & allChatEvents = processor.getAllChatEvents();

    for (int i=index; i < index+count && i < allChatEvents.size(); ++i) {
        auto & event = allChatEvents.getReference(i);

        bool showtime = nowtime > mLastChatMessageStamp + 60.0 || nowtime > mLastChatShowTimeStamp + 60.0;

        Colour usecolor;

        if (event.type == SBChatEvent::SelfType) {
            usecolor = selfTextColour;
            doscroll = true;
        }
        else if (event.type == SBChatEvent::SystemType) {
            usecolor = sysTextColour;
        }
        else {
            if (mChatUserColors.find(event.from) == mChatUserColors.end()) {
                // create a new color for them
                mChatUserColors[event.from] = usecolor = Colour::fromHSL(Random::getSystemRandom().nextFloat(), 0.5f, 0.8f, 1.0f);
            } else {
                usecolor = mChatUserColors[event.from];
            }
            gotuser = true;
        }

        Colour namecolor = usecolor.withAlpha(0.7f);

        mChatTextEditor->setFont(mChatSpacerFont);
        mChatTextEditor->insertTextAtCaret("\n");

        //mChatTextEditor->moveCaretDown(false);
        //mChatTextEditor->moveCaretToStartOfLine(false);

        if (event.type == SBChatEvent::SystemType) {
            mChatTextEditor->setColour(TextEditor::textColourId, usecolor);
            mChatTextEditor->setFont(mChatNameFont);
            mChatTextEditor->insertTextAtCaret("==  ");
            mChatTextEditor->insertTextAtCaret(event.message);

            mChatTextEditor->insertTextAtCaret("      ");
            mChatTextEditor->insertTextAtCaret(Time::getCurrentTime().toString(false, true, false));
            mLastChatShowTimeStamp = nowtime;

            mChatTextEditor->insertTextAtCaret("\n");
        }
        else {
            if (event.from != mLastChatEventFrom || showtime) {
                mChatTextEditor->setColour(TextEditor::textColourId, namecolor);
                mChatTextEditor->setFont(mChatNameFont);
                mChatTextEditor->insertTextAtCaret(event.from);

                mChatTextEditor->insertTextAtCaret("      ");
                mChatTextEditor->insertTextAtCaret(Time::getCurrentTime().toString(false, true, false));

                mChatTextEditor->insertTextAtCaret("\n");
                mLastChatShowTimeStamp = nowtime;
            }

            mChatTextEditor->setColour(TextEditor::textColourId, usecolor);
            if (fixedwidth) {
                mChatTextEditor->setFont(mChatMesgFixedFont);
            } else {
                mChatTextEditor->setFont(mChatMesgFont);
            }

            if (parseStringForUrl(event.message, urlranges)) {
                // found URLS, write them out in blue
                String mesg = event.message;
                int pos = 0;
                for (auto & rng : urlranges) {
                    int urlstart = rng.getStart();
                    int urllen = rng.getLength();

                    // write the text before the next url
                    mChatTextEditor->insertTextAtCaret(event.message.substring(pos, urlstart));

                    // write the URL
                    auto urlstr = event.message.substring(urlstart, urlstart+urllen);
                    auto posInChat = mChatTextEditor->getCaretPosition();
                    mChatTextEditor->setColour(TextEditor::textColourId, urlTextColour);
                    mChatTextEditor->insertTextAtCaret(urlstr);
                    mChatTextEditor->setColour(TextEditor::textColourId, usecolor);

                    // store the range
                    mUrlRanges[Range<int>(posInChat, posInChat+urllen)] = urlstr;
                    //mUrlRanges.add();

                    pos = urlstart + urllen;
                }

                // write what's left
                if (pos < event.message.length()) {
                    mChatTextEditor->insertTextAtCaret(event.message.substring(pos));
                }
            }
            else {
                mChatTextEditor->insertTextAtCaret(event.message);
            }
            mChatTextEditor->insertTextAtCaret("\n");

            mLastChatEventFrom = event.from;
            mLastChatUserMessageStamp = nowtime;
        }

        mLastChatMessageStamp = nowtime;
    }

    if (doscroll) {
        mChatTextEditor->scrollEditorToPositionCaret(0, mChatTextEditor->getLocalBounds().getBottom());
    }

    if (isVisible()) {
        mLastChatViewStamp = mLastChatUserMessageStamp;
    }

    lastShownCount = index + count;
}

void ChatView::visibilityChanged()
{
    if (isVisible()) {
        mLastChatViewStamp = Time::getMillisecondCounterHiRes() * 1e-3;
    }
}


bool ChatView::parseStringForUrl(const String & str, Array<Range<int> > & retranges)
{
    retranges.clearQuick();

    int pos = 0;

    while (pos >= 0 && pos < str.length()) {
        // super basic
        auto found = str.indexOfIgnoreCase(pos, "https://");
        if (found < 0) found = str.indexOfIgnoreCase(pos, "http://");
        if (found < 0) break;

        auto endpos = str.indexOfIgnoreCase(found, " ");
        if (endpos < 0) endpos = str.indexOfIgnoreCase(found, "\n");
        if (endpos < 0) endpos = str.length();

        auto urlstr = str.substring(found, endpos);
        DBG("Found URL: '" << urlstr << "'  at " << found << "," << endpos << "  len: " << endpos - found);
        retranges.add(Range<int>(found, endpos));

        pos = endpos;
    }

    return !retranges.isEmpty();
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

    event.type = SBChatEvent::SelfType;

    // process self

    processor.getAllChatEvents().add(event);
    processNewChatMessages(processor.getAllChatEvents().size()-1, 1);

    mChatSendTextEditor->clear();
    mChatSendTextEditor->repaint();
}




bool ChatView::haveNewSinceLastView() const
{
    return mLastChatUserMessageStamp > mLastChatViewStamp;
}
