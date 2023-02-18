#pragma once

#include "detail.hpp"
#include "message_buffer.hpp"

namespace aoo {
namespace net {

class Client;

struct message;

struct message_packet {
    int32_t type;
    int32_t size;
    const AooByte *data;
    time_tag tt;
    int32_t sequence;
    int32_t totalsize;
    int32_t nframes;
    int32_t frame;
    bool reliable;
};

struct message_ack {
    int32_t seq;
    int32_t frame;
};

class peer {
public:
    peer(const std::string& groupname, AooId groupid,
         const std::string& username, AooId userid, AooId localid,
         ip_address_list&& addrlist, const AooData *metadata,
         ip_address_list&& user_relay, const ip_address_list& group_relay);

    ~peer();

    bool connected() const {
        return connected_.load(std::memory_order_acquire);
    }

    bool match(const ip_address& addr) const;

    bool match(const std::string& group) const;

    bool match(const std::string& group, const std::string& user) const;

    bool match(AooId group) const;

    bool match(AooId group, AooId user) const;

    bool match_wildcard(AooId group, AooId user) const;

    bool relay() const { return relay_; }

    const ip_address& relay_address() const { return relay_address_; }

    const ip_address_list& user_relay() const { return user_relay_;}

    const std::string& group_name() const { return group_name_; }

    AooId group_id() const { return group_id_; }

    const std::string& user_name() const { return user_name_; }

    AooId user_id() const { return user_id_; }

    AooId local_id() const { return local_id_; }

    const ip_address& address() const {
        return real_address_;
    }

    const aoo::metadata& metadata() const { return metadata_; }

    void send(Client& client, const sendfn& fn, time_tag now);

    void send_message(const message& msg, const sendfn& fn, bool binary);

    void handle_osc_message(Client& client, const char *pattern,
                            osc::ReceivedMessageArgumentIterator it,
                            const ip_address& addr);

    void handle_bin_message(Client& client, const AooByte *data,
                            AooSize size, int onset, const ip_address& addr);
private:
    void handle_ping(Client& client, osc::ReceivedMessageArgumentIterator it,
                     const ip_address& addr);

    void handle_pong(Client& client, osc::ReceivedMessageArgumentIterator it,
                     const ip_address& addr);

    void handle_first_ping(Client& client, const aoo::ip_address& addr);

    void handle_client_message(Client& client, osc::ReceivedMessageArgumentIterator it);

    void handle_client_message(Client& client, const AooByte *data, AooSize size);

    void do_handle_client_message(Client& client, const message_packet& p, AooFlag flags);

    void handle_ack(Client& client, osc::ReceivedMessageArgumentIterator it);

    void handle_ack(Client& client, const AooByte *data, AooSize size);

    void do_send(Client& client, const sendfn& fn, time_tag now);

    void send_packet_osc(const message_packet& frame, const sendfn& fn) const;

    void send_packet_bin(const message_packet& frame, const sendfn& fn) const;

    void send_ack(const message_ack& ack, const sendfn& fn);

    void send(const osc::OutboundPacketStream& msg, const sendfn& fn) const {
        send((const AooByte *)msg.Data(), msg.Size(), fn);
    }

    void send(const AooByte *data, AooSize size, const sendfn& fn) const;

    void send(const osc::OutboundPacketStream& msg,
              const ip_address& addr, const sendfn& fn) const {
        send((const AooByte *)msg.Data(), msg.Size(), addr, fn);
    }

    void send(const AooByte *data, AooSize size,
              const ip_address& addr, const sendfn &fn) const;

    const std::string group_name_;
    const std::string user_name_;
    const AooId group_id_;
    const AooId user_id_;
    const AooId local_id_;
    aoo::metadata metadata_;
    bool relay_ = false;
    ip_address_list addrlist_;
    ip_address_list user_relay_;
    ip_address_list group_relay_;
    ip_address real_address_;
    ip_address relay_address_;
    time_tag start_time_;
    double last_pingtime_ = 0;
    time_tag ping_tt1_;
    std::atomic<float> average_rtt_{0};
    std::atomic<bool> connected_{false};
    std::atomic<bool> got_ping_{false};
    std::atomic<bool> binary_{false};
    bool timeout_ = false;
    int32_t next_sequence_reliable_ = 0;
    int32_t next_sequence_unreliable_ = 0;
    message_send_buffer send_buffer_;
    message_receive_buffer receive_buffer_;
    received_message current_msg_;
    aoo::unbounded_mpsc_queue<message_ack> send_acks_;
    aoo::unbounded_mpsc_queue<message_ack> received_acks_;
};

inline std::ostream& operator<<(std::ostream& os, const peer& p) {
    os << "" << p.group_name() << "|" << p.user_name()
       << " (" << p.group_id() << "|" << p.user_id() << ")";
    return os;
}

} // net
} // aoo
