// SPDX-License-Identifier: GPLv3-or-later WITH Appstore-exception
// Copyright (C) 2020 Jesse Chappell



#include "SuggestNewGroupView.h"
#include "GenericItemChooser.h"

SuggestNewGroupView::SuggestNewGroupView(SonobusAudioProcessor& proc) :  smallLNF(14), processor(proc)
{
    smallLNF.textJustification = Justification::centred;

    mRowsContainer = std::make_unique<Component>();

    mViewport = std::make_unique<Viewport>();
    mViewport->setViewedComponent(mRowsContainer.get(), false);


    mTitleLabel = std::make_unique<Label>("title", TRANS("Suggest New Group"));
    mTitleLabel->setJustificationType(Justification::centred);
    mTitleLabel->setFont(Font(16, Font::bold));
    mTitleLabel->setColour(Label::textColourId, Colour(0xeeffffff));

    mGroupStaticLabel = std::make_unique<Label>("title", TRANS("Group"));
    mGroupStaticLabel->setJustificationType(Justification::centred);
    mGroupStaticLabel->setFont(Font(14, Font::plain));
    mGroupStaticLabel->setColour(Label::textColourId, Colour(0xeeffffff));

    mGroupEditor = std::make_unique<TextEditor>("group");
    mGroupEditor->setFont(Font(16, Font::plain));
    mGroupEditor->setColour(Label::textColourId, Colour(0xeeffffff));
    mGroupEditor->setMultiLine(false);
    mGroupEditor->setIndents(8, 8);

    mGroupPassStaticLabel = std::make_unique<Label>("title", TRANS("Password"));
    mGroupPassStaticLabel->setJustificationType(Justification::centred);
    mGroupPassStaticLabel->setFont(Font(16, Font::plain));
    mGroupPassStaticLabel->setColour(Label::textColourId, Colour(0xeeffffff));

    mGroupPassEditor = std::make_unique<TextEditor>("passwd");
    mGroupPassEditor->setFont(Font(14, Font::plain));
    mGroupPassEditor->setColour(Label::textColourId, Colour(0xeeffffff));
    mGroupPassEditor->setTextToShowWhenEmpty(TRANS("optional"), Colour(0x44ffffff));
    mGroupPassEditor->setMultiLine(false);
    mGroupPassEditor->setIndents(8, 8);

    mPublicToggle = std::make_unique<ToggleButton>();
    mPublicToggle->setButtonText(TRANS("Public"));
    mPublicToggle->onStateChange = [this]() {
        if (mPublicToggle->getToggleState()) {
            mGroupPassEditor->setEnabled(false);
            mGroupPassEditor->setAlpha(0.5f);
            mGroupPassStaticLabel->setAlpha(0.5f);
        } else {
            mGroupPassEditor->setEnabled(true);
            mGroupPassEditor->setAlpha(1.0f);
            mGroupPassStaticLabel->setAlpha(1.0f);
        }
    };

    mCloseButton = std::make_unique<SonoDrawableButton>("x", DrawableButton::ButtonStyle::ImageFitted);
    std::unique_ptr<Drawable> ximg(Drawable::createFromImageData(BinaryData::x_icon_svg, BinaryData::x_icon_svgSize));
    mCloseButton->setImages(ximg.get());
    mCloseButton->setTitle(TRANS("Close"));
    mCloseButton->setColour(DrawableButton::backgroundColourId, Colours::transparentBlack);
    mCloseButton->onClick = [this]() {
        dismissSelf();
    };

    mSelectAllButton = std::make_unique<TextButton>("x");
    mSelectAllButton->setLookAndFeel(&smallLNF);
    mSelectAllButton->setButtonText(TRANS("Select All"));
    mSelectAllButton->setTitle(mSelectAllButton->getButtonText());
    mSelectAllButton->onClick = [this]() {
        selectedPeers.clear();
        auto numpeers = processor.getNumberRemotePeers();
        for (int i = 0; i < numpeers; ++i) {
            selectedPeers.insert(processor.getRemotePeerUserName(i));
        }
        updatePeerRows(true);
    };

    mSelectNoneButton = std::make_unique<TextButton>("x");
    mSelectNoneButton->setLookAndFeel(&smallLNF);
    mSelectNoneButton->setButtonText(TRANS("Select None"));
    mSelectNoneButton->setTitle(mSelectNoneButton->getButtonText());
    mSelectNoneButton->onClick = [this]() {
        selectedPeers.clear();
        updatePeerRows(true);
    };


    mRequestButton = std::make_unique<TextButton>("x");
    mRequestButton->setButtonText(TRANS("Suggest and Connect to Group"));
    mRequestButton->setColour(TextButton::buttonColourId, Colour::fromFloatRGBA(0.1, 0.4, 0.6, 0.6));

    mRequestButton->onClick = [this]() {
        if (mGroupEditor->getText().isEmpty()) {
            mGroupEditor->setColour(TextEditor::backgroundColourId, Colours::red);
            mGroupEditor->repaint();
            return;
        } else {
            mGroupEditor->setColour(TextEditor::backgroundColourId, Colours::black);
            mGroupEditor->repaint();
        }

        StringArray peers;
        for (auto peer : selectedPeers) {
            peers.add(peer);
        }

        if (!peers.isEmpty()) {
            processor.suggestNewGroupToPeers(mGroupEditor->getText(), mGroupPassEditor->getText(), peers, mPublicToggle->getToggleState());

            if (connectToGroup) {
                Timer::callAfterDelay(500, [this] {
                    connectToGroup(mGroupEditor->getText(), mGroupPassEditor->getText(), mPublicToggle->getToggleState());
                    dismissSelf();
                });
            }
        }
    };


    mPeerRect = std::make_unique<DrawableRectangle>();
    mPeerRect->setFill(Colour::fromFloatRGBA(0.0, 0.0, 0.0, 1.0));
    mPeerRect->setCornerSize(Point<float>(8,8));

    addAndMakeVisible(mRequestButton.get());
    addAndMakeVisible(mGroupEditor.get());
    addAndMakeVisible(mGroupStaticLabel.get());
    addAndMakeVisible(mGroupPassEditor.get());
    addAndMakeVisible(mGroupPassStaticLabel.get());
    addAndMakeVisible(mPublicToggle.get());
    addAndMakeVisible(mPeerRect.get());
    addAndMakeVisible(mViewport.get());
    addAndMakeVisible(mCloseButton.get());
    addAndMakeVisible(mSelectAllButton.get());
    addAndMakeVisible(mSelectNoneButton.get());
    addAndMakeVisible(mTitleLabel.get());

    int menubuttw = 36;
    int minitemheight = 32;
    int titleheight = 32;
    int minButtonWidth = 90;
    int minLabelWidth = 70;
#if JUCE_IOS || JUCE_ANDROID
    // make the button heights a bit more for touchscreen purposes
    minitemheight = 42;
    menubuttw = 40;
    titleheight = 40;
#endif

    titleBox.items.clear();
    titleBox.flexDirection = FlexBox::Direction::row;
    titleBox.items.add(FlexItem(4, 4).withMargin(0).withFlex(0));
    titleBox.items.add(FlexItem(menubuttw, 4, *mCloseButton).withMargin(0).withFlex(0));
    titleBox.items.add(FlexItem(minButtonWidth, 4, *mTitleLabel).withMargin(0).withFlex(1));
    //titleBox.items.add(FlexItem(menubuttw, 4, *mMenuButton).withMargin(0).withFlex(0));
    titleBox.items.add(FlexItem(menubuttw, 4).withMargin(0).withFlex(0));
    titleBox.items.add(FlexItem(4, 4).withMargin(0).withFlex(0));

    matchButtBox.items.clear();
    matchButtBox.flexDirection = FlexBox::Direction::row;
    matchButtBox.items.add(FlexItem(4, 4).withMargin(0).withFlex(0.5));
    matchButtBox.items.add(FlexItem(minButtonWidth, 4, *mRequestButton).withMargin(0).withFlex(4));
    matchButtBox.items.add(FlexItem(4, 4).withMargin(0).withFlex(0.5));

    selButtBox.items.clear();
    selButtBox.flexDirection = FlexBox::Direction::row;
    selButtBox.items.add(FlexItem(4, 4).withMargin(0).withFlex(1));
    selButtBox.items.add(FlexItem(minButtonWidth, 4, *mSelectAllButton).withMargin(0).withFlex(4));
    selButtBox.items.add(FlexItem(4, 4).withMargin(0).withFlex(1));
    selButtBox.items.add(FlexItem(minButtonWidth, 4, *mSelectNoneButton).withMargin(0).withFlex(4));
    selButtBox.items.add(FlexItem(4, 4).withMargin(0).withFlex(1));

    groupBox.items.clear();
    groupBox.flexDirection = FlexBox::Direction::row;
    groupBox.items.add(FlexItem(minLabelWidth, minitemheight, *mGroupStaticLabel).withMargin(2).withFlex(0));
    groupBox.items.add(FlexItem(minButtonWidth, minitemheight, *mGroupEditor).withMargin(2).withFlex(1));

    groupPassBox.items.clear();
    groupPassBox.flexDirection = FlexBox::Direction::row;
    groupPassBox.items.add(FlexItem(minLabelWidth, minitemheight, *mGroupPassStaticLabel).withMargin(2).withFlex(0));
    groupPassBox.items.add(FlexItem(minButtonWidth, minitemheight, *mGroupPassEditor).withMargin(2).withFlex(1));
    groupPassBox.items.add(FlexItem(minButtonWidth, minitemheight, *mPublicToggle).withMargin(2).withFlex(0));





    mainBox.items.clear();
    mainBox.flexDirection = FlexBox::Direction::column;
    mainBox.items.add(FlexItem(minButtonWidth, titleheight, titleBox).withMargin(0).withFlex(0));
    mainBox.items.add(FlexItem(4, 4).withMargin(0).withFlex(0));
    mainBox.items.add(FlexItem(minButtonWidth, minitemheight + 2, groupBox).withMargin(0).withFlex(0));
    mainBox.items.add(FlexItem(4, 2).withMargin(0).withFlex(0));
    mainBox.items.add(FlexItem(minButtonWidth, minitemheight + 2, groupPassBox).withMargin(0).withFlex(0));
    mainBox.items.add(FlexItem(4, 4).withMargin(0).withFlex(0));
    mainBox.items.add(FlexItem(minButtonWidth, minitemheight, selButtBox).withMargin(0).withFlex(0));
    mainBox.items.add(FlexItem(4, 4).withMargin(0).withFlex(0));
    mainBox.items.add(FlexItem(minButtonWidth, 100, *mViewport).withMargin(0).withFlex(1));
    mainBox.items.add(FlexItem(4, 4).withMargin(0).withFlex(0));
    mainBox.items.add(FlexItem(minButtonWidth, minitemheight, matchButtBox).withMargin(0).withFlex(0));


    updatePeerRows();

    startTimer(1, 500);
}



SuggestNewGroupView::~SuggestNewGroupView()
{
}

void SuggestNewGroupView::dismissSelf()
{
    if (CallOutBox* const cb = findParentComponentOfClass<CallOutBox>()) {
        cb->dismiss();
    } else {
        setVisible(false);
    }
}


void SuggestNewGroupView::paint (Graphics& g)
{
    //g.fillAll (Colour(0xff272727));
}

void SuggestNewGroupView::resized()
{
    mainBox.performLayout(getLocalBounds().reduced(2));

    auto rbounds = mViewport->getBounds();
    mPeerRect->setRectangle(rbounds.toFloat());
}

ToggleButton * SuggestNewGroupView::createPeerToggle()
{
    auto * toggle = new ToggleButton();
    toggle->onStateChange = [this, toggle] {
        if (toggle->getToggleState()) {
            selectedPeers.insert(toggle->getButtonText());
        } else {
            selectedPeers.erase(toggle->getButtonText());
        }
    };

    return toggle;
}

Label * SuggestNewGroupView::createPeerLabel()
{
    auto * label = new Label();
    label->setFont(12);
    label->setColour(Label::textColourId, Colour(0xaaaaaaaa));
    label->setJustificationType(Justification::centredLeft);
    label->setMinimumHorizontalScale(0.4);
    return label;
}

void SuggestNewGroupView::timerCallback(int timerid)
{
    if (timerid == 1) {
        // check if number of group users changes
        if (processor.getNumberRemotePeers() != mPeerToggles.size()) {

            updatePeerRows();
        }

    }

    if (!isShowing()) {
        stopTimer(1);
    }
}


void SuggestNewGroupView::updatePeerRows(bool force)
{
    int minitemheight = 22;
    int minButtonWidth = 90;

    peerRowsBox.items.clear();
    peerRowsBox.flexDirection = FlexBox::Direction::column;

    auto numpeers = processor.getNumberRemotePeers();
    bool layoutchange = false;

    while (mPeerToggles.size() < numpeers) {
        auto * toggle = mPeerToggles.add(createPeerToggle());
        mRowsContainer->addAndMakeVisible(toggle);

        layoutchange = true;
    }
    while (mPeerToggles.size() > numpeers) {
        mPeerToggles.removeLast();
        layoutchange = true;
    }

    for (int i=0; i < mPeerToggles.size(); ++i) {
        auto * toggle = mPeerToggles.getUnchecked(i);

        String peerstr = processor.getRemotePeerUserName(i);
        toggle->setButtonText(peerstr);

        toggle->setToggleState(selectedPeers.find(peerstr) != selectedPeers.end(), dontSendNotification);

        peerRowsBox.items.add(FlexItem(minButtonWidth, minitemheight, *toggle).withMargin(2).withFlex(1));
    }

    if (layoutchange || force) {

        int mch = 0;
        for (auto & item : peerRowsBox.items) {
            mch += item.minHeight + item.margin.top + item.margin.bottom;
        }

        mRowsContainer->setBounds(0, 0, mViewport->getWidth() - 10, mch);

        peerRowsBox.performLayout(mRowsContainer->getLocalBounds().reduced(2));
    }
}

