// SPDX-License-Identifier: GPLv3-or-later WITH Appstore-exception
// Copyright (C) 2020 Jesse Chappell



#pragma once

#include "vector"
#include "JuceHeader.h"
#include "SoundboardButtonColors.h"

/**
 * A sound file that can be played on a soundboard.
 *
 * @author Hannah Schellekens, Sten Wessel
 */
class SoundSample
{
public:

#if (JUCE_IOS || JUCE_MAC)
    /**
     * The file extensions of sound file types that are supported by the soundboard.
     */
    constexpr static const char SUPPORTED_EXTENSIONS[] = "*.wav;*.flac;*.aif;*.ogg;*.mp3;*.m4a;*.caf";
#else
    /**
     * The file extensions of sound file types that are supported by the soundboard.
     */
    constexpr static const char SUPPORTED_EXTENSIONS[] = "*.wav;*.flac;*.aif;*.ogg;*.mp3";
#endif

    /**
     * @param name The name representing the sound sample.
     * @param filePath The absolute file path of the underlying sound file.
     * @param loop Whether the sample should loop on playback.
     * @param buttonColour The colour of the sample button in RGB value with an alpha of 0.
     */
    SoundSample(
            String name,
            String filePath,
            bool loop = false,
            int buttonColour = SoundboardButtonColors::DEFAULT_BUTTON_COLOUR
    );

    String getName() const;

    void setName(String newName);

    /**
     * @return The absolute file path of the underlying sound file.
     */
    String getFilePath() const;

    /**
     * @param filePath The absolute file path of the underlying sound file.
     */
    void setFilePath(String filePath);

    bool isLoop() const;

    void setLoop(bool newLoop);

    /**
     * @return 0xRRGGBB without alpha value.
     */
    [[nodiscard]] int getButtonColour() const;

    /**
     * @param newRgb 0xRRGGBB without alpha value.
     */
    void setButtonColour(int newRgb);

    /**
     * Serialize the sound sample.
     *
     * @return Tree-shaped data structure representing the instance.
     */
    ValueTree serialize() const;

    /**
     * Converts a serialized tree data structure back into a SoundSample instance.
     *
     * @param [in] tree The serialization tree data structure. Must be of the same format as is generated by serialize().
     * @return The deserialized SoundSample instance.
     */
    static SoundSample deserialize(ValueTree tree);

private:
    /**
     * Key for the root node in the serialization tree data structure.
     */
    constexpr static const char SAMPLE_KEY[] = "soundSample";

    /**
     * Key for the name property of the root node in the serialization tree data structure.
     */
    constexpr static const char NAME_KEY[] = "name";

    /**
     * Key for the file path property of the root node in the serialization tree data structure.
     */
    constexpr static const char FILE_PATH_KEY[] = "filePath";

    /**
     * Key for the loop property of the root node in the serialization tree data structure.
     */
    constexpr static const char LOOP_KEY[] = "loop";

    /**
     * Key for the button colour property of the root node in the serialization tree data structure.
     */
    constexpr static const char BUTTON_COLOUR_KEY[] = "buttonColour";

    /**
     * The name representing the sound sample.
     */
    String name;

    /**
     * The absolute file path of the underlying sound file.
     */
    String filePath;

    /**
     * Whether the sample should loop (indefinitely).
     */
    bool loop;

    /**
     * The argb colour the sample button must have with 0 alpha value.
     */
    int buttonColour = SoundboardButtonColors::DEFAULT_BUTTON_COLOUR;
};

/**
 * A named collection of ordered sound samples.
 *
 * @author Hannah Schellekens, Sten Wessel
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

    String getName() const;

    void setName(String);

    /**
     * Get the list of sound samples that are part of this soundboard.
     */
    std::vector<SoundSample>& getSamples();

    /**
     * Serialize the soundboard.
     *
     * @return Tree-shaped data structure representing the instance.
     */
    ValueTree serialize() const;

    /**
     * Converts a serialized tree data structure back into a Soundboard instance.
     *
     * @param [in] tree The serialization tree data structure. Must be of the same format as is generated by serialize().
     * @return The deserialized Soundboard instance.
     */
    static Soundboard deserialize(ValueTree tree);

private:
    /**
     * Key for the root node in the serialization tree data structure.
     */
    constexpr static const char SOUNDBOARD_KEY[] = "soundboard";

    /**
     * Key for the name property of the root node in the serialization tree data structure.
     */
    constexpr static const char NAME_KEY[] = "name";

    /**
     * Key for the samples child node container in the serialization tree data structure.
     */
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