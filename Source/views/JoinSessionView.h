// SPDX-License-Identifier: GPLv3-or-later WITH Appstore-exception
// Copyright (C) 2024 SoundFlip

#pragma once

#include "JuceHeader.h"

class JoinSessionView : public Component
{
public:
    JoinSessionView();
    ~JoinSessionView() override = default;

    void paint(Graphics& g) override;
    void resized() override;

    // Callbacks
    std::function<void()> onJoinClicked;
    std::function<void()> onBackClicked;
    std::function<void(const String& sessionId)> onSessionJoined;

private:
    Label titleLabel;
    Label codeLabel;
    TextEditor codeEditor;
    TextButton joinButton;
    TextButton backButton;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(JoinSessionView)
};