// SPDX-License-Identifier: GPLv3-or-later WITH Appstore-exception
// Mogg Mixer Window – per-stem fader / mute / solo / colour panel

#pragma once

#include "JuceHeader.h"
#include "SonobusPluginProcessor.h"
#include "SonoLookAndFeel.h"

// Palette of distinct stem colours  (can be changed by the user)
static const Colour kStemColours[] = {
    Colour(0xFF4FC3F7), // sky blue
    Colour(0xFFAED581), // light green
    Colour(0xFFFFB74D), // amber
    Colour(0xFFF48FB1), // pink
    Colour(0xFFCE93D8), // lavender
    Colour(0xFF80DEEA), // cyan
    Colour(0xFFFFCC02), // yellow
    Colour(0xFFFF8A65), // deep orange
};
static const int kNumStemColours = (int)(sizeof(kStemColours)/sizeof(kStemColours[0]));

// ============================================================================
class MoggMixerComponent  : public Component,
                             public Timer,
                             public SonobusAudioProcessor::MidiLearnListener
{
public:
    int remotePeerIndex = -1; // -1 means local file playback

    // -------------------------------------------------------------------------
    struct StemStrip : public Component
    {
        StemStrip(int idx_, SonobusAudioProcessor& proc_, MoggMixerComponent* parent_)
            : idx(idx_), processor(proc_), parent(parent_)
        {
            colour = kStemColours[idx % kNumStemColours];
            bool isRemote = parent->remotePeerIndex >= 0;

            // Label
            String name;
            if (isRemote) 
                name = processor.getRemotePeerChannelGroupName(parent->remotePeerIndex, idx);
            else {
                // For local, use our predefined stem names ("Full Mix", "Stem 1", etc)
                name = processor.getFilePlaybackChannelGroupName(idx);
            }

            if (name.isEmpty()) name = "Track " + String(idx+1);

            nameLabel.setText(name, dontSendNotification);
            nameLabel.setJustificationType(Justification::centred);
            nameLabel.setFont(Font(11.0f, Font::bold));
            nameLabel.setColour(Label::textColourId, Colours::white);
            addAndMakeVisible(nameLabel);

            // Colour swatch (click to cycle colour)
            colourSwatch.setTooltip("Click to change track colour");
            colourSwatch.onClick = [this] { cycleColour(); };
            addAndMakeVisible(colourSwatch);

            // Fader  (vertical)
            fader.setSliderStyle(Slider::LinearBarVertical);
            fader.setRange(0.0, 2.0, 0.0);
            fader.setSkewFactor(0.5f);
            fader.setTextBoxStyle(Slider::NoTextBox, false, 0, 0);
            fader.setPopupDisplayEnabled(true, false, nullptr);
            fader.textFromValueFunction = [](double v) {
                return Decibels::toString(Decibels::gainToDecibels((float)v, -60.0f), 1);
            };
            fader.valueFromTextFunction = [](const String& s) -> double {
                return Decibels::decibelsToGain(s.getDoubleValue());
            };
            fader.onValueChange = [this] {
                if (parent->remotePeerIndex >= 0)
                    processor.setRemotePeerChannelGain(parent->remotePeerIndex, idx, (float)fader.getValue());
                else
                    processor.setFilePlaybackGain(idx, (float)fader.getValue());
            };
            
            float startGain = isRemote ? processor.getRemotePeerChannelGain(parent->remotePeerIndex, idx)
                                       : processor.getFilePlaybackGain(idx);
            fader.setValue(startGain, dontSendNotification);
            addAndMakeVisible(fader);

            // Mute
            muteBtn.setButtonText("M");
            muteBtn.setClickingTogglesState(true);
            muteBtn.setColour(TextButton::buttonOnColourId,  Colour(0xFFCC5500));
            muteBtn.setColour(TextButton::buttonColourId,    Colour(0xFF444444));
            muteBtn.setColour(TextButton::textColourOnId,    Colours::white);
            muteBtn.setColour(TextButton::textColourOffId,   Colour(0xFFAAAAAA));
            muteBtn.onStateChange = [this] {
                if (parent->remotePeerIndex >= 0)
                    processor.setRemotePeerChannelMuted(parent->remotePeerIndex, idx, muteBtn.getToggleState());
                else
                    processor.setFilePlaybackMuted(idx, muteBtn.getToggleState());
            };
            
            bool startMute = isRemote ? processor.getRemotePeerChannelMuted(parent->remotePeerIndex, idx)
                                      : processor.getFilePlaybackMuted(idx);
            muteBtn.setToggleState(startMute, dontSendNotification);
            muteBtn.setTooltip("Mute track " + String(idx+1) + " (right-click for MIDI learn)");
            addAndMakeVisible(muteBtn);

            // Solo
            soloBtn.setButtonText("S");
            soloBtn.setClickingTogglesState(true);
            soloBtn.setColour(TextButton::buttonOnColourId,  Colour(0xFFDDCC00));
            soloBtn.setColour(TextButton::buttonColourId,    Colour(0xFF444444));
            soloBtn.setColour(TextButton::textColourOnId,    Colours::black);
            soloBtn.setColour(TextButton::textColourOffId,   Colour(0xFFAAAAAA));
            soloBtn.onStateChange = [this] {
                if (parent->remotePeerIndex >= 0)
                    processor.setRemotePeerChannelSoloed(parent->remotePeerIndex, idx, soloBtn.getToggleState());
                else
                    processor.setFilePlaybackSoloed(idx, soloBtn.getToggleState());
            };
            
            bool startSolo = isRemote ? processor.getRemotePeerChannelSoloed(parent->remotePeerIndex, idx)
                                      : processor.getFilePlaybackSoloed(idx);
            soloBtn.setToggleState(startSolo, dontSendNotification);
            soloBtn.setTooltip("Solo track " + String(idx+1) + " (right-click for MIDI learn)");
            addAndMakeVisible(soloBtn);

            // dB readout label
            dbLabel.setFont(Font(9.0f));
            dbLabel.setJustificationType(Justification::centred);
            dbLabel.setColour(Label::textColourId, Colour(0xFF88CCFF));
            addAndMakeVisible(dbLabel);

            refreshDbLabel();

            // Right-click for MIDI learn
            fader.addMouseListener(this, false);
            muteBtn.addMouseListener(this, false);
            soloBtn.addMouseListener(this, false);

            meter.setMeterSource(&processor.getMoggStemMeterSource(isRemote ? parent->remotePeerIndex : -1, idx));
            addAndMakeVisible(meter);
        }

        void mouseDown(const MouseEvent& e) override
        {
            if (e.mods.isRightButtonDown()) {
                if (e.eventComponent == &fader) {
                    parent->showMidiMenu(&fader, SonobusAudioProcessor::MidiTarget_FileStemGain, idx);
                } else if (e.eventComponent == &muteBtn) {
                    parent->showMidiMenu(&muteBtn, SonobusAudioProcessor::MidiTarget_FileStemMute, idx);
                } else if (e.eventComponent == &soloBtn) {
                    parent->showMidiMenu(&soloBtn, SonobusAudioProcessor::MidiTarget_FileStemSolo, idx);
                }
            }
        }

        void cycleColour()
        {
            // Find next colour in palette
            for (int i=0; i<kNumStemColours; ++i) {
                if (kStemColours[i] == colour) {
                    colour = kStemColours[(i+1) % kNumStemColours];
                    repaint();
                    return;
                }
            }
            colour = kStemColours[0];
            repaint();
        }

        void refreshDbLabel()
        {
            float g = (parent->remotePeerIndex >= 0) ? processor.getRemotePeerChannelGain(parent->remotePeerIndex, idx)
                                                     : processor.getFilePlaybackGain(idx);
            if (g <= 0.0f)
                dbLabel.setText("-inf", dontSendNotification);
            else
                dbLabel.setText(Decibels::toString(Decibels::gainToDecibels(g, -60.0f), 1),
                                dontSendNotification);
        }

        void paint(Graphics& g) override
        {
            auto b = getLocalBounds().reduced(2);
            // Background strip
            g.setColour(Colour(0xFF282830));
            g.fillRoundedRectangle(b.toFloat(), 6.0f);

            // Colour accent bar at top
            g.setColour(colour);
            g.fillRoundedRectangle(b.removeFromTop(6).toFloat(), 3.0f);
        }

        void resized() override
        {
            auto b = getLocalBounds().reduced(4);
            b.removeFromTop(8); // accent bar
            nameLabel.setBounds(b.removeFromTop(18));
            colourSwatch.setBounds(b.removeFromTop(14).reduced(20,2));
            b.removeFromTop(4);
            soloBtn.setBounds(b.removeFromTop(22).reduced(4,0));
            muteBtn.setBounds(b.removeFromTop(22).reduced(4,0));
            b.removeFromTop(4);
            dbLabel.setBounds(b.removeFromTop(14));
            meter.setBounds(b.removeFromRight(15).reduced(2, 4));
            fader.setBounds(b.reduced(2,4));
        }

        int idx;
        SonobusAudioProcessor& processor;
        Colour colour;
        Label nameLabel;
        TextButton colourSwatch { "" };   // clickable swatch
        Slider fader;
        TextButton muteBtn, soloBtn;
        Label dbLabel;
        MoggMixerComponent* parent;

        foleys::LevelMeter meter;
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(StemStrip)
    };

    // -------------------------------------------------------------------------
    MoggMixerComponent(SonobusAudioProcessor& proc, int remoteIdx = -1)
        : processor(proc), remotePeerIndex(remoteIdx)
    {
        setName(remotePeerIndex >= 0 ? "Remote MOGG Mixer" : "MOGG Stem Mixer");
        processor.addMidiLearnListener(this);
        rebuild();
    }

    ~MoggMixerComponent() override 
    { 
        processor.removeMidiLearnListener(this);
        stopTimer(); 
    }

    int getRemotePeerIndex() const { return remotePeerIndex; }

    void midiMappingChanged() override { /* refresh if we held state */ }

    void showMidiMenu(Component* source, SonobusAudioProcessor::MidiTargetType type, int stemIdx)
    {
        if (remotePeerIndex >= 0) return; // No MIDI learn for remote peers for now

        int ccNum = -1, midiCh = 0;
        bool hasMapped = processor.getMidiMapping(type, stemIdx, ccNum, midiCh);

        PopupMenu m;
        if (processor.isMidiLearning() && 
            processor.getMidiLearnTargetType() == type && 
            processor.getMidiLearnTargetData() == stemIdx) 
        {
            m.addItem(1, TRANS("Cancel MIDI Learn"));
        } else {
            m.addItem(1, TRANS("MIDI Learn"));
        }

        if (hasMapped) {
            m.addItem(2, TRANS("Clear MIDI Assignment") + " (CC " + String(ccNum) + ")");
        }

        m.showMenuAsync(PopupMenu::Options().withTargetComponent(source),
            [this, type, stemIdx, hasMapped](int result) {
                if (result == 1) {
                    if (processor.isMidiLearning()) processor.stopMidiLearn();
                    else processor.startMidiLearn(type, stemIdx);
                } else if (result == 2 && hasMapped) {
                    processor.clearMidiMapping(type, stemIdx);
                }
            });
    }

    void rebuild()
    {
        strips.clear();
        int n = (remotePeerIndex >= 0) ? processor.getRemotePeerMoggStemCount(remotePeerIndex)
                                       : processor.getFilePlaybackGroupCount();
        for (int i=0; i<n; ++i) {
            auto* s = new StemStrip(i, processor, this);
            addAndMakeVisible(s);
            strips.add(s);
        }
        startTimerHz(15);
        resized();
    }

    int getIdealWidth()  const { return jmax(400, strips.size() * (kStripW + kGap) + kGap*2); }
    int getIdealHeight() const { return 320; }

    void resized() override
    {
        int x = kGap;
        for (auto* s : strips) {
            s->setBounds(x, kGap, kStripW, getHeight() - kGap*2);
            x += kStripW + kGap;
        }
    }

    void paint(Graphics& g) override
    {
        g.fillAll(Colour(0xFF1A1A22));
        g.setColour(Colour(0xFF333344));
        g.drawRect(getLocalBounds(), 1);

        if (strips.isEmpty()) {
            g.setColour(Colours::grey);
            g.setFont(14.0f);
            g.drawText(remotePeerIndex >= 0 ? "Peer is not sending multitrack" : "No multitrack file loaded", 
                       getLocalBounds(), Justification::centred, true);
        }
    }

    void timerCallback() override
    {
        // Sync UI state from processor
        int n = (remotePeerIndex >= 0) ? processor.getRemotePeerMoggStemCount(remotePeerIndex)
                                       : processor.getFilePlaybackGroupCount();
        if (n != strips.size()) { rebuild(); return; }

        for (auto* s : strips) {
            float g = (remotePeerIndex >= 0) ? processor.getRemotePeerChannelGain(remotePeerIndex, s->idx)
                                             : processor.getFilePlaybackGain(s->idx);
            if (std::abs((float)s->fader.getValue() - g) > 0.001f)
                s->fader.setValue(g, dontSendNotification);

            bool muted  = (remotePeerIndex >= 0) ? processor.getRemotePeerChannelMuted(remotePeerIndex, s->idx)
                                                 : processor.getFilePlaybackMuted(s->idx);
            bool soloed = (remotePeerIndex >= 0) ? processor.getRemotePeerChannelSoloed(remotePeerIndex, s->idx)
                                                 : processor.getFilePlaybackSoloed(s->idx);
            if (s->muteBtn.getToggleState() != muted)
                s->muteBtn.setToggleState(muted, dontSendNotification);
            if (s->soloBtn.getToggleState() != soloed)
                s->soloBtn.setToggleState(soloed, dontSendNotification);

            s->refreshDbLabel();
        }
    }

private:
    static constexpr int kStripW = 72;
    static constexpr int kGap    = 6;

    SonobusAudioProcessor& processor;
    OwnedArray<StemStrip> strips;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MoggMixerComponent)
};

// ============================================================================
// Floating DialogWindow that hosts the mixer
class MoggMixerWindow : public DocumentWindow
{
public:
    MoggMixerWindow(SonobusAudioProcessor& proc, Component* centreRelativeTo, int remoteIdx = -1)
        : DocumentWindow(remoteIdx >= 0 ? "Remote MOGG Mixer" : "MOGG Stem Mixer",
                         Colour(0xFF1A1A22),
                         DocumentWindow::closeButton | DocumentWindow::minimiseButton)
    {
        mixer = std::make_unique<MoggMixerComponent>(proc, remoteIdx);
        setUsingNativeTitleBar(false);
        setContentNonOwned(mixer.get(), true);
        setResizable(true, false);

        int w = mixer->getIdealWidth();
        int h = mixer->getIdealHeight() + getTitleBarHeight() + 4;
        setSize(w, h);

        if (centreRelativeTo)
            centreAroundComponent(centreRelativeTo, w, h);

        setVisible(true);
        setAlwaysOnTop(true);
        
        DBG("MoggMixerWindow created for peer index: " << remoteIdx);
    }

    MoggMixerComponent* getMixer() const { return mixer.get(); }


    void closeButtonPressed() override
    {
        setVisible(false);
        // Caller holds the unique_ptr — it'll delete us when ready
        if (onClosed) onClosed();
    }

    std::function<void()> onClosed;

    MoggMixerComponent* getMixer() { return mixer.get(); }

private:
    std::unique_ptr<MoggMixerComponent> mixer;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MoggMixerWindow)
};
