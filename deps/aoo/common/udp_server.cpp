#include "udp_server.hpp"

#include "common/utils.hpp"

namespace aoo {

void udp_server::start(int port, receive_handler receive, bool threaded) {
    do_close();

    receive_handler_ = std::move(receive);

    auto sock = socket_udp(port);
    if (sock < 0) {
        auto e = socket_errno();
        throw std::runtime_error("couldn't create/bind UDP socket: " + socket_strerror(e));
    }

    if (socket_address(sock, bind_addr_) < 0) {
        auto e = socket_errno(); // cache error
        socket_close(sock);
        throw std::runtime_error("couldn't get socket address: " + socket_strerror(e));
    }

    if (send_buffer_size_ > 0) {
        if (aoo::socket_set_sendbufsize(sock, send_buffer_size_) < 0){
            aoo::socket_error_print("setsendbufsize");
        }
    }

    if (receive_buffer_size_ > 0) {
        if (aoo::socket_set_recvbufsize(sock, receive_buffer_size_) < 0){
            aoo::socket_error_print("setrecvbufsize");
        }
    }

    socket_ = sock;
    running_.store(true);
    threaded_ = threaded;
    if (threaded_) {
        packet_queue_.clear();
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

void udp_server::notify() {
    if (threaded_) {
        event_.set(); // wake up main thread
    } else {
        socket_signal(socket_);
    }
}

void udp_server::do_close() {
    if (socket_ != invalid_socket) {
        socket_close(socket_);
    }
    socket_ = invalid_socket;
    bind_addr_.clear();
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
            if (e == EINTR){
                continue;
            }

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
