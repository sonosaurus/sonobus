#pragma once

#include "../detail.hpp"

// OSC address patterns

#define kAooMsgGroupJoin \
    kAooMsgGroup kAooMsgJoin

#define kAooMsgGroupLeave \
    kAooMsgGroup kAooMsgLeave

#define kAooMsgGroupUpdate \
    kAooMsgGroup kAooMsgUpdate

#define kAooMsgGroupChanged \
    kAooMsgGroup kAooMsgChanged

#define kAooMsgUserUpdate \
    kAooMsgUser kAooMsgUpdate

#define kAooMsgUserChanged \
    kAooMsgUser kAooMsgChanged

#define kAooMsgPeerJoin \
    kAooMsgPeer kAooMsgJoin

#define kAooMsgPeerLeave \
    kAooMsgPeer kAooMsgLeave

#define kAooMsgPeerChanged \
    kAooMsgPeer kAooMsgChanged

// peer messages

#define kAooMsgPeerPing \
    kAooMsgDomain kAooMsgPeer kAooMsgPing

#define kAooMsgPeerPong \
    kAooMsgDomain kAooMsgPeer kAooMsgPong

#define kAooMsgPeerMessage \
    kAooMsgDomain kAooMsgPeer kAooMsgMessage

#define kAooMsgPeerAck \
    kAooMsgDomain kAooMsgPeer kAooMsgAck

// client messages

#define kAooMsgClientPong \
    kAooMsgDomain kAooMsgClient kAooMsgPong

#define kAooMsgClientLogin \
    kAooMsgDomain kAooMsgClient kAooMsgLogin

#define kAooMsgClientQuery \
    kAooMsgDomain kAooMsgClient kAooMsgQuery

#define kAooMsgClientGroupJoin \
    kAooMsgDomain kAooMsgClient kAooMsgGroupJoin

#define kAooMsgClientGroupLeave \
    kAooMsgDomain kAooMsgClient kAooMsgGroupLeave

#define kAooMsgClientGroupUpdate \
    kAooMsgDomain kAooMsgClient kAooMsgGroupUpdate

#define kAooMsgClientUserUpdate \
    kAooMsgDomain kAooMsgClient kAooMsgUserUpdate

#define kAooMsgClientGroupChanged \
    kAooMsgDomain kAooMsgClient kAooMsgGroupChanged

#define kAooMsgClientUserChanged \
    kAooMsgDomain kAooMsgClient kAooMsgUserChanged

#define kAooMsgClientPeerChanged \
    kAooMsgDomain kAooMsgClient kAooMsgPeerChanged

#define kAooMsgClientRequest \
    kAooMsgDomain kAooMsgClient kAooMsgRequest

#define kAooMsgClientPeerJoin \
    kAooMsgDomain kAooMsgClient kAooMsgPeerJoin

#define kAooMsgClientPeerLeave \
    kAooMsgDomain kAooMsgClient kAooMsgPeerLeave

#define kAooMsgClientMessage \
    kAooMsgDomain kAooMsgClient kAooMsgMessage

// server messages

#define kAooMsgServerLogin \
    kAooMsgDomain kAooMsgServer kAooMsgLogin

#define kAooMsgServerQuery \
    kAooMsgDomain kAooMsgServer kAooMsgQuery

#define kAooMsgServerPing \
    kAooMsgDomain kAooMsgServer kAooMsgPing

#define kAooMsgServerGroupJoin \
    kAooMsgDomain kAooMsgServer kAooMsgGroupJoin

#define kAooMsgServerGroupLeave \
    kAooMsgDomain kAooMsgServer kAooMsgGroupLeave

#define kAooMsgServerGroupUpdate \
    kAooMsgDomain kAooMsgServer kAooMsgGroupUpdate

#define kAooMsgServerUserUpdate \
    kAooMsgDomain kAooMsgServer kAooMsgUserUpdate

#define kAooMsgServerRequest \
    kAooMsgDomain kAooMsgServer kAooMsgRequest

namespace aoo {
namespace net {

AooError parse_pattern(const AooByte *msg, int32_t n,
                       AooMsgType& type, int32_t& offset);

AooSize write_relay_message(AooByte *buffer, AooSize bufsize,
                            const AooByte *msg, AooSize msgsize,
                            const ip_address& addr);

std::string encrypt(const std::string& input);

struct ip_host {
    ip_host() = default;
    ip_host(const std::string& _name, int _port)
        : name(_name), port(_port) {}
    ip_host(const AooIpEndpoint& ep)
        : name(ep.hostName), port(ep.port) {}

    std::string name;
    int port = 0;

    bool valid() const {
        return !name.empty() && port > 0;
    }

    bool operator==(const ip_host& other) const {
        return name == other.name && port == other.port;
    }

    bool operator!=(const ip_host& other) const {
        return !operator==(other);
    }
};

#if 0
using ip_address_list = std::vector<ip_address, aoo::allocator<ip_address>>;
#else
using ip_address_list = std::vector<ip_address>;
#endif

osc::OutboundPacketStream& operator<<(osc::OutboundPacketStream& msg, const ip_host& addr);

ip_host osc_read_host(osc::ReceivedMessageArgumentIterator& it);

} // namespace net
} // namespace aoo
