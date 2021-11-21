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
class SoundboardProcessor
{
public:
    SoundboardProcessor(SoundboardChannelProcessor* channelProcessor);

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
     * Sets the given sample button index as currently playing.
     *
     * @param soundboardIndex Index of the soundboard.
     * @param sampleButtonIndex Index of the sample button.
     */
    void setCurrentlyPlaying(int soundboardIndex, int sampleButtonIndex);

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
     * @return The index of the currently playing soundboard.
     */
    [[nodiscard]] std::optional<int> getCurrentlyPlayingSoundboardIndex() const { return currentlyPlayingSoundboardIndex; }

    /**
     * @return The index of the currently playing sample button.
     */
    [[nodiscard]] std::optional<int> getCurrentlyPlayingButtonIndex() const { return currentlyPlayingButtonIndex; }

    /**
     * Adds the given sample to the currently selected soundboard.
     * Does nothing when no soundboard is selected.
     *
     * @param name The name of the new sample.
     * @param absolutePath The absolute path to the file of the new sample.
     * @param loop Whether the sample should loop.
     *
     * @return Pointer to the added sound sample, nullptr when no sound was added.
     */
    SoundSample* addSoundSample(String name, String absolutePath, bool loop);

    /**
     * @param sampleToUpdate The sample to save with already containing the updated state.
     */
    void editSoundSample(SoundSample& sampleToUpdate);

    /**
     * Deletes the given Sound Sample from the soundboard.
     *
     * @param sampleToDelete Reference to the sample to delete.
     */
    void deleteSoundSample(SoundSample& sampleToDelete);

    /**
     * @return The channel processor for playing and sending audio.
     */
    [[nodiscard]] SoundboardChannelProcessor* getChannelProcessor() const { return channelProcessor; }

private:
    /**
     * File where the soundboards data is stored, relative to the user home directory.
     */
    constexpr static const char SOUNDBOARDS_FILE_NAME[] = ".sonobus/soundboards.xml";

    /**
     * Key of the root node in the serialized tree data structure that holds the soundboard data.
     */
    constexpr static const char SOUNDBOARDS_KEY[] = "soundboards";

    /**
     * Key of the root node property in the serialized tree data structure that holds the index of the currently selected soundboard.
     */
    constexpr static const char SELECTED_KEY[] = "selected";

    /**
     * Gets the file where the soundboard data is stored on disk.
     *
     * @return The soundboard file.
     */
    static File getSoundboardsFile();

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
     * Index of the soundboard that is currently playing audio.
     */
    std::optional<int> currentlyPlayingSoundboardIndex;

    /**
     * Index of the button that is currently playing audio.
     */
    std::optional<int> currentlyPlayingButtonIndex;

    /**
     * Writes the soundboard data to the given file.
     *
     * If the file does not yet exist, it will be created. When the file exists, it will be overwritten.
     *
     * @param [in] file The file to store the soundboard data in.
     */
    void writeSoundboardsToFile(const File& file) const;

    /**
     * Reads the soundboard data from the given file.
     *
     * Is is assumed that the file is correctly formatted.
     *
     * @param [in] file The file storing the soundboard data.
     */
    void readSoundboardsFromFile(const File& file);

    /**
     * Saves the current soundboard data to disk.
     */
    void saveToDisk() const;

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