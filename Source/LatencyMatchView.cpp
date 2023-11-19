// SPDX-License-Identifier: GPLv3-or-later WITH Appstore-exception
// Copyright (C) 2020 Jesse Chappell



#include "LatencyMatchView.h"
#include "GenericItemChooser.h"

LatencyMatchView::LatencyMatchView(SonobusAudioProcessor& proc) :  sonoSliderLNF(12), processor(proc)
{
    sonoSliderLNF.textJustification = Justification::centredRight;

    mRowsContainer = std::make_unique<Component>();

    mViewport = std::make_unique<Viewport>();
    mViewport->setViewedComponent(mRowsContainer.get(), false);

    mMainSlider = std::make_unique<Slider>(Slider::LinearBar,  Slider::TextBoxAbove);
    mMainSlider->setTextValueSuffix(" ms");
    //mMainSlider->getProperties().set ("noFill", true);
    mMainSlider->setScrollWheelEnabled(false);

    mTitleLabel = std::make_unique<Label>("title", TRANS("Group Latency Match"));
    mTitleLabel->setJustificationType(Justification::centred);
    mTitleLabel->setFont(Font(16, Font::bold));
    mTitleLabel->setColour(Label::textColourId, Colour(0xeeffffff));

    mMainSliderLabel = std::make_unique<Label>("title", TRANS("Target Latency"));
    mMainSliderLabel->setJustificationType(Justification::centred);
    mMainSliderLabel->setFont(Font(14, Font::plain));
    mMainSliderLabel->setColour(Label::textColourId, Colour(0xeeffffff));

    mCloseButton = std::make_unique<SonoDrawableButton>("x", DrawableButton::ButtonStyle::ImageFitted);
    std::unique_ptr<Drawable> ximg(Drawable::createFromImageData(BinaryData::x_icon_svg, BinaryData::x_icon_svgSize));
    mCloseButton->setImages(ximg.get());
    mCloseButton->setColour(DrawableButton::backgroundColourId, Colours::transparentBlack);
    mCloseButton->onClick = [this]() {
        if (CallOutBox* const cb = findParentComponentOfClass<CallOutBox>()) {
            cb->dismiss();
        } else {
            setVisible(false);
        }
    };

    mRequestMatchButton = std::make_unique<TextButton>("x");
    mRequestMatchButton->setButtonText(TRANS("Request Group Match"));
    //mRequestMatchButton->setColour(DrawableButton::backgroundColourId, Colours::transparentBlack);
    mRequestMatchButton->onClick = [this]() {
        auto valms = mMainSlider->getValue();
        processor.sendLatencyMatchToAll(valms);
        processor.commitLatencyMatch(valms);
    };


    addAndMakeVisible(mRequestMatchButton.get());
    addAndMakeVisible(mMainSlider.get());
    addAndMakeVisible(mMainSliderLabel.get());
    addAndMakeVisible(mViewport.get());
    addAndMakeVisible(mCloseButton.get());
    addAndMakeVisible(mTitleLabel.get());

    int menubuttw = 36;
    int minitemheight = 36;
    int titleheight = 32;
    int minButtonWidth = 90;
#if JUCE_IOS || JUCE_ANDROID
    // make the button heights a bit more for touchscreen purposes
    minitemheight = 44;
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
    matchButtBox.items.add(FlexItem(4, 4).withMargin(0).withFlex(1));
    matchButtBox.items.add(FlexItem(minButtonWidth, 4, *mRequestMatchButton).withMargin(0).withFlex(1));
    matchButtBox.items.add(FlexItem(4, 4).withMargin(0).withFlex(1));

    mainSliderBox.items.clear();
    mainSliderBox.flexDirection = FlexBox::Direction::row;
    mainSliderBox.items.add(FlexItem(minButtonWidth, minitemheight, *mMainSlider).withMargin(5).withFlex(1));



    mainBox.items.clear();
    mainBox.flexDirection = FlexBox::Direction::column;
    mainBox.items.add(FlexItem(minButtonWidth, titleheight, titleBox).withMargin(0).withFlex(0));
    mainBox.items.add(FlexItem(minButtonWidth, 100, *mViewport).withMargin(0).withFlex(1));
    mainBox.items.add(FlexItem(4, 4).withMargin(0).withFlex(0));
    mainBox.items.add(FlexItem(minButtonWidth, minitemheight + 10, mainSliderBox).withMargin(0).withFlex(0));
    mainBox.items.add(FlexItem(4, 4).withMargin(0).withFlex(0));
    mainBox.items.add(FlexItem(minButtonWidth, minitemheight, matchButtBox).withMargin(0).withFlex(0));

}



LatencyMatchView::~LatencyMatchView()
{
}

void LatencyMatchView::paint (Graphics& g)
{
    //g.fillAll (Colour(0xff272727));
}

void LatencyMatchView::resized()
{
    mainBox.performLayout(getLocalBounds().reduced(2));
    mMainSliderLabel->setBounds(mMainSlider->getBounds().withTrimmedRight(mMainSlider->getBounds().getWidth()*0.25));

}

Slider * LatencyMatchView::createPeerLatSlider()
{
    auto * slider = new Slider(Slider::LinearBar,  Slider::TextBoxAbove);
    slider->setEnabled(false);
    slider->setLookAndFeel(&sonoSliderLNF);
    slider->setColour(Slider::backgroundColourId, Colour(0xff050505));

    slider->setTextValueSuffix(" ms");
    slider->getProperties().set ("noFill", true);
    slider->setScrollWheelEnabled(false);

    return slider;
}

Label * LatencyMatchView::createPeerLabel()
{
    auto * label = new Label();
    label->setFont(12);
    label->setColour(Label::textColourId, Colour(0xaaaaaaaa));
    label->setJustificationType(Justification::centredLeft);
    label->setMinimumHorizontalScale(0.4);
    return label;
}

void LatencyMatchView::startLatMatchProcess()
{
    processor.beginLatencyMatchProcedure();
    initialCheckDone = false;

    startTimer(1, 500);
}

void LatencyMatchView::stopLatMatchProcess()
{
    stopTimer(1);
}

void LatencyMatchView::timerCallback(int timerid)
{
    if (timerid == 1) {
        // check if done
        bool done = processor.isLatencyMatchProcedureReady();

        updatePeerSliders();

        if (done) {
            stopLatMatchProcess();
            DBG("DONE WITH LAT MATCH INFO");
            initialCheckDone = true;
        }

        startTimer(2, 3000);
    }
    else if (timerid == 2) {
        // request the data again
        updatePeerSliders();

        processor.beginLatencyMatchProcedure();

    }

    if (!isShowing()) {
        stopTimer(1);
        stopTimer(2);
        DBG("Stopping all latmatch timers");
    }
}


void LatencyMatchView::updatePeerSliders()
{
    int minitemheight = 22;
    int minButtonWidth = 90;

    peerSlidersBox.items.clear();
    peerSlidersBox.flexDirection = FlexBox::Direction::column;


    Array<SonobusAudioProcessor::LatInfo> latlist;

    processor.getLatencyInfoList(latlist);

    // todo sort it
    latlist.sort();

    auto numlats = latlist.size();
    bool layoutchange = false;

    while (mPeerSliders.size() < numlats) {
        auto * slider = mPeerSliders.add(createPeerLatSlider());
        mRowsContainer->addAndMakeVisible(slider);

        auto * label = mPeerLabels.add(createPeerLabel());
        mRowsContainer->addAndMakeVisible(label);
        layoutchange = true;
    }
    while (mPeerSliders.size() > numlats) {
        mPeerSliders.removeLast();
        mPeerLabels.removeLast();
        layoutchange = true;
    }

    float maxlat = 0.0f;
    for (int i=0; i < latlist.size(); ++i) {
        maxlat = jmax(maxlat, latlist.getReference(i).latencyMs);
    }

    for (int i=0; i < mPeerSliders.size(); ++i) {
        auto * slider = mPeerSliders.getUnchecked(i);
        auto * label = mPeerLabels.getUnchecked(i);
        auto & latinfo = latlist.getReference(i);

        slider->setRange(0.0, maxlat, 0.1);
        slider->setValue(latinfo.latencyMs, dontSendNotification);


        String peerstr;
        peerstr << latinfo.sourceName << String(juce::CharPointer_UTF8 (" \xe2\x86\x92 ")) << latinfo.destName;
        label->setText(peerstr, dontSendNotification);

        peerSlidersBox.items.add(FlexItem(minButtonWidth, minitemheight, *slider).withMargin(1).withFlex(0));
    }

    if (!mMainSlider->isMouseOverOrDragging()) {
        mMainSlider->setRange(0.0, std::max(maxlat, 0.2f), 0.1);
        
        if (!initialCheckDone) {
            mMainSlider->setValue(maxlat, dontSendNotification);
        }
    }

    if (layoutchange) {

        int mch = 0;
        for (auto & item : peerSlidersBox.items) {
            mch += item.minHeight + item.margin.top + item.margin.bottom;
        }

        mRowsContainer->setBounds(0, 0, mViewport->getWidth() - 10, mch);

        peerSlidersBox.performLayout(mRowsContainer->getLocalBounds().reduced(2));

        for (int i=0; i < mPeerLabels.size(); ++i) {
            auto * slider = mPeerSliders.getUnchecked(i);
            auto * label = mPeerLabels.getUnchecked(i);
            label->setBounds(slider->getBounds().withTrimmedRight(slider->getBounds().getWidth()*0.25));
        }
    }
}

