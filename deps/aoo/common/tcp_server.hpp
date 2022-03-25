#pragma once

#include <functional>
#include <vector>
#include <atomic>

#ifdef _WIN32
#include <winsock2.h>
#else
#include <sys/poll.h>
#include <unistd.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <errno.h>
#endif

#include "common/net_utils.hpp"

namespace aoo {

// TODO: make multi-threaded

class tcp_server
{
public:
    tcp_server() {}
    ~tcp_server();

    tcp_server(const tcp_server&) = delete;
    tcp_server& operator=(const tcp_server&) = delete;

    // returns client ID
    using accept_handler = std::function<AooId(int errorcode, const aoo::ip_address& address, AooSocket socket)>;

    using receive_handler = std::function<void(AooId client, int errorcode, const AooByte *data, AooSize size)>;

    void start(int port, accept_handler accept, receive_handler receive);
    void run();
    void stop();

    int send(AooId client, const AooByte *data, AooSize size);
    bool close(AooId client);
private:
    void receive();
    bool accept_client();
    void receive_from_clients();
    void on_error(int code) {
        accept_handler_(0, ip_address(), -1);
    }
    void on_error(AooId id, int code) {
        receive_handler_(id, code, nullptr, 0);
    }
    void do_close();

    int listen_socket_;
    int event_socket_;
    std::atomic<bool> running_ = false;

    std::vector<pollfd> poll_array_;
    static const size_t listen_index = 0;
    static const size_t event_index = 1;
    static const size_t client_index = 2;

    accept_handler accept_handler_;
    receive_handler receive_handler_;

    struct client {
        ip_address address;
        AooSocket socket;
        AooId id;
    };
    std::vector<client> clients_;
    std::vector<size_t> stale_clients_;
    static const int max_stale_clients = 100;
};

} // aoo
