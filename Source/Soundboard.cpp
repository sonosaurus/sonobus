//
// Created by Hannah Schellekens on 2021-10-26.
//


#include "Soundboard.h"
#include <utility>

SoundSample::SoundSample(String newName, String newFilePath)
        : name(std::move(newName)), filePath(std::move(newFilePath))
{}

String SoundSample::getName()
{
    return this->name;
}

void SoundSample::setName(String newName)
{
    this->name = std::move(newName);
}

String SoundSample::getFilePath()
{
    return this->filePath;
}

void SoundSample::setFilePath(String newFilePath)
{
    this->filePath = std::move(newFilePath);
}

ValueTree SoundSample::serialize() const
{
    ValueTree tree(SAMPLE_KEY);
    tree.setProperty(NAME_KEY, name, nullptr);
    tree.setProperty(FILE_PATH_KEY, filePath, nullptr);

    return tree;
}

SoundSample SoundSample::deserialize(const ValueTree tree)
{
    SoundSample soundSample(tree.getProperty(NAME_KEY), tree.getProperty(FILE_PATH_KEY));

    return soundSample;
}

Soundboard::Soundboard(String newName)
        : name(std::move(newName)), samples(std::vector<SoundSample>())
{}

String Soundboard::getName()
{
    return this->name;
}

void Soundboard::setName(String newName)
{
    this->name = std::move(newName);
}

std::vector<SoundSample> &Soundboard::getSamples()
{
    return this->samples;
}

ValueTree Soundboard::serialize() const
{
    ValueTree tree(SOUNDBOARD_KEY);

    tree.setProperty(NAME_KEY, name, nullptr);

    ValueTree samplesTree(SAMPLES_KEY);

    tree.addChild(samplesTree, 0, nullptr);
    for (const auto &sample : samples) {
        samplesTree.addChild(sample.serialize(), 0, nullptr);
    }

    return tree;
}

Soundboard Soundboard::deserialize(ValueTree tree)
{
    Soundboard soundboard(tree.getProperty(NAME_KEY));

    auto samplesTree = tree.getChildWithName(SAMPLES_KEY);
    auto& samples = soundboard.getSamples();
    for( int i = 0; i < samplesTree.getNumChildren(); ++i )
    {
        samples.push_back(SoundSample::deserialize(tree.getChild(i)));
    }

    return soundboard;
}