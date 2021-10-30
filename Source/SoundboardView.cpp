// SPDX-License-Identifier: GPLv3-or-later WITH Appstore-exception
// Copyright (C) 2020 Jesse Chappell



#include "SoundboardView.h"
#include "SoundboardEditView.h"
#include "SampleEditView.h"

SoundboardView::SoundboardView(SoundboardChannelProcessor* channelProcessor)
        : processor(std::make_unique<SoundboardProcessor>(channelProcessor))
{
    processor->getChannelProcessor()->attach(*this);

    setOpaque(true);

    createSoundboardTitle();
    createSoundboardSelectionPanel();
    createBasePanels();

    updateSoundboardSelector();
    updateButtons();

    mLastSampleBrowseDirectory = std::make_unique<String>(
            File::getSpecialLocation(File::userMusicDirectory).getFullPathName());
}

SoundboardView::~SoundboardView()
{
    processor->getChannelProcessor()->detach(*this);
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
    mBoardSelectComboBox->setTitle(TRANS("Select Soundboard"));
    mBoardSelectComboBox->setColour(SonoTextButton::outlineColourId, Colour::fromFloatRGBA(0.6, 0.6, 0.6, 0.4));
    mBoardSelectComboBox->addChoiceListener(this);
    addAndMakeVisible(mBoardSelectComboBox.get());

    soundboardSelectionBox.items.clear();
    soundboardSelectionBox.flexDirection = FlexBox::Direction::row;
    soundboardSelectionBox.items.add(FlexItem(ELEMENT_MARGIN, ELEMENT_MARGIN).withMargin(0).withFlex(0));
    soundboardSelectionBox.items.add(
            FlexItem(MENU_BUTTON_WIDTH, TITLE_HEIGHT, *mBoardSelectComboBox).withMargin(0).withFlex(1));
    soundboardSelectionBox.items.add(FlexItem(ELEMENT_MARGIN, ELEMENT_MARGIN).withMargin(0).withFlex(0));
}

void SoundboardView::updateSoundboardSelector()
{
    // Index shenanigans will go wrong when there are no soundboards, so return early:
    // doesn't matter anyway as the soundboard selector only need to be cleared.
    if (processor->getNumberOfSoundboards() == 0) {
        // Apparently, when you clear items the last item name is still visible, but no items are present.
        // Enjoy this amazing hack.
        mBoardSelectComboBox->clearItems();
        mBoardSelectComboBox->addItem("", 0);
        mBoardSelectComboBox->setSelectedItemIndex(0);
        mBoardSelectComboBox->clearItems();
        return;
    }

    // Repopulate the selector.
    mBoardSelectComboBox->clearItems();
    auto soundboardCount = processor->getNumberOfSoundboards();
    for (int i = 0; i < soundboardCount; ++i) {
        mBoardSelectComboBox->addItem(processor->getSoundboard(i).getName(), i);
    }

    // Select the currently selected item.
    auto selectedIndex = processor->getSelectedSoundboardIndex();
    if (selectedIndex.has_value()) {
        mBoardSelectComboBox->setSelectedItemIndex(selectedIndex.value());
    }
}

void SoundboardView::updateButtons()
{
    currentlyPlayingButton = nullptr;
    buttonBox.items.clear();
    mSoundButtons.clear();

    auto selectedBoardIndex = mBoardSelectComboBox->getSelectedItemIndex();
    if (selectedBoardIndex >= processor->getNumberOfSoundboards()) {
        return;
    }

    auto& selectedBoard = processor->getSoundboard(selectedBoardIndex);

    for (auto& sample: selectedBoard.getSamples()) {
        auto textButton = std::make_unique<SonoPlaybackProgressButton>(sample.getName(), sample.getName());
        textButton->setColour(DrawableButton::backgroundColourId, Colours::transparentBlack);

        auto buttonAddress = textButton.get();
        textButton->onPrimaryClick = [this, &sample, buttonAddress]() {
            playSample(sample);
            currentlyPlayingButton = buttonAddress;
        };
        textButton->onSecondaryClick = [this, &sample, buttonAddress]() {
            clickedEditSoundSample(*buttonAddress, sample);
        };
        addAndMakeVisible(textButton.get());

        buttonBox.items.add(FlexItem(MENU_BUTTON_WIDTH, TITLE_HEIGHT, *textButton).withMargin(0).withFlex(0));
        mSoundButtons.emplace_back(std::move(textButton));
    }

    mAddSampleButton = std::make_unique<SonoDrawableButton>(
            "addSample",
            SonoDrawableButton::ButtonStyle::ImageOnButtonBackground
    );
    std::unique_ptr<Drawable> imageAdd(
            Drawable::createFromImageData(BinaryData::plus_icon_svg, BinaryData::plus_icon_svgSize)
    );
    mAddSampleButton->setTooltip(TRANS("Add Sample"));
    mAddSampleButton->setImages(imageAdd.get());
    mAddSampleButton->setColour(DrawableButton::backgroundColourId, Colours::transparentBlack);
    mAddSampleButton->setLookAndFeel(&dashedButtonLookAndFeel);
    mAddSampleButton->onClick = [this]() {
        clickedAddSoundSample();
    };
    addAndMakeVisible(mAddSampleButton.get());

    buttonBox.items.add(FlexItem(MENU_BUTTON_WIDTH, TITLE_HEIGHT, *mAddSampleButton).withMargin(0).withFlex(0));

    // Trigger repaint
    resized();
}

void SoundboardView::playSample(const SoundSample& sample)
{
    auto channelProcessor = processor->getChannelProcessor();
    auto isLoadSuccessful = channelProcessor->loadFile(URL(File(sample.getFilePath())));
    if (!isLoadSuccessful) {
        AlertWindow::showMessageBoxAsync(
                AlertWindow::WarningIcon,
                TRANS("Cannot play file"),
                TRANS("The selected audio file failed to load. The file cannot be played.")
        );
        return;
    }

    channelProcessor->play();
}

void SoundboardView::showMenuButtonContextMenu()
{
    Array<GenericItemChooserItem> items;
    items.add(GenericItemChooserItem(TRANS("New soundboard"), {}, nullptr, false));
    items.add(GenericItemChooserItem(TRANS("Rename soundboard"), {}, nullptr, false));
    items.add(GenericItemChooserItem(TRANS("Delete soundboard"), {}, nullptr, false));

    Component* parent = mMenuButton->findParentComponentOfClass<AudioProcessorEditor>();
    if (!parent) {
        parent = mMenuButton->findParentComponentOfClass<Component>();
    }
    Rectangle<int> bounds = parent->getLocalArea(nullptr, mMenuButton->getScreenBounds());

    SafePointer <SoundboardView> safeThis(this);
    auto callback = [safeThis](GenericItemChooser* chooser, int index) mutable {
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
        Soundboard& createdSoundboard = processor->addSoundboard(name, true);
        updateSoundboardSelector();
        updateButtons();
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
    auto callback = [this](const String& name) {
        int selectedSoundboardIndex = mBoardSelectComboBox->getSelectedItemIndex();
        processor->renameSoundboard(selectedSoundboardIndex, name);
        updateSoundboardSelector();
    };

    auto& currentSoundboard = processor->getSoundboard(mBoardSelectComboBox->getSelectedItemIndex());
    auto content = std::make_unique<SoundboardEditView>(callback, &currentSoundboard);
    content->setSize(SoundboardEditView::DEFAULT_VIEW_WIDTH, SoundboardEditView::DEFAULT_VIEW_HEIGHT);

    CallOutBox::launchAsynchronously(
            std::move(content),
            mBoardSelectComboBox->getScreenBounds(),
            nullptr
    );
}

void SoundboardView::clickedDeleteSoundboard()
{
    // Cannot delete if there are no soundboards.
    if (processor->getNumberOfSoundboards() == 0) {
        return;
    }

    Array<GenericItemChooserItem> items;

    auto titleItem = GenericItemChooserItem(TRANS("Delete soundboard?"), {}, nullptr, false);
    titleItem.disabled = true;
    items.add(titleItem);

    items.add(GenericItemChooserItem(TRANS("No, keep soundboard"), {}, nullptr, true));
    items.add(GenericItemChooserItem(TRANS("Yes, delete soundboard"), {}, nullptr, false));

    Component* parent = mBoardSelectComboBox->findParentComponentOfClass<AudioProcessorEditor>();
    if (!parent) {
        parent = mBoardSelectComboBox->findParentComponentOfClass<Component>();
    }
    Rectangle<int> bounds = parent->getLocalArea(nullptr, mBoardSelectComboBox->getScreenBounds());

    SafePointer <SoundboardView> safeThis(this);
    auto callback = [safeThis](GenericItemChooser* chooser, int index) mutable {
        // Delete soundboard.
        if (index == 2) {
            int selectedIndex = safeThis->mBoardSelectComboBox->getSelectedItemIndex();
            safeThis->processor->deleteSoundboard(selectedIndex);
            safeThis->updateSoundboardSelector();
            safeThis->updateButtons();
        }
    };

    GenericItemChooser::launchPopupChooser(items, bounds, parent, callback, -1, 128);
}

void SoundboardView::clickedAddSoundSample()
{
    auto callback = [this](SampleEditView& editView) {
        auto sampleName = editView.getSampleName();
        auto filePath = editView.getAbsoluteFilePath();

        processor->addSoundSample(sampleName, filePath);
        updateButtons();
    };

    auto content = std::make_unique<SampleEditView>(callback, nullptr, mLastSampleBrowseDirectory.get());
    content->setSize(SampleEditView::DEFAULT_VIEW_WIDTH, SampleEditView::DEFAULT_VIEW_HEIGHT);

    CallOutBox::launchAsynchronously(
            std::move(content),
            mAddSampleButton->getScreenBounds(),
            nullptr
    );
}

void SoundboardView::clickedEditSoundSample(const SonoPlaybackProgressButton& button, SoundSample& sample)
{
    auto callback = [this, &sample](SampleEditView& editView) {
        auto sampleName = editView.getSampleName();
        auto filePath = editView.getAbsoluteFilePath();

        sample.setName(sampleName);
        sample.setFilePath(filePath);
        processor->editSoundSample(sample);
        updateButtons();
    };

    auto content = std::make_unique<SampleEditView>(callback, &sample, mLastSampleBrowseDirectory.get());
    content->setSize(SampleEditView::DEFAULT_VIEW_WIDTH, SampleEditView::DEFAULT_VIEW_HEIGHT);

    CallOutBox::launchAsynchronously(
            std::move(content),
            button.getScreenBounds(),
            nullptr
    );
}

void SoundboardView::paint(Graphics& g)
{
    g.fillAll(Colour(0xff272727));
}

void SoundboardView::resized()
{
    mainBox.performLayout(getLocalBounds().reduced(2));
}

void SoundboardView::choiceButtonSelected(SonoChoiceButton* choiceButton, int index, int ident)
{
    processor->selectSoundboard(index);
    updateButtons();
}

void SoundboardView::onPlaybackPositionChanged(SoundboardChannelProcessor& channelProcessor)
{
    if (currentlyPlayingButton == nullptr) {
        return;
    }

    auto position = channelProcessor.getLength() != 0.0
                    ? channelProcessor.getCurrentPosition() / channelProcessor.getLength()
                    : 0.0;
    currentlyPlayingButton->setPlaybackPosition(position);
    currentlyPlayingButton->repaint();
}
