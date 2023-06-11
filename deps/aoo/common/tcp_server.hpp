#pragma once

#include <functional>
#include <vector>
#include <atomic>

#ifdef _WIN32
# include <winsock2.h>
#else
# include <sys/poll.h>
# include <unistd.h>
# include <netdb.h>
# include <netinet/in.h>
# include <netinet/tcp.h>
# include <arpa/inet.h>
# include <errno.h>
#endif

#include "common/net_utils.hpp"

namespace aoo {

class tcp_server
{
public:
#ifdef _WIN32
    static const AooSocket invalid_socket = (AooSocket)INVALID_SOCKET;
#else
    static const AooSocket invalid_socket = -1;
#endif

    tcp_server() {}
    ~tcp_server();

    tcp_server(const tcp_server&) = delete;
    tcp_server& operator=(const tcp_server&) = delete;

    // returns client ID
    using accept_handler = std::function<AooId(int errorcode, const aoo::ip_address& address)>;

    using receive_handler = std::function<void(int errorcode, AooId client, const ip_address& address,
                                               const AooByte *data, AooSize size)>;

    void start(int port, accept_handler accept, receive_handler receive);
    void run();
    void stop();
    void notify();

    int send(AooId client, const AooByte *data, AooSize size);
    bool close(AooId client);
    int client_count() const { return client_count_; }
private:
    struct client {
        ip_address address;
        AooSocket socket;
        AooId id;
    };

    void loop();
    void accept_client();
    void on_accept_error(int code, const ip_address& addr);
    void receive_from_clients();
    void on_client_error(const client& c, int code) {
        receive_handler_(code, c.id, c.address, nullptr, 0);
    }
    void close_and_remove_client(int index);
    void do_close();

    int listen_socket_ = invalid_socket;
    int event_socket_ = invalid_socket;
    int last_error_ = 0;
    std::atomic<bool> running_{false};

    std::vector<pollfd> poll_array_;
    static const size_t listen_index = 0;
    static const size_t event_index = 1;
    static const size_t client_index = 2;

    accept_handler accept_handler_;
    receive_handler receive_handler_;

    std::vector<client> clients_;
    std::vector<size_t> stale_clients_;
    int32_t client_count_ = 0;
    static const int max_stale_clients = 100;
};

} // aoo
