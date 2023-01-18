// SPDX-License-Identifier: GPLv3-or-later WITH Appstore-exception
// Copyright (C) 2020 Jesse Chappell



#include "SoundboardProcessor.h"
#include "SonoUtility.h"
#include <utility>

SoundboardProcessor::SoundboardProcessor(SoundboardChannelProcessor* channelProcessor, File supportDir) : channelProcessor(
        channelProcessor)
{
    soundboardsFile = supportDir.getChildFile("soundboards.xml");

    loadFromDisk();
}

SoundboardProcessor::~SoundboardProcessor()
{
    saveToDisk();
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
    // When a sample from this soundboard was playing, stop playback
    auto& activeSamples = channelProcessor->getActiveSamples();
    std::vector<juce::URL> urls;
    
    for (const auto& sample : soundboards[index].getSamples()) {
        auto playbackManager = activeSamples.find(&sample);
        if (playbackManager != activeSamples.end()) {
            playbackManager->second->unload();
        }
        urls.push_back(sample.getFileURL());
    }

    soundboards.erase(soundboards.begin() + index);

#if JUCE_ANDROID
    for (const auto& url : urls) {
        if ( ! isSampleURLInUse(url)) {
            AndroidDocumentPermission::releasePersistentReadWriteAccess(url);
        }
    }
#endif

    
    // If the last soundboard was selected
    if (selectedSoundboardIndex == soundboards.size()) {
        auto selected = *selectedSoundboardIndex;
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
    auto originalIndices = sortIndexPreview(soundboards);

    // Determine new indices of the selected soundboard.
    if (originalSelectedIndex < 0) {
        selectedSoundboardIndex = { 0 };
    }
    else {
        auto iterator = std::find(originalIndices.begin(), originalIndices.end(), originalSelectedIndex);
        selectedSoundboardIndex = { std::distance(originalIndices.begin(), iterator) };
    }

    // Above was just a sort preview and logic on that. End with actually sorting the list of soundboards.
    std::sort(soundboards.begin(), soundboards.end(), [](const Soundboard& a, const Soundboard& b) {
        return a.getName() < b.getName();
    });
}

SoundSample* SoundboardProcessor::addSoundSample(String name, String absolutePath, std::optional<int> index)
{
    // Per definition: do nothing when no soundboard is selected or specified.
    if (!index.has_value() && !selectedSoundboardIndex.has_value()) {
        return nullptr;
    }

    auto sindex = index.has_value() ? *index : *selectedSoundboardIndex;
    if (sindex < 0 || sindex >= soundboards.size())
        return nullptr;

    auto& soundboard = soundboards[sindex];
    auto& sampleList = soundboard.getSamples();

    SoundSample sampleToAdd = SoundSample(std::move(name), std::move(absolutePath));
    sampleList.emplace_back(std::move(sampleToAdd));

    saveToDisk();

    return &sampleList[sampleList.size() - 1];
}

bool SoundboardProcessor::moveSoundSample(int fromSampleIndex, int toSampleIndex, std::optional<int> index)
{
    if (!index.has_value() && !selectedSoundboardIndex.has_value()) {
        return false;
    }
    auto sindex = index.has_value() ? *index : *selectedSoundboardIndex;
    if (sindex < 0 || sindex >= soundboards.size())
        return false;

    auto& soundboard = soundboards[sindex];
    auto& sampleList = soundboard.getSamples();

    if (fromSampleIndex < 0 || fromSampleIndex >= sampleList.size()) {
        return false;
    }

    // stop all playback, just in case
    stopAllPlayback();
    
    auto sampcopy = sampleList[fromSampleIndex];
    auto destiter = std::next(sampleList.begin(), toSampleIndex);
    
    sampleList.insert(destiter, std::move(sampcopy));

    // remove the original
    int origpos = fromSampleIndex < toSampleIndex ? fromSampleIndex : fromSampleIndex+1;
    auto origiter = std::next(sampleList.begin(), origpos);
    sampleList.erase(origiter);
    
    saveToDisk();

    return true;
}


void SoundboardProcessor::editSoundSample(SoundSample& sampleToUpdate, bool saveIt)
{
#if JUCE_ANDROID
    AndroidDocumentPermission::takePersistentReadOnlyAccess(sampleToUpdate.getFileURL());
#endif

    if (saveIt) {
        saveToDisk();
    }

    updatePlaybackSettings(sampleToUpdate);
}

void SoundboardProcessor::updatePlaybackSettings(SoundSample& sampleToUpdate)
{
    // Immediately update transport source with new playback settings when this sample is currently playing
    auto& activeSamples = channelProcessor->getActiveSamples();
    auto playbackManager = activeSamples.find(&sampleToUpdate);
    if (playbackManager != activeSamples.end()) {
        playbackManager->second->reloadPlaybackSettingsFromSample();
    }
}

bool SoundboardProcessor::isSampleURLInUse(const juce::URL & url)
{
    for (auto& soundboard : soundboards) {
        auto& sampleList = soundboard.getSamples();
        
        for (auto & sample : sampleList) {
            if (sample.getFileURL() == url) {
                return true;
            }
        }
    }
    return false;
}

bool SoundboardProcessor::deleteSoundSample(SoundSample& sampleToDelete, std::optional<int> index)
{
    if (!index.has_value() && !selectedSoundboardIndex.has_value()) {
        return false;
    }

    auto sindex = index.has_value() ? *index : *selectedSoundboardIndex;
    if (sindex < 0 || sindex >= soundboards.size())
        return false;

    auto& soundboard = soundboards[sindex];
    auto& sampleList = soundboard.getSamples();

    for (auto iter = sampleList.begin(); iter != sampleList.end(); ++iter) {
        auto& sample = *iter;
        if (&sample == &sampleToDelete) {
            // If it is currently playing, unload
            auto& activeSamples = channelProcessor->getActiveSamples();
            auto playbackManager = activeSamples.find(&sample);
            if (playbackManager != activeSamples.end()) {
                playbackManager->second->unload();
            }

            auto url = sample.getFileURL();
            
            sampleList.erase(iter);

#if JUCE_ANDROID
            if ( ! isSampleURLInUse(url)) {
                AndroidDocumentPermission::releasePersistentReadWriteAccess(url);
            }
#endif

            break;
        }
    }

    saveToDisk();
    return true;
}

void SoundboardProcessor::stopAllPlayback()
{
    channelProcessor->unloadAll();
}

void SoundboardProcessor::onPlaybackFinished(SamplePlaybackManager* playbackManager)
{
    if (auto * sample = playbackManager->getSample()) {
        if (sample->getEndPlaybackBehaviour() == SoundSample::EndPlaybackBehaviour::NEXT_AT_END) {
            // trigger the next one in the relevant soundboard
            for (auto& soundboard : soundboards) {
                auto& sampleList = soundboard.getSamples();
                
                bool playit = false;
                bool foundboard = false;
                for (auto & samp : sampleList) {
                    if (playit) {
                        // we are the next, trigger playback
                        DBG("Triggering next sample");
                        auto playbackManagerMaybe = channelProcessor->loadSample(samp);
                        if (playbackManagerMaybe.has_value()) {
                            playbackManagerMaybe->get()->attach(this);
                            playbackManagerMaybe->get()->play();
                            if (onPlaybackStateChange) {
                                onPlaybackStateChange();
                            }
                        }
                        break;
                    }

                    if (foundboard) break;
                    
                    if (&samp == sample) {
                        playit = true;
                        foundboard = true;
                        continue;
                    }
                }
            }
            return false;
        }
    }
}



void SoundboardProcessor::writeSoundboardsToFile(const File& file)
{
    ValueTree tree(SOUNDBOARDS_KEY);

    tree.setProperty(SELECTED_KEY, selectedSoundboardIndex.value_or(-1), nullptr);
    tree.setProperty(HOTKEYS_MUTED_KEY, hotkeysMuted, nullptr);
    tree.setProperty(HOTKEYS_NUMERIC_KEY, numericHotkeyAllowed, nullptr);

    int i = 0;
    for (auto& soundboard: soundboards) {
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
    hotkeysMuted = tree.getProperty(HOTKEYS_MUTED_KEY, hotkeysMuted);
    numericHotkeyAllowed = tree.getProperty(HOTKEYS_NUMERIC_KEY, numericHotkeyAllowed);

    soundboards.clear();

    for (const auto& child: tree) {
        soundboards.emplace_back(Soundboard::deserialize(child));
    }
}

void SoundboardProcessor::saveToDisk()
{
    writeSoundboardsToFile(soundboardsFile);
}

void SoundboardProcessor::loadFromDisk()
{
    readSoundboardsFromFile(soundboardsFile);
    reorderSoundboards();
}
