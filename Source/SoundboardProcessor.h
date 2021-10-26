//
// Created by Hannah Schellekens on 2021-10-26.
//


#pragma once

#include "Soundboard.h"
#include <vector>

/**
 * Controller for SoundboardView.
 *
 * @author Hannah Schellekens
 */
class SoundboardProcessor
{
public:
    /**
     * Gets called whenever the process of adding a soundboard must start.
     */
    void onAddSoundboard();

    /**
     * Gets called whenever the process of renaming a soundboard must start.
     */
    void onRenameSoundboard();

    /**
     * Gets called whenever the process of deleting a soundboard must start.
     */
    void onDeleteSoundboard();

private:
    /**
     * List of all available/known soundboards.
     */
    const std::vector<Soundboard> soundboards;
};