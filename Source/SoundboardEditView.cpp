// SPDX-License-Identifier: GPLv3-or-later WITH Appstore-exception
// Copyright (C) 2020 Jesse Chappell



#include "SoundboardEditView.h"

#include <utility>

SoundboardEditView::SoundboardEditView(std::function<void (String)> callback, const Soundboard* soundboard)
        : editModeEnabled(soundboard != nullptr),
          initialName(soundboard == nullptr ? "" : soundboard->getName()),
          submitCallback(std::move(callback))
{
    setOpaque(true);

    mMessageLabel = std::make_unique<Label>("messageLabel", TRANS("Name of the soundboard:"));
    mMessageLabel->setJustificationType(Justification::left);
    mMessageLabel->setFont(Font(12, Font::plain));
    mMessageLabel->setColour(Label::textColourId, Colour(0xeeffffff));
    addAndMakeVisible(mMessageLabel.get());

    mInputField = std::make_unique<TextEditor>("nameInput");
    mInputField->setText(initialName);
    mInputField->onReturnKey = [this]() {
        submitDialog();
    };
    mInputField->onEscapeKey = [this]() {
        dismissDialog();
    };
    addAndMakeVisible(mInputField.get());

    mSubmitButton = std::make_unique<SonoTextButton>(isEditMode() ? TRANS("Rename Soundboard") : TRANS("Create Soundboard"));
    mSubmitButton->onClick = [this]() {
        submitDialog();
    };
    addAndMakeVisible(mSubmitButton.get());

    mCancelButton = std::make_unique<SonoTextButton>(TRANS("Cancel"));
    mCancelButton->onClick = [this]() {
        dismissDialog();
    };
    addAndMakeVisible(mCancelButton.get());

    buttonBox.flexDirection = FlexBox::Direction::row;
    buttonBox.items.add(FlexItem(ELEMENT_MARGIN, ELEMENT_MARGIN).withMargin(0));
    buttonBox.items.add(FlexItem(DEFAULT_VIEW_WIDTH / 4 * 2.2, CONTROL_HEIGHT, *mSubmitButton).withMargin(0).withFlex(3));
    buttonBox.items.add(FlexItem(ELEMENT_MARGIN, ELEMENT_MARGIN).withMargin(0));
    buttonBox.items.add(FlexItem(DEFAULT_VIEW_WIDTH / 4, CONTROL_HEIGHT, *mCancelButton).withMargin(0).withFlex(1));
    buttonBox.items.add(FlexItem(ELEMENT_MARGIN, ELEMENT_MARGIN).withMargin(0));

    contentBox.flexDirection = FlexBox::Direction::column;
    contentBox.items.add(FlexItem(ELEMENT_MARGIN, ELEMENT_MARGIN).withMargin(0));
    contentBox.items.add(FlexItem(DEFAULT_VIEW_WIDTH, CONTROL_HEIGHT, *mMessageLabel).withMargin(0).withFlex(0));
    contentBox.items.add(FlexItem(ELEMENT_MARGIN, ELEMENT_MARGIN).withMargin(0));
    contentBox.items.add(FlexItem(DEFAULT_VIEW_WIDTH - 24, CONTROL_HEIGHT, *mInputField).withMargin(4).withFlex(0));
    contentBox.items.add(FlexItem(ELEMENT_MARGIN, ELEMENT_MARGIN).withMargin(0));
    contentBox.items.add(FlexItem(ELEMENT_MARGIN, CONTROL_HEIGHT, buttonBox).withMargin(4).withFlex(0));

    mainBox.items.clear();
    mainBox.flexDirection = FlexBox::Direction::row;
    mainBox.items.add(FlexItem(DEFAULT_VIEW_WIDTH, ELEMENT_MARGIN, contentBox).withMargin(0).withFlex(1));
}

String SoundboardEditView::getInputName() const
{
    return mInputField->getText();
}

void SoundboardEditView::submitDialog()
{
    auto inputtedName = mInputField->getText().trim();
    if (inputtedName.isEmpty()) {
        mInputField->setColour(TextEditor::backgroundColourId, Colour(0xcc911707));
        return;
    }

    this->initialName = inputtedName;

    submitCallback(inputtedName);
    dismissDialog();
}

void SoundboardEditView::dismissDialog()
{
    auto callOutBox = findParentComponentOfClass<CallOutBox>();
    if (callOutBox) {
        callOutBox->dismiss();
    }
}

void SoundboardEditView::paint(Graphics& g)
{
    g.fillAll(Colour(0xff131313));
}

void SoundboardEditView::resized()
{
    mainBox.performLayout(getLocalBounds().reduced(2));
}