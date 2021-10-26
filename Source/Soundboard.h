//
// Created by Hannah Schellekens on 2021-10-26.
//


#pragma once

#include "vector"
#include "JuceHeader.h"

/**
 * A sound file that can be played on a soundboard.
 *
 * @author Hannah Schellekens
 */
class SoundSample
{
public:
    /**
     * @param name The name representing the sound sample.
     * @param filePath The absolute file path of the underlying sound file.
     */
    SoundSample(String name, String filePath);

    SoundSample(SoundSample&& other) : name(std::move(other.name)),
                                       filePath(std::move(other.filePath)) { }

    String getName();
    void setName(String newName);

    /**
     * @return The absolute file path of the underlying sound file.
     */
    String getFilePath();

    /**
     * @param filePath The absolute file path of the underlying sound file.
     */
    void setFilePath(String filePath);

    ValueTree serialize() const;

    static SoundSample deserialize(ValueTree tree);

private:
    constexpr static const char SAMPLE_KEY[] = "soundSample";
    constexpr static const char NAME_KEY[] = "name";
    constexpr static const char FILE_PATH_KEY[] = "filePath";

    /**
     * The name representing the sound sample.
     */
    String name;

    /**
     * The absolute file path of the underlying sound file.
     */
    String filePath;
};

/**
 * A named collection of ordered sound samples.
 *
 * @author Hannah Schellekens
 */
class Soundboard
{
public:
    /**
     * Create a new soundboard with the given name.
     *
     * @param name The name of the soundboard.
     */
    explicit Soundboard(String name);

    String getName();
    void setName(String);

    /**
     * Get the list of sound samples that are part of this soundboard.
     */
    std::vector<SoundSample>& getSamples();

    ValueTree serialize() const;

    void saveToFile(const File& file) const;

    static Soundboard deserialize(ValueTree tree);

    static Soundboard readFromFile(const File& file);

private:
    constexpr static const char SOUNDBOARD_KEY[] = "soundboard";
    constexpr static const char NAME_KEY[] = "name";
    constexpr static const char SAMPLES_KEY[] = "samples";

    /**
     * The name of the soundboard.
     */
    String name;

    /**
     * All the sounds that are available on the soundboard.
     */
    std::vector<SoundSample> samples;
};