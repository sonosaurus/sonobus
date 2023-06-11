#pragma once

#include "oscpack/osc/OscReceivedElements.h"

#include "common/utils.hpp"

#include <vector>
#include <cassert>

namespace aoo {
namespace net {

class osc_stream_receiver {
public:
    // TODO: should we rather pass the handler to the constructor
    // and store it in a std::function?
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
    assert(n > 0);

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
                auto msgsize = aoo::from_bytes<int32_t>(buffer_.data());
                // OSC packet size must be a multiple of 4!
                if (msgsize <= 0 || ((msgsize & 3) != 0)) {
                    reset();
                    throw osc::MalformedPacketException("bad OSC packet size");
                }
                message_size_ = msgsize;
                // NB: do not reserve the buffer because the message size
                // may come from a bad actor and may be huge!
            }
        } else {
            // message data in progress
            int32_t bytes_received = buffer_.size() - sizeof(message_size_);
            int32_t bytes_missing = message_size_ - bytes_received;
            // When we get the first data bytes, check if this is really an OSC message
            // to prevent bad actors from overflowing our buffer with bogus data.
            if (bytes_received == 0 && ptr[0] != '/') {
                reset();
                throw osc::MalformedMessageException("not an OSC message");
            }
            auto count = std::min<int32_t>(bytes_missing, remaining);
            buffer_.insert(buffer_.end(), ptr, ptr + count);
            ptr += count;
            remaining -= count;
            if (bytes_missing == count) {
                // message complete!
                assert(buffer_.size() == message_size_ + sizeof(message_size_));
                osc::ReceivedPacket packet(buffer_.data() + sizeof(message_size_), message_size_);
                handler(packet);
                reset();
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
