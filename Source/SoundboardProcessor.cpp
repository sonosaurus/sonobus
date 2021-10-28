// SPDX-License-Identifier: GPLv3-or-later WITH Appstore-exception
// Copyright (C) 2020 Jesse Chappell



#include "SoundboardProcessor.h"
#include <utility>

SoundboardProcessor::SoundboardProcessor()
{
    loadFromDisk();
}

Soundboard &SoundboardProcessor::addSoundboard(const String &name)
{
    auto newSoundboard = Soundboard(name);
    soundboards.push_back(std::move(newSoundboard));

    saveToDisk();

    return soundboards[getNumberOfSoundboards() - 1];
}

void SoundboardProcessor::renameSoundboard(int index, String newName)
{
    auto& toRename = soundboards[index];
    toRename.setName(std::move(newName));

    saveToDisk();
}

void SoundboardProcessor::deleteSoundboard(int index)
{
    soundboards.erase(soundboards.begin() + index);

    saveToDisk();
}

int SoundboardProcessor::getIndexOfSoundboard(const Soundboard &soundboard) const
{
    auto numberOfSoundboards = getNumberOfSoundboards();
    for (int i = 0; i < numberOfSoundboards; ++i) {
        auto soundboardInList = &getSoundboard(i);
        if (&soundboard == soundboardInList) {
            return i;
        }
    }
    return -1;
}

void SoundboardProcessor::writeSoundboardsToFile(const File &file) const
{
    ValueTree tree(SOUNDBOARDS_KEY);

    int i = 0;
    for (const auto &soundboard: soundboards) {
        tree.addChild(soundboard.serialize(), i++, nullptr);
    }

    // Make sure  the parent directory exists
    file.getParentDirectory().createDirectory();

    tree.createXml()->writeTo(file);
}

void SoundboardProcessor::readSoundboardsFromFile(const File &file)
{
    if (!file.existsAsFile()) {
        return;
    }

    XmlDocument doc(file);
    auto tree = ValueTree::fromXml(*doc.getDocumentElement());

    soundboards.clear();

    for (const auto &child: tree) {
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
}


