// SPDX-License-Identifier: GPLv3-or-later WITH Appstore-exception
// Copyright (C) 2020 Jesse Chappell



#include "SoundboardProcessor.h"
#include "SonoUtility.h"
#include <utility>

SoundboardProcessor::SoundboardProcessor(SoundboardChannelProcessor* channelProcessor) : channelProcessor(
        channelProcessor)
{
    loadFromDisk();
}

Soundboard& SoundboardProcessor::addSoundboard(const String& name, const bool select)
{
    auto newSoundboard = Soundboard(name);
    soundboards.push_back(std::move(newSoundboard));

    if (select) {
        selectedSoundboardIndex = getNumberOfSoundboards() - 1;
    }

    reorderSoundboards();
    saveToDisk();

    return soundboards[selectedSoundboardIndex.value_or(0)];
}

void SoundboardProcessor::renameSoundboard(int index, String newName)
{
    auto& toRename = soundboards[index];
    toRename.setName(std::move(newName));

    reorderSoundboards();
    saveToDisk();
}

void SoundboardProcessor::deleteSoundboard(int index)
{
    soundboards.erase(soundboards.begin() + index);

    if (currentlyPlayingSoundboardIndex == index) {
        channelProcessor->pause();
        currentlyPlayingSoundboardIndex = {};
        currentlyPlayingButtonIndex = {};
    }

    // If the last soundboard was selected
    if (selectedSoundboardIndex == soundboards.size()) {
        auto selected = selectedSoundboardIndex.value();
        selectedSoundboardIndex = selected > 0 ? std::optional<int>(selected - 1) : std::nullopt;
    }

    reorderSoundboards();
    saveToDisk();
}

void SoundboardProcessor::selectSoundboard(int index)
{
    if (getNumberOfSoundboards() == 0) {
        selectedSoundboardIndex = {};
    }
    else {
        selectedSoundboardIndex = jmax(0, jmin(index, static_cast<int>(getNumberOfSoundboards())));
    }

    saveToDisk();
}

void SoundboardProcessor::reorderSoundboards()
{
    // Figure out what the new (sorted) indices will be.
    auto originalSelectedIndex = selectedSoundboardIndex.value_or(-1);
    auto originalPlayingIndex = currentlyPlayingSoundboardIndex.value_or(-1);
    auto originalIndices = sortIndexPreview(soundboards);

    // Determine new indices of the selected soundboard.
    if (originalSelectedIndex < 0) {
        selectedSoundboardIndex = {0 };
    }
    else {
        auto iterator = std::find(originalIndices.begin(), originalIndices.end(), originalSelectedIndex);
        selectedSoundboardIndex = { std::distance(originalIndices.begin(), iterator) };
    }

    if (originalPlayingIndex < 0) {
        currentlyPlayingSoundboardIndex = {0 };
    }
    else if (originalPlayingIndex >= soundboards.size()) {
        // It can happen that the playing index is out of bounds.
        // Specifically this occurs whenever a soundboard before the playing index has been deleted.
        currentlyPlayingSoundboardIndex = { soundboards.size() - 1 };
    }
    else {
        auto iterator = std::find(originalIndices.begin(), originalIndices.end(), originalPlayingIndex);
        currentlyPlayingSoundboardIndex = { std::distance(originalIndices.begin(), iterator) };
    }

    // Above was just a sort preview and logic on that. End with actually sorting the list of soundboards.
    std::sort(soundboards.begin(), soundboards.end(), [](const Soundboard& a, const Soundboard& b) {
        return a.getName() < b.getName();
    });
}

void SoundboardProcessor::setCurrentlyPlaying(int soundboardIndex, int sampleButtonIndex)
{
    currentlyPlayingSoundboardIndex = soundboardIndex;
    currentlyPlayingButtonIndex = sampleButtonIndex;
}

SoundSample* SoundboardProcessor::addSoundSample(String name, String absolutePath, bool loop)
{
    // Per definition: do nothing when no soundboard is selected.
    if (!selectedSoundboardIndex.has_value()) {
        return nullptr;
    }

    auto& soundboard = soundboards[selectedSoundboardIndex.value()];
    auto& sampleList = soundboard.getSamples();

    SoundSample sampleToAdd = SoundSample(std::move(name), std::move(absolutePath), loop);
    sampleList.emplace_back(std::move(sampleToAdd));

    saveToDisk();

    return &sampleList[sampleList.size() - 1];
}

void SoundboardProcessor::editSoundSample(SoundSample& sampleToUpdate)
{
    saveToDisk();
}

void SoundboardProcessor::deleteSoundSample(SoundSample& sampleToDelete)
{
    auto& soundboard = soundboards[getSelectedSoundboardIndex().value()];
    auto& sampleList = soundboard.getSamples();

    auto sampleCount = sampleList.size();
    for (int i = 0; i < sampleCount; ++i) {
        auto& sample = sampleList[i];
        if (&sample == &sampleToDelete) {
            sampleList.erase(sampleList.begin() + i, sampleList.begin() + i + 1);

            if (currentlyPlayingSoundboardIndex == selectedSoundboardIndex) {
                // If it is playing, stop playback.
                if (currentlyPlayingButtonIndex == i) {
                    channelProcessor->pause();
                    currentlyPlayingSoundboardIndex = {};
                    currentlyPlayingButtonIndex = {};
                }
                else if (currentlyPlayingButtonIndex > i) {
                    currentlyPlayingButtonIndex = currentlyPlayingButtonIndex.value() - 1;
                }
            }
            break;
        }
    }

    saveToDisk();
}

void SoundboardProcessor::writeSoundboardsToFile(const File& file) const
{
    ValueTree tree(SOUNDBOARDS_KEY);

    tree.setProperty(SELECTED_KEY, selectedSoundboardIndex.value_or(-1), nullptr);

    int i = 0;
    for (const auto& soundboard: soundboards) {
        tree.addChild(soundboard.serialize(), i++, nullptr);
    }

    // Make sure  the parent directory exists
    file.getParentDirectory().createDirectory();

    tree.createXml()->writeTo(file);
}

void SoundboardProcessor::readSoundboardsFromFile(const File& file)
{
    if (!file.existsAsFile()) {
        return;
    }

    XmlDocument doc(file);
    auto tree = ValueTree::fromXml(*doc.getDocumentElement());

    int selected = tree.getProperty(SELECTED_KEY);
    selectedSoundboardIndex = selected >= 0 ? std::optional<size_t>(selected) : std::nullopt;

    soundboards.clear();

    for (const auto& child: tree) {
        soundboards.emplace_back(Soundboard::deserialize(child));
    }
}

File SoundboardProcessor::getSoundboardsFile()
{
    return File::getSpecialLocation(File::userHomeDirectory).getChildFile(SOUNDBOARDS_FILE_NAME);
}

void SoundboardProcessor::saveToDisk() const
{
    writeSoundboardsToFile(getSoundboardsFile());
}

void SoundboardProcessor::loadFromDisk()
{
    readSoundboardsFromFile(getSoundboardsFile());
    reorderSoundboards();
}


