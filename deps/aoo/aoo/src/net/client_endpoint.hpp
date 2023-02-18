#pragma once

#include "aoo/aoo_net.h"
#include "aoo/aoo_server.h"

#include "detail.hpp"
#include "osc_stream_receiver.hpp"

namespace aoo {
namespace net {

class client_endpoint;
class Server;

//--------------------------- user -----------------------------//

class user {
public:
    user(const std::string& name, const std::string& pwd, AooId id,
         AooId group, AooId client, const AooData *md,
         const ip_host& relay, bool persistent)
        : name_(name), pwd_(pwd), id_(id), group_(group),
          client_(client), md_(md), relay_(relay), persistent_(persistent) {}
    ~user() {}

    const std::string& name() const { return name_; }

    const std::string& pwd() const { return pwd_; }

    bool check_pwd(const char *pwd) const {
        return pwd_.empty() || (pwd && (pwd == pwd_));
    }

    AooId id() const { return id_; }

    void set_metadata(const AooData& md) {
        md_ = aoo::metadata(&md);
    }

    const aoo::metadata& metadata() const { return md_; }

    AooId group() const { return group_; }

    bool active() const { return client_ != kAooIdInvalid; }

    void set_client(AooId client) {
        client_ = client;
    }

    void unset() { client_ = kAooIdInvalid; }

    AooId client() const {return client_; }

    void set_relay(const ip_host& relay) {
        relay_ = relay;
    }

    const ip_host& relay_addr() const { return relay_; }

    bool persistent() const { return persistent_; }
private:
    std::string name_;
    std::string pwd_;
    AooId id_;
    AooId group_;
    AooId client_;
    aoo::metadata md_;
    ip_host relay_;
    bool persistent_;
};

inline std::ostream& operator<<(std::ostream& os, const user& u) {
    os << "'" << u.name() << "'(" << u.id() << ")[" << u.client() << "]";
    return os;
}

//--------------------------- group -----------------------------//

using user_list = std::vector<user>;

class group {
public:
    group(const std::string& name, const std::string& pwd, AooId id,
         const AooData *md, const ip_host& relay, bool persistent)
        : name_(name), pwd_(pwd), id_(id), md_(md), relay_(relay), persistent_(persistent) {}

    const std::string& name() const { return name_; }

    const std::string& pwd() const { return pwd_; }

    bool check_pwd(const char *pwd) const {
        return pwd_.empty() || (pwd && (pwd == pwd_));
    }

    AooId id() const { return id_; }

    void set_metadata(const AooData& md) {
        md_ = aoo::metadata(&md);
    }

    const aoo::metadata& metadata() const { return md_; }

    const ip_host& relay_addr() const { return relay_; }

    bool persistent() const { return persistent_; }

    bool user_auto_create() const { return user_auto_create_; }

    user* add_user(user&& u);

    user* find_user(const std::string& name);

    user* find_user(AooId id);

    user* find_user(const client_endpoint& client);

    bool remove_user(AooId id);

    int32_t user_count() const { return users_.size(); }

    const user_list& users() const { return users_; }

    AooId get_next_user_id();
private:
    std::string name_;
    std::string pwd_;
    AooId id_;
    aoo::metadata md_;
    ip_host relay_;
    bool persistent_;
    bool user_auto_create_ = AOO_USER_AUTO_CREATE; // TODO
    user_list users_;
    AooId next_user_id_{0};
};

inline std::ostream& operator<<(std::ostream& os, const group& g) {
    os << "'" << g.name() << "'(" << g.id() << ")";
    return os;
}

//--------------------- client_endpoint --------------------------//

class client_endpoint {
public:
    client_endpoint(int sockfd, AooId id, AooServerReplyFunc replyfn, void *context)
        : id_(id), sockfd_(sockfd), replyfn_(replyfn), context_(context) {}

    ~client_endpoint() {}

    client_endpoint(client_endpoint&&) = default;
    client_endpoint& operator=(client_endpoint&&) = default;

    AooId id() const { return id_; }

    AooSocket sockfd() const { return sockfd_; }

    bool valid() const {
        return !public_addresses_.empty();
    }

    void add_public_address(const ip_address& addr) {
        public_addresses_.push_back(addr);
    }

    const ip_address_list& public_addresses() const {
        return public_addresses_;
    }

    bool match(const ip_address& addr) const;

    void send_message(const osc::OutboundPacketStream& msg) const;

    void send_error(Server& server, AooId token, AooRequestType type,
                    AooError result, int32_t errcode, const char *errmsg);

    void send_notification(Server& server, const AooData& data) const;

    void send_peer_add(Server& server, const group& grp, const user& usr,
                       const client_endpoint& client) const;

    void send_peer_remove(Server& server, const group& grp, const user& usr) const;

    void send_group_update(Server& server, const group& grp);

    void send_user_update(Server& server, const user& usr);

    void send_peer_update(Server& server, const user& peer);

    void on_group_join(const group& grp, const user& usr);

    void on_group_leave(const group& grp, const user& usr, bool force);

    void on_close(Server& server);

    void handle_message(Server& server, const AooByte *data, int32_t n);
private:
    AooId id_;
    int sockfd_; // LATER use this to get information about the client (e.g. IP protocol)
    AooServerReplyFunc replyfn_;
    void *context_;
    osc_stream_receiver receiver_;
    ip_address_list public_addresses_;
    struct group_user {
        AooId group;
        AooId user;
    };
    std::vector<group_user> group_users_;
};

} // net
} // aoo
