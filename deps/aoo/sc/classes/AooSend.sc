AooSend : UGen {
	var <>desc;
	var <>tag;
	var <>port;
	var <>id;

	*ar { arg port, id=0, state=1, channels, tag;
		^this.multiNewList([\audio, tag, port, id, state] ++ channels);
	}
	*kr { ^this.shouldNotImplement(thisMethod) }

	init { arg tag ... args;
		this.tag = tag;
		this.port = args[0];
		this.id = args[1];
		inputs = args;
		^0; // doesn't have any output
	}

	optimizeGraph {
		Aoo.prMakeMetadata(this);
	}

	synthIndex_ { arg index;
		super.synthIndex_(index); // !
		// update metadata (ignored if reconstructing from disk)
		this.desc.notNil.if { this.desc.index = index; }
	}
}

AooSendCtl : AooCtl {
	classvar <>ugenClass;

	var <>sinks;

	*initClass {
		Class.initClassTree(AooSend);
		ugenClass = AooSend;
	}

	init { arg synth, synthIndex, port, id;
		super.init(synth, synthIndex, port, id);
		sinks = [];
	}

	prHandleEvent { arg type ...args;
		// /invite, /uninvite, /add, /remove, /ping
		// currently, all events start with endpoint + id
		var addr = this.prResolveAddr(AooAddr(args[0], args[1]));
		var id = args[2];
		var sink = ( addr: addr, id: id );
		var event = [type, addr, id] ++ args[3..];
		// If sink doesn't exist, fake an '/add' event.
		// This happens if the sink has been added
		// automatically before we could create the controller.
		(this.prFind(sink).isNil and: { type != '/add' }).if {
			this.prAdd(sink);
			this.eventHandler.value('/add', addr, id);
		};
		type.switch(
			'/add', { this.prAdd(sink) },
			'/remove', { this.prRemove(sink) }
		);
		^event;
	}

	add { arg addr, id, channelOnset, action;
		var replyID = AooCtl.prNextReplyID;
		addr = this.prResolveAddr(addr);
		this.prMakeOSCFunc({ arg ip, port, id;
			var newAddr;
			ip.notNil.if {
				newAddr = this.prResolveAddr(AooAddr(ip, port));
				this.prAdd(( addr: newAddr, id: id ));
				action.value(newAddr, id);
			} { action.value }
		}, '/aoo/add', replyID).oneShot;
		this.prSendMsg('/add', replyID, addr.ip, addr.port, id, channelOnset);
	}

	remove { arg addr, id, action;
		var replyID = AooCtl.prNextReplyID;
		addr = this.prResolveAddr(addr);
		this.prMakeOSCFunc({ arg ip, port, id;
			var newAddr;
			ip.notNil.if {
				newAddr = this.prResolveAddr(AooAddr(ip, port));
				this.prRemove(( addr: newAddr, id: id ));
				action.value(newAddr, id);
			} { action.value }
		}, '/aoo/remove', replyID).oneShot;
		this.prSendMsg('/remove', replyID, addr.ip, addr.port, id);
	}

	prAdd { arg sink;
		this.sinks = this.sinks.add(sink);
	}

	prRemove { arg sink;
		var index = this.sinks.indexOfEqual(sink);
		index !? { this.sinks.removeAt(index) };
	}

	prFind { arg sink;
		var index = this.sinks.indexOfEqual(sink);
		^index !? { this.sinks[index] };
	}

	removeAll { arg action;
		var replyID = AooCtl.prNextReplyID;
		this.prMakeOSCFunc({
			this.sinks = [];
			action.value;
		}, '/aoo/remove', replyID).oneShot;
		this.prSendMsg('/remove', replyID);
	}

	accept { arg enable;
		this.prSendMsg('/accept', enable);
	}

	format { arg fmt, action;
		var replyID = AooCtl.prNextReplyID;
		fmt.isKindOf(AooFormat).not.if {
			MethodError("aoo: bad type for 'fmt' parameter", this).throw;
		};
		this.prMakeOSCFunc({ arg codec ...args;
			var f = codec.switch(
				\pcm, { AooFormatPCM(*args) },
				\opus, { AooFormatOpus(*args) },
				{ "%: unknown format '%'".format(this.class.name, codec).error; nil }
			);
			action.value(f);
		}, '/aoo/format', replyID).oneShot;
		this.prSendMsg('/format', replyID, *fmt.asOSCArgArray);
	}

	channelOnset { arg addr, id, onset;
		addr = this.prResolveAddr(addr);
		this.prSendMsg('/channel', addr.ip, addr.port, id, onset);
	}

	packetSize_ { arg size;
		this.prSendMsg('/packetsize', size);
	}

	pingInterval_ { arg sec;
		this.prSendMsg('/ping', sec);
	}

	resendBufsize_ { arg sec;
		this.prSendMsg('/resend', sec);
	}

	redundancy_ { arg n;
		this.prSendMsg('/redundancy', n);
	}

	timeFilterBW_ { arg bw;
		this.prSendMsg('/timefilter', bw);
	}
}
