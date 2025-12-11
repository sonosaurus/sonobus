// SPDX-License-Identifier: GPLv3-or-later WITH Appstore-exception
// Copyright (C) 2024 SoundFlip

#pragma once

#include "JuceHeader.h"

class ActiveSessionView : public Component
{
public:
    ActiveSessionView();
    ~ActiveSessionView() override = default;

    void paint(Graphics& g) override;
    void resized() override;

    // Callbacks
    std::function<void()> onEndClicked;
    std::function<void()> onRecordClicked;
    std::function<void()> onChatClicked;
    std::function<void()> onInviteClicked;
    std::function<void()> onSessionEnded;

private:
    Label titleLabel;
    Label statusLabel;
    TextButton endSessionButton;
    TextButton recordButton;
    TextButton chatButton;
    TextButton inviteButton;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ActiveSessionView)
};