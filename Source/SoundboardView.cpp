// SPDX-License-Identifier: GPLv3-or-later WITH Appstore-exception
// Copyright (C) 2020 Jesse Chappell



#include "SoundboardView.h"
#include "SoundboardEditView.h"
#include <iostream>

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
    // TODO: 7 is placeholder height
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

    auto soundboards = processor->getNumberOfSoundboards();
    for (int i = 0; i < soundboards; ++i) {
        mBoardSelectComboBox->addItem(processor->getSoundboard(i).getName(), i);
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

    auto selectedBoardIndex = mBoardSelectComboBox->getSelectedItemIndex();
    if (selectedBoardIndex >= processor->getNumberOfSoundboards()) {
        return;
    }

    auto &selectedBoard = processor->getSoundboard(selectedBoardIndex);

    for (const auto &sample: selectedBoard.getSamples()) {
        auto textButton = std::make_unique<TextButton>(sample.getName(), sample.getName());
        textButton->setColour(DrawableButton::backgroundColourId, Colours::transparentBlack);
        addAndMakeVisible(textButton.get());

        buttonBox.items.add(FlexItem(MENU_BUTTON_WIDTH, TITLE_HEIGHT, *textButton).withMargin(0).withFlex(0));
        mSoundButtons.emplace_back(std::move(textButton));
    }

    mAddSampleButton = std::make_unique<SonoDrawableButton>("addSample", SonoDrawableButton::ButtonStyle::ImageOnButtonBackground);
    std::unique_ptr<Drawable> imageAdd(Drawable::createFromImageData(BinaryData::plus_icon_svg, BinaryData::plus_icon_svgSize));
    mAddSampleButton->setTooltip(TRANS("Add Sample"));
    mAddSampleButton->setImages(imageAdd.get());
    mAddSampleButton->setColour(DrawableButton::backgroundColourId, Colours::transparentBlack);
    mAddSampleButton->setLookAndFeel(&dashedButtonLookAndFeel);
    mAddSampleButton->onClick = [this]() {

    };
    addAndMakeVisible(mAddSampleButton.get());

    buttonBox.items.add(FlexItem(MENU_BUTTON_WIDTH, TITLE_HEIGHT, *mAddSampleButton).withMargin(0).withFlex(0));
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

    SafePointer <SoundboardView> safeThis(this);
    auto callback = [safeThis](GenericItemChooser *chooser, int index) mutable {
        switch (index) {
            case 0:
                safeThis->clickedAddSoundboard();
                break;
            case 1:
                safeThis->clickedRenameSoundboard();
                break;
            case 2:
                safeThis->clickedDeleteSoundboard();
        }
    };

    GenericItemChooser::launchPopupChooser(items, bounds, parent, callback, -1, parent->getHeight() - 30);
}

void SoundboardView::clickedAddSoundboard()
{
    auto callback = [this](const String& name) {
        processor->addSoundboard(name);

        // TODO: Replace with actual update to the soundboard combobox.
        std::cout << "Soundboards:" << std::endl;
        auto soundboards = processor->getNumberOfSoundboards();
        for (int i = 0; i < soundboards; ++i) {
            std::cout << "> " << processor->getSoundboard(i).getName() << std::endl;
        }
    };

    auto content = std::make_unique<SoundboardEditView>(callback, nullptr);
    content->setSize(256, 100);

    CallOutBox::launchAsynchronously(
            std::move(content),
            mTitleLabel->getScreenBounds(),
            nullptr
    );
}

void SoundboardView::clickedRenameSoundboard()
{
    // TODO: Add functionality.
}

void SoundboardView::clickedDeleteSoundboard()
{
    // TODO: Add functionality.
}

void SoundboardView::paint(Graphics &g)
{
    g.fillAll(Colour(0xff272727));
}

void SoundboardView::resized()
{
    mainBox.performLayout(getLocalBounds().reduced(2));
}