#pragma once

#include <vector>
#include <stdint.h>

namespace aoo {

class SLIP {
public:
    static const uint8_t END = 192;
    static const uint8_t ESC = 219;
    static const uint8_t ESC_END = 220;
    static const uint8_t ESC_ESC = 221;

    void setup(int32_t buffersize);
    void reset();

    int32_t read_available() const { return balance_; }
    int32_t read_bytes(uint8_t *buffer, int32_t size);

    int32_t write_available() const { return buffer_.size() - balance_; }
    int32_t write_bytes(const uint8_t *data, int32_t size);

    int32_t read_packet(uint8_t *buffer, int32_t size);

    bool write_packet(const uint8_t *data, int32_t size);
private:
    std::vector<uint8_t> buffer_;
    int32_t rdhead_ = 0;
    int32_t wrhead_ = 0;
    int32_t balance_ = 0;
};

inline void SLIP::setup(int32_t buffersize){
    buffer_.resize(buffersize);
    reset();
}

inline void SLIP::reset(){
    rdhead_ = 0;
    wrhead_ = 0;
    balance_ = 0;
}

inline int32_t SLIP::read_bytes(uint8_t *buffer, int32_t size){
    auto capacity = (int32_t)buffer_.size();
    if (size > balance_){
        size = balance_;
    }
    auto end = rdhead_ + size;
    int32_t n1, n2;
    if (end > capacity){
        n1 = capacity - rdhead_;
        n2 = end - capacity;
    } else {
        n1 = size;
        n2 = 0;
    }
    std::copy(&buffer_[rdhead_], &buffer_[rdhead_ + n1], buffer);
    std::copy(&buffer_[0], &buffer_[n2], buffer + n1);
    rdhead_ += size;
    if (rdhead_ >= capacity){
        rdhead_ -= capacity;
    }
    balance_ -= size;
    return size;
}

inline int32_t SLIP::write_bytes(const uint8_t *data, int32_t size){
    auto capacity = (int32_t)buffer_.size();
    auto space = capacity - balance_;
    if (size > space){
        size = space;
    }
    auto end = wrhead_ + size;
    int32_t split;
    if (end > capacity){
        split = capacity - wrhead_;
    } else {
        split = size;
    }
    std::copy(data, data + split, &buffer_[wrhead_]);
    std::copy(data + split, data + size, &buffer_[0]);
    wrhead_ += size;
    if (wrhead_ >= capacity){
        wrhead_ -= capacity;
    }
    balance_ += size;
    return size;
}

inline int32_t SLIP::read_packet(uint8_t *buffer, int32_t size){
    int32_t nbytes = 0;
    int32_t head = rdhead_;

    auto read_byte = [&](uint8_t& c) {
        if (nbytes >= balance_){
            return false;
        }
        c = buffer_[head++];
        if (head >= (int32_t)buffer_.size()){
            head = 0;
        }
        nbytes++;
        return true;
    };

    uint8_t data;

    // swallow leading END tokens
    do {
        if (!read_byte(data)){
            // no data
            return 0;
        }
    } while (data == END);

    // try to read packet
    int32_t packetsize = 0;
    int32_t counter = 0;

    while (data != END) {
        if (data == ESC){
            // escape character - read another byte
            if (read_byte(data)){
                if (data == ESC_END){
                    data = END;
                } else if (data == ESC_ESC){
                    data = ESC;
                } else if (data == END){
                    // incomplete escape sequence before END
                    break;
                } else {
                    // bad SLIP packet... just ignore
                }
            } else {
                // too little data
                return 0;
            }
        }

        // ignore excessive bytes
        if (counter < size){
            buffer[counter] = data;
            packetsize++;
        }
        counter++;

        // try to get more data
        if (!read_byte(data)){
            // too little data
            return 0;
        }
    }

    // update
    rdhead_ = head;
    balance_ -= nbytes;
    return packetsize;
}

inline bool SLIP::write_packet(const uint8_t *data, int32_t size){
    int32_t available = buffer_.size() - balance_;
    int32_t nbytes = 0;
    int32_t head = wrhead_;

    auto write_byte = [&](uint8_t c) {
        if (nbytes >= available){
            return false;
        }
        buffer_[head++] = c;
        if (head >= (int32_t)buffer_.size()){
            head = 0;
        }
        nbytes++;
        return true;
    };

    if ((size + 2) <= available){
        // begin packet
        write_byte(END);
        // write and escape bytes
        for (int i = 0; i < size; ++i){
            auto c = data[i];
            switch (c){
            case END:
                if (!(write_byte(ESC) && write_byte(ESC_END))){
                    return false;
                }
                break;
            case ESC:
                if (!(write_byte(ESC) && write_byte(ESC_ESC))){
                    return false;
                }
                break;
            default:
                if (!write_byte(c)){
                    return false;
                }
            }
        }
        // end packet
        if (write_byte(END)){
            // update
            wrhead_ = head;
            balance_ += nbytes;
            return true;
        } else {
            return false;
        }
    } else {
        return false;
    }
}


} // aoo
