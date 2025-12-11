// SPDX-License-Identifier: GPLv3-or-later WITH Appstore-exception
// Copyright (C) 2024 SoundFlip

#pragma once

#include "JuceHeader.h"

class StartSessionView : public Component
{
public:
    StartSessionView();
    ~StartSessionView() override = default;

    void paint(Graphics& g) override;
    void resized() override;

    // Callbacks
    std::function<void()> onStartClicked;
    std::function<void()> onBackClicked;
    std::function<void(const String& sessionId)> onSessionCreated;

private:
    Label titleLabel;
    Label sessionNameLabel;
    TextEditor sessionNameEditor;
    TextButton createButton;
    TextButton backButton;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(StartSessionView)
};