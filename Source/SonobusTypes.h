// SPDX-License-Identifier: GPLv3-or-later
// Copyright (C) 2020 Jesse Chappell

#pragma once


class SonobusCommands
{
public:
    
    enum {
        MuteAllInput = 1,
        MuteAllPeers,
        TogglePlayPause,
        ToggleLoop,
        TrimSelectionToNewFile,
        CloseFile,
        Connect,
        Disconnect,
        ShareFile,
        RevealFile,
        ShowOptions,
        OpenFile,
        RecordToggle,
        CheckForNewVersion
    };
    
};
