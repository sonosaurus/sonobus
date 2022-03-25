#include "AooSend.hpp"

#if USE_CODEC_OPUS
# include "aoo/codec/aoo_opus.h"
#endif

static InterfaceTable *ft;

/*////////////////// AooSend ////////////////*/

void AooSend::init(int32_t port, AooId id) {
    auto data = CmdData::create<OpenCmd>(world());
    if (data){
        data->port = port;
        data->id = id;
        data->sampleRate = unit().sampleRate();
        data->blockSize = unit().bufferSize();
        data->numChannels = static_cast<AooSendUnit&>(unit()).numChannels();

        doCmd(data,
            [](World *world, void *data){
                LOG_DEBUG("try to get node");
                // open in NRT thread
                auto cmd = (OpenCmd *)data;
                auto node = INode::get(world, cmd->port);
                if (node){
                    AooSource *source = AooSource_new(cmd->id, 0, nullptr);
                    if (source){
                        NodeLock lock(*node);
                        if (node->client()->addSource(source, cmd->id) == kAooOk){
                            source->setup(cmd->sampleRate, cmd->blockSize,
                                          cmd->numChannels);

                            source->setEventHandler(
                                [](void *user, const AooEvent *event, int32_t){
                                    static_cast<AooSend *>(user)->handleEvent(event);
                                }, cmd->owner.get(), kAooEventModePoll);

                            source->setBufferSize(DEFBUFSIZE);

                            AooFormatStorage f;
                            makeDefaultFormat(f, cmd->sampleRate, cmd->blockSize, cmd->numChannels);
                            source->setFormat(f.header);

                            cmd->node = std::move(node);
                            cmd->obj = source;

                            return true; // success!
                        } else {
                            LOG_ERROR("aoo source with ID " << cmd->id << " on port "
                                      << node->port() << " already exists");
                        }
                    }
                }
                return false;
            },
            [](World *world, void *data){
                auto cmd = (OpenCmd *)data;
                auto& owner = static_cast<AooSend&>(*cmd->owner);
                owner.source_.reset(cmd->obj);
                owner.setNode(std::move(cmd->node));
                LOG_DEBUG("AooSend initialized");
                return false; // done
            }
        );
    }
}

void AooSend::onDetach() {
    auto data = CmdData::create<CmdData>(world());
    if (data){
        doCmd(data,
            [](World *world, void *data){
                // release in NRT thread
                auto cmd = (CmdData*)data;
                auto& owner = static_cast<AooSend&>(*cmd->owner);
                auto node = owner.node();
                if (node){
                    // release node
                    NodeLock lock(*node);
                    node->client()->removeSource(owner.source());
                    lock.unlock(); // !

                    owner.setNode(nullptr);
                }
                // release source
                owner.source_ = nullptr;
                return false; // done
            }
        );
    }
}

void AooSend::handleEvent(const AooEvent *event){
    char buf[256];
    osc::OutboundPacketStream msg(buf, 256);

    switch (event->type){
    case kAooEventPingReply:
    {
        auto e = (const AooEventPingReply *)event;
        aoo::ip_address addr((const sockaddr *)e->endpoint.address, e->endpoint.addrlen);
        double diff1 = aoo_ntpTimeDuration(e->t1, e->t2);
        double diff2 = aoo_ntpTimeDuration(e->t2, e->t3);
        double rtt = aoo_ntpTimeDuration(e->t1, e->t3);

        beginEvent(msg, "/ping", addr, e->endpoint.id);
        msg << diff1 << diff2 << rtt << e->packetLoss;
        sendMsgRT(msg);
        break;
    }
    case kAooEventInvite:
    {
        auto e = (const AooEventInvite *)event;
        aoo::ip_address addr((const sockaddr *)e->endpoint.address, e->endpoint.addrlen);

        if (accept_){
            // TODO
        } else {
            beginEvent(msg, "/invite", addr, e->endpoint.id);
            sendMsgRT(msg);
        }
        break;
    }
    case kAooEventUninvite:
    {
        auto e = (const AooEventUninvite *)event;
        aoo::ip_address addr((const sockaddr *)e->endpoint.address, e->endpoint.addrlen);

        if (accept_){
            // TODO
        } else {
            beginEvent(msg, "/uninvite", addr, e->endpoint.id);
            sendMsgRT(msg);
        }
        break;
    }
    default:
        LOG_DEBUG("AooSend: got unknown event " << event->type);
        break;
    }
}

bool AooSend::addSink(const aoo::ip_address& addr, AooId id,
                      bool active, int32_t channelOnset){
    AooEndpoint ep { addr.address(), (AooAddrSize)addr.length(), id };
    AooFlag flags = active ? kAooSinkActive : 0;
    if (source()->addSink(ep, flags) == kAooOk){
        if (channelOnset > 0){
            source()->setSinkChannelOnset(ep, channelOnset);
        }
        return true;
    } else {
        return false;
    }
}

bool AooSend::removeSink(const aoo::ip_address& addr, AooId id){
    AooEndpoint ep { addr.address(), (AooAddrSize)addr.length(), id };
    return source()->removeSink(ep) == kAooOk;
}

void AooSend::removeAll(){
    source()->removeAll();
}

/*////////////////// AooSendUnit ////////////////*/

AooSendUnit::AooSendUnit(){
    int32_t port = in0(0);
    AooId id = in0(1);
    auto delegate = rt::make_shared<AooSend>(mWorld, *this);
    delegate->init(port, id);
    delegate_ = std::move(delegate);

    set_calc_function<AooSendUnit, &AooSendUnit::next>();
}

void AooSendUnit::next(int numSamples){
    auto source = delegate().source();
    if (source){
        // check if play state has changed
        bool playing = in0(2);
        if (playing != playing_) {
            if (playing){
                source->startStream(nullptr);
            } else {
                source->stopStream();
            }
            playing_ = playing;
        }

        auto vec = mInBuf + channelOnset_;
        uint64_t t = getOSCTime(mWorld);

        if (source->process(vec, numSamples, t) == kAooOk){
            delegate().node()->notify();
        }

        source->pollEvents();
    }
}

/*//////////////////// Unit commands ////////////////////*/

namespace {

void aoo_send_add(AooSendUnit *unit, sc_msg_iter* args){
    auto cmd = UnitCmd::create(unit->mWorld, args);
    unit->delegate().doCmd(cmd,
        [](World *world, void *cmdData){
            auto data = (UnitCmd *)cmdData;
            auto& owner = static_cast<AooSend&>(*data->owner);

            sc_msg_iter args(data->size, data->data);
            skipUnitCmd(&args);

            auto replyID = args.geti();

            char buf[256];
            osc::OutboundPacketStream msg(buf, sizeof(buf));
            owner.beginReply(msg, "/aoo/add", replyID);

            aoo::ip_address addr;
            AooId id;
            if (owner.node()->getSinkArg(&args, addr, id)){
                auto active = args.geti();
                auto channelOnset = args.geti();

                // only send IP address on success
                if (owner.addSink(addr, id, active, channelOnset)){
                    msg << addr.name() << addr.port() << id;
                }
            }

            owner.sendMsgNRT(msg);

            return false; // done
        });
}

void aoo_send_remove(AooSendUnit *unit, sc_msg_iter* args){
    auto cmd = UnitCmd::create(unit->mWorld, args);
    unit->delegate().doCmd(cmd,
        [](World *world, void *cmdData){
            auto data = (UnitCmd *)cmdData;
            auto& owner = static_cast<AooSend&>(*data->owner);

            sc_msg_iter args(data->size, data->data);
            skipUnitCmd(&args);

            auto replyID = args.geti();

            char buf[256];
            osc::OutboundPacketStream msg(buf, sizeof(buf));
            owner.beginReply(msg, "/aoo/remove", replyID);

            if (args.remain() > 0){
                aoo::ip_address addr;
                AooId id;
                if (owner.node()->getSinkArg(&args, addr, id)){
                    if (owner.removeSink(addr, id)){
                        // only send IP address on success
                        msg << addr.name() << addr.port() << id;
                    }
                }
            } else {
                owner.removeAll();
            }

            owner.sendMsgNRT(msg);

            return false; // done
        });
}

void aoo_send_accept(AooSendUnit *unit, sc_msg_iter* args){
    unit->delegate().setAccept(args->geti());
}

void aoo_send_format(AooSendUnit *unit, sc_msg_iter* args){
    auto cmd = UnitCmd::create(unit->mWorld, args);
    unit->delegate().doCmd(cmd,
        [](World *world, void *cmdData){
            auto data = (UnitCmd *)cmdData;
            auto& owner = static_cast<AooSend&>(*data->owner);

            sc_msg_iter args(data->size, data->data);
            skipUnitCmd(&args);

            auto replyID = args.geti();

            char buf[256];
            osc::OutboundPacketStream msg(buf, sizeof(buf));
            owner.beginReply(msg, "/aoo/format", replyID);

            AooFormatStorage f;
            int nchannels = static_cast<AooSendUnit&>(owner.unit()).numChannels();
            if (parseFormat(owner.unit(), nchannels, &args, f)){
                if (f.header.numChannels > nchannels){
                    LOG_ERROR("AooSend: 'channel' argument (" << f.header.numChannels
                              << ") out of range");
                    f.header.numChannels = nchannels;
                }
                auto err = owner.source()->setFormat(f.header);
                if (err == kAooOk){
                    // only send format on success
                    serializeFormat(msg, f.header);
                } else {
                    LOG_ERROR("AooSend: couldn't set format: " << aoo_strerror(err));
                }
            }

            owner.sendMsgNRT(msg);

            return false; // done
        });
}

#if USE_CODEC_OPUS

bool get_opus_bitrate(AooSource *src, osc::OutboundPacketStream& msg) {
    // NOTE: because of a bug in opus_multistream_encoder (as of opus v1.3.2)
    // OPUS_GET_BITRATE always returns OPUS_AUTO
    opus_int32 value;
    auto err = AooSource_getOpusBitrate(src, 0, &value);
    if (err != kAooOk){
        LOG_ERROR("could not get bitrate: " << aoo_strerror(err));
        return false;
    }
    switch (value){
    case OPUS_AUTO:
        msg << "auto";
        break;
    case OPUS_BITRATE_MAX:
        msg << "max";
        break;
    default:
        msg << (float)(value * 0.001); // bit -> kBit
        break;
    }
    return true;
}

void set_opus_bitrate(AooSource *src, sc_msg_iter &args) {
    // "auto", "max" or number
    opus_int32 value;
    if (args.nextTag() == 's'){
        const char *s = args.gets();
        if (!strcmp(s, "auto")){
            value = OPUS_AUTO;
        } else if (!strcmp(s, "max")){
            value = OPUS_BITRATE_MAX;
        } else {
            LOG_ERROR("bad bitrate argument '" << s << "'");
            return;
        }
    } else {
        opus_int32 bitrate = args.getf() * 1000.0; // kBit -> bit
        if (bitrate > 0){
            value = bitrate;
        } else {
            LOG_ERROR("bitrate argument " << bitrate << " out of range");
            return;
        }
    }
    auto err = AooSource_setOpusBitrate(src, 0, value);
    if (err != kAooOk){
        LOG_ERROR("could not set bitrate: " << aoo_strerror(err));
    }
}

bool get_opus_complexity(AooSource *src, osc::OutboundPacketStream& msg){
    opus_int32 value;
    auto err = AooSource_getOpusComplexity(src, 0, &value);
    if (err != kAooOk){
        LOG_ERROR("could not get complexity: " << aoo_strerror(err));
        return false;
    }
    msg << value;
    return true;
}

void set_opus_complexity(AooSource *src, sc_msg_iter &args){
    // 0-10
    opus_int32 value = args.geti();
    if (value < 0 || value > 10){
        LOG_ERROR("complexity value " << value << " out of range");
        return;
    }
    auto err = AooSource_setOpusComplexity(src, 0, value);
    if (err != kAooOk){
        LOG_ERROR("could not set complexity: " << aoo_strerror(err));
    }
}

bool get_opus_signal(AooSource *src, osc::OutboundPacketStream& msg){
    opus_int32 value;
    auto err = AooSource_getOpusSignalType(src, 0, &value);
    if (err != kAooOk){
        LOG_ERROR("could not get signal type: " << aoo_strerror(err));
        return false;
    }
    switch (value){
    case OPUS_SIGNAL_MUSIC:
        msg << "music";
        break;
    case OPUS_SIGNAL_VOICE:
        msg << "voice";
        break;
    default:
        msg << "auto";
        break;
    }
    return true;
}

void set_opus_signal(AooSource *src, sc_msg_iter &args){
    // "auto", "music", "voice"
    opus_int32 value;
    const char *s = args.gets();
    if (!strcmp(s, "auto")){
        value = OPUS_AUTO;
    } else if (!strcmp(s, "music")){
        value = OPUS_SIGNAL_MUSIC;
    } else if (!strcmp(s, "voice")){
        value = OPUS_SIGNAL_VOICE;
    } else {
        LOG_ERROR("unsupported signal type '" << s << "'");
        return;
    }
    auto err = AooSource_setOpusSignalType(src, 0, value);
    if (err != kAooOk){
        LOG_ERROR("could not set signal type: " << aoo_strerror(err));
    }
}

#endif

void aoo_send_codec_set(AooSendUnit *unit, sc_msg_iter* args){
    auto cmd = UnitCmd::create(unit->mWorld, args);
    unit->delegate().doCmd(cmd,
        [](World *world, void *cmdData){
            auto data = (UnitCmd *)cmdData;
            auto& owner = static_cast<AooSend&>(*data->owner);

            sc_msg_iter args(data->size, data->data);
            skipUnitCmd(&args);

            auto codec = args.gets();
            auto param = args.gets();

        #if USE_CODEC_OPUS
            if (!strcmp(codec, "opus")){
                opus_int32 value;
                if (!strcmp(param ,"bitrate")){
                    set_opus_bitrate(owner.source(), args);
                    return false; // done
                } else if (!strcmp(param, "complexity")){
                    set_opus_complexity(owner.source(), args);
                    return false; // done
                } else if (!strcmp(param, "signal")){
                    set_opus_signal(owner.source(), args);
                    return false; // done
                }
            }
        #endif

            LOG_ERROR("unknown parameter '" << param
                      << "' for codec '" << codec << "'");

            return false; // done
        });
}

void aoo_send_codec_get(AooSendUnit *unit, sc_msg_iter* args){
    auto cmd = UnitCmd::create(unit->mWorld, args);
    unit->delegate().doCmd(cmd,
        [](World *world, void *cmdData){
            auto data = (UnitCmd *)cmdData;
            auto& owner = static_cast<AooSend&>(*data->owner);

            sc_msg_iter args(data->size, data->data);
            skipUnitCmd(&args);

            auto replyID = args.geti();
            auto codec = args.gets();
            auto param = args.gets();

            char buf[256];
            osc::OutboundPacketStream msg(buf, sizeof(buf));
            owner.beginReply(msg, "/aoo/codec/get", replyID);
            msg << codec << param;

        #if USE_CODEC_OPUS
            if (!strcmp(codec, "opus")){
                if (!strcmp(param, "bitrate")){
                    if (get_opus_bitrate(owner.source(), msg)){
                        goto codec_sendit;
                    } else {
                        return false;
                    }
                } else if (!strcmp(param, "complexity")){
                    if (get_opus_complexity(owner.source(), msg)){
                        goto codec_sendit;
                    } else {
                        return false;
                    }
                } else if (!strcmp(param, "signal")){
                    if (get_opus_signal(owner.source(), msg)){
                        goto codec_sendit;
                    } else {
                        return false;
                    }
                }
            }
        #endif
codec_sendit:
            owner.sendMsgNRT(msg);

            return false; // done
        });
}

void aoo_send_channel(AooSendUnit *unit, sc_msg_iter* args){
    auto cmd = UnitCmd::create(unit->mWorld, args);
    unit->delegate().doCmd(cmd,
        [](World *world, void *cmdData){
            auto data = (UnitCmd *)cmdData;
            auto& owner = static_cast<AooSend&>(*data->owner);

            sc_msg_iter args(data->size, data->data);
            skipUnitCmd(&args);

            aoo::ip_address addr;
            AooId id;
            if (owner.node()->getSinkArg(&args, addr, id)){
                auto channelOnset = args.geti();
                AooEndpoint ep { addr.address(), (AooAddrSize)addr.length(), id };
                owner.source()->setSinkChannelOnset(ep, channelOnset);
            }

            return false; // done
        });
}

void aoo_send_packetsize(AooSendUnit *unit, sc_msg_iter* args){
    unit->delegate().source()->setPacketSize(args->geti());
}

void aoo_send_ping(AooSendUnit *unit, sc_msg_iter* args){
    unit->delegate().source()->setPingInterval(args->getf());
}

void aoo_send_resend(AooSendUnit *unit, sc_msg_iter* args){
    auto cmd = CmdData::create<OptionCmd>(unit->mWorld);
    cmd->f = args->getf();
    unit->delegate().doCmd(cmd,
        [](World *world, void *cmdData){
            auto data = (OptionCmd *)cmdData;
            auto& owner = static_cast<AooSend&>(*data->owner);
            owner.source()->setResendBufferSize(data->f);

            return false; // done
        });
}

void aoo_send_redundancy(AooSendUnit *unit, sc_msg_iter* args){
    unit->delegate().source()->setRedundancy(args->geti());
}

void aoo_send_dll_bw(AooSendUnit *unit, sc_msg_iter* args){
    unit->delegate().source()->setDllBandwidth(args->getf());
}

using AooSendUnitCmdFunc = void (*)(AooSendUnit*, sc_msg_iter*);

// make sure that unit commands only run after the instance
// has been successfully initialized.
template<AooSendUnitCmdFunc fn>
void runUnitCmd(AooSendUnit* unit, sc_msg_iter* args) {
    if (unit->initialized() && unit->delegate().initialized()) {
        fn(unit, args);
    } else {
        LOG_WARNING("AooSend instance not initialized");
    }
}

#define AooUnitCmd(cmd) \
    DefineUnitCmd("AooSend", "/" #cmd, (UnitCmdFunc)runUnitCmd<aoo_send_##cmd>)

} // namespace

/*////////////////// Setup ///////////////////////*/

void AooSendLoad(InterfaceTable *inTable){
    ft = inTable;

    registerUnit<AooSendUnit>(ft, "AooSend");

    AooUnitCmd(add);
    AooUnitCmd(remove);
    AooUnitCmd(accept);
    AooUnitCmd(format);
    AooUnitCmd(codec_set);
    AooUnitCmd(codec_get);
    AooUnitCmd(channel);
    AooUnitCmd(packetsize);
    AooUnitCmd(ping);
    AooUnitCmd(resend);
    AooUnitCmd(redundancy);
    AooUnitCmd(dll_bw);
}
