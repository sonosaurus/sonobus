// SPDX-License-Identifier: GPLv3-or-later WITH Appstore-exception
// Copyright (C) 2024 SoundFlip

#include "SettingsView.h"

SettingsView::SettingsView(std::function<AudioDeviceManager*()> getAudioDeviceManagerFunc)
    : getAudioDeviceManager(getAudioDeviceManagerFunc)
{
    backButton = std::make_unique<TextButton>("< Back");
    backButton->addListener(this);
    addAndMakeVisible(backButton.get());

    titleLabel = std::make_unique<Label>("title", "Settings");
    titleLabel->setFont(Font(24.0f, Font::bold));
    titleLabel->setColour(Label::textColourId, Colours::white);
    titleLabel->setJustificationType(Justification::centredRight);
    addAndMakeVisible(titleLabel.get());

    // Audio Input
    audioInputLabel = std::make_unique<Label>("inputLabel", "Audio Input");
    audioInputLabel->setFont(Font(14.0f));
    audioInputLabel->setColour(Label::textColourId, Colours::grey);
    addAndMakeVisible(audioInputLabel.get());

    audioInputCombo = std::make_unique<ComboBox>("audioInput");
    audioInputCombo->addListener(this);
    addAndMakeVisible(audioInputCombo.get());

    // Audio Output
    audioOutputLabel = std::make_unique<Label>("outputLabel", "Audio Output");
    audioOutputLabel->setFont(Font(14.0f));
    audioOutputLabel->setColour(Label::textColourId, Colours::grey);
    addAndMakeVisible(audioOutputLabel.get());

    audioOutputCombo = std::make_unique<ComboBox>("audioOutput");
    audioOutputCombo->addListener(this);
    addAndMakeVisible(audioOutputCombo.get());

    // Sample Rate
    sampleRateLabel = std::make_unique<Label>("sampleRateLabel", "Sample Rate");
    sampleRateLabel->setFont(Font(14.0f));
    sampleRateLabel->setColour(Label::textColourId, Colours::grey);
    addAndMakeVisible(sampleRateLabel.get());

    sampleRateCombo = std::make_unique<ComboBox>("sampleRate");
    sampleRateCombo->addListener(this);
    addAndMakeVisible(sampleRateCombo.get());

    // Buffer Size
    bufferSizeLabel = std::make_unique<Label>("bufferSizeLabel", "Buffer Size");
    bufferSizeLabel->setFont(Font(14.0f));
    bufferSizeLabel->setColour(Label::textColourId, Colours::grey);
    addAndMakeVisible(bufferSizeLabel.get());

    bufferSizeCombo = std::make_unique<ComboBox>("bufferSize");
    bufferSizeCombo->addListener(this);
    addAndMakeVisible(bufferSizeCombo.get());

    // Recording Folder
    recordingFolderLabel = std::make_unique<Label>("folderLabel", "Recording Folder");
    recordingFolderLabel->setFont(Font(14.0f));
    recordingFolderLabel->setColour(Label::textColourId, Colours::grey);
    addAndMakeVisible(recordingFolderLabel.get());

    recordingFolderPath = std::make_unique<Label>("folderPath", "/Users/you/Music/SoundFlip");
    recordingFolderPath->setFont(Font(12.0f));
    recordingFolderPath->setColour(Label::textColourId, Colours::white);
    addAndMakeVisible(recordingFolderPath.get());

    changeFolderButton = std::make_unique<TextButton>("Change");
    changeFolderButton->addListener(this);
    addAndMakeVisible(changeFolderButton.get());

    // Sign Out
    signOutButton = std::make_unique<TextButton>("Sign Out");
    signOutButton->addListener(this);
    signOutButton->setColour(TextButton::buttonColourId, Colour(0xffe74c3c));
    addAndMakeVisible(signOutButton.get());

    // Populate devices on construction
    populateAudioDevices();
}

SettingsView::~SettingsView()
{
}

void SettingsView::populateAudioDevices()
{
    auto* deviceManager = getAudioDeviceManager ? getAudioDeviceManager() : nullptr;
    if (!deviceManager)
    {
        // No device manager available - add placeholder items
        audioInputCombo->clear();
        audioInputCombo->addItem("No audio device manager", 1);
        audioInputCombo->setSelectedId(1, dontSendNotification);
        
        audioOutputCombo->clear();
        audioOutputCombo->addItem("No audio device manager", 1);
        audioOutputCombo->setSelectedId(1, dontSendNotification);
        
        sampleRateCombo->clear();
        sampleRateCombo->addItem("48000 Hz", 1);
        sampleRateCombo->setSelectedId(1, dontSendNotification);
        
        bufferSizeCombo->clear();
        bufferSizeCombo->addItem("256 samples", 1);
        bufferSizeCombo->setSelectedId(1, dontSendNotification);
        return;
    }

    // Clear existing items
    audioInputCombo->clear();
    audioOutputCombo->clear();
    sampleRateCombo->clear();
    bufferSizeCombo->clear();
    inputDeviceNames.clear();
    outputDeviceNames.clear();
    sampleRates.clear();
    bufferSizes.clear();

    auto* currentType = deviceManager->getCurrentDeviceTypeObject();
    if (!currentType)
    {
        DBG("No current device type");
        return;
    }

    auto currentSetup = deviceManager->getAudioDeviceSetup();

    // Populate input devices
    auto inputNames = currentType->getDeviceNames(true);
    int inputIndex = 1;
    int selectedInput = 0;
    for (const auto& name : inputNames)
    {
        audioInputCombo->addItem(name, inputIndex);
        inputDeviceNames.add(name);
        if (name == currentSetup.inputDeviceName)
            selectedInput = inputIndex;
        inputIndex++;
    }
    if (inputNames.isEmpty())
    {
        audioInputCombo->addItem("No input devices found", 1);
    }
    if (selectedInput > 0)
        audioInputCombo->setSelectedId(selectedInput, dontSendNotification);
    else if (audioInputCombo->getNumItems() > 0)
        audioInputCombo->setSelectedId(1, dontSendNotification);

    // Populate output devices
    auto outputNames = currentType->getDeviceNames(false);
    int outputIndex = 1;
    int selectedOutput = 0;
    for (const auto& name : outputNames)
    {
        audioOutputCombo->addItem(name, outputIndex);
        outputDeviceNames.add(name);
        if (name == currentSetup.outputDeviceName)
            selectedOutput = outputIndex;
        outputIndex++;
    }
    if (outputNames.isEmpty())
    {
        audioOutputCombo->addItem("No output devices found", 1);
    }
    if (selectedOutput > 0)
        audioOutputCombo->setSelectedId(selectedOutput, dontSendNotification);
    else if (audioOutputCombo->getNumItems() > 0)
        audioOutputCombo->setSelectedId(1, dontSendNotification);

    // Populate sample rates and buffer sizes from current device
    auto* currentDevice = deviceManager->getCurrentAudioDevice();
    if (currentDevice)
    {
        // Sample rates
        auto availableRates = currentDevice->getAvailableSampleRates();
        int rateIndex = 1;
        int selectedRate = 0;
        for (auto rate : availableRates)
        {
            String rateStr = String((int)rate) + " Hz";
            sampleRateCombo->addItem(rateStr, rateIndex);
            sampleRates.add(String((int)rate));
            if ((int)rate == (int)currentSetup.sampleRate)
                selectedRate = rateIndex;
            rateIndex++;
        }
        if (selectedRate > 0)
            sampleRateCombo->setSelectedId(selectedRate, dontSendNotification);
        else if (sampleRateCombo->getNumItems() > 0)
            sampleRateCombo->setSelectedId(1, dontSendNotification);

        // Buffer sizes
        auto availableBuffers = currentDevice->getAvailableBufferSizes();
        int bufferIndex = 1;
        int selectedBuffer = 0;
        for (auto size : availableBuffers)
        {
            String sizeStr = String(size) + " samples";
            bufferSizeCombo->addItem(sizeStr, bufferIndex);
            bufferSizes.add(String(size));
            if (size == currentSetup.bufferSize)
                selectedBuffer = bufferIndex;
            bufferIndex++;
        }
        if (selectedBuffer > 0)
            bufferSizeCombo->setSelectedId(selectedBuffer, dontSendNotification);
        else if (bufferSizeCombo->getNumItems() > 0)
            bufferSizeCombo->setSelectedId(1, dontSendNotification);
    }
    else
    {
        // No current device - add common defaults
        sampleRateCombo->addItem("44100 Hz", 1);
        sampleRateCombo->addItem("48000 Hz", 2);
        sampleRateCombo->addItem("96000 Hz", 3);
        sampleRateCombo->setSelectedId(2, dontSendNotification);
        sampleRates.add("44100");
        sampleRates.add("48000");
        sampleRates.add("96000");

        bufferSizeCombo->addItem("128 samples", 1);
        bufferSizeCombo->addItem("256 samples", 2);
        bufferSizeCombo->addItem("512 samples", 3);
        bufferSizeCombo->addItem("1024 samples", 4);
        bufferSizeCombo->setSelectedId(2, dontSendNotification);
        bufferSizes.add("128");
        bufferSizes.add("256");
        bufferSizes.add("512");
        bufferSizes.add("1024");
    }
}

void SettingsView::comboBoxChanged(ComboBox* comboBoxThatHasChanged)
{
    auto* deviceManager = getAudioDeviceManager ? getAudioDeviceManager() : nullptr;
    if (!deviceManager)
        return;

    auto currentSetup = deviceManager->getAudioDeviceSetup();
    bool needsUpdate = false;

    if (comboBoxThatHasChanged == audioInputCombo.get())
    {
        int idx = audioInputCombo->getSelectedId() - 1;
        if (idx >= 0 && idx < inputDeviceNames.size())
        {
            currentSetup.inputDeviceName = inputDeviceNames[idx];
            needsUpdate = true;
            DBG("Selected input device: " << currentSetup.inputDeviceName);
        }
    }
    else if (comboBoxThatHasChanged == audioOutputCombo.get())
    {
        int idx = audioOutputCombo->getSelectedId() - 1;
        if (idx >= 0 && idx < outputDeviceNames.size())
        {
            currentSetup.outputDeviceName = outputDeviceNames[idx];
            needsUpdate = true;
            DBG("Selected output device: " << currentSetup.outputDeviceName);
        }
    }
    else if (comboBoxThatHasChanged == sampleRateCombo.get())
    {
        int idx = sampleRateCombo->getSelectedId() - 1;
        if (idx >= 0 && idx < sampleRates.size())
        {
            currentSetup.sampleRate = sampleRates[idx].getDoubleValue();
            needsUpdate = true;
            DBG("Selected sample rate: " << currentSetup.sampleRate);
        }
    }
    else if (comboBoxThatHasChanged == bufferSizeCombo.get())
    {
        int idx = bufferSizeCombo->getSelectedId() - 1;
        if (idx >= 0 && idx < bufferSizes.size())
        {
            currentSetup.bufferSize = bufferSizes[idx].getIntValue();
            needsUpdate = true;
            DBG("Selected buffer size: " << currentSetup.bufferSize);
        }
    }

    if (needsUpdate)
    {
        String error = deviceManager->setAudioDeviceSetup(currentSetup, true);
        if (error.isNotEmpty())
        {
            DBG("Audio device error: " << error);
            // Refresh to show actual state
            populateAudioDevices();
        }
        else
        {
            // Refresh sample rates and buffer sizes as they may have changed
            // when switching devices
            if (comboBoxThatHasChanged == audioInputCombo.get() ||
                comboBoxThatHasChanged == audioOutputCombo.get())
            {
                populateAudioDevices();
            }
        }
    }
}

void SettingsView::visibilityChanged()
{
    if (isVisible())
    {
        populateAudioDevices();
    }
}

void SettingsView::paint(Graphics& g)
{
    g.fillAll(Colour(0xff1a1a2e));
}

void SettingsView::resized()
{
    auto bounds = getLocalBounds().reduced(20);

    backButton->setBounds(20, 20, 80, 30);
    titleLabel->setBounds(bounds.getWidth() - 80, 20, 80, 30);

    int yPos = 70;
    int rowHeight = 55;
    int labelHeight = 20;
    int comboHeight = 35;

    audioInputLabel->setBounds(20, yPos, bounds.getWidth(), labelHeight);
    audioInputCombo->setBounds(20, yPos + 22, bounds.getWidth(), comboHeight);
    yPos += rowHeight;

    audioOutputLabel->setBounds(20, yPos, bounds.getWidth(), labelHeight);
    audioOutputCombo->setBounds(20, yPos + 22, bounds.getWidth(), comboHeight);
    yPos += rowHeight;

    sampleRateLabel->setBounds(20, yPos, bounds.getWidth(), labelHeight);
    sampleRateCombo->setBounds(20, yPos + 22, bounds.getWidth(), comboHeight);
    yPos += rowHeight;

    bufferSizeLabel->setBounds(20, yPos, bounds.getWidth(), labelHeight);
    bufferSizeCombo->setBounds(20, yPos + 22, bounds.getWidth(), comboHeight);
    yPos += rowHeight;

    recordingFolderLabel->setBounds(20, yPos, bounds.getWidth(), labelHeight);
    recordingFolderPath->setBounds(20, yPos + 22, bounds.getWidth() - 80, 20);
    changeFolderButton->setBounds(bounds.getWidth() - 50, yPos + 19, 70, 28);

    signOutButton->setBounds(20, bounds.getHeight() - 20, bounds.getWidth(), 45);
}

void SettingsView::buttonClicked(Button* buttonThatWasClicked)
{
    if (buttonThatWasClicked == backButton.get())
    {
        if (onBackClicked)
            onBackClicked();
    }
    else if (buttonThatWasClicked == signOutButton.get())
    {
        if (onSignOutClicked)
            onSignOutClicked();
    }
    else if (buttonThatWasClicked == changeFolderButton.get())
    {
        if (onChangeRecordingFolderClicked)
            onChangeRecordingFolderClicked();
    }
}