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
    submitCallback(std::move(callback)),
    lastOpenedDirectory(lastOpenedDirectoryString),
    selectedColour(soundSample == nullptr ? SoundboardButtonColors::DEFAULT_BUTTON_COLOUR : soundSample->getButtonColour())
{
    setOpaque(true);

    createNameInputs();
    createFilePathInputs();
    createColourInput();
    createPlaybackOptionInputs();
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
    contentBox.items.add(FlexItem(DEFAULT_VIEW_WIDTH - 4, CONTROL_HEIGHT, filePathBox).withMargin(0).withFlex(0));

    contentBox.items.add(FlexItem(ELEMENT_MARGIN, ELEMENT_MARGIN * 2).withMargin(0));
    contentBox.items.add(FlexItem(DEFAULT_VIEW_WIDTH, CONTROL_HEIGHT, *mColourInputLabel).withMargin(0).withFlex(0));
    contentBox.items.add(FlexItem(DEFAULT_VIEW_WIDTH - 4, 32 * 2, colourButtonBox).withMargin(0).withFlex(0));

    contentBox.items.add(FlexItem(ELEMENT_MARGIN, ELEMENT_MARGIN * 2).withMargin(0));
    contentBox.items.add(FlexItem(DEFAULT_VIEW_WIDTH, CONTROL_HEIGHT, *mPlaybackOptionsLabel).withMargin(0).withFlex(0));
    contentBox.items.add(FlexItem(DEFAULT_VIEW_WIDTH - 4, 32, playbackOptionsRowBox).withMargin(0).withFlex(0));

    contentBox.items.add(FlexItem(ELEMENT_MARGIN, ELEMENT_MARGIN * 6).withMargin(0).withFlex(1));
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

    createLoopButton();
    playbackOptionsRowBox.items.add(FlexItem(56, 56, *mLoopButton).withMargin(0).withFlex(0, 0));
    playbackOptionsRowBox.items.add(FlexItem(ELEMENT_MARGIN, ELEMENT_MARGIN).withMargin(0));
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

void SampleEditView::paint(Graphics& g)
{
    g.fillAll(Colour(0xff131313));
}

void SampleEditView::resized()
{
    mainBox.performLayout(getLocalBounds().reduced(2));
}