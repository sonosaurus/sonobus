#include "Aoo.hpp"

#include "aoo/aoo_source.hpp"

// for hardware buffer sizes up to 1024 @ 44.1 kHz
#define DEFBUFSIZE 0.025

using OpenCmd = _OpenCmd<AooSource>;

/*////////////////// AooSend ////////////////*/

class AooSendUnit;

class AooSend final : public AooDelegate {
public:
    using AooDelegate::AooDelegate;

    void init(int32_t port, AooId id);

    void onDetach() override;

    void handleEvent(const AooEvent *event);

    AooSource * source() { return source_.get(); }

    bool addSink(const aoo::ip_address& addr, AooId id,
                 bool active, int32_t channelOnset);
    bool removeSink(const aoo::ip_address& addr, AooId id);
    void removeAll();

    void setAccept(bool b){
        accept_ = b;
    }
private:
    AooSource::Ptr source_;
    bool accept_ = true;
};

/*////////////////// AooSendUnit ////////////////*/

class AooSendUnit final : public AooUnit {
public:
    AooSendUnit();

    void next(int numSamples);

    AooSend& delegate() {
        return static_cast<AooSend&>(*delegate_);
    }

    int numChannels() const {
        return numInputs() - channelOnset_;
    }
private:
    static const int channelOnset_ = 3;

    bool playing_ = false;
};

