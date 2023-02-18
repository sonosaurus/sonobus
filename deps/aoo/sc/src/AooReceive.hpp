#include "Aoo.hpp"

#include "aoo/aoo_sink.hpp"

#define DEFAULT_LATENCY 0.050

using OpenCmd = _OpenCmd<AooSink>;

/*////////////////// AooReceive ////////////////*/

class AooReceive final : public AooDelegate {
public:
    using AooDelegate::AooDelegate;

    void init(int32_t port, AooId id, AooSeconds latency);

    void onDetach() override;

    void handleEvent(const AooEvent *event);

    AooSink* sink() { return sink_.get(); }
private:
    AooSink::Ptr sink_;
};

/*////////////////// AooReceiveUnit ////////////////*/

class AooReceiveUnit final : public AooUnit {
public:
    AooReceiveUnit();

    void next(int numSamples);

    AooReceive& delegate() {
        return static_cast<AooReceive&>(*delegate_);
    }
};
