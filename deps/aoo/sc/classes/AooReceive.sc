AooReceive : MultiOutUGen {
	var <>desc;
	var <>tag;
	var <>port;
	var <>id;

	*ar { arg port, id=0, numChannels=1, bufsize, tag;
		^this.multiNewList([\audio, tag, port, id, numChannels, bufsize]);
	}
	*kr { ^this.shouldNotImplement(thisMethod) }

	init { arg tag, port, id, numChannels, bufsize;
		this.tag = tag;
		this.port = port;
		this.id = id;
		inputs = [port, id, bufsize ?? 0];
		^this.initOutputs(numChannels, rate)
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

AooReceiveCtl : AooCtl {
	classvar <>ugenClass;

	var <>sources;

	*initClass {
		Class.initClassTree(AooReceive);
		ugenClass = AooReceive;
	}

	init { arg synth, synthIndex, port, id;
		super.init(synth, synthIndex, port, id);
		sources = [];
	}

	prHandleEvent { arg type ... args;
		// /format, /add, /state, /block/*, /ping
		// currently, all events start with endpoint + id
		var addr = this.prResolveAddr(AooAddr(args[0], args[1]));
		var id = args[2];
		var source = ( addr: addr, id: id );
		var codec, fmt;
		var event = (type == '/format').if {
			// make AooFormat object from OSC args
			codec = args[3];
			fmt = codec.switch(
				\pcm, { AooFormatPCM(*args[4..]) },
				\opus, { AooFormatOpus(*args[4..]) },
				{ "%: unknown format '%'".format(this.class.name, codec).error; nil }
			);
			[type, addr, id] ++ fmt;
		} {
			// pass OSC args unchanged
			[type, addr, id] ++ args[3..]
		};
		// If source doesn't exist, fake an '/add' event.
		// This happens if the source has been added
		// before we could create the controller.
		(this.prFind(source).isNil and: { type != '/add' }).if {
			this.prAdd(source);
			this.eventHandler.value('/add', addr, id);
		};
		type.switch(
			'/add', { this.prAdd(source) },
			'/remove', { this.prRemove(source) }
		);
		^event;
	}

	prAdd { arg source;
		this.sources = this.sources.add(source);
	}

	prRemove { arg source;
		var index = this.sources.indexOfEqual(source);
		index !? { this.sources.removeAt(index) };
	}

	prFind { arg source;
		var index = this.sources.indexOfEqual(source);
		^index !? { this.sources[index] };
	}

	invite { arg addr, id;
		addr = this.prResolveAddr(addr);
		this.prSendMsg('/invite', nil, addr.ip, addr.port, id);
	}

	uninvite { arg addr, id;
		addr = this.prResolveAddr(addr);
		this.prSendMsg('/uninvite', nil, addr.ip, addr.port, id);
	}

	uninviteAll {
		this.prSendMsg('/uninvite');
	}

	packetSize_ { arg size;
		this.prSendMsg('/packetsize',size);
	}

	bufsize_ { arg sec;
		this.prSendMsg('/bufsize', sec);
	}

	redundancy_ { arg n;
		this.prSendMsg('/redundancy', n);
	}

	timeFilterBW_ { arg bw;
		this.prSendMsg('/timefilter', bw);
	}

	reset { arg addr, id;
		addr = this.prResolveAddr(addr);
		this.prSendMsg('/reset', addr.ip, addr.port, id);
	}

	resetAll {
		this.prSendMsg('/reset');
	}

	resend_ { arg enable;
		this.prSendMsg('/resend', nil, enable);
	}

	resendLimit_ { arg limit;
		this.prSendMsg('/resend_limit', nil, limit);
	}

	resendInterval_ { arg sec;
		this.prSendMsg('/resend_interval', nil, sec);
	}
}
