#include "udp_server.hpp"

#include "common/utils.hpp"

namespace aoo {


#ifdef _WIN32
const int kInvalidSocket = (int)INVALID_SOCKET;
#else
const int kInvalidSocket = -1;
#endif

void udp_server::start(int port, receive_handler receive,
                       bool threaded, int rcvbufsize, int sndbufsize) {
    do_close();

    receive_handler_ = std::move(receive);

    socket_ = socket_udp(port);
    if (socket_ < 0) {
        auto e = socket_errno();
        throw std::runtime_error("couldn't create/bind UDP socket: "
                                 + socket_strerror(e));
    }

    if (socket_address(socket_, addr_) < 0) {
        auto e = socket_errno();
        throw std::runtime_error("couldn't get socket address: "
                                 + socket_strerror(e));
        socket_close(socket_);
        socket_ = kInvalidSocket;
    }

    if (rcvbufsize > 0) {
        if (aoo::socket_set_recvbufsize(socket_, rcvbufsize) < 0){
            aoo::socket_error_print("setrecvbufsize");
        }
    }

    if (sndbufsize > 0) {
        if (aoo::socket_set_sendbufsize(socket_, sndbufsize) < 0){
            aoo::socket_error_print("setsendbufsize");
        }
    }

    packet_queue_.clear();

    running_.store(true);
    threaded_ = threaded;
    if (threaded_) {
        // TODO lower thread priority
        thread_ = std::thread(&udp_server::receive, this, -1);
    }
}

void udp_server::run(double timeout) {
    if (threaded_) {
        while (running_.load()) {
            packet_queue_.consume_all([&](auto& packet){
                receive_handler_(0, packet.address,
                                 packet.data.data(), packet.data.size());
            });
            if (timeout >= 0) {
                // LATER implement timed wait for semaphore
                return;
            } else {
                event_.wait();
            }
        }
    } else {
        receive(timeout);
    }
}

void udp_server::stop() {
    bool running = running_.exchange(false);
    if (running) {
        // wake up receive
        if (!socket_signal(socket_)) {
            // force wakeup by closing the socket.
            // this is not nice and probably undefined behavior,
            // the MSDN docs explicitly forbid it!
            socket_close(socket_);
        }
        if (threaded_) {
            // wake up main thread
            event_.set();
            // join receive thread
            if (thread_.joinable()) {
                thread_.join();
            }
        }
    }
}

void udp_server::do_close() {
    if (socket_ >= 0) {
        socket_close(socket_);
    }
    socket_ = kInvalidSocket;
    if (thread_.joinable()) {
        thread_.join();
    }
}

udp_server::~udp_server() {
    stop();
    do_close();
}

int udp_server::send(const aoo::ip_address& addr, const AooByte *data, AooSize size) {
    return ::sendto(socket_, (const char *)data, size, 0, addr.address(), addr.length());
}

void udp_server::receive(double timeout) {
    while (running_.load()) {
        aoo::ip_address address;
        auto result = socket_receive(socket_, buffer_.data(), buffer_.size(),
                                     &address, timeout);
        if (result > 0) {
            if (threaded_) {
                packet_queue_.produce([&](auto& packet){
                    packet.data.assign(buffer_.data(), buffer_.data() + result);
                    packet.address = address;
                });
                event_.set(); // notify main thread (if blocking)
            } else {
                receive_handler_(0, address, buffer_.data(), result);
            }
        } else if (result < 0) {
            int e = socket_errno();
        #ifdef _WIN32
            // ignore ICMP Port Unreachable message!
            if (e == WSAECONNRESET) {
                continue;
            }
        #endif
            LOG_DEBUG("udp_server: recv() failed: " << socket_strerror(e));
            receive_handler_(e, address, nullptr, 0);

            // notify main thread (if blocking)
            if (threaded_) {
                running_.store(false);
                event_.set();
            }

            return;
        } else {
            // ignore empty packet (used for signalling)
        }
    }
}

} // aoo
