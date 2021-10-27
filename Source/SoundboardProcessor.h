// SPDX-License-Identifier: GPLv3-or-later WITH Appstore-exception
// Copyright (C) 2020 Jesse Chappell



#pragma once

#include "Soundboard.h"
#include <vector>

/**
 * Controller for SoundboardView.
 *
 * @author Hannah Schellekens, Sten Wessel
 */
class SoundboardProcessor
{
public:
    SoundboardProcessor();

    /**
     * Adds a new soundboard.
     *
     * @param name The name of the soundboard to create.
     */
    void addSoundboard(const String& name);

    /**
     * Gets called whenever the process of renaming a soundboard must start.
     *
     * @param toRename Reference to the soundboard to rename.
     * @param newName The new name for the soundboard.
     */
    void renameSoundboard(Soundboard& toRename, String newName);

    /**
     * Deletes a soundboard.
     *
     * @param toRemove A reference to the soundboard to remove.
     */
    void deleteSoundboard(Soundboard& toRemove);

    /**
     * Gets the soundboard at the given index.
     *
     * If the index is out of bounds, undefined behavior may occur.
     *
     * @param [in] index The index of the soundboard.
     * @return The Soundboard at position `index`.
     */
    [[nodiscard]] const Soundboard& getSoundboard(size_t index) const { return soundboards[index]; }

    [[nodiscard]] size_t getNumberOfSoundboards() const { return soundboards.size(); }

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
     * Gets the file where the soundboard data is stored on disk.
     *
     * @return The soundboard file.
     */
    static File getSoundboardsFile();

    /**
     * List of all available/known soundboards.
     */
    std::vector<Soundboard> soundboards;

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
};