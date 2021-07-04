#include "AooReceive.hpp"

static InterfaceTable* ft;

/*////////////////// AooReceive ////////////////*/

void AooReceive::init(int32_t port, aoo_id id, int32 bufsize) {
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
                    aoo::sink::pointer sink(aoo::sink::create(cmd->id, 0));
                    if (sink){
                        NodeLock lock(*node);
                        if (node->client()->add_sink(sink.get(), cmd->id) == AOO_OK){
                            sink->setup(cmd->sampleRate, cmd->blockSize,
                                        cmd->numChannels);

                            sink->set_eventhandler(
                                [](void *user, const aoo_event *event, int32_t){
                                    static_cast<AooReceive *>(user)->handleEvent(event);
                                }, cmd->owner.get(), AOO_EVENT_POLL);

                            if (cmd->bufferSize <= 0) {
                                sink->set_buffersize(DEFBUFSIZE);
                            } else {
                                sink->set_buffersize(cmd->bufferSize);
                            }

                            cmd->node = std::move(node);
                            cmd->obj = std::move(sink);

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
                owner.sink_ = std::move(cmd->obj);
                owner.setNode(std::move(cmd->node));
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
                    node->client()->remove_sink(owner.sink());
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

void AooReceive::handleEvent(const aoo_event *event){
    char buf[256];
    osc::OutboundPacketStream msg(buf, sizeof(buf));

    switch (event->type){
    case AOO_SOURCE_ADD_EVENT:
    {
        auto e = (const aoo_source_event *)event;
        aoo::ip_address addr((const sockaddr *)e->ep.address, e->ep.addrlen);

        beginEvent(msg, "/add", addr, e->ep.id);
        sendMsgRT(msg);
        break;
    }
    case AOO_SOURCE_REMOVE_EVENT:
    {
        auto e = (const aoo_source_event *)event;
        aoo::ip_address addr((const sockaddr *)e->ep.address, e->ep.addrlen);

        beginEvent(msg, "/remove", addr, e->ep.id);
        sendMsgRT(msg);
        break;
    }
    case AOO_INVITE_TIMEOUT_EVENT:
    {
        auto e = (const aoo_source_event *)event;
        aoo::ip_address addr((const sockaddr *)e->ep.address, e->ep.addrlen);

        beginEvent(msg, "/invite/timeout", addr, e->ep.id);
        sendMsgRT(msg);
        break;
    }
    case AOO_FORMAT_CHANGE_EVENT:
    {
        auto e = (const aoo_format_change_event *)event;
        aoo::ip_address addr((const sockaddr *)e->ep.address, e->ep.addrlen);

        beginEvent(msg, "/format", addr, e->ep.id);
        serializeFormat(msg, *e->format);
        sendMsgRT(msg);
        break;
    }
    case AOO_STREAM_STATE_EVENT:
    {
        auto e = (const aoo_stream_state_event *)event;
        aoo::ip_address addr((const sockaddr *)e->ep.address, e->ep.addrlen);

        beginEvent(msg, "/state", addr, e->ep.id);
        msg << e->state;
        sendMsgRT(msg);
        break;
    }
    case AOO_BLOCK_LOST_EVENT:
    {
        auto e = (const aoo_block_lost_event *)event;
        aoo::ip_address addr((const sockaddr *)e->ep.address, e->ep.addrlen);

        beginEvent(msg, "/block/lost", addr, e->ep.id);
        msg << e->count;
        sendMsgRT(msg);
        break;
    }
    case AOO_BLOCK_REORDERED_EVENT:
    {
        auto e = (const aoo_block_reordered_event *)event;
        aoo::ip_address addr((const sockaddr *)e->ep.address, e->ep.addrlen);

        beginEvent(msg, "/block/reordered", addr, e->ep.id);
        msg << e->count;
        sendMsgRT(msg);
        break;
    }
    case AOO_BLOCK_RESENT_EVENT:
    {
        auto e = (const aoo_block_resent_event *)event;
        aoo::ip_address addr((const sockaddr *)e->ep.address, e->ep.addrlen);

        beginEvent(msg, "/block/resent", addr, e->ep.id);
        msg << e->count;
        sendMsgRT(msg);
        break;
    }
    case AOO_BLOCK_GAP_EVENT:
    {
        auto e = (const aoo_block_gap_event *)event;
        aoo::ip_address addr((const sockaddr *)e->ep.address, e->ep.addrlen);

        beginEvent(msg, "/block/gap", addr, e->ep.id);
        msg << e->count;
        sendMsgRT(msg);
        break;
    }
    case AOO_PING_EVENT:
    {
        auto e = (const aoo_ping_event *)event;
        aoo::ip_address addr((const sockaddr *)e->ep.address, e->ep.addrlen);

        double diff = aoo_osctime_duration(e->tt1, e->tt2);

        beginEvent(msg, "/ping", addr, e->ep.id);
        msg << diff;
        sendMsgRT(msg);
        break;
    }
    default:
        break;
    }
}

/*////////////////// AooReceiveUnit ////////////////*/

AooReceiveUnit::AooReceiveUnit() {
    int32_t port = in0(0);
    aoo_id id = in0(1);
    int32_t bufsize = in0(2) * 1000.f; // sec -> ms
    auto delegate = rt::make_shared<AooReceive>(mWorld, *this);
    delegate->init(port, id, bufsize);
    delegate_ = std::move(delegate);

    set_calc_function<AooReceiveUnit, &AooReceiveUnit::next>();
}

void AooReceiveUnit::next(int numSamples){
    auto sink = delegate().sink();
    if (sink){
        uint64_t t = getOSCTime(mWorld);

        if (sink->process(mOutBuf, numSamples, t) == AOO_OK){
            delegate().node()->notify();
        } else {
            ClearUnitOutputs(this, numSamples);
        }

        sink->poll_events();
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
            aoo_id id;
            if (owner.node()->getSourceArg(&args, addr, id)){
                aoo_endpoint ep { addr.address(),
                            (int32_t)addr.length(), id };
                if (owner.sink()->invite_source(ep) == AOO_OK) {
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
                aoo_id id;
                if (owner.node()->getSourceArg(&args, addr, id)){
                    aoo_endpoint ep { addr.address(),
                                (int32_t)addr.length(), id };
                    if (owner.sink()->uninvite_source(ep) == AOO_OK) {
                        // only send IP address on success
                        msg << addr.name() << addr.port() << id;
                    }
                }
            } else {
                owner.sink()->uninvite_all();
            }

            owner.sendMsgNRT(msg);

            return false; // done
        });
}

void aoo_recv_bufsize(AooReceiveUnit *unit, sc_msg_iter *args){
    int32_t ms = args->getf() * 1000.f;

    auto cmd = CmdData::create<OptionCmd>(unit->mWorld);
    cmd->i = ms;
    unit->delegate().doCmd(cmd,
        [](World *world, void *cmdData){
            auto data = (OptionCmd *)cmdData;
            auto& owner = static_cast<AooReceive&>(*data->owner);
            owner.sink()->set_buffersize(data->i);

            return false; // done
        });
}

void aoo_recv_dll_bw(AooReceiveUnit *unit, sc_msg_iter *args){
    unit->delegate().sink()->set_dll_bandwidth(args->getf());
}

void aoo_recv_packetsize(AooReceiveUnit *unit, sc_msg_iter *args){
    unit->delegate().sink()->set_packetsize(args->geti());
}

void aoo_recv_resend(AooReceiveUnit *unit, sc_msg_iter *args){
    unit->delegate().sink()->set_resend_data(args->geti());
}

void aoo_recv_resend_limit(AooReceiveUnit *unit, sc_msg_iter *args){
    unit->delegate().sink()->set_resend_limit(args->geti());
}

void aoo_recv_resend_interval(AooReceiveUnit *unit, sc_msg_iter *args){
    int32_t ms = args->getf() * 1000.f;
    unit->delegate().sink()->set_resend_interval(ms);
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
                aoo_id id;
                if (owner.node()->getSourceArg(&args, addr, id)){
                    aoo_endpoint ep { addr.address(),
                                (int32_t)addr.length(), id };
                    owner.sink()->reset_source(ep);
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
