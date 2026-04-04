//
// Created by Hannah Schellekens on 2021-10-26.
//


#include "Soundboard.h"
#include <utility>
#include "CrossPlatformUtils.h"

SoundSample::SoundSample(
        String newName, URL newFileURL, EndPlaybackBehaviour endBehavior, uint32 buttonColour, int hotkeyCode,
        PlaybackBehaviour playbackBehaviour,  ButtonBehaviour buttonBehaviour, ReplayBehaviour replayBehaviour,
        float newGain
) : name(std::move(newName)),
        fileURL(std::move(newFileURL)),
        endPlaybackBehaviour(endBehavior),
        buttonColour(buttonColour),
        hotkeyCode(hotkeyCode),
        playbackBehaviour(playbackBehaviour),
        buttonBehaviour(buttonBehaviour),
        replayBehaviour(replayBehaviour),
        lastPlaybackPosition(0.0),
        gain(newGain)
{}

String SoundSample::getName() const
{
    return this->name;
}

void SoundSample::setName(String newName)
{
    this->name = std::move(newName);
}

juce::URL SoundSample::getFileURL() const
{
    return fileURL;
}

void SoundSample::setFileURL(juce::URL newFileUrl)
{
    fileURL = std::move(newFileUrl);
}


void SoundSample::setEndPlaybackBehaviour(EndPlaybackBehaviour newEndBehavior)
{
    endPlaybackBehaviour = newEndBehavior;
}

int SoundSample::getButtonColour() const
{
    return buttonColour;
}

void SoundSample::setButtonColour(int newRgb)
{
    buttonColour = newRgb;
}

int SoundSample::getHotkeyCode() const
{
    return hotkeyCode;
}

void SoundSample::setHotkeyCode(int newKeyCode)
{
    hotkeyCode = newKeyCode;
}

SoundSample::PlaybackBehaviour SoundSample::getPlaybackBehaviour() const
{
    return playbackBehaviour;
}

void SoundSample::setPlaybackBehaviour(PlaybackBehaviour newBehaviour)
{
    playbackBehaviour = newBehaviour;
}

SoundSample::ButtonBehaviour SoundSample::getButtonBehaviour() const
{
    return buttonBehaviour;
}

void SoundSample::setButtonBehaviour(ButtonBehaviour newBehaviour)
{
    buttonBehaviour = newBehaviour;
}

SoundSample::ReplayBehaviour SoundSample::getReplayBehaviour() const
{
    return replayBehaviour;
}

void SoundSample::setReplayBehaviour(ReplayBehaviour newBehaviour)
{
    replayBehaviour = newBehaviour;
}

double SoundSample::getLastPlaybackPosition() const
{
    return lastPlaybackPosition;
}

void SoundSample::setLastPlaybackPosition(double position)
{
    lastPlaybackPosition = position;
}

ValueTree SoundSample::serialize()
{
    ValueTree tree(SAMPLE_KEY);
    tree.setProperty(NAME_KEY, name, nullptr);
    
    if (fileURL.isLocalFile()) {
        tree.setProperty(FILE_PATH_KEY, fileURL.getLocalFile().getFullPathName(), nullptr); // old style, just in case
    }
    
    tree.setProperty(FILE_URL_KEY, fileURL.toString(true), nullptr);

#if JUCE_IOS
    // store bookmark data if necessary
    if (void * bookmark = getURLBookmark(fileURL)) {
        const void * data = nullptr;
        size_t datasize = 0;
        if (urlBookmarkToBinaryData(bookmark, data, datasize)) {
            DBG("Audio file has bookmark, storing it in state, size: " << datasize);
            tree.setProperty(FILE_URL_BOOKMARK_KEY, var(data, datasize), nullptr);
        } else {
            DBG("Bookmark is not valid!");
        }
    }
#endif
    
    tree.setProperty(END_PLAYBACK_BEHAVIOUR_KEY, endPlaybackBehaviour, nullptr);
    tree.setProperty(LOOP_KEY, endPlaybackBehaviour == LOOP_AT_END, nullptr); // backwards compat
    tree.setProperty(BUTTON_COLOUR_KEY, (int64)buttonColour, nullptr);
    tree.setProperty(HOTKEY_KEY, hotkeyCode, nullptr);
    tree.setProperty(PLAYBACK_BEHAVIOUR_KEY, playbackBehaviour, nullptr);
    tree.setProperty(BUTTON_BEHAVIOUR_KEY, buttonBehaviour, nullptr);
    tree.setProperty(REPLAY_BEHAVIOUR_KEY, replayBehaviour, nullptr);
    tree.setProperty(GAIN_KEY, gain, nullptr);

    return tree;
}

SoundSample SoundSample::deserialize(const ValueTree tree)
{
    int playbackBehaviour = tree.getProperty(PLAYBACK_BEHAVIOUR_KEY, PlaybackBehaviour::SIMULTANEOUS);
    int buttonBehaviour = tree.getProperty(BUTTON_BEHAVIOUR_KEY, ButtonBehaviour::TOGGLE);
    int replayBehaviour = tree.getProperty(REPLAY_BEHAVIOUR_KEY, ReplayBehaviour::REPLAY_FROM_START);
    
    // backwards compat
    bool oldloop = tree.getProperty(LOOP_KEY, false);
    int endPlaybackBehaviour = tree.getProperty(END_PLAYBACK_BEHAVIOUR_KEY, oldloop ? EndPlaybackBehaviour::LOOP_AT_END : EndPlaybackBehaviour::STOP_AT_END);

    URL fileurl;
    String fileurlstr = tree.getProperty(FILE_URL_KEY, "");
    if (fileurlstr.isEmpty()) {
        // old style
        String filepathstr = tree.getProperty(FILE_PATH_KEY, "");
        fileurl = URL(File(filepathstr));
    } else {
        fileurl = URL(fileurlstr);
    }
    
#if JUCE_IOS
    // check for bookmark
    auto bptr = tree.getPropertyPointer(FILE_URL_BOOKMARK_KEY);
    if (bptr) {
        if (auto * block = bptr->getBinaryData()) {
            DBG("Has file bookmark");
            void * bookmark = binaryDataToUrlBookmark(block->getData(), block->getSize());
            setURLBookmark(fileurl, bookmark);
            fileurl = generateUpdatedURL(fileurl);
        }
    }
#endif

    
    SoundSample soundSample(
        tree.getProperty(NAME_KEY),
        fileurl,
        static_cast<EndPlaybackBehaviour>(endPlaybackBehaviour),
        (uint32)(int64)tree.getProperty(BUTTON_COLOUR_KEY, (int64)SoundboardButtonColors::DEFAULT_BUTTON_COLOUR),
        tree.getProperty(HOTKEY_KEY, -1),
        static_cast<PlaybackBehaviour>(playbackBehaviour),
        static_cast<ButtonBehaviour>(buttonBehaviour),
        static_cast<ReplayBehaviour>(replayBehaviour),
        tree.getProperty(GAIN_KEY, 1.0)
    );

    return soundSample;
}

Soundboard::Soundboard(String newName)
        : name(std::move(newName)), samples(std::vector<SoundSample>())
{}

String Soundboard::getName() const
{
    return this->name;
}

void Soundboard::setName(String newName)
{
    this->name = std::move(newName);
}

std::vector<SoundSample>& Soundboard::getSamples()
{
    return this->samples;
}

ValueTree Soundboard::serialize()
{
    ValueTree tree(SOUNDBOARD_KEY);

    tree.setProperty(NAME_KEY, name, nullptr);

    ValueTree samplesTree(SAMPLES_KEY);

    tree.addChild(samplesTree, 0, nullptr);

    int i = 0;
    for (auto &sample : samples) {
        samplesTree.addChild(sample.serialize(), i++, nullptr);
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
        samples.emplace_back(SoundSample::deserialize(samplesTree.getChild(i)));
    }

    return soundboard;
}
