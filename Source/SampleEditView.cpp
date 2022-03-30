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
}

void SampleEditView::setEditMode(bool flag)
{
    editModeEnabled = flag;
    if (mDeleteButton) {
        mDeleteButton->setButtonText( isEditMode() ? TRANS("Delete") : TRANS("Cancel"));
    }
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
        submitDialog(false);
    };
    mNameInput->onEscapeKey = [this]() {
        dismissDialog();
    };
    addAndMakeVisible(mNameInput.get());
}

void SampleEditView::createFilePathInputs()
{
    mFilePathLabel = std::make_unique<Label>("filePathLabel", TRANS("File"));
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
    mColourPicker->colourChangedCallback = [this]() {
        updateColourButtonCheckmark();
        submitDialog(false); // no dismiss
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

    createButtonBehaviourButton();

    createLoopButton();

    createPlaybackBehaviourButton();

    createReplayBehaviourButton();
}

void SampleEditView::createVolumeInputs()
{
    mVolumeLabel = std::make_unique<Label>("gainLabel", TRANS("Gain"));
    mVolumeLabel->setJustificationType(Justification::left);
    mVolumeLabel->setFont(Font(14, Font::bold));
    mVolumeLabel->setColour(Label::textColourId, Colour(0xeeffffff));
    addAndMakeVisible(mVolumeLabel.get());

    mVolumeSlider = std::make_unique<Slider>(Slider::LinearHorizontal,  Slider::TextBoxRight);
    mVolumeSlider->setRange(0.0, 2.0, 0.0);
    mVolumeSlider->setSkewFactor(0.5);
    mVolumeSlider->setName("volumeSlider");
    mVolumeSlider->setTitle(TRANS("Playback gain level"));
    mVolumeSlider->setSliderSnapsToMousePosition(false);
    mVolumeSlider->setDoubleClickReturnValue(true, 1.0);
    mVolumeSlider->setTextBoxIsEditable(true);
    mVolumeSlider->setScrollWheelEnabled(false);
    mVolumeSlider->setMouseDragSensitivity(256);
    mVolumeSlider->setTextBoxStyle(Slider::TextBoxRight, false, 60, 32);
    mVolumeSlider->valueFromTextFunction = [](const String& s) -> float { return Decibels::decibelsToGain(s.getFloatValue()); };
    mVolumeSlider->textFromValueFunction = [](float v) -> String { return Decibels::toString(Decibels::gainToDecibels(v), 1); };
    mVolumeSlider->setValue(initialGain);
    mVolumeSlider->setChangeNotificationOnlyOnRelease(true);
    mVolumeSlider->onValueChange = [this]() {
        submitDialog(false);
    };

    addAndMakeVisible(mVolumeSlider.get());


}

void SampleEditView::createLoopButton()
{
    auto loopImg = Drawable::createFromImageData(BinaryData::loop_icon_svg, BinaryData::loop_icon_svgSize);
    auto loopOffImg = Drawable::createFromImageData(BinaryData::loop_off_icon_png, BinaryData::loop_off_icon_pngSize);
    auto loopImages = std::vector<std::unique_ptr<Drawable>>();
    loopImages.push_back(std::move(loopOffImg));
    loopImages.push_back(std::move(loopImg));
    auto labels = std::vector<String>{TRANS("Loop Off"), TRANS("Loop On")};

    mLoopButton = std::make_unique<SonoMultiStateDrawableButton>("loop", std::move(loopImages), std::move(labels));
    mLoopButton->setColour(DrawableButton::backgroundColourId, Colour::fromFloatRGBA(0.5, 0.5, 0.5, 0.3));
    mLoopButton->setState(initialLoop);
    mLoopButton->onStateChange = [this]() {
        submitDialog(false);
    };

    addAndMakeVisible(mLoopButton.get());
}


void SampleEditView::createReplayBehaviourButton()
{
    auto replayImage = Drawable::createFromImageData(BinaryData::replay_icon_svg, BinaryData::replay_icon_svgSize);
    auto continueImage = Drawable::createFromImageData(BinaryData::continue_svg, BinaryData::continue_svgSize);
    auto buttonImages = std::vector<std::unique_ptr<Drawable>>();
    buttonImages.push_back(std::move(replayImage));
    buttonImages.push_back(std::move(continueImage));
    auto labels = std::vector<String>{TRANS("Replay"), TRANS("Continue")};
    mReplayBehaviourButton = std::make_unique<SonoMultiStateDrawableButton>("replayBehaviour", std::move(buttonImages), std::move(labels));
    mReplayBehaviourButton->setColour(DrawableButton::backgroundColourId, Colour::fromFloatRGBA(0.5, 0.5, 0.5, 0.3));
    mReplayBehaviourButton->setState(initialReplayBehaviour);
    mReplayBehaviourButton->onStateChange = [this]() {
        submitDialog(false);
    };

    addAndMakeVisible(mReplayBehaviourButton.get());
}

void SampleEditView::createPlaybackBehaviourButton()
{
    auto simImg = Drawable::createFromImageData(BinaryData::play_simultaneous_svg, BinaryData::play_simultaneous_svgSize);
    auto b2bImg = Drawable::createFromImageData(BinaryData::play_back_to_back_svg, BinaryData::play_back_to_back_svgSize);
    auto bgImg = Drawable::createFromImageData(BinaryData::play_background_svg, BinaryData::play_background_svgSize);
    auto imgs = std::vector<std::unique_ptr<Drawable>>();
    imgs.push_back(std::move(simImg));
    imgs.push_back(std::move(b2bImg));
    imgs.push_back(std::move(bgImg));
    auto labels = std::vector<String>{TRANS("Simultaneous"), TRANS("Back to Back"), TRANS("Background")};
    mPlaybackBehaviourButton = std::make_unique<SonoMultiStateDrawableButton>("playbackBehaviour", std::move(imgs), std::move(labels));
    mPlaybackBehaviourButton->setColour(DrawableButton::backgroundColourId, Colour::fromFloatRGBA(0.5, 0.5, 0.5, 0.3));
    mPlaybackBehaviourButton->setState(initialPlaybackBehaviour);
    mPlaybackBehaviourButton->onStateChange = [this]() {
        submitDialog(false);
    };

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
    mButtonBehaviourButton->onStateChange = [this]() {
        submitDialog(false);
    };

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
        submitDialog(false);
    };
    addAndMakeVisible(mRemoveHotkeyButton.get());

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
        // and call submit callback (no dismiss)
        submitDialog(false);
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
            std::unique_ptr<Drawable> emptyImage = std::make_unique<DrawableRectangle>();
            colourButton->setImages(emptyImage.get());
        }
    }
}

void SampleEditView::createButtonBar()
{
    mDeleteButton = std::make_unique<SonoTextButton>(isEditMode() ? TRANS("Delete") : TRANS("Cancel"));
    mDeleteButton->setColour(SonoTextButton::buttonColourId, Colour(0xcc911707));
    mDeleteButton->onClick = [this]() {
        if (isEditMode()) {
            deleteSample = true;
            submitDialog();
            // and make sure no more callbacks happen
            submitCallback = nullptr;
        }
        else {
            dismissDialog();
        }
    };
    addAndMakeVisible(mDeleteButton.get());

    buttonBox.flexDirection = FlexBox::Direction::row;
    buttonBox.items.add(FlexItem(ELEMENT_MARGIN, ELEMENT_MARGIN).withMargin(0).withFlex(1));
    buttonBox.items.add(FlexItem(ELEMENT_MARGIN, ELEMENT_MARGIN).withMargin(0));
    buttonBox.items.add(FlexItem(DEFAULT_VIEW_WIDTH / 4, CONTROL_HEIGHT, *mDeleteButton).withMargin(0).withFlex(1).withMaxWidth(140));
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

void SampleEditView::submitDialog(bool dismiss)
{
    auto inputtedName = mNameLabel->getText().trim();
    if (inputtedName.isEmpty()) {
        mNameInput->setColour(TextEditor::backgroundColourId, Colour(0xcc911707));
        return;
    }

    this->initialName = inputtedName;

    if (submitCallback)
        submitCallback(*this);

    if (dismiss) {
        dismissDialog();
    }
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
    submitDialog(false);
    return true;
}

void SampleEditView::paint(Graphics& g)
{
    g.fillAll(Colour::fromFloatRGBA(0.1, 0.1, 0.1, 1.0));
}

void SampleEditView::resized()
{
    const int minlabwidth = 60;
    const int mintextwidth = 100;
    const int mintextbuttwidth = 72;
    const int rowheight = 32;
    const int labrowheight = 24;
    const int volheight = 38;
    const int playbackboxw = 50;
    const int playbackboxh = 50;

    FlexBox contentBox;
    contentBox.flexDirection = FlexBox::Direction::column;


    FlexBox namebox;
    namebox.flexDirection = FlexBox::Direction::row;
    namebox.items.add(FlexItem(minlabwidth, rowheight, *mNameLabel).withMargin(0).withFlex(0));
    namebox.items.add(FlexItem(ELEMENT_MARGIN, ELEMENT_MARGIN).withMargin(0));
    namebox.items.add(FlexItem(mintextwidth, rowheight, *mNameInput).withMargin(0).withFlex(1));

    contentBox.items.add(FlexItem(ELEMENT_MARGIN, ELEMENT_MARGIN * 2).withMargin(0));
    contentBox.items.add(FlexItem(mintextwidth, rowheight, namebox).withMargin(0));

    FlexBox filebox;
    filebox.flexDirection = FlexBox::Direction::row;
    filebox.items.add(FlexItem(minlabwidth, rowheight, *mFilePathLabel).withMargin(0).withFlex(0));
    filebox.items.add(FlexItem(ELEMENT_MARGIN, ELEMENT_MARGIN).withMargin(0));
    filebox.items.add(FlexItem(mintextwidth, rowheight, *mFilePathInput).withMargin(0).withFlex(1));
    filebox.items.add(FlexItem(ELEMENT_MARGIN, ELEMENT_MARGIN).withMargin(0));
    filebox.items.add(FlexItem(mintextbuttwidth, rowheight, *mBrowseFilePathButton).withMargin(0).withFlex(0));

    contentBox.items.add(FlexItem(ELEMENT_MARGIN, ELEMENT_MARGIN * 2).withMargin(0));
    contentBox.items.add(FlexItem(mintextwidth, rowheight, filebox).withMargin(0));


    contentBox.items.add(FlexItem(ELEMENT_MARGIN, ELEMENT_MARGIN * 2).withMargin(0));
    contentBox.items.add(FlexItem(mintextwidth, labrowheight, *mColourInputLabel).withMargin(0).withFlex(0));
    contentBox.items.add(FlexItem(ELEMENT_MARGIN, ELEMENT_MARGIN).withMargin(0));
    contentBox.items.add(FlexItem(32*6, 32 * 2, colourButtonBox).withMargin(0).withFlex(0));

    contentBox.items.add(FlexItem(ELEMENT_MARGIN, ELEMENT_MARGIN * 2).withMargin(0));
    contentBox.items.add(FlexItem(mintextwidth, labrowheight, *mPlaybackOptionsLabel).withMargin(0).withFlex(0));


    // Add row flexbox container for playback options
    FlexBox playbackOptionsRowBox;
    playbackOptionsRowBox.flexDirection = FlexBox::Direction::row;
    playbackOptionsRowBox.items.add(FlexItem(ELEMENT_MARGIN * 2, ELEMENT_MARGIN).withMargin(0));
    playbackOptionsRowBox.items.add(FlexItem(playbackboxw, playbackboxh, *mButtonBehaviourButton).withMargin(0).withFlex(0, 0));
    playbackOptionsRowBox.items.add(FlexItem(ELEMENT_MARGIN, ELEMENT_MARGIN).withMargin(0));
    playbackOptionsRowBox.items.add(FlexItem(playbackboxw, playbackboxh, *mLoopButton).withMargin(0).withFlex(0, 0));
    playbackOptionsRowBox.items.add(FlexItem(ELEMENT_MARGIN, ELEMENT_MARGIN).withMargin(0));
    playbackOptionsRowBox.items.add(FlexItem(playbackboxw, playbackboxh, *mPlaybackBehaviourButton).withMargin(0).withFlex(0, 0));
    playbackOptionsRowBox.items.add(FlexItem(ELEMENT_MARGIN, ELEMENT_MARGIN).withMargin(0));
    playbackOptionsRowBox.items.add(FlexItem(playbackboxw, playbackboxh, *mReplayBehaviourButton).withMargin(0).withFlex(0, 0));
    playbackOptionsRowBox.items.add(FlexItem(ELEMENT_MARGIN, ELEMENT_MARGIN).withMargin(0));

    contentBox.items.add(FlexItem(ELEMENT_MARGIN, ELEMENT_MARGIN).withMargin(0));
    contentBox.items.add(FlexItem(playbackboxw * 4, playbackboxh, playbackOptionsRowBox).withMargin(0).withFlex(0));

    FlexBox volbox;
    volbox.flexDirection = FlexBox::Direction::row;
    volbox.items.add(FlexItem(minlabwidth, rowheight, *mVolumeLabel).withMargin(0).withFlex(0));
    volbox.items.add(FlexItem(mintextwidth, volheight, *mVolumeSlider).withMargin(0).withFlex(1).withMaxWidth(400));

    contentBox.items.add(FlexItem(ELEMENT_MARGIN, ELEMENT_MARGIN * 2).withMargin(0));
    contentBox.items.add(FlexItem(mintextwidth, volheight, volbox).withMargin(0));


    FlexBox hotkeyButtonRowBox;
    hotkeyButtonRowBox.flexDirection = FlexBox::Direction::row;
    hotkeyButtonRowBox.items.add(FlexItem(minlabwidth, labrowheight, *mHotkeyLabel).withMargin(0).withFlex(0));
    hotkeyButtonRowBox.items.add(FlexItem(ELEMENT_MARGIN, ELEMENT_MARGIN).withMargin(0));
    hotkeyButtonRowBox.items.add(FlexItem(mintextwidth, rowheight, *mHotkeyButton).withMargin(0).withFlex(1));
    hotkeyButtonRowBox.items.add(FlexItem(ELEMENT_MARGIN, ELEMENT_MARGIN).withMargin(0));
    hotkeyButtonRowBox.items.add(FlexItem(mintextwidth, rowheight, *mRemoveHotkeyButton).withMargin(0).withFlex(1));
    hotkeyButtonRowBox.items.add(FlexItem(ELEMENT_MARGIN, ELEMENT_MARGIN).withMargin(0));

    contentBox.items.add(FlexItem(ELEMENT_MARGIN, ELEMENT_MARGIN * 1).withMargin(0));
    contentBox.items.add(FlexItem(2*mintextwidth, rowheight, hotkeyButtonRowBox).withMargin(0).withFlex(0));

    contentBox.items.add(FlexItem(ELEMENT_MARGIN, ELEMENT_MARGIN * 2).withMargin(0).withFlex(1));
    contentBox.items.add(FlexItem(mintextwidth, rowheight, buttonBox).withMargin(0).withFlex(0));


    int minh = 0;
    for (auto & item : contentBox.items) {
        minh += item.minHeight + item.margin.top + item.margin.bottom;
    }
    minContentHeight = minh + 4;
    minContentWidth = jmax(32*6, playbackboxw*4) + 10;

    contentBox.performLayout(getLocalBounds().reduced(2));

}

