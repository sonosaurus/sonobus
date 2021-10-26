//
// Created by Hannah Schellekens on 2021-10-26.
//


#include "Soundboard.h"
#include <utility>

SoundSample::SoundSample(std::string newName, std::string newFilePath)
        : name(std::move(newName)), filePath(std::move(newFilePath))
{}

std::string SoundSample::getName()
{
    return this->name;
}

void SoundSample::setName(std::string newName)
{
    this->name = std::move(newName);
}

std::string SoundSample::getFilePath()
{
    return this->filePath;
}

void SoundSample::setFilePath(std::string newFilePath)
{
    this->filePath = std::move(newFilePath);
}

Soundboard::Soundboard(std::string newName)
        : name(std::move(newName)), samples(std::vector<SoundSample>())
{}

std::string Soundboard::getName()
{
    return this->name;
}

void Soundboard::setName(std::string newName)
{
    this->name = std::move(newName);
}

std::vector<SoundSample> &Soundboard::getSamples()
{
    return this->samples;
}