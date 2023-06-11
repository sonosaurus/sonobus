#pragma once

#include <functional>
#include <atomic>
#include <thread>
#include <vector>

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
#include "common/lockfree.hpp"
#include "common/sync.hpp"

namespace aoo {

class udp_server
{
public:
#ifdef _WIN32
    static const int invalid_socket = (int)INVALID_SOCKET;
#else
    static const int invalid_socket = -1;
#endif
    static const size_t max_udp_packet_size = 65536;

    using receive_handler = std::function<void(int error, const aoo::ip_address& addr,
                                               const AooByte *data, AooSize size)>;

    udp_server() : buffer_(max_udp_packet_size) {}
    ~udp_server();

    int port() const { return bind_addr_.port(); }
    int socket() const { return socket_; }
    aoo::ip_address::ip_type type() const { return bind_addr_.type(); }

    udp_server(const udp_server&) = delete;
    udp_server& operator=(const udp_server&) = delete;

    // call before start()
    void set_send_buffer_size(int size) {
        send_buffer_size_ = size;
    }
    void set_receive_buffer_size(int size) {
        receive_buffer_size_ = size;
    }

    void start(int port, receive_handler receive, bool threaded = false);
    void run(double timeout = -1);
    void stop();
    void notify();

    int send(const aoo::ip_address& addr, const AooByte *data, AooSize size);
private:
    void receive(double timeout);
    void do_close();

    int socket_ = invalid_socket;
    aoo::ip_address bind_addr_;
    int send_buffer_size_ = 0;
    int receive_buffer_size_ = 0;
    std::atomic<bool> running_{false};
    bool threaded_ = false;

    std::vector<AooByte> buffer_;

    struct udp_packet {
        std::vector<AooByte> data;
        ip_address address;
    };
    using packet_queue = aoo::lockfree::unbounded_mpsc_queue<udp_packet>;
    packet_queue packet_queue_;

    std::thread thread_;
    aoo::sync::event event_;

    receive_handler receive_handler_;
};

} // aoo
