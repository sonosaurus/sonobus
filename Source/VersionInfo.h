// SPDX-License-Identifier: GPLv3-or-later WITH Appstore-exception
// Copyright (C) 2020 Jesse Chappell



#pragma once


//==============================================================================
class VersionInfo
{
public:
    struct Asset
    {
        const String name;
        const String url;
    };

    static std::unique_ptr<VersionInfo> fetchFromUpdateServer (const String& versionString);
    static std::unique_ptr<VersionInfo> fetchLatestFromUpdateServer();
    static std::unique_ptr<InputStream> createInputStreamForAsset (const Asset& asset, int& statusCode);

    bool isNewerVersionThanCurrent();

    const String versionString;
    const String releaseNotes;
    const std::vector<Asset> assets;

private:
    VersionInfo() = default;

    static std::unique_ptr<VersionInfo> fetch (const String&);
};
