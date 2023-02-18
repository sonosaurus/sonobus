#include "peer.hpp"
#include "client.hpp"
#include "client_events.hpp"

#include "../binmsg.hpp"

#include <algorithm>
#include <sstream>

// debugging

#define FORCE_RELAY 0

namespace aoo {
namespace net {

// OSC peer message:
const int32_t kMessageMaxAddrSize = kAooMsgDomainLen + kAooMsgPeerLen + 16 + kAooMsgDataLen;
// address pattern string: max 16 bytes
// typetag string: max. 12 bytes
// args (including type + blob size): max. 44 bytes
const int32_t kMessageHeaderSize = kMessageMaxAddrSize + 56;

// binary peer message:
// args: 28 bytes (max.)
const int32_t kBinMessageHeaderSize = kAooBinMsgLargeHeaderSize + 28;

//------------------------- peer ------------------------------//

peer::peer(const std::string& groupname, AooId groupid,
           const std::string& username, AooId userid, AooId localid,
           ip_address_list&& addrlist, const AooData *metadata,
           ip_address_list&& user_relay, const ip_address_list& group_relay)
    : group_name_(groupname), user_name_(username), group_id_(groupid), user_id_(userid),
      local_id_(localid), metadata_(metadata), addrlist_(std::move(addrlist)),
      user_relay_(std::move(user_relay)), group_relay_(group_relay)
{
    start_time_ = time_tag::now();

    LOG_DEBUG("AooClient: create peer " << *this);
}

peer::~peer(){
    LOG_DEBUG("AooClient: destroy peer " << *this);
}

bool peer::match(const ip_address& addr) const {
    if (connected()){
        return real_address_ == addr;
    } else {
        return false;
    }
}

bool peer::match(const std::string& group) const {
    return group_name_ == group; // immutable!
}

bool peer::match(const std::string& group, const std::string& user) const {
    return group_name_ == group && user_name_ == user; // immutable!
}

bool peer::match(AooId group) const {
    return group_id_ == group; // immutable!
}

bool peer::match(AooId group, AooId user) const {
    return group_id_ == group && user_id_ == user;
}

bool peer::match_wildcard(AooId group, AooId user) const {
    return (group_id_ == group || group == kAooIdInvalid) &&
            (user_id_ == user || user == kAooIdInvalid);
}

void peer::send(Client& client, const sendfn& fn, time_tag now) {
    if (connected()) {
        do_send(client, fn, now);
    } else if (!timeout_) {
        // try to establish UDP connection with peer
        auto elapsed_time = time_tag::duration(start_time_, now);
        auto delta = elapsed_time - last_pingtime_;
        if (elapsed_time > client.query_timeout()){
            // time out -> try to relay
            if (!group_relay_.empty() && !relay()) {
                start_time_ = now;
                last_pingtime_ = 0;
                // for now we just try with the first relay address.
                // LATER try all of them.
                relay_address_ = group_relay_.front();
                relay_ = true;
                LOG_WARNING("AooClient: UDP handshake with " << *this
                            << " timed out, try to relay over " << relay_address_);
                return;
            }

            // couldn't establish connection!
            const char *what = relay() ? "relay" : "peer-to-peer";
            LOG_ERROR("AooClient: couldn't establish UDP " << what
                      << " connection to " << *this << "; timed out after "
                      << client.query_timeout() << " seconds");

            std::stringstream ss;
            ss << "couldn't establish connection with peer " << *this;

            // TODO: do we really need to send the error event?
            auto e1 = std::make_unique<error_event>(0, ss.str());
            client.send_event(std::move(e1));

            auto e2 = std::make_unique<peer_event>(
                        kAooEventPeerTimeout, *this);
            client.send_event(std::move(e2));

            timeout_ = true;

            return;
        }
        // send handshakes in fast succession to all addresses
        // until we get a reply from one of them (see handle_message())
        if (delta >= client.query_interval()){
            char buf[64];
            osc::OutboundPacketStream msg(buf, sizeof(buf));
            // /aoo/peer/ping <group> <user> <tt>
            // NB: we send *our* user ID, see above. The empty timetag
            // distinguishes a handshake ping from a regular ping!
            //
            // With the group and user ID, peers can identify us even if
            // we're behind a symmetric NAT. This trick doesn't work
            // if both parties are behind a symmetrict NAT; in that case,
            // UDP hole punching simply doesn't work.
            msg << osc::BeginMessage(kAooMsgPeerPing)
                << group_id_ << local_id_ << osc::TimeTag(0)
                << osc::EndMessage;

            for (auto& addr : addrlist_) {
                send(msg, addr, fn);
            }

            last_pingtime_ = elapsed_time;

            LOG_DEBUG("AooClient: send handshake ping to " << *this);
        }
    }
}

void peer::do_send(Client& client, const sendfn& fn, time_tag now) {
    auto elapsed_time = time_tag::duration(start_time_, now);
    auto delta = elapsed_time - last_pingtime_;
    // 1) send regular ping
    if (delta >= client.ping_interval()) {
        // /aoo/peer/ping
        // NB: we send *our* user ID, so that the receiver can easily match the message.
        // We do not have to specifiy the peer's user ID because the group Id is already
        // sufficient. (There can only be one user per client in a group.)
        char buf[64];
        osc::OutboundPacketStream msg(buf, sizeof(buf));
        msg << osc::BeginMessage(kAooMsgPeerPing)
            << group_id_ << local_id_ << osc::TimeTag(now)
            << osc::EndMessage;

        send(msg, fn);

        last_pingtime_ = elapsed_time;

        LOG_DEBUG("AooClient: send ping to " << *this << " (" << now << ")");
    }
    // 2) reply to /ping message
    // NB: we send *our* user ID, see above.
    if (got_ping_.exchange(false, std::memory_order_acquire)) {
        // The empty timetag distinguishes a handshake
        // pong from a regular pong.
        auto tt1 = ping_tt1_;
        if (!tt1.is_empty()) {
            // regular pong
            char buf[64];
            osc::OutboundPacketStream msg(buf, sizeof(buf));
            msg << osc::BeginMessage(kAooMsgPeerPong)
                << group_id_ << local_id_
                << osc::TimeTag(tt1) << osc::TimeTag(now)
                << osc::EndMessage;

            send(msg, fn);

            LOG_DEBUG("AooClient: send ping reply to " << *this
                      << " (" << time_tag(tt1) << " " << now << ")");
        } else {
            // handshake pong
            char buf[64];
            osc::OutboundPacketStream msg(buf, sizeof(buf));
            msg << osc::BeginMessage(kAooMsgPeerPong)
                << group_id_ << local_id_
                << osc::TimeTag(0) << osc::TimeTag(0)
                << osc::EndMessage;

            send(msg, fn);

            LOG_DEBUG("AooClient: send handshake ping reply to " << *this);
        }
    }
    // 3) send outgoing acks
    // LATER send them in batches!
    message_ack ack;
    while (send_acks_.try_pop(ack)) {
        send_ack(ack, fn);
    }
    // 4) handle incoming acks
    while (received_acks_.try_pop(ack)) {
        if (auto msg = send_buffer_.find(ack.seq)) {
            if (ack.frame >= 0) {
                msg->ack_frame(ack.frame);
            } else {
                // negative -> all frames
                msg->ack_all();
            }
        } else {
        #if AOO_DEBUG_CLIENT_MESSAGE
            LOG_DEBUG("AooClient: got outdated ack (seq: " << ack.seq
                      << ", frame: " << ack.frame << ") from " << *this);
        #endif
        }
    }
    // 5) pop acknowledged messages
    while (!send_buffer_.empty()) {
        auto& msg = send_buffer_.front();
        if (msg.complete()) {
        #if AOO_DEBUG_CLIENT_MESSAGE
            LOG_DEBUG("AooClient: pop acknowledged message (seq: "
                      << msg.sequence_ << ") from " << *this);
        #endif
            send_buffer_.pop();
        } else {
            break;
        }
    }
    // 6) resend messages
    if (!send_buffer_.empty()) {
        bool binary = client.binary();
        for (auto& msg : send_buffer_) {
            if (msg.need_resend(elapsed_time)) {
                message_packet p;
                p.type = msg.data_.type();
                p.data = nullptr;
                p.size = 0;
                p.tt = msg.tt_;
                p.sequence = msg.sequence_;
                p.totalsize = msg.data_.size();
                p.nframes = msg.nframes_;
                p.frame = 0;
                p.reliable = true;
                for (int i = 0; i < p.nframes; ++i) {
                    if (!msg.has_frame(i)) {
                        p.frame = i;
                        msg.get_frame(i, p.data, p.size);
                    #if AOO_DEBUG_CLIENT_MESSAGE
                        LOG_DEBUG("AooClient: resend message (seq: " << msg.sequence_
                                  << ", frame: " << i << ") to " << *this);
                    #endif
                        if (binary) {
                            send_packet_bin(p, fn);
                        } else {
                            send_packet_osc(p, fn);
                        }
                    }
                }
            }
        }
    }
}

// OSC:
// /aoo/peer/msg <group> <user> <flags> <seq> <total> <nframes> <frame> <tt> <type> <data>
//
// binary:
// header (group + user), flags (int16), size (int16), seq,
// [total (int32), nframes (int16), frame (int16)], [tt (uint64)], [type (str)], data (bin)
//
// 'total', 'nframes' and 'frame' are omitted for single-frame messages.
// 'tt' and 'type' are only sent with the first frame. 'tt' might be ommited if zero.

// if 'flags' contains kAooMessageReliable, the other end sends an ack messages:
//
// OSC:
// /aoo/peer/ack <count> <seq1> <frame1> <seq2> <frame2> etc. // frame < 0 -> all
//
// binary:
// type (int8), cmd (int8), count (int16), <seq1> <frame1> <seq2> <frame2> etc.
//
// LATER: seq1 (int32), offset1 (int16), bitset1 (int16), etc. // offset < 0 -> all

// prevent excessive resending in low-latency networks
#define AOO_CLIENT_MIN_RESEND_TIME 0.02

void peer::send_message(const message& m, const sendfn& fn, bool binary) {
    // LATER make packet size settable at runtime, see AooSource::send_data()
    const int32_t maxsize = AOO_PACKET_SIZE - (binary ? kBinMessageHeaderSize : kMessageHeaderSize);
    auto d = std::div((int32_t)m.data_.size(), maxsize);

    message_packet p;
    p.type = m.data_.type();
    p.data = nullptr;
    p.size = 0;
    p.tt = m.tt_;
    // p.sequence =
    p.totalsize = m.data_.size();
    p.nframes = d.quot + (d.rem != 0);
    p.frame = 0;
    p.reliable = m.reliable_;

    if (p.reliable) {
        p.sequence = next_sequence_reliable_++;
        auto framesize = d.quot ? maxsize : d.rem;
        // wait twice the average RTT before resending
        auto rtt = average_rtt_.load(std::memory_order_relaxed);
        auto interval = std::max<float>(rtt * 2, AOO_CLIENT_MIN_RESEND_TIME);
        sent_message sm(m.data_, m.tt_, p.sequence, p.nframes, framesize, interval);
        send_buffer_.push(std::move(sm));
    } else {
        // NB: use different sequences for reliable and unreliable messages!
        p.sequence = next_sequence_unreliable_++;
    }

    if (p.nframes > 1) {
        // multi-frame message
        for (int32_t i = 0; i < p.nframes; ++i) {
            p.frame = i;
            if (i == (p.nframes - 1)) {
                p.size = d.rem;
                p.data = m.data_.data() + m.data_.size() - d.rem;
            } else {
                p.size = maxsize;
                p.data = m.data_.data() + (i * maxsize);
            }
            if (binary) {
                send_packet_bin(p, fn);
            } else {
                send_packet_osc(p, fn);
            }
        }
    } else {
        // single-frame message
        p.data = m.data_.data();
        p.size = m.data_.size();
        if (binary) {
            send_packet_bin(p, fn);
        } else {
            send_packet_osc(p, fn);
        }
    }
}

void peer::send_packet_osc(const message_packet& p, const sendfn& fn) const {
    char buf[AOO_MAX_PACKET_SIZE];
    osc::OutboundPacketStream msg(buf, sizeof(buf));
    AooFlag flags = p.reliable * kAooMessageReliable;
    // NB: we send *our* ID (= the sender)
    msg << osc::BeginMessage(kAooMsgPeerMessage)
        << group_id() << local_id() << (int32_t)flags
        << p.sequence << p.totalsize << p.nframes << p.frame
        << osc::TimeTag(p.tt) << p.type << osc::Blob(p.data, p.size)
        << osc::EndMessage;
    send(msg, fn);
}

void peer::send_packet_bin(const message_packet& p, const sendfn& fn) const {
    AooByte buf[AOO_MAX_PACKET_SIZE];

    AooFlag flags = (p.reliable * kAooBinMsgMessageReliable) |
            ((p.nframes > 1) * kAooBinMsgMessageFrames) |
            ((!p.tt.is_empty()) * kAooBinMsgMessageTimestamp);

    // NB: we send *our* user ID
    auto offset = aoo::binmsg_write_header(buf, sizeof(buf), kAooTypePeer,
                                           kAooBinMsgCmdMessage, group_id(), local_id());
    auto ptr = buf + offset;
    auto end = buf + sizeof(buf);
    aoo::write_bytes<int32_t>(p.sequence, ptr);
    aoo::write_bytes<uint16_t>(flags, ptr);
    aoo::write_bytes<uint16_t>(p.size, ptr);
    if (flags & kAooBinMsgMessageFrames) {
        aoo::write_bytes<int32_t>(p.totalsize, ptr);
        aoo::write_bytes<int16_t>(p.nframes, ptr);
        aoo::write_bytes<int16_t>(p.frame, ptr);
    }
    // only send type and timetag with first frame
    if (p.frame == 0) {
        if (flags & kAooBinMsgMessageTimestamp) {
            aoo::write_bytes<uint64_t>(p.tt, ptr);
        }
        aoo::write_bytes<int32_t>(p.type, ptr);
    }
    // write actual data
    assert((end - ptr) >= p.size);
    memcpy(ptr, p.data, p.size);
    ptr += p.size;

    send(buf, ptr - buf, fn);
}

// if the message is reliable, the other end sends an ack messages:
// /aoo/peer/ack <group> <user> <count> <seq1> <frame1> <seq2> <frame2> etc.
// resp.
// header, count (int32), seq1 (int32), frame1 (int32), seq2 (int32), frame2 (int32), etc. // frame < 0 -> all
void peer::send_ack(const message_ack &ack, const sendfn& fn) {
#if AOO_DEBUG_CLIENT_MESSAGE
    LOG_DEBUG("AooClient: send ack (seq: " << ack.seq << ", frame: " << ack.frame
              << ") to " << *this);
#endif
    if (binary_.load(std::memory_order_relaxed)) {
        AooByte buf[64];
        auto onset = binmsg_write_header(buf, sizeof(buf), kAooTypePeer,
                                         kAooBinMsgCmdAck, group_id_, local_id_);
        auto ptr = buf + onset;
        aoo::write_bytes<int32_t>(1, ptr);
        aoo::write_bytes<int32_t>(ack.seq, ptr);
        aoo::write_bytes<int32_t>(ack.frame, ptr);

        send(buf, ptr - buf, fn);
    } else {
        char buf[64];
        osc::OutboundPacketStream msg(buf, sizeof(buf));
        msg << osc::BeginMessage(kAooMsgPeerAck)
            << group_id_ << local_id_
            << (int32_t)1 << ack.seq << ack.frame
            << osc::EndMessage;

        send(msg, fn);
    }
}

void peer::handle_osc_message(Client& client, const char *pattern,
                              osc::ReceivedMessageArgumentIterator it,
                              const ip_address& addr) {
    LOG_DEBUG("AooClient: got OSC message " << pattern << " from " << *this);

    if (!strcmp(pattern, kAooMsgPing)) {
        handle_ping(client, it, addr);
    } else if (!strcmp(pattern, kAooMsgPong)) {
        handle_pong(client, it, addr);
    } else if (!strcmp(pattern, kAooMsgMessage)) {
        handle_client_message(client, it);
    } else if (!strcmp(pattern, kAooMsgAck)) {
        handle_ack(client, it);
    } else {
        LOG_WARNING("AooClient: got unknown message "
                    << pattern << " from " << *this);
    }
}

void peer::handle_bin_message(Client& client, const AooByte *data, AooSize size,
                              int onset, const ip_address& addr) {
    LOG_DEBUG("AooClient: got binary message from " << *this);

    switch (binmsg_cmd(data, size)) {
    case kAooBinMsgCmdMessage:
        handle_client_message(client, data + onset, size - onset);
        break;
    case kAooBinMsgCmdAck:
        handle_ack(client, data + onset, size - onset);
        break;
    default:
        LOG_WARNING("AooClient: got unknown binary message from " << *this);
    }
}

void peer::handle_first_ping(Client &client, const aoo::ip_address& addr) {
    // first ping
#if FORCE_RELAY
    // force relay
    if (!relay()){
        return;
    }
#endif
    // Try to find matching address.
    // If we receive a message from a peer behind a symmetric NAT,
    // its IP address will be different from the one we obtained
    // from the server, that's why we're sending and checking the
    // group/user ID in the first place.
    // NB: this only works if *we* are behind a full cone or restricted cone NAT.
    if (std::find(addrlist_.begin(), addrlist_.end(), addr) == addrlist_.end()) {
        LOG_WARNING("AooClient: peer " << *this << " is located behind a symmetric NAT!");
    }

    real_address_ = addr;

    connected_.store(true, std::memory_order_release);

    // push event
    auto e = std::make_unique<peer_event>(kAooEventPeerJoin, *this);
    client.send_event(std::move(e));

    LOG_VERBOSE("AooClient: successfully established connection with "
                << *this << " " << addr << (relay() ? " (relayed)" : ""));
}

void peer::handle_ping(Client& client, osc::ReceivedMessageArgumentIterator it,
                       const ip_address& addr) {
    if (!connected()) {
        handle_first_ping(client, addr);
    }

    time_tag tt1 = (it++)->AsTimeTag();
    // reply to both handshake and regular pings!!!
    // otherwise, the handshake might fail on the other side.
    // This is not 100% threadsafe, but regular pings will never
    // be sent fast enough to actually cause a race condition.
    // TODO: maybe use a queue instead?
    ping_tt1_ = tt1;
    got_ping_.store(true, std::memory_order_release);
    if (!tt1.is_empty()) {
        // regular ping
        time_tag tt2 = time_tag::now();

        // only send event for regular ping!
        auto e = std::make_unique<peer_ping_event>(*this, tt1, tt2);
        client.send_event(std::move(e));

        LOG_DEBUG("AooClient: got ping from " << *this
                  << "(tt1: " << tt1 << ", tt2: " << tt2 << ")");
    } else {
        // handshake ping
        LOG_DEBUG("AooClient: got handshake ping from " << *this);
    }
}

void peer::handle_pong(Client& client, osc::ReceivedMessageArgumentIterator it,
                       const ip_address& addr) {
    if (!connected()) {
        handle_first_ping(client, addr);
    }

    time_tag tt1 = (it++)->AsTimeTag();
    if (!tt1.is_empty()) {
        // regular pong
        time_tag tt2 = (it++)->AsTimeTag();
        time_tag tt3 = time_tag::now();

        auto rtt = time_tag::duration(tt1, tt3);
    #if 1
        // NB: we are the only thread writing to average_rtt_, so we don't need a CAS loop!
        auto avg = average_rtt_.load(std::memory_order_relaxed);
        if (avg > 0) {
            // simple low-pass filtering; maybe use cumulative average instead?
            const float coeff = 0.5;
            auto newval = avg * coeff + rtt * (1.0 - coeff);
            average_rtt_.store(newval);
        } else {
            // first ping
            average_rtt_.store(rtt);
        }
    #else
        average_rtt_.store(rtt);
    #endif

        // only send event for regular pong!
        auto e = std::make_unique<peer_pong_event>(*this, tt1, tt2, tt3);
        client.send_event(std::move(e));

        LOG_DEBUG("AooClient: got pong from " << *this
                  << "(tt1: " << tt1 << ", tt2: " << tt2 << ", tt3: " << tt3
                  << ", rtt: " << rtt << ", average: " << average_rtt_.load() << ")");
    } else {
        // handshake pong
        LOG_DEBUG("AooClient: got handshake pong from " << *this);
    }
}

void peer::handle_client_message(Client &client, osc::ReceivedMessageArgumentIterator it) {
    auto flags = (AooFlag)(it++)->AsInt32();

    message_packet p;
    p.sequence = (it++)->AsInt32();
    p.totalsize = (it++)->AsInt32();
    p.nframes = (it++)->AsInt32();
    p.frame = (it++)->AsInt32();
    p.tt = (time_tag)(it++)->AsTimeTag();
    auto data = osc_read_metadata(it);
    p.type = data.type;
    p.data = data.data;
    p.size = data.size;

    // tell the send thread that it should answer with OSC messages
    binary_.store(false, std::memory_order_relaxed);

    do_handle_client_message(client, p, flags);
}

void peer::handle_client_message(Client &client, const AooByte *data, AooSize size) {
    auto ptr = data;
    int32_t remaining = size;
    AooFlag flags = 0;
    message_packet p;

    if (remaining < 8) {
        goto bad_message;
    }
    p.sequence = aoo::read_bytes<int32_t>(ptr);
    flags = aoo::read_bytes<uint16_t>(ptr);
    p.size = aoo::read_bytes<uint16_t>(ptr);
    remaining -= 8;

    if (flags & kAooBinMsgMessageFrames) {
        if (remaining < 8) {
            goto bad_message;
        }
        p.totalsize = aoo::read_bytes<int32_t>(ptr);
        p.nframes = aoo::read_bytes<int16_t>(ptr);
        p.frame = aoo::read_bytes<int16_t>(ptr);
        remaining -= 8;
    } else {
        p.totalsize = p.size;
        p.nframes = 1;
        p.frame = 0;
    }
    // type and timetag is only sent with first frame
    p.type = kAooDataUnspecified;
    p.tt = 0;
    if (p.frame == 0) {
        if (flags & kAooBinMsgMessageTimestamp) {
            if (remaining < 8) {
                goto bad_message;
            }
            p.tt = aoo::read_bytes<uint64_t>(ptr);
            remaining -= 8;
        }
        p.type = aoo::read_bytes<int32_t>(ptr);
        remaining -= 4;
    }

    if (remaining < p.size) {
        goto bad_message;
    }
    p.data = ptr;

    // tell the send thread that it should answer with binary messages
    binary_.store(true, std::memory_order_relaxed);

    do_handle_client_message(client, p, flags);

    return;

bad_message:
    LOG_ERROR("AooClient: received malformed binary message from " << *this);
}

void peer::do_handle_client_message(Client& client, const message_packet& p, AooFlag flags) {
    if (flags & kAooMessageReliable) {
        // *** reliable message ***
        auto last_pushed = receive_buffer_.last_pushed();
        auto last_popped = receive_buffer_.last_popped();
        if (p.sequence <= last_popped) {
            // outdated message
        #if AOO_DEBUG_CLIENT_MESSAGE
            LOG_DEBUG("AooClient: ignore outdated message (seq: "
                      << p.sequence << ", frame: " << p.frame << ") from " << *this);
        #endif
            // don't forget to acknowledge!
            send_acks_.push(p.sequence, p.frame);
            return;
        }
        if (p.sequence > last_pushed) {
            // check if we have skipped messages
            // (sequence always starts at 0)
            int32_t skipped = (last_pushed >= 0) ? p.sequence - last_pushed - 1 : p.sequence;
            if (skipped > 0) {
            #if AOO_DEBUG_CLIENT_MESSAGE
                LOG_DEBUG("AooClient: skipped " << skipped << " messages from " << *this);
            #endif
                // insert empty messages
                int32_t onset = (last_pushed >= 0) ? last_pushed + 1 : 0;
                for (int i = 0; i < skipped; ++i) {
                    receive_buffer_.push(received_message(onset + i));
                }
            }
            // add new message
        #if AOO_DEBUG_CLIENT_MESSAGE
            LOG_DEBUG("AooClient: add new message (seq: " << p.sequence
                      << ", frame: " << p.frame << ") from " << *this);
        #endif
            auto& msg = receive_buffer_.push(received_message(p.sequence));
            msg.init(p.nframes, p.totalsize);
            msg.add_frame(p.frame, p.data, p.size);
            if (p.frame == 0) {
                assert(p.type != kAooDataUnspecified);
                msg.set_info(p.type, p.tt);
            }
        } else {
            // add to existing message
        #if AOO_DEBUG_CLIENT_MESSAGE
            LOG_DEBUG("AooClient: add to existing message (seq: " << p.sequence
                      << ", frame: " << p.frame << ") from " << *this);
        #endif
            if (auto msg = receive_buffer_.find(p.sequence)) {
                if (!msg->initialized()) {
                    msg->init(p.nframes, p.totalsize);
                }
                if (!msg->has_frame(p.frame)) {
                    msg->add_frame(p.frame, p.data, p.size);
                    if (p.frame == 0) {
                        assert(p.type != kAooDataUnspecified);
                        msg->set_info(p.type, p.tt);
                    }
                } else {
                #if AOO_DEBUG_CLIENT_MESSAGE
                    LOG_DEBUG("AooClient: ignore duplicate message (seq: " << p.sequence
                              << ", frame: " << p.frame << ") from " << *this);
                #endif
                }
            } else {
                LOG_ERROR("AooClient: could not find message (seq: "
                          << p.sequence << ") from " << *this);
            }
        }
        // check for completed messages
        while (!receive_buffer_.empty()) {
            auto& msg = receive_buffer_.front();
            if (msg.complete()) {
                AooData md { msg.type(), msg.data(), (AooSize)msg.size() };
                auto e = std::make_unique<peer_message_event>(
                            group_id(), user_id(), msg.tt_, md);
                client.send_event(std::move(e));

                receive_buffer_.pop();
            } else {
                break;
            }
        }
        // schedule acknowledgement
        send_acks_.push(p.sequence, p.frame);
    } else {
        // *** unreliable message ***
        if (p.nframes > 1) {
            // try to reassemble message
            if (current_msg_.sequence_ != p.sequence) {
            #if AOO_DEBUG_CLIENT_MESSAGE
                LOG_DEBUG("AooClient: new multi-frame message from " << *this);
            #endif
                // start new message (any incomplete previous message is discarded!)
                current_msg_.init(p.sequence, p.nframes, p.totalsize);
            }
            current_msg_.add_frame(p.frame, p.data, p.size);
            if (p.frame == 0) {
                assert(p.type != kAooDataUnspecified);
                current_msg_.set_info(p.type, p.tt);
            }
            if (current_msg_.complete()) {
                AooData d { current_msg_.type(), current_msg_.data(), (AooSize)current_msg_.size() };
                auto e = std::make_unique<peer_message_event>(
                            group_id(), user_id(), p.tt, d);
                client.send_event(std::move(e));
            }
        } else {
            // output immediately
            AooData d { p.type, p.data, (AooSize)p.size };
            auto e = std::make_unique<peer_message_event>(
                        group_id(), user_id(), p.tt, d);
            client.send_event(std::move(e));
        }
    }
}

void peer::handle_ack(Client &client, osc::ReceivedMessageArgumentIterator it) {
    auto count = (it++)->AsInt32();
    while (count--) {
        auto seq = (it++)->AsInt32();
        auto frame = (it++)->AsInt32();
    #if AOO_DEBUG_CLIENT_MESSAGE
        LOG_DEBUG("AooClient: got ack (seq: " << seq
                  << ", frame: " << frame << ") from " << *this);
    #endif
        received_acks_.push(seq, frame);
    }
}

void peer::handle_ack(Client &client, const AooByte *data, AooSize size) {
    auto ptr = data;
    if (size >= 4) {
        auto count = aoo::read_bytes<int32_t>(ptr);
        if ((size - 4) >= (count * 8)) {
            while (count--) {
                auto seq = aoo::read_bytes<int32_t>(ptr);
                auto frame = aoo::read_bytes<int32_t>(ptr);
            #if AOO_DEBUG_CLIENT_MESSAGE
                LOG_DEBUG("AooClient: got ack (seq: " << seq
                          << ", frame: " << frame << ") from " << *this);
            #endif
                received_acks_.push(seq, frame);
            }
            return; // done
        }
    }
    LOG_ERROR("AooClient: got malformed binary ack message from " << *this);
}

// only called if peer is connected!
void peer::send(const AooByte *data, AooSize size, const sendfn &fn) const {
    assert(connected());
    send(data, size, real_address_, fn);
}

void peer::send(const AooByte *data, AooSize size,
                const ip_address &addr, const sendfn &fn) const {
    if (relay()) {
    #if AOO_DEBUG_RELAY
        if (binmsg_check(data, size)) {
            LOG_DEBUG("AooClient: relay binary message to peer " << *this);
        } else {
            LOG_DEBUG("AooClient: relay message " << (const char *)data << " to peer " << *this);
        }
    #endif
        AooByte buf[AOO_MAX_PACKET_SIZE];
        auto result = write_relay_message(buf, sizeof(buf), data, size, addr);

        fn(buf, result, relay_address_);
    } else {
        fn(data, size, addr);
    }
}

} // net
} // aoo
