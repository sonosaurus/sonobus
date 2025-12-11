// SPDX-License-Identifier: GPLv3-or-later WITH Appstore-exception
// Copyright (C) 2024 SoundFlip

#pragma once

#include <JuceHeader.h>

class ActiveSessionView : public Component,
                          public Button::Listener,
                          public Slider::Listener
{
public:
    ActiveSessionView();
    ~ActiveSessionView() override;

    void paint(Graphics&) override;
    void resized() override;
    void buttonClicked(Button* buttonThatWasClicked) override;
    void sliderValueChanged(Slider* slider) override;

    std::function<void()> onEndClicked;
    std::function<void()> onRecordClicked;
    std::function<void()> onChatClicked;
    std::function<void()> onInviteClicked;

private:
    std::unique_ptr<Label> sessionNameLabel;
    std::unique_ptr<TextButton> endButton;
    std::unique_ptr<TextButton> menuButton;

    std::unique_ptr<Label> youLabel;
    std::unique_ptr<Label> youLevelLabel;
    std::unique_ptr<TextButton> youMuteButton;

    std::unique_ptr<Label> peerLabel;
    std::unique_ptr<Label> peerLevelLabel;
    std::unique_ptr<Slider> peerVolumeSlider;

    std::unique_ptr<Label> connectionStatusLabel;

    std::unique_ptr<TextButton> recordButton;
    std::unique_ptr<TextButton> chatButton;
    std::unique_ptr<TextButton> inviteButton;

    bool isRecording = false;
    bool isMuted = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ActiveSessionView)
};