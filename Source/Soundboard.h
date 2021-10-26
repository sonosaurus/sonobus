//
// Created by Hannah Schellekens on 2021-10-26.
//


#pragma once

#include "string"
#include "vector"

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
    SoundSample(std::string name, std::string filePath);

    std::string getName();
    void setName(std::string newName);

    /**
     * @return The absolute file path of the underlying sound file.
     */
    std::string getFilePath();

    /**
     * @param filePath The absolute file path of the underlying sound file.
     */
    void setFilePath(std::string filePath);

private:
    /**
     * The name representing the sound sample.
     */
    std::string name;

    /**
     * The absolute file path of the underlying sound file.
     */
    std::string filePath;
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
    explicit Soundboard(std::string name);

    std::string getName();
    void setName(std::string);

    /**
     * Get the list of sound samples that are part of this soundboard.
     */
    std::vector<SoundSample>& getSamples();

private:
    /**
     * The name of the soundboard.
     */
    std::string name;

    /**
     * All the sounds that are available on the soundboard.
     */
    std::vector<SoundSample> samples;
};