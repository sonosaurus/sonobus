// SPDX-License-Identifier: GPLv3-or-later WITH Appstore-exception
// Copyright (C) 2020 Jesse Chappell



#include "SoundboardProcessor.h"
#include <iostream>

void SoundboardProcessor::addSoundboard(const String& name)
{
    auto newSoundboard = Soundboard(name);
    soundboards.push_back(std::move(newSoundboard));
}

void SoundboardProcessor::renameSoundboard(Soundboard& toRename, String newName)
{
    // TODO: Actually prompt the user.
    std::cout << "Renaming current soundboard... (not implemented)" << std::endl;
}

void SoundboardProcessor::deleteSoundboard(Soundboard& toRemove)
{
    // TODO: Actually prompt the user.
    std::cout << "Deleting soundboard... (not implemented)" << std::endl;
}

void SoundboardProcessor::writeSoundboardsToFile(const File& file) const
{
    ValueTree tree(SOUNDBOARDS_KEY);

    for (const auto &soundboard : soundboards) {
        tree.addChild(soundboard.serialize(), 0, nullptr);
    }

    tree.createXml()->writeTo(file);
}

void SoundboardProcessor::readSoundboardsFromFile(const File& file)
{
    XmlDocument doc(file);
    auto tree = ValueTree::fromXml(*doc.getDocumentElement());

    soundboards.clear();

    for (const auto &child : tree) {
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


