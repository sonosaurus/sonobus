// SPDX-License-Identifier: GPLv3-or-later WITH Appstore-exception
// Copyright (C) 2026 Jesse Chappell

#include "SonoBusAooMetadata.h"

#include <nlohmann/json.hpp>
using json = nlohmann::json;

namespace sonobus
{

//
// GroupMetadata
//

static constexpr std::string_view nameKey = "name";
static constexpr std::string_view descriptionKey = "desc";
static constexpr std::string_view isPublicKey = "ispub";
static constexpr std::string_view isManagedKey = "isman";
static constexpr std::string_view managersKey = "mans";


void to_json(json& j, const GroupMetadata & p) {
    j = json{
        {nameKey, p.name},
        {descriptionKey, p.description},
        {isPublicKey, p.isPublic},
        {isManagedKey, p.isManaged},
        {managersKey, p.managers}
    };
}

void from_json(const json& j, GroupMetadata & p) {
    // TODO catch exceptions/ conditionals for optional stuff
    j.at(nameKey).get_to(p.name);
    j.at(descriptionKey).get_to(p.description);
    j.at(isPublicKey).get_to(p.isPublic);
    j.at(isManagedKey).get_to(p.isManaged);
    j.at(managersKey).get_to(p.managers);
}

//
// SinkMetadata
//

static constexpr std::string_view prefSendFormatKey = "psfmt";
static constexpr std::string_view maxBitRateKey = "maxbr";


void to_json(json& j, const SinkMetadata & p) {
    j = json{
        {prefSendFormatKey, p.preferredSendFormatIndex},
        {maxBitRateKey, p.maxBitRate}
    };
}

void from_json(const json& j, SinkMetadata & p) {
    // TODO catch exceptions/ conditionals for optional stuff
    j.at(prefSendFormatKey).get_to(p.preferredSendFormatIndex);
    j.at(maxBitRateKey).get_to(p.maxBitRate);
}

//
// SourceMetadata
//

static constexpr std::string_view sendFormatKey = "sfmt";
static constexpr std::string_view layoutKey = "layout";


void to_json(json& j, const SourceMetadata & p) {
    j = json{
        {sendFormatKey, p.sendFormatIndex},
        {maxBitRateKey, p.maxBitRate},
        {layoutKey, json::binary_t(p.layout)}
    };
}

void from_json(const json& j, SourceMetadata & p) {
    // TODO catch exceptions/ conditionals for optional stuff
    j.at(sendFormatKey).get_to(p.sendFormatIndex);
    j.at(maxBitRateKey).get_to(p.maxBitRate);
}

//
// PublicGroupSubscribeRequestMetadata
//

static constexpr std::string_view subscribeKey = "sub";

void to_json(json& j, const PublicGroupSubscribeRequestMetadata & p) {
    j = json{
        {nameKey, p.name},
        {subscribeKey, p.subscribed},
    };
}

void from_json(const json& j, PublicGroupSubscribeRequestMetadata & p) {
    // TODO catch exceptions/ conditionals for optional stuff
    auto name = j.at(nameKey);

    if (name != p.name) {
        throw(nlohmann::detail::type_error::create(302, nlohmann::detail::concat("request type: ", name,  " does not match: ", p.name), &j));
    }

    j.at(subscribeKey).get_to(p.subscribed);
}

//
// PublicGroupRequestMetadata
//

void to_json(json& j, const PublicGroupRequestMetadata & p) {
    j = json{
        {nameKey, p.name},
    };
}

void from_json(const json& j, PublicGroupRequestMetadata & p) {
    auto name = j.at(nameKey);

    if (name != p.name) {
        throw(nlohmann::detail::type_error::create(302, nlohmann::detail::concat("request type: ", name,  " does not match: ", p.name), &j));
    }

}

//
// PublicGroupUpdateMetadata
//

static constexpr std::string_view groupIdKey = "grpid";
static constexpr std::string_view removedKey = "remvd";
static constexpr std::string_view groupNameKey = "grpnm";
static constexpr std::string_view userListKey = "users";

void to_json(nlohmann::json& j, const PublicGroupUpdateMetadata & p) {
    j = json{
        {nameKey, p.name},
        {groupIdKey, p.groupId},
        {removedKey, p.removed},
        {groupNameKey, p.groupName},
        {userListKey, p.users},
    };
}
void from_json(const nlohmann::json& j, PublicGroupUpdateMetadata & p) {
    auto name = j.at(nameKey);

    if (name != p.name) {
        throw(nlohmann::detail::type_error::create(302, nlohmann::detail::concat("request type: ", name,  " does not match: ", p.name), &j));
    }

    j.at(groupIdKey).get_to(p.groupId);
    j.at(groupNameKey).get_to(p.groupName);
    j.at(removedKey).get_to(p.removed);
    j.at(userListKey).get_to(p.users);
}




}
