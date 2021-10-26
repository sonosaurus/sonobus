// SPDX-License-Identifier: GPLv3-or-later WITH Appstore-exception
// Copyright (C) 2020 Jesse Chappell



#include "SoundboardView.h"

SoundboardView::SoundboardView() : processor(std::make_unique<SoundboardProcessor>())
{
    setOpaque(true);

    createSoundboardTitle();
    createSoundboardSelectionPanel();
    createBasePanels();

    updateButtons();
}

void SoundboardView::createBasePanels()
{
    buttonBox.items.clear();
    buttonBox.flexDirection = FlexBox::Direction::column;

    soundboardContainerBox.items.clear();
    soundboardContainerBox.flexDirection = FlexBox::Direction::column;
    soundboardContainerBox.items.add(FlexItem(TITLE_LABEL_WIDTH, TITLE_HEIGHT, titleBox).withMargin(0).withFlex(0));
    soundboardContainerBox.items.add(FlexItem(ELEMENT_MARGIN, ELEMENT_MARGIN).withMargin(0));
    soundboardContainerBox.items.add(
            FlexItem(TITLE_LABEL_WIDTH, TITLE_HEIGHT, soundboardSelectionBox).withMargin(0).withFlex(0));
    soundboardContainerBox.items.add(FlexItem(ELEMENT_MARGIN, ELEMENT_MARGIN).withMargin(0));
    soundboardContainerBox.items.add(
            FlexItem(TITLE_LABEL_WIDTH, TITLE_HEIGHT * 7, buttonBox).withMargin(0).withFlex(0));

    mainBox.items.clear();
    mainBox.flexDirection = FlexBox::Direction::row;
    mainBox.items.add(FlexItem(TITLE_LABEL_WIDTH, ELEMENT_MARGIN, soundboardContainerBox).withMargin(0).withFlex(1));
}

void SoundboardView::createSoundboardTitle()
{
    createSoundboardTitleLabel();
    createSoundboardTitleCloseButton();
    createSoundboardMenu();

    titleBox.items.clear();
    titleBox.flexDirection = FlexBox::Direction::row;
    titleBox.items.add(FlexItem(ELEMENT_MARGIN, ELEMENT_MARGIN).withMargin(0).withFlex(0));
    titleBox.items.add(FlexItem(MENU_BUTTON_WIDTH, ELEMENT_MARGIN, *mCloseButton).withMargin(0).withFlex(0));
    titleBox.items.add(FlexItem(TITLE_LABEL_WIDTH, ELEMENT_MARGIN, *mTitleLabel).withMargin(0).withFlex(1));
    titleBox.items.add(FlexItem(MENU_BUTTON_WIDTH, ELEMENT_MARGIN, *mMenuButton).withMargin(0).withFlex(0));
    titleBox.items.add(FlexItem(ELEMENT_MARGIN, ELEMENT_MARGIN).withMargin(0).withFlex(0));
}

void SoundboardView::createSoundboardTitleLabel()
{
    mTitleLabel = std::make_unique<Label>("title", TRANS("Soundboard"));
    mTitleLabel->setJustificationType(Justification::centred);
    mTitleLabel->setFont(Font(TITLE_FONT_HEIGHT, Font::bold));
    mTitleLabel->setColour(Label::textColourId, Colour(0xeeffffff));
    addAndMakeVisible(mTitleLabel.get());
}

void SoundboardView::createSoundboardTitleCloseButton()
{
    mCloseButton = std::make_unique<SonoDrawableButton>("x", DrawableButton::ButtonStyle::ImageFitted);
    std::unique_ptr<Drawable> imageCross(
            Drawable::createFromImageData(BinaryData::x_icon_svg, BinaryData::x_icon_svgSize));
    mCloseButton->setImages(imageCross.get());
    mCloseButton->setTitle(TRANS("Close Soundboard"));
    mCloseButton->setColour(DrawableButton::backgroundColourId, Colours::transparentBlack);
    mCloseButton->onClick = [this]() {
        setVisible(false);
    };
    addAndMakeVisible(mCloseButton.get());
}

void SoundboardView::createSoundboardMenu()
{
    mMenuButton = std::make_unique<SonoDrawableButton>("menu", SonoDrawableButton::ButtonStyle::ImageFitted);
    std::unique_ptr<Drawable> imageMenu(Drawable::createFromImageData(BinaryData::dots_svg, BinaryData::dots_svgSize));
    mMenuButton->setTitle(TRANS("Soundboard Menu"));
    mMenuButton->setImages(imageMenu.get());
    mMenuButton->setColour(DrawableButton::backgroundColourId, Colours::transparentBlack);
    mMenuButton->onClick = [this]() {
        showMenuButtonContextMenu();
    };
    addAndMakeVisible(mMenuButton.get());
}

void SoundboardView::createSoundboardSelectionPanel()
{
    mBoardSelectComboBox = std::make_unique<SonoChoiceButton>();
    mBoardSelectComboBox->setTitle(TRANS("Soundboard #1"));
    mBoardSelectComboBox->setColour(SonoTextButton::outlineColourId, Colour::fromFloatRGBA(0.6, 0.6, 0.6, 0.4));
    //mBoardSelectComboBox->addChoiceListener(this);

    for (int i = 1; i <= 4 /* Number of soundboards */; ++i) {
        mBoardSelectComboBox->addItem(TRANS("Soundboard #") + std::to_string(i), i - 1 /* identifier */);
    }
    mBoardSelectComboBox->setSelectedId(0);

    addAndMakeVisible(mBoardSelectComboBox.get());

    soundboardSelectionBox.items.clear();
    soundboardSelectionBox.flexDirection = FlexBox::Direction::row;
    soundboardSelectionBox.items.add(FlexItem(ELEMENT_MARGIN, ELEMENT_MARGIN).withMargin(0).withFlex(0));
    soundboardSelectionBox.items.add(
            FlexItem(MENU_BUTTON_WIDTH, TITLE_HEIGHT, *mBoardSelectComboBox).withMargin(0).withFlex(1));
    soundboardSelectionBox.items.add(FlexItem(ELEMENT_MARGIN, ELEMENT_MARGIN).withMargin(0).withFlex(0));
}

void SoundboardView::updateButtons()
{
    buttonBox.items.clear();
    mSoundButtons.clear();

    // 7 is placeholder value, see also SdbVw::createBasePanels: TITLE_HEIGHT * 7.
    // In the future: determine buttons based on the selected soundboard.
    for (int i = 1; i <= 7; ++i) {
        auto sound = (i == 5) ? "Mambo" : "Sound";
        auto testTextButton = std::make_unique<TextButton>(std::string(sound) + " Number " + std::to_string(i),
                                                           std::string(sound) + " " + std::to_string(i));
        testTextButton->setColour(DrawableButton::backgroundColourId, Colours::transparentBlack);
        addAndMakeVisible(testTextButton.get());

        buttonBox.items.add(FlexItem(MENU_BUTTON_WIDTH, TITLE_HEIGHT, *testTextButton).withMargin(0).withFlex(0));
        mSoundButtons.push_back(std::move(testTextButton));
    }
}

void SoundboardView::showMenuButtonContextMenu()
{
    Array<GenericItemChooserItem> items;

    items.add(GenericItemChooserItem(TRANS("New soundboard"), {}, nullptr, false));
    items.add(GenericItemChooserItem(TRANS("Rename soundboard"), {}, nullptr, false));
    items.add(GenericItemChooserItem(TRANS("Delete soundboard"), {}, nullptr, false));

    Component *parent = mMenuButton->findParentComponentOfClass<AudioProcessorEditor>();
    if (!parent) {
        parent = mMenuButton->findParentComponentOfClass<Component>();
    }
    Rectangle<int> bounds = parent->getLocalArea(nullptr, mMenuButton->getScreenBounds());

    SafePointer<SoundboardView> safeThis(this);
    auto callback = [safeThis](GenericItemChooser *chooser, int index) mutable {
        switch (index) {
            case 0:
                safeThis->processor->onAddSoundboard();
                break;
            case 1:
                safeThis->processor->onRenameSoundboard();
                break;
            case 2:
                safeThis->processor->onDeleteSoundboard();
        }
    };

    GenericItemChooser::launchPopupChooser(items, bounds, parent, callback, -1, parent->getHeight() - 30);
}

void SoundboardView::paint(Graphics &g)
{
    g.fillAll(Colour(0xff272727));
}

void SoundboardView::resized()
{
    mainBox.performLayout(getLocalBounds().reduced(2));
}