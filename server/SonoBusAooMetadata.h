// SPDX-License-Identifier: GPLv3-or-later WITH Appstore-exception
// Copyright (C) 2026 Jesse Chappell

#pragma once

#include <string>
#include <vector>

#include <aoo_types.h>

#include <nlohmann/json.hpp>

namespace sonobus
{

struct ScopedAooData
{
    ScopedAooData() : data({kAooDataUnspecified, nullptr, 0 }) {}

    const AooData & get() {
        return data;
    }

    void set (AooDataType type, std::vector<std::uint8_t> buf)
    {
        tmpdata = buf;
        data.type = type;
        data.data = tmpdata.data();
        data.size = tmpdata.size();
    }

private:
    std::vector<std::uint8_t> tmpdata;
    AooData data;

};

// methods to serialize from/to AooData
template < typename ValueType >
bool fromAooData(const AooData & data, ValueType & value)
{
    // deserialize it with MessagePack
    nlohmann::json j = nlohmann::json::from_msgpack(data.data, data.data + data.size, true, false);

    if (j.is_discarded()) {
        return false;
    }

    try {
        j.get_to(value);
    }
    catch (const nlohmann::json::exception& e) {
        return false;
    }

    return true;
}

template < typename ValueType >
bool toAooData(ScopedAooData & outdata, const ValueType & value)
{
    try {
        nlohmann::json j = value;

        // messagepack type not standard yet, use kAooDataBinary for now

        outdata.set(kAooDataBinary, nlohmann::json::to_msgpack(j));
    }
    catch (const nlohmann::json::exception& e) {
        return false;
    }

    return true;
}

// Metadata structs

struct GroupMetadata
{
    std::string name;
    std::string description;

    bool isPublic = false;

    bool isManaged = false;
    std::vector<std::string> managers;
};
void to_json(nlohmann::json& j, const GroupMetadata & p);
void from_json(const nlohmann::json& j, GroupMetadata & p);


struct SinkMetadata
{
    int preferredSendFormatIndex = -1; // -1 is default
    int maxBitRate = 0; // zero is no max
};
void to_json(nlohmann::json& j, const SinkMetadata & p);
void from_json(const nlohmann::json& j, SinkMetadata & p);


struct SourceMetadata
{
    int sendFormatIndex = -1; // -1 is default
    int maxBitRate = 0; // zero is no max
    std::vector<uint8_t> layout;
};
void to_json(nlohmann::json& j, const SourceMetadata & p);
void from_json(const nlohmann::json& j, SourceMetadata & p);



// request metadata

struct BaseRequestMetadata {
    BaseRequestMetadata(const std::string & n = "") : name(n) {}
    std::string name;
};


struct PublicGroupSubscribeRequestMetadata : public BaseRequestMetadata
{
    PublicGroupSubscribeRequestMetadata(bool sub = false) : BaseRequestMetadata("pubgrpsub"), subscribed(sub) {}

    bool subscribed = false;
};
void to_json(nlohmann::json& j, const PublicGroupSubscribeRequestMetadata & p);
void from_json(const nlohmann::json& j, PublicGroupSubscribeRequestMetadata & p);

struct PublicGroupRequestMetadata : public BaseRequestMetadata
{
    PublicGroupRequestMetadata() : BaseRequestMetadata("pubgrpreq") {}

};
void to_json(nlohmann::json& j, const PublicGroupRequestMetadata & p);
void from_json(const nlohmann::json& j, PublicGroupRequestMetadata & p);

struct PublicGroupUpdateMetadata : public BaseRequestMetadata
{
    PublicGroupUpdateMetadata() : BaseRequestMetadata("pubgrpup") {}
    AooId groupId = kAooIdInvalid;
    bool removed = false;
    std::string groupName;
    std::vector<std::string> users;
};
void to_json(nlohmann::json& j, const PublicGroupUpdateMetadata & p);
void from_json(const nlohmann::json& j, PublicGroupUpdateMetadata & p);



}
