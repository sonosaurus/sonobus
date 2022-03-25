#pragma once

#include "oscpack/osc/OscReceivedElements.h"

#include "common/utils.hpp"

#include <vector>
#include <cassert>

namespace aoo {
namespace net {

class osc_stream_receiver {
public:
    template<typename Fn>
    void handle_message(const char *data, int32_t n, Fn&& handler);

    void reset();
private:
    std::vector<char> buffer_;
    int32_t message_size_ = 0;
};

//------------------------------------------------------------------------------------//

template<typename Fn>
inline void osc_stream_receiver::handle_message(const char *data, int32_t n, Fn&& handler) {
    const char *ptr = data;
    int32_t remaining = n;

    while (remaining > 0) {
        if (buffer_.size() < sizeof(message_size_)) {
            // message size in progress
            auto bytes_missing = sizeof(message_size_) - buffer_.size();
            auto count = std::min<int32_t>(bytes_missing, remaining);
            buffer_.insert(buffer_.end(), ptr, ptr + count);
            ptr += count;
            remaining -= count;
            if (buffer_.size() == sizeof(message_size_)) {
                // got message size
                message_size_ = aoo::from_bytes<int32_t>(buffer_.data());
                if (message_size_ < 0) {
                    buffer_.clear();
                    char msg[256];
                    snprintf(msg, sizeof(msg), "bad OSC message size: %d", message_size_);
                    throw osc::Exception(msg);
                }
            }
        } else {
            // message data in progress
            int32_t bytes_received = buffer_.size() - sizeof(message_size_);
            int32_t bytes_missing = message_size_ - bytes_received;
            auto count = std::min<int32_t>(bytes_missing, remaining);
            buffer_.insert(buffer_.end(), ptr, ptr + count);
            ptr += count;
            remaining -= count;
            if (bytes_missing == count) {
                // message complete!
                assert(buffer_.size() == message_size_ + sizeof(message_size_));
                osc::ReceivedPacket packet(buffer_.data() + sizeof(message_size_),
                                           message_size_);
                handler(packet);
                message_size_ = 0;
                buffer_.clear();
            }
        }
    }
    // wait for more data
}

inline void osc_stream_receiver::reset() {
    buffer_.clear();
    message_size_ = 0;
}

} // net
} // aoo
