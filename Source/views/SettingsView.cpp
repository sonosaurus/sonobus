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
    // populateAudioDevices();
}

SettingsView::~SettingsView()
{
}

void SettingsView::populateAudioDevices()
{
    auto* deviceManager = getAudioDeviceManager ? getAudioDeviceManager() : nullptr;
    
    DBG("SettingsView::populateAudioDevices - deviceManager: " << (deviceManager ? "valid" : "null"));
    
    if (!deviceManager)
    {
        DBG("SettingsView::populateAudioDevices - No device manager available");
        
        // Add placeholder items
        audioInputCombo->clear();
        audioInputCombo->addItem("No Audio Device Available", 1);
        audioInputCombo->setSelectedId(1, dontSendNotification);
        
        audioOutputCombo->clear();
        audioOutputCombo->addItem("No Audio Device Available", 1);
        audioOutputCombo->setSelectedId(1, dontSendNotification);
        
        sampleRateCombo->clear();
        sampleRateCombo->addItem("48000 Hz", 1);
        sampleRateCombo->setSelectedId(1, dontSendNotification);
        
        bufferSizeCombo->clear();
        bufferSizeCombo->addItem("256 samples", 1);
        bufferSizeCombo->setSelectedId(1, dontSendNotification);
        
        return;
    }
    
    // Clear existing items and stored names
    audioInputCombo->clear();
    audioOutputCombo->clear();
    sampleRateCombo->clear();
    bufferSizeCombo->clear();
    inputDeviceNames.clear();
    outputDeviceNames.clear();
    sampleRates.clear();
    bufferSizes.clear();
    
    // Get current device setup
    auto currentSetup = deviceManager->getAudioDeviceSetup();
    DBG("SettingsView::populateAudioDevices - current input device: '" << currentSetup.inputDeviceName << "'");
    DBG("SettingsView::populateAudioDevices - current output device: '" << currentSetup.outputDeviceName << "'");
    DBG("SettingsView::populateAudioDevices - current sample rate: " << currentSetup.sampleRate);
    DBG("SettingsView::populateAudioDevices - current buffer size: " << currentSetup.bufferSize);
    
    auto* currentType = deviceManager->getCurrentDeviceTypeObject();
    DBG("SettingsView::populateAudioDevices - currentType: " << (currentType ? currentType->getTypeName() : "null"));
    
    // Helper lambda to populate devices from a device type
    auto populateDevicesFromType = [&](AudioIODeviceType* type, bool scanFirst) {
        if (type == nullptr) return;
        
        if (scanFirst)
        {
            DBG("Scanning for devices of type: " << type->getTypeName());
            type->scanForDevices();
        }
        
        auto inputs = type->getDeviceNames(true);
        DBG("  Found " << inputs.size() << " input devices from " << type->getTypeName());
        for (const auto& name : inputs)
        {
            DBG("    Input: '" << name << "'");
            if (!inputDeviceNames.contains(name))
            {
                inputDeviceNames.add(name);
                audioInputCombo->addItem(name, inputDeviceNames.size());
            }
        }
        
        auto outputs = type->getDeviceNames(false);
        DBG("  Found " << outputs.size() << " output devices from " << type->getTypeName());
        for (const auto& name : outputs)
        {
            DBG("    Output: '" << name << "'");
            if (!outputDeviceNames.contains(name))
            {
                outputDeviceNames.add(name);
                audioOutputCombo->addItem(name, outputDeviceNames.size());
            }
        }
    };
    
    // First try the current device type
    if (currentType)
    {
        populateDevicesFromType(currentType, true);
    }
    
    // If we didn't find any devices, or if currentType is null, try all available types
    if (inputDeviceNames.isEmpty() || outputDeviceNames.isEmpty() || !currentType)
    {
        DBG("Trying all available device types as fallback...");
        
        auto& availableTypes = deviceManager->getAvailableDeviceTypes();
        DBG("Found " << availableTypes.size() << " available device types");
        
        for (auto* type : availableTypes)
        {
            if (type != nullptr && type != currentType) // Don't re-scan currentType
            {
                populateDevicesFromType(type, true);
            }
        }
    }
    
    // Select the correct input device or show placeholder
    if (audioInputCombo->getNumItems() > 0)
    {
        int selectedInputIndex = 1;
        for (int i = 0; i < inputDeviceNames.size(); ++i)
        {
            if (inputDeviceNames[i] == currentSetup.inputDeviceName)
            {
                selectedInputIndex = i + 1;
                break;
            }
        }
        audioInputCombo->setSelectedId(selectedInputIndex, dontSendNotification);
        DBG("Selected input device index: " << selectedInputIndex << " (" << inputDeviceNames[selectedInputIndex - 1] << ")");
    }
    else
    {
        DBG("WARNING: No input devices found!");
        audioInputCombo->addItem("No Input Devices Found", 1);
        audioInputCombo->setSelectedId(1, dontSendNotification);
    }
    
    // Select the correct output device or show placeholder
    if (audioOutputCombo->getNumItems() > 0)
    {
        int selectedOutputIndex = 1;
        for (int i = 0; i < outputDeviceNames.size(); ++i)
        {
            if (outputDeviceNames[i] == currentSetup.outputDeviceName)
            {
                selectedOutputIndex = i + 1;
                break;
            }
        }
        audioOutputCombo->setSelectedId(selectedOutputIndex, dontSendNotification);
        DBG("Selected output device index: " << selectedOutputIndex << " (" << outputDeviceNames[selectedOutputIndex - 1] << ")");
    }
    else
    {
        DBG("WARNING: No output devices found!");
        audioOutputCombo->addItem("No Output Devices Found", 1);
        audioOutputCombo->setSelectedId(1, dontSendNotification);
    }
    
    // Populate sample rates and buffer sizes from current device
    auto* currentDevice = deviceManager->getCurrentAudioDevice();
    DBG("SettingsView::populateAudioDevices - currentDevice: " << (currentDevice ? currentDevice->getName() : "null"));
    
    if (currentDevice)
    {
        // Populate sample rates
        auto availableSampleRates = currentDevice->getAvailableSampleRates();
        DBG("Available sample rates: " << availableSampleRates.size());
        
        int selectedRateIndex = 1;
        for (int i = 0; i < availableSampleRates.size(); ++i)
        {
            int rate = (int)availableSampleRates[i];
            sampleRates.add(String(rate));
            sampleRateCombo->addItem(String(rate) + " Hz", i + 1);
            DBG("  Sample rate: " << rate);
            
            if (rate == (int)currentSetup.sampleRate)
                selectedRateIndex = i + 1;
        }
        
        if (sampleRateCombo->getNumItems() == 0)
        {
            sampleRates.add("48000");
            sampleRateCombo->addItem("48000 Hz", 1);
        }
        sampleRateCombo->setSelectedId(selectedRateIndex, dontSendNotification);
        
        // Populate buffer sizes
        auto availableBufferSizes = currentDevice->getAvailableBufferSizes();
        DBG("Available buffer sizes: " << availableBufferSizes.size());
        
        int selectedBufferIndex = 1;
        for (int i = 0; i < availableBufferSizes.size(); ++i)
        {
            int size = availableBufferSizes[i];
            bufferSizes.add(String(size));
            bufferSizeCombo->addItem(String(size) + " samples", i + 1);
            DBG("  Buffer size: " << size);
            
            if (size == currentSetup.bufferSize)
                selectedBufferIndex = i + 1;
        }
        
        if (bufferSizeCombo->getNumItems() == 0)
        {
            bufferSizes.add("256");
            bufferSizeCombo->addItem("256 samples", 1);
        }
        bufferSizeCombo->setSelectedId(selectedBufferIndex, dontSendNotification);
    }
    else
    {
        // No current device - add defaults
        DBG("No current audio device, using default sample rates and buffer sizes");
        
        const int defaultSampleRates[] = { 44100, 48000, 88200, 96000 };
        int selectedRateIndex = 2; // Default to 48000
        for (int i = 0; i < 4; ++i)
        {
            int rate = defaultSampleRates[i];
            sampleRates.add(String(rate));
            sampleRateCombo->addItem(String(rate) + " Hz", i + 1);
            
            if (rate == (int)currentSetup.sampleRate)
                selectedRateIndex = i + 1;
        }
        sampleRateCombo->setSelectedId(selectedRateIndex, dontSendNotification);
        
        const int defaultBufferSizes[] = { 64, 128, 256, 512, 1024, 2048 };
        int selectedBufferIndex = 3; // Default to 256
        for (int i = 0; i < 6; ++i)
        {
            int size = defaultBufferSizes[i];
            bufferSizes.add(String(size));
            bufferSizeCombo->addItem(String(size) + " samples", i + 1);
            
            if (size == currentSetup.bufferSize)
                selectedBufferIndex = i + 1;
        }
        bufferSizeCombo->setSelectedId(selectedBufferIndex, dontSendNotification);
    }
    
    DBG("SettingsView::populateAudioDevices - complete. Inputs: " << inputDeviceNames.size() << ", Outputs: " << outputDeviceNames.size());
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
    DBG("SettingsView::visibilityChanged - isVisible: " << (isVisible() ? "true" : "false"));
    
    if (isVisible())
    {
        DBG("SettingsView::visibilityChanged - calling populateAudioDevices");
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