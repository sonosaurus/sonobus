// SPDX-License-Identifier: GPLv3-or-later WITH Appstore-exception
// Copyright (C) 2020 Jesse Chappell



#include <sstream>

#include "SoundboardView.h"
#include "SoundboardEditView.h"
#include "SampleEditView.h"

SoundboardView::SoundboardView(SoundboardChannelProcessor* channelProcessor)
        : processor(std::make_unique<SoundboardProcessor>(channelProcessor))
{
    setOpaque(true);

    createSoundboardTitle();
    createSoundboardSelectionPanel();
    createControlPanel();
    createBasePanels();

    updateSoundboardSelector();
    updateButtons();

    mLastSampleBrowseDirectory = std::make_unique<String>(
            File::getSpecialLocation(File::userMusicDirectory).getFullPathName());
}

void SoundboardView::createBasePanels()
{
    buttonBox.items.clear();
    buttonBox.flexDirection = FlexBox::Direction::column;

    buttonViewport.setViewedComponent(&buttonContainer, false);
    addAndMakeVisible(buttonViewport);

    soundboardContainerBox.items.clear();
    soundboardContainerBox.flexDirection = FlexBox::Direction::column;
    soundboardContainerBox.items.add(FlexItem(TITLE_LABEL_WIDTH, TITLE_HEIGHT, titleBox).withMargin(0).withFlex(0));
    soundboardContainerBox.items.add(FlexItem(ELEMENT_MARGIN, ELEMENT_MARGIN).withMargin(0));
    soundboardContainerBox.items.add(
            FlexItem(TITLE_LABEL_WIDTH, TITLE_HEIGHT, soundboardSelectionBox).withMargin(0).withFlex(0));
    soundboardContainerBox.items.add(FlexItem(ELEMENT_MARGIN, ELEMENT_MARGIN).withMargin(0));
    soundboardContainerBox.items.add(
            FlexItem(buttonViewport).withMargin(2).withFlex(1));
    soundboardContainerBox.items.add(FlexItem(TITLE_LABEL_WIDTH, 48, controlsBox).withMargin(2).withFlex(0));

    mainBox.items.clear();
    mainBox.flexDirection = FlexBox::Direction::column;
    mainBox.alignItems = FlexBox::AlignItems::stretch;
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

void SoundboardView::createControlPanel()
{
    mHotkeyStateButton = std::make_unique<SonoDrawableButton>("Hotkey switch", DrawableButton::ButtonStyle::ImageFitted);
    std::unique_ptr<Drawable> keyboardImage(Drawable::createFromImageData(BinaryData::keyboard_svg, BinaryData::keyboard_svgSize));
    std::unique_ptr<Drawable> keyboardDisabledImage(Drawable::createFromImageData(BinaryData::keyboard_disabled_svg, BinaryData::keyboard_disabled_svgSize));
    mHotkeyStateButton->setImages(keyboardImage.get(), nullptr, nullptr, nullptr, keyboardDisabledImage.get());
    mHotkeyStateButton->setForegroundImageRatio(0.75f);
    mHotkeyStateButton->setClickingTogglesState(true);
    mHotkeyStateButton->setColour(DrawableButton::backgroundColourId, Colours::transparentBlack);
    mHotkeyStateButton->setColour(DrawableButton::backgroundOnColourId, Colour::fromFloatRGBA(0.2, 0.2, 0.2, 0.7));
    mHotkeyStateButton->setTitle(TRANS("Toggle hotkeys"));
    mHotkeyStateButton->setTooltip(TRANS("Toggles whether sound samples can be played using hotkeys."));
    mHotkeyStateButton->setToggleState(processor->isHotkeysSelected(), NotificationType::dontSendNotification);
    mHotkeyStateButton->onClick = [this]() {
        processor->setHotkeysSelected(mHotkeyStateButton->getToggleState());
    };
    addAndMakeVisible(mHotkeyStateButton.get());

    mStopAllPlayback = std::make_unique<SonoDrawableButton>("StopAllPlayback", DrawableButton::ButtonStyle::ImageFitted);
    std::unique_ptr<Drawable> speakerImage(Drawable::createFromImageData(BinaryData::speaker_disabled_grey_svg, BinaryData::speaker_disabled_grey_svgSize));
    mStopAllPlayback->setImages(speakerImage.get());
    mStopAllPlayback->setForegroundImageRatio(0.75f);
    mStopAllPlayback->setColour(DrawableButton::backgroundColourId, Colours::transparentBlack);
    mStopAllPlayback->setTitle(TRANS("Stop all playback"));
    mStopAllPlayback->setTooltip(TRANS("Stops all playing samples."));
    mStopAllPlayback->onClick = [this]() {
        processor->stopAllPlayback();
    };
    addAndMakeVisible(mStopAllPlayback.get());

    controlsBox.items.clear();
    controlsBox.flexDirection = FlexBox::Direction::row;
    controlsBox.justifyContent = FlexBox::JustifyContent::center;
    controlsBox.items.add(FlexItem(38, 34, *mStopAllPlayback).withMargin(4).withFlex(0));
    controlsBox.items.add(FlexItem(38, 34, *mHotkeyStateButton).withMargin(4).withFlex(0));
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
    buttonBox.items.clear();
    mSoundButtons.clear();
    buttonContainer.removeAllChildren();

    auto selectedBoardIndex = mBoardSelectComboBox->getSelectedItemIndex();
    if (selectedBoardIndex >= processor->getNumberOfSoundboards()) {
        return;
    }

    auto& selectedBoard = processor->getSoundboard(selectedBoardIndex);

    for (auto& sample : selectedBoard.getSamples()) {
        auto playbackButton = std::make_unique<SonoPlaybackProgressButton>(sample.getName(), sample.getName());
        playbackButton->setButtonColour(sample.getButtonColour());

        auto buttonAddress = playbackButton.get();
        if (sample.getButtonBehaviour() == SoundSample::ButtonBehaviour::HOLD) {
            playbackButton->onPrimaryClick = [this, &sample, buttonAddress]() {};
            playbackButton->setMouseListener(std::make_unique<HoldSampleButtonMouseListener>(buttonAddress, &sample, this));
        }
        else if (sample.getButtonBehaviour() == SoundSample::ButtonBehaviour::ONE_SHOT) {
            playbackButton->onPrimaryClick = [this, &sample, buttonAddress]() {
                playSample(sample, buttonAddress);
            };
        }
        else if (sample.getButtonBehaviour() == SoundSample::ButtonBehaviour::TOGGLE) {
            playbackButton->onPrimaryClick = [this, &sample, buttonAddress]() {
                if (processor->getChannelProcessor()->findPlaybackManager(sample).has_value()) {
                    stopSample(sample);
                }
                else {
                    playSample(sample, buttonAddress);
                }
            };
        }

        playbackButton->onSecondaryClick = [this, &sample, buttonAddress]() {
            clickedEditSoundSample(*buttonAddress, sample);
        };

        auto playbackManager = getSoundboardProcessor()->getChannelProcessor()->findPlaybackManager(sample);
        if (playbackManager.has_value()) {
            playbackButton->attachToPlaybackManager(playbackManager.value());
        }

        buttonContainer.addAndMakeVisible(playbackButton.get());

        buttonBox.items.add(FlexItem(MENU_BUTTON_WIDTH, TITLE_HEIGHT, *playbackButton).withMargin(0).withFlex(0));

        mSoundButtons.emplace_back(std::move(playbackButton));
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
    mAddSampleButton->setColour(TextButton::buttonColourId, Colours::transparentBlack);
    mAddSampleButton->setLookAndFeel(&dashedButtonLookAndFeel);
    mAddSampleButton->onClick = [this]() {
        clickedAddSoundSample();
    };
    buttonContainer.addAndMakeVisible(mAddSampleButton.get());

    buttonBox.items.add(FlexItem(MENU_BUTTON_WIDTH, TITLE_HEIGHT, *mAddSampleButton).withMargin(0).withFlex(0));

    // Trigger repaint
    resized();
}

void SoundboardView::playSample(SoundSample& sample, SonoPlaybackProgressButton* button)
{
    auto channelProcessor = processor->getChannelProcessor();
    auto playbackManagerMaybe = channelProcessor->loadSample(sample);
    if (!playbackManagerMaybe.has_value()) {
        AlertWindow::showMessageBoxAsync(
                AlertWindow::WarningIcon,
                TRANS("Cannot play file"),
                TRANS("The selected audio file failed to load. The file cannot be played.")
        );
        return;
    }

    auto playbackManager = playbackManagerMaybe.value();

    if (button != nullptr) {
        button->attachToPlaybackManager(playbackManager);
    }

    playbackManager->play();
}

void SoundboardView::stopSample(const SoundSample& sample)
{
    auto playbackManagerMaybe = processor->getChannelProcessor()->findPlaybackManager(sample);
    if (!playbackManagerMaybe.has_value()) return;

    playbackManagerMaybe.value()->pause();
}

bool SoundboardView::playSampleAtIndex(int sampleIndex)
{
    if (sampleIndex < 0) {
        return false;
    }

    auto selectedSoundboardIndex = mBoardSelectComboBox->getSelectedItemIndex();
    if (selectedSoundboardIndex >= getSoundboardProcessor()->getNumberOfSoundboards()) {
        return false;
    }

    auto& soundboard = getSoundboardProcessor()->getSoundboard(selectedSoundboardIndex);
    auto& samples = soundboard.getSamples();
    if (sampleIndex >= samples.size()) {
        return false;
    }

    auto& soundSampleAtIndex = samples[sampleIndex];
    playSample(soundSampleAtIndex);
    return true;
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
        auto buttonColour = editView.getButtonColour();
        auto loop = editView.isLoop();
        auto playbackBehaviour = editView.getPlaybackBehaviour();
        auto buttonBehaviour = editView.getButtonBehaviour();
        auto replayBehaviour = editView.getReplayBehaviour();
        auto gain = editView.getGain();
        auto hotkeyCode = editView.getHotkeyCode();

        SoundSample* createdSample = processor->addSoundSample(sampleName, filePath);
        createdSample->setButtonColour(buttonColour);
        createdSample->setLoop(loop);
        createdSample->setPlaybackBehaviour(playbackBehaviour);
        createdSample->setButtonBehaviour(buttonBehaviour);
        createdSample->setReplayBehaviour(replayBehaviour);
        createdSample->setGain(gain);
        createdSample->setHotkeyCode(hotkeyCode);

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
        if (editView.isDeleteSample()) {
            processor->deleteSoundSample(sample);
        }
        else {
            auto sampleName = editView.getSampleName();
            auto filePath = editView.getAbsoluteFilePath();
            auto buttonColour = editView.getButtonColour();
            auto loop = editView.isLoop();
            auto playbackBehaviour = editView.getPlaybackBehaviour();
            auto buttonBehaviour = editView.getButtonBehaviour();
            auto replayBehaviour = editView.getReplayBehaviour();
            auto gain = editView.getGain();
            auto hotkeyCode = editView.getHotkeyCode();

            sample.setName(sampleName);
            sample.setFilePath(filePath);
            sample.setButtonColour(buttonColour);
            sample.setLoop(loop);
            sample.setPlaybackBehaviour(playbackBehaviour);
            sample.setButtonBehaviour(buttonBehaviour);
            sample.setReplayBehaviour(replayBehaviour);
            sample.setGain(gain);
            sample.setHotkeyCode(hotkeyCode);
            processor->editSoundSample(sample);
        }
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
    // Compute the inner container size manually, as all automatic layout computation seem to be rendered useless
    // as a consequence of using viewport.
    int buttonsHeight = std::accumulate(buttonBox.items.begin(), buttonBox.items.end(), 0,
        [](int sum, const FlexItem& item) { return sum + item.currentBounds.getHeight(); });
    buttonContainer.setSize(buttonViewport.getMaximumVisibleWidth(), buttonsHeight);

    mainBox.performLayout(getLocalBounds().reduced(2));
    buttonBox.performLayout(buttonContainer.getLocalBounds());
}

void SoundboardView::processKeystroke(const KeyPress& keyPress)
{
    // Only process keystrokes when the soundboard view is opened.
    // This is to prevent sounds from 'magically' playing.
    if (!this->isVisible()) {
        return;
    }

    // When hotkeys are disabled, the hotkeystate button is toggled on.
    if (mHotkeyStateButton->getToggleState()) {
        return;
    }

    // Process default keybinds (1-9).
    // 0 is already assigned to jump to start.
    auto keyCode = keyPress.getKeyCode();

    if (keyCode >= 49 /* 1 */ && keyCode <= 57 /* 9 */) {
        if (playSampleAtIndex(keyCode - 49)) {
            return;
        }
    }

    if (keyCode >= KeyPress::numberPad1 && keyCode <= KeyPress::numberPad9) {
        if (playSampleAtIndex(keyCode - KeyPress::numberPad1)) {
            return;
        }
    }

    // Look for custom keybinds.
    auto selectedSoundboardIndex = mBoardSelectComboBox->getSelectedItemIndex();
    if (selectedSoundboardIndex >= getSoundboardProcessor()->getNumberOfSoundboards()) {
        return;
    }

    auto& soundboard = getSoundboardProcessor()->getSoundboard(selectedSoundboardIndex);
    auto& samples = soundboard.getSamples();

    auto sampleCount = samples.size();
    for (int i = 0; i < sampleCount; ++i) {
        auto& sample = samples[i];
        if (sample.getHotkeyCode() == keyPress.getKeyCode()) {
            playSampleAtIndex(i);
        }
    }
}

void SoundboardView::choiceButtonSelected(SonoChoiceButton* choiceButton, int index, int ident)
{
    processor->selectSoundboard(index);
    updateButtons();
}

bool SoundboardView::isInterestedInFileDrag(const StringArray& files)
{
    if (files.isEmpty()) return false;

    // Check whether the files match one of the supported extensions
    for (const auto& filePath : files) {
        auto hasSupportedExt = false;

        std::string wildcardExt;
        std::stringstream is = std::stringstream(SoundSample::SUPPORTED_EXTENSIONS);
        while (std::getline(is, wildcardExt, ';')) {
            if (filePath.matchesWildcard(wildcardExt, true)) {
                hasSupportedExt = true;
                break;
            }
        }

        if (!hasSupportedExt) return false;
    }

    return true;
}

void SoundboardView::fileDraggedAt(int x, int y)
{
    mAddSampleButton->setEnabled(false);
    mAddSampleButton->setColour(TextButton::buttonColourId, Colours::white.withAlpha(0.8f));

    mAddSampleButton->repaint();
}

void SoundboardView::fileDragStopped()
{
    mAddSampleButton->setEnabled(true);
    mAddSampleButton->setColour(TextButton::buttonColourId, Colours::transparentBlack);
    mAddSampleButton->repaint();
}

void SoundboardView::fileDragEnter(const StringArray& files, int x, int y)
{
    fileDraggedAt(x, y);
}

void SoundboardView::fileDragMove(const StringArray& files, int x, int y)
{
    fileDraggedAt(x, y);
}

void SoundboardView::fileDragExit(const StringArray& files)
{
    fileDragStopped();
}

void SoundboardView::filesDropped(const StringArray& files, int x, int y)
{
    fileDragStopped();

    SoundSample* createdSample;
    for (const auto& filePath : files) {
        auto sampleName = File(filePath).getFileNameWithoutExtension();
        createdSample = processor->addSoundSample(sampleName, filePath);
    }

    updateButtons();

    // Open the edit view by default if only 1 file was dragged
    if (files.size() == 1) {
        clickedEditSoundSample(*mSoundButtons[mSoundButtons.size() - 1].get(), *createdSample);
    }
}

HoldSampleButtonMouseListener::HoldSampleButtonMouseListener(SonoPlaybackProgressButton* button, SoundSample* sample, SoundboardView* view) :
        button(button),
        sample(sample),
        view(view)
{

}

void HoldSampleButtonMouseListener::mouseDown(const MouseEvent& event)
{
    view->playSample(*sample, button);
}

void HoldSampleButtonMouseListener::mouseUp(const MouseEvent& event)
{
    view->stopSample(*sample);
}

