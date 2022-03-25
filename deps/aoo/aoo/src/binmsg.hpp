#pragma once

#include "aoo/aoo.h"
#include "common/net_utils.hpp"
#include "common/utils.hpp"

#include <cassert>

namespace aoo {

inline int binmsg_write_header(AooByte *buffer, AooSize size,
                               AooMsgType type, AooInt32 cmd) {
    if (size >= 4) {
        buffer[0] = (AooByte)type | kAooBinMsgDomainBit;
        buffer[1] = (AooByte)cmd;
        buffer[2] = 0;
        buffer[3] = 0;
        return 4;
    } else {
        return 0;
    }
}

inline int binmsg_write_header(AooByte *buffer, AooSize size,
                               AooMsgType type, AooInt32 cmd,
                               AooId to, AooId from) {
    if (to >= 256 || from >= 256) {
        // large IDs
        if (size < 12) {
            return 0;
        }
        buffer[0] = (AooByte)type | kAooBinMsgDomainBit;
        buffer[1] = (AooByte)cmd | kAooBinMsgSizeBit; // !
        buffer[2] = 0;
        buffer[3] = 0;
        aoo::to_bytes<AooId>(to, buffer + 4);
        aoo::to_bytes<AooId>(from, buffer + 8);
        return 12;
    } else {
        // short IDs
        if (size < 4) {
            return 0;
        }
        buffer[0] = (AooByte)type | kAooBinMsgDomainBit;
        buffer[1] = (AooByte)cmd;
        buffer[2] = (AooByte)to;
        buffer[3] = (AooByte)from;
        return 4;
    }
}

inline bool binmsg_check(const AooByte *data, AooSize size) {
    return size >= 4 && (data[0] & kAooBinMsgDomainBit);
}

inline int binmsg_headersize(const AooByte *data, AooSize size) {
    assert(data[0] & kAooBinMsgDomainBit);
    assert(size >= 4);
    bool large = data[1] & kAooBinMsgSizeBit;
    auto n = large ? 12 : 4;
    if (size >= n) {
        return n;
    } else {
        return 0;
    }
}

inline AooMsgType binmsg_type(const AooByte *data, AooSize size) {
    assert(data[0] & kAooBinMsgDomainBit);
    assert(size >= 4);
    return data[0] & ~kAooBinMsgDomainBit;
}

inline AooByte binmsg_cmd(const AooByte *data, AooSize size) {
    assert(data[0] & kAooBinMsgDomainBit);
    assert(size >= 4);
    return data[1] & ~kAooBinMsgSizeBit;
}

inline AooId binmsg_to(const AooByte *data, AooSize size) {
    assert(data[0] & kAooBinMsgDomainBit);
    assert(size >= 4);
    bool large = data[1] & kAooBinMsgSizeBit;
    if (large) {
        if (size >= 12) {
            return aoo::from_bytes<AooId>(data + 4);
        }
    } else {
        return data[2];
    }
    return kAooIdInvalid;
}

inline AooId binmsg_from(const AooByte *data, AooSize size) {
    assert(data[0] & kAooBinMsgDomainBit);
    assert(size >= 4);
    bool large = data[1] & kAooBinMsgSizeBit;
    if (large) {
        assert(size >= 12);
        return aoo::from_bytes<AooId>(data + 8);
    } else {
        return (AooId)data[3];
    }
}

inline AooId binmsg_group(const AooByte *data, AooSize size) {
    return binmsg_to(data, size);
}

inline AooId binmsg_user(const AooByte *data, AooSize size) {
    return binmsg_from(data, size);
}

// type + domain bit (uint8), cmd [IPv6|IPv4] (uint8), port (uint16),
// a) IPv4 address (4 bytes)
// b) IPv6 address (16 bytes)
inline int binmsg_write_relay(AooByte *buffer, AooSize size,
                              const aoo::ip_address& addr) {
    if (addr.type() == ip_address::IPv6) {
        if (size >= 20) {
            buffer[0] = kAooTypeRelay | kAooBinMsgDomainBit;
            buffer[1] = kAooBinMsgCmdRelayIPv6;
            aoo::to_bytes<uint16_t>(addr.port(), buffer + 2);
            memcpy(buffer + 4, addr.address_bytes(), 16);
            return 20;
        }
    } else {
        if (size >= 8) {
            buffer[0] = kAooTypeRelay | kAooBinMsgDomainBit;
            buffer[1] = kAooBinMsgCmdRelayIPv4;
            aoo::to_bytes<uint16_t>(addr.port(), buffer + 2);
            memcpy(buffer + 4, addr.address_bytes(), 4);
            return 8;
        }
    }
    return 0;
}

inline int binmsg_read_relay(const AooByte *buffer, AooSize size,
                             aoo::ip_address& addr) {
    assert(size >= 4);
    assert(buffer[0] & kAooBinMsgDomainBit);
    assert((buffer[0] & ~kAooBinMsgDomainBit) == kAooTypeRelay);

    if (buffer[1] == kAooBinMsgCmdRelayIPv6) {
        // IPv6
        if (size >= 20) {
            auto port = aoo::from_bytes<uint16_t>(buffer + 2);
            addr = ip_address(buffer + 4, 16, port, ip_address::IPv6);
            return 20;
        }
    } else {
        // IPv4
        if (size >= 8) {
            auto port = aoo::from_bytes<uint16_t>(buffer + 2);
            addr = ip_address(buffer + 4, 4, port, ip_address::IPv4);
            return 8;
        }
    }
    return 0;
}

} // aoo
