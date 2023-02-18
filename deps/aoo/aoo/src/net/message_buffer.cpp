#include "message_buffer.hpp"

namespace aoo {
namespace net {

//------------------------ sent_message -----------------------------//

#define AOO_MAX_RESEND_INTERVAL 1.0
#define AOO_RESEND_INTERVAL_BACKOFF 2.0

bool sent_message::need_resend(double now) {
    if (time_ > 0) {
        if ((now - time_) >= interval_) {
            time_ = now;
            interval_ *= AOO_RESEND_INTERVAL_BACKOFF;
            if (interval_ > AOO_MAX_RESEND_INTERVAL) {
                interval_ = AOO_MAX_RESEND_INTERVAL;
            }
            return true;
        }
    } else {
        time_ = now;
    }
    return false;
}

//------------------------ message_send_buffer ----------------------//

sent_message& message_send_buffer::push(sent_message&& msg) {
    data_.push_back(std::move(msg));
    return data_.back();
}

void message_send_buffer::pop() {
    data_.pop_front();
}

sent_message* message_send_buffer::find(int32_t seq) {
    for (auto& item : data_) {
        if (item.sequence_ ==  seq) {
            return &item;
        }
    }
    return nullptr;
}


//------------------------ received_message ------------------------//

void received_message::add_frame(int32_t which, const AooByte *data, int32_t n) {
    assert(!buffer_.empty());
    assert(which < nframes_);
    if (which == nframes_ - 1){
        std::copy(data, data + n, buffer_.end() - n);
    } else {
        std::copy(data, data + n, buffer_.data() + (which * n));
        framesize_ = n; // LATER allow varying framesizes
    }
    frames_[which] = 0;
}

//------------------------- message_receive_buffer ------------------//

received_message& message_receive_buffer::push(received_message&& msg) {
    last_pushed_ = msg.sequence_;
    data_.push_back(std::move(msg));
    return data_.back();
}

void message_receive_buffer::pop() {
    last_popped_ = data_.front().sequence_;
    data_.pop_front();
}

received_message* message_receive_buffer::find(int32_t seq) {
    for (auto& item : data_) {
        if (item.sequence_ ==  seq) {
            return &item;
        }
    }
    return nullptr;
}

} // namespace net
} // namespace aoo
