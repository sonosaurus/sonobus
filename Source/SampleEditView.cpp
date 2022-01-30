// SPDX-License-Identifier: GPLv3-or-later WITH Appstore-exception
// Copyright (C) 2020 Jesse Chappell



#include "SampleEditView.h"

#include <utility>
#include <iostream>

SampleEditView::SampleEditView(
        std::function<void(SampleEditView&)> callback,
        const SoundSample* soundSample,
        String* lastOpenedDirectoryString
) : editModeEnabled(soundSample != nullptr),
    initialName(soundSample == nullptr ? "" : soundSample->getName()),
    initialFilePath(soundSample == nullptr ? "" : soundSample->getFilePath()),
    initialLoop(soundSample != nullptr && soundSample->isLoop()),
    initialPlaybackBehaviour(soundSample == nullptr ? SoundSample::PlaybackBehaviour::SIMULTANEOUS : soundSample->getPlaybackBehaviour()),
    initialButtonBehaviour(soundSample == nullptr ? SoundSample::ButtonBehaviour::TOGGLE : soundSample->getButtonBehaviour()),
    initialReplayBehaviour(soundSample == nullptr ? SoundSample::ReplayBehaviour::REPLAY_FROM_START : soundSample->getReplayBehaviour()),
    initialGain(soundSample == nullptr ? 1.0 : soundSample->getGain()),
    submitCallback(std::move(callback)),
    lastOpenedDirectory(lastOpenedDirectoryString),
    selectedColour(soundSample == nullptr ? SoundboardButtonColors::DEFAULT_BUTTON_COLOUR : soundSample->getButtonColour()),
    hotkeyCode(soundSample == nullptr ? -1 : soundSample->getHotkeyCode())
{
    setOpaque(true);

    createNameInputs();
    createFilePathInputs();
    createColourInput();
    createPlaybackOptionInputs();
    createVolumeInputs();
    createHotkeyInput();
    createButtonBar();
    initialiseLayouts();
}

void SampleEditView::initialiseLayouts()
{
    contentBox.flexDirection = FlexBox::Direction::column;
    contentBox.items.add(FlexItem(ELEMENT_MARGIN, ELEMENT_MARGIN * 2).withMargin(0));
    contentBox.items.add(FlexItem(DEFAULT_VIEW_WIDTH, CONTROL_HEIGHT, *mNameLabel).withMargin(0).withFlex(0));
    contentBox.items.add(FlexItem(DEFAULT_VIEW_WIDTH - 36, CONTROL_HEIGHT, *mNameInput).withMargin(4).withFlex(0));

    contentBox.items.add(FlexItem(ELEMENT_MARGIN, ELEMENT_MARGIN * 2).withMargin(0));
    contentBox.items.add(FlexItem(DEFAULT_VIEW_WIDTH, CONTROL_HEIGHT, *mFilePathLabel).withMargin(0).withFlex(0));
    contentBox.items.add(FlexItem(ELEMENT_MARGIN, ELEMENT_MARGIN).withMargin(0));
    contentBox.items.add(FlexItem(DEFAULT_VIEW_WIDTH - 4, CONTROL_HEIGHT, filePathBox).withMargin(0).withFlex(0));

    contentBox.items.add(FlexItem(ELEMENT_MARGIN, ELEMENT_MARGIN * 2).withMargin(0));
    contentBox.items.add(FlexItem(DEFAULT_VIEW_WIDTH, CONTROL_HEIGHT, *mColourInputLabel).withMargin(0).withFlex(0));
    contentBox.items.add(FlexItem(ELEMENT_MARGIN, ELEMENT_MARGIN).withMargin(0));
    contentBox.items.add(FlexItem(DEFAULT_VIEW_WIDTH - 4, 32 * 2, colourButtonBox).withMargin(0).withFlex(0));

    contentBox.items.add(FlexItem(ELEMENT_MARGIN, ELEMENT_MARGIN * 2).withMargin(0));
    contentBox.items.add(FlexItem(DEFAULT_VIEW_WIDTH, CONTROL_HEIGHT, *mPlaybackOptionsLabel).withMargin(0).withFlex(0));
    contentBox.items.add(FlexItem(ELEMENT_MARGIN, ELEMENT_MARGIN).withMargin(0));
    contentBox.items.add(FlexItem(DEFAULT_VIEW_WIDTH - 4, 36, playbackOptionsRowBox).withMargin(0).withFlex(0));

    contentBox.items.add(FlexItem(ELEMENT_MARGIN, ELEMENT_MARGIN * 6).withMargin(0));
    contentBox.items.add(FlexItem(DEFAULT_VIEW_WIDTH, CONTROL_HEIGHT, *mVolumeLabel).withMargin(0).withFlex(0));
    contentBox.items.add(FlexItem(ELEMENT_MARGIN, ELEMENT_MARGIN).withMargin(0));
    contentBox.items.add(FlexItem(DEFAULT_VIEW_WIDTH - 4, 48, volumeRowBox).withMargin(0).withFlex(0));

    contentBox.items.add(FlexItem(ELEMENT_MARGIN, ELEMENT_MARGIN * 1).withMargin(0));
    contentBox.items.add(FlexItem(DEFAULT_VIEW_WIDTH, CONTROL_HEIGHT, *mHotkeyLabel).withMargin(0).withFlex(0));
    contentBox.items.add(FlexItem(150, CONTROL_HEIGHT, hotkeyButtonRowBox).withMargin(4).withFlex(0));

    contentBox.items.add(FlexItem(ELEMENT_MARGIN, 0).withMargin(0).withFlex(1));
    contentBox.items.add(FlexItem(DEFAULT_VIEW_WIDTH - 4, CONTROL_HEIGHT, buttonBox).withMargin(0).withFlex(0));

    mainBox.items.clear();
    mainBox.flexDirection = FlexBox::Direction::row;
    mainBox.items.add(FlexItem(DEFAULT_VIEW_WIDTH, ELEMENT_MARGIN, contentBox).withMargin(0).withFlex(1));
}

void SampleEditView::createNameInputs()
{
    mNameLabel = std::make_unique<Label>("nameLabel", TRANS("Name"));
    mNameLabel->setJustificationType(Justification::left);
    mNameLabel->setFont(Font(14, Font::bold));
    mNameLabel->setColour(Label::textColourId, Colour(0xeeffffff));
    addAndMakeVisible(mNameLabel.get());

    mNameInput = std::make_unique<TextEditor>("nameInput");
    mNameInput->setText(initialName);
    mNameInput->onReturnKey = [this]() {
        submitDialog();
    };
    mNameInput->onEscapeKey = [this]() {
        dismissDialog();
    };
    addAndMakeVisible(mNameInput.get());
}

void SampleEditView::createFilePathInputs()
{
    mFilePathLabel = std::make_unique<Label>("filePathLabel", TRANS("Absolute file path"));
    mFilePathLabel->setJustificationType(Justification::left);
    mFilePathLabel->setFont(Font(14, Font::bold));
    mFilePathLabel->setColour(Label::textColourId, Colour(0xeeffffff));
    addAndMakeVisible(mFilePathLabel.get());

    mFilePathInput = std::make_unique<TextEditor>("filePathInput");
    mFilePathInput->setText(initialFilePath);
    addAndMakeVisible(mFilePathInput.get());

    mBrowseFilePathButton = std::make_unique<SonoTextButton>(TRANS("Browse"));
    mBrowseFilePathButton->onClick = [this]() {
        browseFilePath();
    };
    addAndMakeVisible(mBrowseFilePathButton.get());

    filePathBox.flexDirection = FlexBox::Direction::row;
    filePathBox.items.add(FlexItem(ELEMENT_MARGIN, ELEMENT_MARGIN).withMargin(0));
    filePathBox.items.add(FlexItem(DEFAULT_VIEW_WIDTH - 84, CONTROL_HEIGHT, *mFilePathInput).withMargin(0).withFlex(0));
    filePathBox.items.add(FlexItem(ELEMENT_MARGIN, ELEMENT_MARGIN).withMargin(0));
    filePathBox.items.add(FlexItem(72, CONTROL_HEIGHT, *mBrowseFilePathButton).withMargin(0).withFlex(0));
    filePathBox.items.add(FlexItem(ELEMENT_MARGIN, ELEMENT_MARGIN).withMargin(0));
}

void SampleEditView::createColourInput()
{
    mColourInputLabel = std::make_unique<Label>("colourInputLabel", TRANS("Button colour"));
    mColourInputLabel->setJustificationType(Justification::left);
    mColourInputLabel->setFont(Font(14, Font::bold));
    mColourInputLabel->setColour(Label::textColourId, Colour(0xeeffffff));
    addAndMakeVisible(mColourInputLabel.get());

    // Add default 11 colour buttons.
    colourButtonRowTopBox.flexDirection = FlexBox::Direction::row;
    colourButtonRowTopBox.items.add(FlexItem(ELEMENT_MARGIN * 2, ELEMENT_MARGIN).withMargin(0));
    for (int i = 0; i < 6; ++i) {
        auto colourButton = createColourButton(i);
        colourButtonRowTopBox.items.add(FlexItem(32, 32, *colourButton).withMargin(0).withFlex(1));
        colourButtonRowTopBox.items.add(FlexItem(ELEMENT_MARGIN, ELEMENT_MARGIN).withMargin(0));
        mColourButtons.emplace_back(std::move(colourButton));
    }

    colourButtonRowBottomBox.flexDirection = FlexBox::Direction::row;
    colourButtonRowBottomBox.items.add(FlexItem(ELEMENT_MARGIN * 2, ELEMENT_MARGIN).withMargin(0));
    for (int i = 6; i < 11; ++i) {
        auto colourButton = createColourButton(i);
        colourButtonRowBottomBox.items.add(FlexItem(32, 32, *colourButton).withMargin(0).withFlex(1));
        colourButtonRowBottomBox.items.add(FlexItem(ELEMENT_MARGIN, ELEMENT_MARGIN).withMargin(0));
        mColourButtons.emplace_back(std::move(colourButton));
    }

    // Add colour picker button.
    auto customColourButton = std::make_unique<SonoDrawableButton>("colourPicker", DrawableButton::ButtonStyle::ImageOnButtonBackground);
    mColourPicker = std::make_unique<SoundSampleButtonColourPicker>(&selectedColour, customColourButton.get());
    mColourPicker->resetCheckmarkCallback = [this]() {
        updateColourButtonCheckmark();
    };
    customColourButton->onClick = [this]() {
        mColourPicker->show(getScreenBounds());
    };
    customColourButton->setColour(
            SonoTextButton::buttonColourId,
            Colour(selectedColour | SoundboardButtonColors::DEFAULT_BUTTON_COLOUR_ALPHA)
    );
    addAndMakeVisible(customColourButton.get());

    std::unique_ptr<Drawable> eyedropperImage(Drawable::createFromImageData(BinaryData::eyedropper_svg, BinaryData::eyedropper_svgSize));
    customColourButton->setImages(eyedropperImage.get());
    customColourButton->setForegroundImageRatio(0.55f);

    colourButtonRowBottomBox.items.add(FlexItem(32, 32, *customColourButton).withMargin(0).withFlex(1));
    colourButtonRowBottomBox.items.add(FlexItem(ELEMENT_MARGIN, ELEMENT_MARGIN).withMargin(0));
    mColourButtons.emplace_back(std::move(customColourButton));

    colourButtonBox.flexDirection = FlexBox::Direction::column;
    colourButtonBox.items.add(FlexItem(32 * 6, 32, colourButtonRowTopBox).withMargin(0).withFlex(0));
    colourButtonBox.items.add(FlexItem(ELEMENT_MARGIN, ELEMENT_MARGIN).withMargin(0));
    colourButtonBox.items.add(FlexItem(32 * 6, 32, colourButtonRowBottomBox).withMargin(0).withFlex(0));

    updateColourButtonCheckmark();
}

void SampleEditView::createPlaybackOptionInputs()
{
    mPlaybackOptionsLabel = std::make_unique<Label>("playbackOptionsLabel", TRANS("Playback options"));
    mPlaybackOptionsLabel->setJustificationType(Justification::left);
    mPlaybackOptionsLabel->setFont(Font(14, Font::bold));
    mPlaybackOptionsLabel->setColour(Label::textColourId, Colour(0xeeffffff));
    addAndMakeVisible(mPlaybackOptionsLabel.get());

    // Add row flexbox container for playback options
    playbackOptionsRowBox.flexDirection = FlexBox::Direction::row;
    playbackOptionsRowBox.items.add(FlexItem(ELEMENT_MARGIN * 2, ELEMENT_MARGIN).withMargin(0));

    createButtonBehaviourButton();
    playbackOptionsRowBox.items.add(FlexItem(56, 56, *mButtonBehaviourButton).withMargin(0).withFlex(0, 0));
    playbackOptionsRowBox.items.add(FlexItem(ELEMENT_MARGIN, ELEMENT_MARGIN).withMargin(0));

    createLoopButton();
    playbackOptionsRowBox.items.add(FlexItem(56, 56, *mLoopButton).withMargin(0).withFlex(0, 0));
    playbackOptionsRowBox.items.add(FlexItem(ELEMENT_MARGIN, ELEMENT_MARGIN).withMargin(0));

    createPlaybackBehaviourButton();
    playbackOptionsRowBox.items.add(FlexItem(56, 56, *mPlaybackBehaviourButton).withMargin(0).withFlex(0, 0));
    playbackOptionsRowBox.items.add(FlexItem(ELEMENT_MARGIN, ELEMENT_MARGIN).withMargin(0));

    createReplayBehvaiourButton();
    playbackOptionsRowBox.items.add(FlexItem(56, 56, *mReplayBehaviourButton).withMargin(0).withFlex(0, 0));
    playbackOptionsRowBox.items.add(FlexItem(ELEMENT_MARGIN, ELEMENT_MARGIN).withMargin(0));
}

void SampleEditView::createVolumeInputs()
{
    mVolumeLabel = std::make_unique<Label>("gainLabel", TRANS("Gain"));
    mVolumeLabel->setJustificationType(Justification::left);
    mVolumeLabel->setFont(Font(14, Font::bold));
    mVolumeLabel->setColour(Label::textColourId, Colour(0xeeffffff));
    addAndMakeVisible(mVolumeLabel.get());

    mVolumeSlider = std::make_unique<Slider>(Slider::RotaryHorizontalVerticalDrag,  Slider::TextBoxRight);
    mVolumeSlider->setRange(0.0, 2.0, 0.0);
    mVolumeSlider->setSkewFactor(0.5);
    mVolumeSlider->setName("volumeSlider");
    mVolumeSlider->setTitle(TRANS("Playback gain level"));
    mVolumeSlider->setSliderSnapsToMousePosition(false);
    mVolumeSlider->setTextBoxIsEditable(true);
    mVolumeSlider->setScrollWheelEnabled(true);
    mVolumeSlider->setMouseDragSensitivity(256);
    mVolumeSlider->setTextBoxStyle(Slider::TextBoxRight, false, 60, 14);
    mVolumeSlider->valueFromTextFunction = [](const String& s) -> float { return Decibels::decibelsToGain(s.getFloatValue()); };
    mVolumeSlider->textFromValueFunction = [](float v) -> String { return Decibels::toString(Decibels::gainToDecibels(v), 1); };
    mVolumeSlider->setValue(initialGain);
    addAndMakeVisible(mVolumeSlider.get());

    volumeRowBox.flexDirection = FlexBox::Direction::row;
    volumeRowBox.items.add(FlexItem(ELEMENT_MARGIN * 2, ELEMENT_MARGIN).withMargin(0));
    volumeRowBox.items.add(FlexItem(128, 36, *mVolumeSlider).withMargin(0).withFlex(0, 0));
}

void SampleEditView::createLoopButton()
{
    mLoopButton = std::make_unique<SonoDrawableButton>("loop", DrawableButton::ButtonStyle::ImageAboveTextLabel);
    auto loopImg = Drawable::createFromImageData(BinaryData::loop_icon_svg, BinaryData::loop_icon_svgSize);
    auto loopOffImg = Drawable::createFromImageData(BinaryData::loop_off_icon_png, BinaryData::loop_off_icon_pngSize);
    mLoopButton->setImages(loopOffImg.get(), nullptr, nullptr, nullptr, loopImg.get());
    mLoopButton->setClickingTogglesState(true);
    mLoopButton->setToggleState(initialLoop, dontSendNotification);
    mLoopButton->setTooltip(TRANS("Loop On/Off"));
    mLoopButton->setTitle(TRANS("Loop"));
    mLoopButton->setButtonText(TRANS("Loop"));
    mLoopButton->setColour(DrawableButton::backgroundColourId, Colour::fromFloatRGBA(0.5, 0.5, 0.5, 0.3));

    addAndMakeVisible(mLoopButton.get());
}


void SampleEditView::createReplayBehvaiourButton()
{
    auto replayImage = Drawable::createFromImageData(BinaryData::replay_icon_svg, BinaryData::replay_icon_svgSize);
    auto continueImage = Drawable::createFromImageData(BinaryData::continue_svg, BinaryData::continue_svgSize);
    auto buttonImages = std::vector<std::unique_ptr<Drawable>>();
    buttonImages.push_back(std::move(replayImage));
    buttonImages.push_back(std::move(continueImage));
    auto labels = std::vector<String>{TRANS("Replay"), TRANS("Continue")};
    mReplayBehaviourButton = std::make_unique<SonoMultiStateDrawableButton>("replayBehaviour", std::move(buttonImages), std::move(labels));
    mReplayBehaviourButton->setColour(DrawableButton::backgroundColourId, Colour::fromFloatRGBA(0.5, 0.5, 0.5, 0.3));
    mReplayBehaviourButton->setState(0 /* TODO: link up with enum */);

    addAndMakeVisible(mReplayBehaviourButton.get());
}

void SampleEditView::createPlaybackBehaviourButton()
{
    auto simImg = Drawable::createFromImageData(BinaryData::play_simultaneous_svg, BinaryData::play_simultaneous_svgSize);
    auto b2bImg = Drawable::createFromImageData(BinaryData::play_back_to_back_svg, BinaryData::play_back_to_back_svgSize);
    auto imgs = std::vector<std::unique_ptr<Drawable>>();
    imgs.push_back(std::move(simImg));
    imgs.push_back(std::move(b2bImg));
    auto labels = std::vector<String>{TRANS("Simultaneous"), TRANS("Back to Back")};
    mPlaybackBehaviourButton = std::make_unique<SonoMultiStateDrawableButton>("playbackBehaviour", std::move(imgs), std::move(labels));
    mPlaybackBehaviourButton->setColour(DrawableButton::backgroundColourId, Colour::fromFloatRGBA(0.5, 0.5, 0.5, 0.3));
    mPlaybackBehaviourButton->setState(initialPlaybackBehaviour);

    addAndMakeVisible(mPlaybackBehaviourButton.get());
}

void SampleEditView::createButtonBehaviourButton()
{
    auto toggleImg = Drawable::createFromImageData(BinaryData::toggle_svg, BinaryData::toggle_svgSize);
    auto holdImg = Drawable::createFromImageData(BinaryData::hold_svg, BinaryData::hold_svgSize);
    auto oneshotImg = Drawable::createFromImageData(BinaryData::oneshot_svg, BinaryData::oneshot_svgSize);
    auto imgs = std::vector<std::unique_ptr<Drawable>>();
    imgs.push_back(std::move(toggleImg));
    imgs.push_back(std::move(holdImg));
    imgs.push_back(std::move(oneshotImg));
    auto labels = std::vector<String>{TRANS("Toggle"), TRANS("Hold"), TRANS("1-shot")};
    mButtonBehaviourButton = std::make_unique<SonoMultiStateDrawableButton>("buttonBehaviour", std::move(imgs), std::move(labels));
    mButtonBehaviourButton->setColour(DrawableButton::backgroundColourId, Colour::fromFloatRGBA(0.5, 0.5, 0.5, 0.3));
    mButtonBehaviourButton->setState(initialButtonBehaviour);

    addAndMakeVisible(mButtonBehaviourButton.get());
}

void SampleEditView::createHotkeyInput()
{
    mHotkeyLabel = std::make_unique<Label>("hotkeyLabel", TRANS("Hotkey"));
    mHotkeyLabel->setJustificationType(Justification::left);
    mHotkeyLabel->setFont(Font(14, Font::bold));
    mHotkeyLabel->setColour(Label::textColourId, Colour(0xeeffffff));
    addAndMakeVisible(mHotkeyLabel.get());

    auto keyPress = juce::KeyPress(hotkeyCode);
    auto buttonText = hotkeyCode == -1 ? TRANS("Click to change...") : keyPress.getTextDescriptionWithIcons();
    mHotkeyButton = std::make_unique<SonoTextButton>(buttonText);
    mHotkeyButton->setClickingTogglesState(true);
    mHotkeyButton->setToggleState(false, dontSendNotification);
    mHotkeyButton->onClick = [this]() {
        if (mHotkeyButton->getToggleState()) {
            mHotkeyButton->setButtonText(TRANS("Press a key..."));
        }
    };
    addAndMakeVisible(mHotkeyButton.get());

    mRemoveHotkeyButton = std::make_unique<SonoTextButton>(TRANS("Remove hotkey"));
    mRemoveHotkeyButton->onClick = [this]() {
        mHotkeyButton->setButtonText(TRANS("Click to change..."));
        mHotkeyButton->setToggleState(false, dontSendNotification);
        hotkeyCode = -1;
    };
    addAndMakeVisible(mRemoveHotkeyButton.get());

    hotkeyButtonRowBox.flexDirection = FlexBox::Direction::row;
    hotkeyButtonRowBox.items.add(FlexItem(ELEMENT_MARGIN, ELEMENT_MARGIN).withMargin(0));
    hotkeyButtonRowBox.items.add(FlexItem(DEFAULT_VIEW_WIDTH / 2 - 12, CONTROL_HEIGHT, *mHotkeyButton).withMargin(0).withFlex(0));
    hotkeyButtonRowBox.items.add(FlexItem(ELEMENT_MARGIN, ELEMENT_MARGIN).withMargin(0));
    hotkeyButtonRowBox.items.add(FlexItem(DEFAULT_VIEW_WIDTH / 2 - 8, CONTROL_HEIGHT, *mRemoveHotkeyButton).withMargin(0).withFlex(0));
    hotkeyButtonRowBox.items.add(FlexItem(ELEMENT_MARGIN, ELEMENT_MARGIN).withMargin(0));
}

std::unique_ptr<SonoDrawableButton> SampleEditView::createColourButton(const int index)
{
    auto colourButton = std::make_unique<SonoDrawableButton>("colour", DrawableButton::ButtonStyle::ImageOnButtonBackground);
    colourButton->setColour(
            SonoTextButton::buttonColourId,
            Colour(BUTTON_COLOURS[index] | SoundboardButtonColors::DEFAULT_BUTTON_COLOUR_ALPHA)
    );
    colourButton->onClick = [this, index]() {
        selectedColour = BUTTON_COLOURS[index];
        updateColourButtonCheckmark();

        // Also update the eyedropper to reflect the current colour.
        auto& pickerButton = mColourButtons[mColourButtons.size() - 1];
        pickerButton->setColour(
                SonoTextButton::buttonColourId,
                Colour(selectedColour | SoundboardButtonColors::DEFAULT_BUTTON_COLOUR_ALPHA)
        );
    };

    addAndMakeVisible(colourButton.get());
    return colourButton;
}

void SampleEditView::updateColourButtonCheckmark()
{
    std::unique_ptr<Drawable> checkImage(Drawable::createFromImageData(BinaryData::checkmark_svg, BinaryData::checkmark_svgSize));
    auto selectedIndex = indexOfColour(selectedColour);
    auto buttonCount = mColourButtons.size() - 1; /* last one is custom colour */

    for (int i = 0; i < buttonCount; ++i) {
        auto& colourButton = mColourButtons[i];

        // Show checkmark.
        if (i == selectedIndex) {
            colourButton->setForegroundImageRatio(0.55f);
            colourButton->setImages(checkImage.get());
        }
        // Hide checkmark.
        else {
            colourButton->setImages(nullptr);
        }
    }
}

void SampleEditView::createButtonBar()
{
    mSubmitButton = std::make_unique<SonoTextButton>(isEditMode() ? TRANS("Update Sample") : TRANS("Save Sample"));
    mSubmitButton->onClick = [this]() {
        submitDialog();
    };
    addAndMakeVisible(mSubmitButton.get());

    mDeleteButton = std::make_unique<SonoTextButton>(isEditMode() ? TRANS("Delete") : TRANS("Cancel"));
    mDeleteButton->setColour(SonoTextButton::buttonColourId, Colour(0xcc911707));
    mDeleteButton->onClick = [this]() {
        if (isEditMode()) {
            deleteSample = true;
            submitDialog();
        }
        else {
            dismissDialog();
        }
    };
    addAndMakeVisible(mDeleteButton.get());

    buttonBox.flexDirection = FlexBox::Direction::row;
    buttonBox.items.add(FlexItem(ELEMENT_MARGIN, ELEMENT_MARGIN).withMargin(0));
    buttonBox.items.add(
            FlexItem(DEFAULT_VIEW_WIDTH / 4 * 2.2, CONTROL_HEIGHT, *mSubmitButton).withMargin(0).withFlex(3));
    buttonBox.items.add(FlexItem(ELEMENT_MARGIN, ELEMENT_MARGIN).withMargin(0));
    buttonBox.items.add(FlexItem(DEFAULT_VIEW_WIDTH / 4, CONTROL_HEIGHT, *mDeleteButton).withMargin(0).withFlex(1));
    buttonBox.items.add(FlexItem(ELEMENT_MARGIN, ELEMENT_MARGIN).withMargin(0));
}

String SampleEditView::getSampleName() const
{
    return mNameInput->getText().trim();
}

String SampleEditView::getAbsoluteFilePath() const
{
    return mFilePathInput->getText().trim();
}

float SampleEditView::getGain() const
{
    return mVolumeSlider->getValue();
}

void SampleEditView::browseFilePath()
{
    // Determine where to open the file chooser.
    File defaultDirectory = File::getSpecialLocation(File::userMusicDirectory);
    if (lastOpenedDirectory != nullptr) {
        defaultDirectory = File(*lastOpenedDirectory);
    }

    // Open the file chooser.
    mFileChooser = std::make_unique<FileChooser>(
            TRANS("Select an audio file..."),
            defaultDirectory,
            SoundSample::SUPPORTED_EXTENSIONS,
            true,
            false,
            this->getParentComponent()
    );
    auto folderFlags = FileBrowserComponent::openMode | FileBrowserComponent::canSelectFiles;

    mFileChooser->launchAsync(folderFlags, [this](const FileChooser& chooser) {
        File chosenFile = chooser.getResult();
        mFilePathInput->setText(chosenFile.getFullPathName(), true);

        inferSampleName();

        if (lastOpenedDirectory != nullptr) {
            *lastOpenedDirectory = chosenFile.getParentDirectory().getFullPathName();
        }
    });
}

void SampleEditView::inferSampleName()
{
    auto absolutePath = getAbsoluteFilePath();
    if (!mNameInput->isEmpty() || absolutePath.isEmpty()) {
        return;
    }

    auto probablyFile = File(getAbsoluteFilePath());
    mNameInput->setText(probablyFile.getFileNameWithoutExtension());
}

void SampleEditView::submitDialog()
{
    auto inputtedName = mNameLabel->getText().trim();
    if (inputtedName.isEmpty()) {
        mNameInput->setColour(TextEditor::backgroundColourId, Colour(0xcc911707));
        return;
    }

    this->initialName = inputtedName;

    submitCallback(*this);
    dismissDialog();
}

void SampleEditView::dismissDialog()
{
    auto callOutBox = findParentComponentOfClass<CallOutBox>();
    if (callOutBox) {
        callOutBox->dismiss();
    }
}

bool SampleEditView::keyPressed(const KeyPress& keyPress)
{
    // Set new hotkey only when the hotkey button is ready to receive a new hotkey.
    if (!mHotkeyButton->getToggleState()) {
        return false;
    }

    auto keyPressWithoutModifier = juce::KeyPress(keyPress.getKeyCode());
    mHotkeyButton->setButtonText(keyPressWithoutModifier.getTextDescriptionWithIcons());
    mHotkeyButton->setToggleState(false, dontSendNotification);
    hotkeyCode = keyPress.getKeyCode();
    return true;
}

void SampleEditView::paint(Graphics& g)
{
    g.fillAll(Colour(0xff131313));
}

void SampleEditView::resized()
{
    mainBox.performLayout(getLocalBounds().reduced(2));
}