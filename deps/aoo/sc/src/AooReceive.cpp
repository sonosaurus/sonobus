#include "AooReceive.hpp"

static InterfaceTable* ft;

/*////////////////// AooReceive ////////////////*/

void AooReceive::init(int32_t port, AooId id, AooSeconds bufsize) {
    auto data = CmdData::create<OpenCmd>(world());
    if (data){
        data->port = port;
        data->id = id;
        data->sampleRate = unit().sampleRate();
        data->blockSize = unit().bufferSize();
        data->numChannels = unit().numOutputs();
        data->bufferSize = bufsize;

        doCmd(data,
            [](World *world, void *data){
                // open in NRT thread
                auto cmd = (OpenCmd *)data;
                auto node = INode::get(world, cmd->port);
                if (node){
                    AooSink * sink = AooSink_new(cmd->id, 0, nullptr);
                    if (sink){
                        NodeLock lock(*node);
                        if (node->client()->addSink(sink, cmd->id) == kAooOk){
                            sink->setup(cmd->sampleRate, cmd->blockSize,
                                        cmd->numChannels);

                            sink->setEventHandler(
                                [](void *user, const AooEvent *event, int32_t){
                                    static_cast<AooReceive *>(user)->handleEvent(event);
                                }, cmd->owner.get(), kAooEventModePoll);

                            if (cmd->bufferSize <= 0) {
                                sink->setBufferSize(DEFBUFSIZE);
                            } else {
                                sink->setBufferSize(cmd->bufferSize);
                            }

                            cmd->node = std::move(node);
                            cmd->obj = sink;

                            return true; // success!
                        } else {
                            LOG_ERROR("aoo sink with ID " << cmd->id << " on port "
                                      << node->port() << " already exists");
                        }
                    }
                }
                return false;
            },
            [](World *world, void *data){
                auto cmd = (OpenCmd *)data;
                auto& owner = static_cast<AooReceive&>(*cmd->owner);
                owner.sink_.reset(cmd->obj);
                owner.setNode(cmd->node);
                LOG_DEBUG("AooReceive initialized");
                return false; // done
            }
        );
    }
}

void AooReceive::onDetach() {
    auto data = CmdData::create<CmdData>(world());
    if (data){
        doCmd(data,
            [](World *world, void *data){
                // release in NRT thread
                auto cmd = (CmdData*)data;
                auto& owner = static_cast<AooReceive&>(*cmd->owner);
                auto node = owner.node();
                if (node){
                    // release node
                    NodeLock lock(*node);
                    node->client()->removeSink(owner.sink());
                    lock.unlock(); // !

                    owner.setNode(nullptr);
                }
                // release sink
                owner.sink_ = nullptr;
                return false; // done
            }
        );
    }
}

void AooReceive::handleEvent(const AooEvent *event){
    char buf[256];
    osc::OutboundPacketStream msg(buf, sizeof(buf));

    switch (event->type){
    case kAooEventSourceAdd:
    {
        auto e = (const AooEventSourceAdd *)event;
        aoo::ip_address addr((const sockaddr *)e->endpoint.address,
                             e->endpoint.addrlen);

        beginEvent(msg, "/add", addr, e->endpoint.id);
        sendMsgRT(msg);
        break;
    }
    case kAooEventSourceRemove:
    {
        auto e = (const AooEventSourceRemove *)event;
        aoo::ip_address addr((const sockaddr *)e->endpoint.address,
                             e->endpoint.addrlen);

        beginEvent(msg, "/remove", addr, e->endpoint.id);
        sendMsgRT(msg);
        break;
    }
    case kAooEventInviteTimeout:
    {
        auto e = (const AooEventInviteTimeout *)event;
        aoo::ip_address addr((const sockaddr *)e->endpoint.address,
                             e->endpoint.addrlen);

        beginEvent(msg, "/invite/timeout", addr, e->endpoint.id);
        sendMsgRT(msg);
        break;
    }
    case kAooEventFormatChange:
    {
        auto e = (const AooEventFormatChange *)event;
        aoo::ip_address addr((const sockaddr *)e->endpoint.address,
                             e->endpoint.addrlen);

        beginEvent(msg, "/format", addr, e->endpoint.id);
        serializeFormat(msg, *e->format);
        sendMsgRT(msg);
        break;
    }
    case kAooEventStreamStart:
    {
        auto e = (const AooEventStreamStart *)event;
        aoo::ip_address addr((const sockaddr *)e->endpoint.address,
                             e->endpoint.addrlen);

        beginEvent(msg, "/start", addr, e->endpoint.id);
        // TODO metadata
        sendMsgRT(msg);
        break;
    }
    case kAooEventStreamStop:
    {
        auto e = (const AooEventStreamStop *)event;
        aoo::ip_address addr((const sockaddr *)e->endpoint.address,
                             e->endpoint.addrlen);

        beginEvent(msg, "/stop", addr, e->endpoint.id);
        sendMsgRT(msg);
        break;
    }
    case kAooEventStreamState:
    {
        auto e = (const AooEventStreamState *)event;
        aoo::ip_address addr((const sockaddr *)e->endpoint.address,
                             e->endpoint.addrlen);

        beginEvent(msg, "/state", addr, e->endpoint.id);
        msg << e->state;
        sendMsgRT(msg);
        break;
    }
    case kAooEventBlockLost:
    {
        auto e = (const AooEventBlockLost *)event;
        aoo::ip_address addr((const sockaddr *)e->endpoint.address,
                             e->endpoint.addrlen);

        beginEvent(msg, "/block/lost", addr, e->endpoint.id);
        msg << e->count;
        sendMsgRT(msg);
        break;
    }
    case kAooEventBlockDropped:
    {
        auto e = (const AooEventBlockDropped *)event;
        aoo::ip_address addr((const sockaddr *)e->endpoint.address,
                             e->endpoint.addrlen);

        beginEvent(msg, "/block/dropped", addr, e->endpoint.id);
        msg << e->count;
        sendMsgRT(msg);
        break;
    }
    case kAooEventBlockReordered:
    {
        auto e = (const AooEventBlockReordered *)event;
        aoo::ip_address addr((const sockaddr *)e->endpoint.address,
                             e->endpoint.addrlen);

        beginEvent(msg, "/block/reordered", addr, e->endpoint.id);
        msg << e->count;
        sendMsgRT(msg);
        break;
    }
    case kAooEventBlockResent:
    {
        auto e = (const AooEventBlockResent *)event;
        aoo::ip_address addr((const sockaddr *)e->endpoint.address,
                             e->endpoint.addrlen);

        beginEvent(msg, "/block/resent", addr, e->endpoint.id);
        msg << e->count;
        sendMsgRT(msg);
        break;
    }
    case kAooEventPing:
    {
        auto e = (const AooEventPing *)event;
        aoo::ip_address addr((const sockaddr *)e->endpoint.address,
                             e->endpoint.addrlen);

        double diff = aoo_ntpTimeDuration(e->t1, e->t2);

        beginEvent(msg, "/ping", addr, e->endpoint.id);
        msg << diff;
        sendMsgRT(msg);
        break;
    }
    default:
        LOG_DEBUG("AooReceive: got unknown event " << event->type);
        break;
    }
}

/*////////////////// AooReceiveUnit ////////////////*/

AooReceiveUnit::AooReceiveUnit() {
    int32_t port = in0(0);
    AooId id = in0(1);
    AooSeconds bufsize = in0(2);
    auto delegate = rt::make_shared<AooReceive>(mWorld, *this);
    delegate->init(port, id, bufsize);
    delegate_ = std::move(delegate);

    set_calc_function<AooReceiveUnit, &AooReceiveUnit::next>();
}

void AooReceiveUnit::next(int numSamples){
    auto sink = delegate().sink();
    if (sink){
        uint64_t t = getOSCTime(mWorld);

        if (sink->process(mOutBuf, numSamples, t) == kAooOk){
            delegate().node()->notify();
        } else {
            ClearUnitOutputs(this, numSamples);
        }

        sink->pollEvents();
    } else {
        ClearUnitOutputs(this, numSamples);
    }
}

/*//////////////////// Unit commands ////////////////////*/

namespace  {

void aoo_recv_invite(AooReceiveUnit *unit, sc_msg_iter *args){
    auto cmd = UnitCmd::create(unit->mWorld, args);
    unit->delegate().doCmd(cmd,
        [](World *world, void *cmdData){
            auto data = (UnitCmd *)cmdData;
            auto& owner = static_cast<AooReceive&>(*data->owner);

            sc_msg_iter args(data->size, data->data);
            skipUnitCmd(&args);

            auto replyID = args.geti();

            char buf[256];
            osc::OutboundPacketStream msg(buf, sizeof(buf));
            owner.beginReply(msg, "/aoo/invite", replyID);

            aoo::ip_address addr;
            AooId id;
            if (owner.node()->getSourceArg(&args, addr, id)){
                AooEndpoint ep { addr.address(),
                            (AooAddrSize)addr.length(), id };
                if (owner.sink()->inviteSource(ep, nullptr) == kAooOk) {
                    // only send IP address on success
                    msg << addr.name() << addr.port() << id;
                }
            }

            owner.sendMsgNRT(msg);

            return false; // done
        });
}

void aoo_recv_uninvite(AooReceiveUnit *unit, sc_msg_iter *args){
    auto cmd = UnitCmd::create(unit->mWorld, args);
    unit->delegate().doCmd(cmd,
        [](World *world, void *cmdData){
            auto data = (UnitCmd *)cmdData;
            auto& owner = static_cast<AooReceive&>(*data->owner);

            sc_msg_iter args(data->size, data->data);
            skipUnitCmd(&args);

            auto replyID = args.geti();

            char buf[256];
            osc::OutboundPacketStream msg(buf, sizeof(buf));
            owner.beginReply(msg, "/aoo/uninvite", replyID);

            if (args.remain() > 0){
                aoo::ip_address addr;
                AooId id;
                if (owner.node()->getSourceArg(&args, addr, id)){
                    AooEndpoint ep { addr.address(),
                                (AooAddrSize)addr.length(), id };
                    if (owner.sink()->uninviteSource(ep) == kAooOk) {
                        // only send IP address on success
                        msg << addr.name() << addr.port() << id;
                    }
                }
            } else {
                owner.sink()->uninviteAll();
            }

            owner.sendMsgNRT(msg);

            return false; // done
        });
}

void aoo_recv_bufsize(AooReceiveUnit *unit, sc_msg_iter *args){
    auto cmd = CmdData::create<OptionCmd>(unit->mWorld);
    cmd->f = args->getf();
    unit->delegate().doCmd(cmd,
        [](World *world, void *cmdData){
            auto data = (OptionCmd *)cmdData;
            auto& owner = static_cast<AooReceive&>(*data->owner);
            owner.sink()->setBufferSize(data->f);

            return false; // done
        });
}

void aoo_recv_dll_bw(AooReceiveUnit *unit, sc_msg_iter *args){
    unit->delegate().sink()->setDllBandwidth(args->getf());
}

void aoo_recv_packetsize(AooReceiveUnit *unit, sc_msg_iter *args){
    unit->delegate().sink()->setPacketSize(args->geti());
}

void aoo_recv_resend(AooReceiveUnit *unit, sc_msg_iter *args){
    unit->delegate().sink()->setResendData(args->geti());
}

void aoo_recv_resend_limit(AooReceiveUnit *unit, sc_msg_iter *args){
    unit->delegate().sink()->setResendLimit(args->geti());
}

void aoo_recv_resend_interval(AooReceiveUnit *unit, sc_msg_iter *args){
    unit->delegate().sink()->setResendInterval(args->getf());
}

void aoo_recv_reset(AooReceiveUnit *unit, sc_msg_iter *args){
    auto cmd = UnitCmd::create(unit->mWorld, args);
    unit->delegate().doCmd(cmd,
        [](World *world, void *cmdData){
            auto data = (UnitCmd *)cmdData;
            auto& owner = static_cast<AooReceive&>(*data->owner);

            sc_msg_iter args(data->size, data->data);
            skipUnitCmd(&args);

            if (args.remain() > 0){
                aoo::ip_address addr;
                AooId id;
                if (owner.node()->getSourceArg(&args, addr, id)){
                    AooEndpoint ep { addr.address(),
                                (AooAddrSize)addr.length(), id };
                    owner.sink()->resetSource(ep);
                }
            } else {
                owner.sink()->reset();
            }

            return false; // done
        });
}

using AooReceiveUnitCmdFunc = void (*)(AooReceiveUnit*, sc_msg_iter*);

// make sure that unit commands only run after the instance
// has been fully initialized.
template<AooReceiveUnitCmdFunc fn>
static void runUnitCmd(AooReceiveUnit* unit, sc_msg_iter* args) {
    if (unit->initialized() && unit->delegate().initialized()) {
        fn(unit, args);
    } else {
        LOG_WARNING("AooReceive instance not initialized");
    }
}

#define AooUnitCmd(cmd) \
    DefineUnitCmd("AooReceive", "/" #cmd, (UnitCmdFunc)runUnitCmd<aoo_recv_##cmd>)

} // namespace

/*/////////////////// Setup /////////////////////////*/

void AooReceiveLoad(InterfaceTable* inTable) {
    ft = inTable;

    registerUnit<AooReceiveUnit>(ft, "AooReceive");

    AooUnitCmd(invite);
    AooUnitCmd(uninvite);
    AooUnitCmd(bufsize);
    AooUnitCmd(dll_bw);
    AooUnitCmd(packetsize);
    AooUnitCmd(resend);
    AooUnitCmd(resend_limit);
    AooUnitCmd(resend_interval);
    AooUnitCmd(reset);
}
