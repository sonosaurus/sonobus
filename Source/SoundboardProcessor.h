// SPDX-License-Identifier: GPLv3-or-later WITH Appstore-exception
// Copyright (C) 2020 Jesse Chappell



#pragma once

#include "JuceHeader.h"

#include <vector>
#include <optional>
#include <numeric>
#include <algorithm>
#include "Soundboard.h"
#include "SoundboardChannelProcessor.h"

/**
 * Controller for SoundboardView.
 *
 * @author Hannah Schellekens, Sten Wessel
 */
class SoundboardProcessor : public PlaybackPositionListener
{
public:
    SoundboardProcessor(SoundboardChannelProcessor* channelProcessor, File supportDir);
    virtual ~SoundboardProcessor();
    
    /**
     * Adds a new soundboard.
     *
     * @param name The name of the soundboard to create.
     * @param select Whether to select the soundboard after creation (false by default).
     * @return Reference to the created soundboard.
     */
    Soundboard& addSoundboard(const String& name, bool select = false);

    /**
     * Renames a soundboard.
     *
     * @param toRename The index of the soundboard to rename.
     * @param newName The new name for the soundboard.
     */
    void renameSoundboard(int index, String newName);

    /**
     * Deletes a soundboard.
     *
     * @param toRemove The index of the soundboard to remove.
     */
    void deleteSoundboard(int index);

    /**
     * Selects the soundboard at the given index.
     *
     * When the index > number of soundboards, the last soundboard is selected.
     * When the index <= 0, the first soundboard is selected.
     *
     * @param index The index of the soundboard to select.
     */
    void selectSoundboard(int index);

    /**
     * Updates the order of the soundboards.
     */
    void reorderSoundboards();

    /**
     * Gets the soundboard at the given index.
     *
     * If the index is out of bounds, undefined behavior may occur.
     *
     * @param [in] index The index of the soundboard.
     * @return The Soundboard at position `index`.
     */
    [[nodiscard]] Soundboard& getSoundboard(size_t index) { return soundboards[index]; }

    /**
     * @return The current amount of soundboards present.
     */
    [[nodiscard]] size_t getNumberOfSoundboards() const { return soundboards.size(); }

    /**
     * @return The index of the currently selected soundboard.
     */
    [[nodiscard]] std::optional<int> getSelectedSoundboardIndex() const { return selectedSoundboardIndex; }

    /**
     * Adds the given sample to the currently selected soundboard.
     * Does nothing when no soundboard is selected.
     *
     * @param name The name of the new sample.
     * @param absolutePath The absolute path to the file of the new sample.
     * @param index Optional index of soundboard, if not specified the selected soundboard is used
     *
     * @return Pointer to the added sound sample, nullptr when no sound was added.
     */
    SoundSample* addSoundSample(String name, String absolutePath, std::optional<int> index = std::nullopt );

    /**
     * Moves a particular sample to a different position in the sample list of the sound board
     *
     * @param fromSampleIndex The sample index to move.
     * @param toSampleIndex The sample index to move it to.
     * @param index Optional index of soundboard, if not specified the selected soundboard is used
     *
     * @return Returns true if sample successfully moved, otherwise false
     */
    bool moveSoundSample(int fromSampleIndex, int toSampleIndex, std::optional<int> index = std::nullopt );

    /**
     * @param url Checks to see if the given sample URL is used anywhere, in any soundboard
     */

    bool isSampleURLInUse(const juce::URL & url);

    /**
     * @param sampleToUpdate The sample to save with already containing the updated state.
     */
    void editSoundSample(SoundSample& sampleToUpdate, bool saveIt=true);

    /**
     * @param sampleToUpdate update playback settings
     */
    void updatePlaybackSettings(SoundSample& sampleToUpdate);

    /**
     * Deletes the given Sound Sample from the soundboard.
     *
     * @param sampleToDelete Reference to the sample to delete.
     * @param index Optional index of soundboard, if not specified the selected soundboard is used
     */
    bool deleteSoundSample(SoundSample& sampleToDelete, std::optional<int> index = std::nullopt );

    /**
     * Stops all playing sound samples from playing.
     */
    void stopAllPlayback();

    /**
     * @return The channel processor for playing and sending audio.
     */
    [[nodiscard]] SoundboardChannelProcessor* getChannelProcessor() const { return channelProcessor; }

    /**
     * @return Whether the hotkeys are muted (true) or not (false).
     */
    [[nodiscard]] bool isHotkeysMuted() const { return hotkeysMuted; }

    /**
     * Set Whether the hotkeys are muted (true) or not (false).
     */
    void setHotkeysMuted(bool selected)
    {
        hotkeysMuted = selected;
        saveToDisk();
    }

    /**
     * @return Whether the default numeric hotkeys are allowed.
     */
    [[nodiscard]] bool isDefaultNumericHotkeyAllowed() const { return numericHotkeyAllowed; }

    /**
     * Set Whether the hotkeys are muted (true) or not (false).
     */
    void setDefaultNumericHotkeyAllowed(bool selected)
    {
        numericHotkeyAllowed = selected;
        saveToDisk();
    }

    
    /**
     * Saves the current soundboard data to disk.
     */
    void saveToDisk();


    /**
     * Currently called when a new sample has been triggered by NEXT_ON_END sample ending
     */
    std::function<void()> onPlaybackStateChange;
    
protected:

    void onPlaybackFinished(SamplePlaybackManager* playbackManager) override;

    /**
     * Key of the root node in the serialized tree data structure that holds the soundboard data.
     */
    constexpr static const char SOUNDBOARDS_KEY[] = "soundboards";

    /**
     * Key of the root node property in the serialized tree data structure that holds the index of the currently selected soundboard.
     */
    constexpr static const char SELECTED_KEY[] = "selected";

    /*
     * Key of the root node property in the serialized tree data structure that stores whether hotkeys must be muted.
     */
    constexpr static const char HOTKEYS_MUTED_KEY[] = "hotkeysMuted";
    constexpr static const char HOTKEYS_NUMERIC_KEY[] = "hotkeysAllowNumeric";

    File soundboardsFile;

    /**
     * Channel processor for playing and sending audio.
     */
     SoundboardChannelProcessor* channelProcessor;

    /**
     * List of all available/known soundboards.
     */
    std::vector<Soundboard> soundboards;

    /**
     * Index of the currently selected soundboard.
     */
    std::optional<int> selectedSoundboardIndex;

    /**
     * Whether the hotkeys are muted or not.
     *
     * When hotkeys are muted (true), samples cannot be played by using hotkeys.
     * When hotkeys are not muted (false), samples can be played using hotkeys.
     */
    bool hotkeysMuted = false;

    bool numericHotkeyAllowed = true;

    /**
     * Writes the soundboard data to the given file.
     *
     * If the file does not yet exist, it will be created. When the file exists, it will be overwritten.
     *
     * @param [in] file The file to store the soundboard data in.
     */
    void writeSoundboardsToFile(const File& file);

    /**
     * Reads the soundboard data from the given file.
     *
     * Is is assumed that the file is correctly formatted.
     *
     * @param [in] file The file storing the soundboard data.
     */
    void readSoundboardsFromFile(const File& file);


    /**
     * Loads the current soundboard data from disk.
     */
    void loadFromDisk();

    /**
     * Returns a vector containing the original indices of the elements in a sorted vector.
     * Sorts by default order.
     * Does not actually sort the original vector.
     *
     * For example, take a sequence [4, 2, 1, 3]. This gets sorted to [1, 2, 3, 4] and will return the following
     * sequence [2, 1, 3, 0].
     *
     * @param vectorToSort The vector that must be sorted.
     * @return A vector containing the original indices of the elements in the sorted vector.
     */
    static std::vector<size_t> sortIndexPreview(const std::vector<Soundboard>& vectorToSort)
    {
        // Original index locations.
        std::vector<size_t> originalIndices(vectorToSort.size());
        std::iota(originalIndices.begin(), originalIndices.end(), 0);

        std::sort(originalIndices.begin(), originalIndices.end(), [&vectorToSort](size_t a, size_t b) {
            return vectorToSort[a].getName() < vectorToSort[b].getName();
        });

        return originalIndices;
    }
};
