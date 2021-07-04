Aoo {
	classvar serverMap;

	*initClass {
		StartUp.add {
			serverMap = IdentityDictionary.new;
		}
	}

	*prMakeAddr {
		^{ arg break;
			var addr, port = rrand(49152, 65535);
			thisProcess.openUDPPort(port).if {
				addr = NetAddr(NetAddr.localAddr.ip, port);
				break.value(addr);
			};
		}.block;
	}

	*prGetServerAddr { arg server, action;
		var clientID, local, remote;
		remote = serverMap.at(server);
		remote.notNil.if {
			action.value(remote);
		} {
			local = NetAddr.localAddr;
			clientID = UniqueID.next;

			OSCFunc({ arg msg, time, addr;
				"client registered by %:%".format(addr.ip, addr.port).postln;
				serverMap.put(server, addr);
				action.value(addr);
			}, '/aoo/register', argTemplate: [clientID]).oneShot;

			server.sendMsg('/cmd', '/aoo_register',
				local.ip, local.port, clientID);
		}
	}

	*prMakeMetadata { arg ugen;
		var metadata, key = ugen.class.asSymbol;
		// For older SC versions, where metadata might be 'nil'
		ugen.synthDef.metadata ?? { ugen.synthDef.metadata = () };
		// Add metadata entry if needed:
		metadata = ugen.synthDef.metadata[key];
		metadata ?? {
			metadata = ();
			ugen.synthDef.metadata[key] = metadata;
		};
		ugen.desc = ( port: ugen.port, id: ugen.id );
		// There can only be a single AooSend without a tag. In this case, the metadata will contain
		// a (single) item at the pseudo key 'false', see ...
		ugen.tag.notNil.if {
			// check for AOO UGen without tag
			metadata.at(false).notNil.if {
				Error("SynthDef '%' contains multiple % instances - can't omit 'tag' argument!".format(ugen.synthDef.name, ugen.class.name)).throw;
			};
			// check for duplicate tagbol
			metadata.at(ugen.tag).notNil.if {
				Error("SynthDef '%' contains duplicate tag '%'".format(ugen.synthDef.name, ugen.tag)).throw;
			};
			metadata.put(ugen.tag, ugen.desc);
		} {
			// metadata must not contain other AooSend instances!
			(metadata.size > 0).if {
				Error("SynthDef '%' contains multiple % instances - can't omit 'tag' argument!".format(ugen.synthDef.name, ugen.class.name)).throw;
			};
			metadata.put(false, ugen.desc);
		};
	}

	*prFindUGens { arg class, synth, synthDef;
		var desc, metadata, ugens;
		// if the synthDef is nil, we try to get the metadata from the global SynthDescLib
		synthDef.notNil.if {
			metadata = synthDef.metadata;
		} {
			desc = SynthDescLib.global.at(synth.defName);
			desc.isNil.if { MethodError("couldn't find SynthDef '%' in global SynthDescLib!".format(synth.defName), this).throw };
			metadata = desc.metadata; // take metadata from SynthDesc, not SynthDef (SC bug)!
		};
		ugens = metadata[class.name.asSymbol];
		(ugens.size == 0).if { MethodError("SynthDef '%' doesn't contain any % instances!".format(synth.defName, class.name), this).throw; };
		^ugens;
	}

	*prFindMetadata { arg class, synth, tag, synthDef;
		var ugens, desc, info;
		ugens = Aoo.prFindUGens(class, synth, synthDef);
		tag.notNil.if {
			// try to find UGen with given tag
			tag = tag.asSymbol; // !
			desc = ugens[tag];
			desc ?? {
				MethodError("SynthDef '%' doesn't contain an % instance with tag '%'!".format(
					synth.defName, class.name, tag), this).throw;
			};
		} {
			// otherwise just get the first (and only) plugin
			(ugens.size > 1).if {
				MethodError("SynthDef '%' contains more than 1 % - please use the 'tag' argument!".format(synth.defName, class.name), this).throw;
			};
			desc = ugens.asArray[0];
		};
		^desc;
	}
}

AooFormat {
	classvar <>codec = \unknown;

	var <>channels;
	var <>blockSize;
	var <>sampleRate;

	asOSCArgArray {
		// replace 'nil' with 'auto' Symbol
		arg array = this.instVarSize.collect { arg i;
			this.instVarAt(i) ?? { \auto };
		};
		^[ this.class.codec ] ++ array;
	}

	printOn { arg stream;
		stream << this.class.name << "("
		<<* this.asOSCArgArray[1..] << ")";
	}
}

AooFormatPCM : AooFormat {
	classvar <>codec = \pcm;

	var <>bitDepth;

	*new { arg channels, blockSize, sampleRate, bitDepth;
		^super.newCopyArgs(channels, blockSize, sampleRate, bitDepth);
	}
}

AooFormatOpus : AooFormat {
	classvar <>codec = \opus;

	var <>bitRate;
	var <>complexity;
	var <>signalType;
	var <>applicationType;

	*new { arg channels, blockSize, sampleRate, bitRate, complexity, signalType, applicationType;
		^super.newCopyArgs(channels, blockSize, sampleRate, bitRate, complexity, signalType, applicationType);
	}
}

AooAddr {
	var <>ip;
	var <>port;

	*new { arg ip, port;
		^super.newCopyArgs(ip.asSymbol, port.asInteger);
	}

	== { arg that;
		that.isKindOf(AooAddr).if {
			^((that.ip == ip) and: { that.port == port });
		}
		^false;
	}

	matchItem { arg item;
		^(item == this);
	}

	hash {
		^this.instVarHash(#[\ip, \port])
	}

	printOn { arg stream;
		stream << this.class.name << "("
		<<* [ip, port] << ")";
	}
}

AooPeer : AooAddr {
	var <>group;
	var <>user;
	var <>id;

	*new { arg group, user;
		^super.newCopyArgs(nil, nil, group.asSymbol, user.asSymbol);
	}

	printOn { arg stream;
		stream << this.class.name << "("
		<<* [group, user, id, ip, port] << ")";
	}
}

// if you need to test for address/peer, func gets wrapped in this
AooAddrMessageMatcher : AbstractMessageMatcher {
	var addr;

	*new { arg addr, func;
		^super.newCopyArgs(func, addr);
	}

	value { arg msg, time, testPeer, recvPort;
		// compare by IP+port
		(testPeer == addr).if {
			func.value(msg, time, testPeer, recvPort);
		}
	}
}

AooPeerMessageMatcher : AbstractMessageMatcher {
	var peer;

	*new { arg peer, func;
		^super.newCopyArgs(func, peer);
	}

	value { arg msg, time, testPeer, recvPort;
		// compare by group+user.
		// this even works if IP address has changed!
		((testPeer.group == peer.group) and:
			{ testPeer.user == peer.user }).if {
			func.value(msg, time, testPeer, recvPort);
		}
	}
}

// NOTE: We can't pass the port number to OSCFunc,
// as it would try to open the port and throw an exception.
// Instead, every client has its own dispatcher instance.
// I think few people will use multiple AooClient instances
// simultaneously, and even fewer will complain that they
// can't have a single OSCFunc that matches all clients.
AooDispatcher : OSCMessageDispatcher {
	var <>client;

	*new { arg client;
		^super.new.init.client_(client);
	}

	wrapFunc {|funcProxy|
		var func, srcID, argTemplate;
		func = funcProxy.func;
		srcID = funcProxy.srcID;
		// ignore recvPort
		argTemplate = funcProxy.argTemplate;
		argTemplate.notNil.if {
			func = OSCArgsMatcher(argTemplate, func)
		};
		srcID.class.switch(
			AooPeer, { func = AooPeerMessageMatcher(srcID, func) },
			AooAddr, { func = AooAddrMessageMatcher(srcID, func) }
		);
		^func;
	}

	value { arg msg, time, addr, recvPort;
		var peer, realMsg, port = msg[1];
		((msg[0] == '/aoo/msg') and: { port == client.port }).if {
			peer = client.findPeer(AooAddr(msg[2], msg[3]));
			peer.notNil.if {
				realMsg = msg[4..];
				active[realMsg[0]].value(realMsg, time, peer, port);
			} {
				"%: got message from unknown peer".format(this.class.name).warn;
			}
		}
	}
}

AooCtl {
	classvar nextReplyID=0;
	// public
	var <>synth;
	var <>synthIndex;
	var <>port;
	var <>id;
	var <>replyAddr;
	var <>eventHandler;

	var eventOSCFunc;

	*new { arg synth, tag, synthDef, action;
		var data = Aoo.prFindMetadata(this.ugenClass, synth, tag, synthDef);
		var result = super.new.init(synth, data.index, data.port, data.id);
		action !? {
			forkIfNeeded {
				synth.server.sync;
				action.value(result);
			};
		};
		^result;
	}

	*collect { arg synth, tags, synthDef, action;
		var result = ();
		var ugens = Aoo.prFindUGens(this.ugenClass, synth, synthDef);
		tags.notNil.if {
			tags.do { arg key;
				var value;
				key = key.asSymbol; // !
				value = ugens.at(key);
				value.notNil.if {
					result.put(key, this.new(synth, value.index));
				} { "can't find % with tag %".format(this.name, key).warn; }
			}
		} {
			// get all plugins, except those without tag (shouldn't happen)
			ugens.pairsDo { arg key, value;
				(key.class == Symbol).if {
					result.put(key, this.new(synth, value.index, value.port, value.id));
				} { "ignoring % without tag".format(this.ugenClass.name).warn; }
			}
		};
		action !? {
			forkIfNeeded {
				synth.server.sync;
				action.value(result);
			};
		};
		^result;
	}

	init { arg synth, synthIndex, port, id;
		this.synth = synth;
		this.synthIndex = synthIndex;
		this.port = port;
		this.id = id;

		Aoo.prGetServerAddr(synth.server, { arg addr;
			// "listen: % % %".format(addr, synth.nodeID, synthIndex).postln;
			replyAddr = addr;
			eventOSCFunc = OSCFunc({ arg msg;
				var event = this.prHandleEvent(*msg[3..]);
				this.eventHandler.value(*event);
			}, '/aoo/event', addr, argTemplate: [synth.nodeID, synthIndex]);
		});

		synth.onFree {
			this.free;
		};
	}

	free {
		eventOSCFunc.free;
		synth = nil;
	}

	*prNextReplyID {
		^nextReplyID = nextReplyID + 1;
	}

	prSendMsg { arg cmd... args;
		synth.server.listSendMsg(this.prMakeMsg(cmd, *args));
	}

	prMakeMsg { arg cmd ... args;
		^['/u_cmd', synth.nodeID, synthIndex, cmd] ++ args;
	}

	prMakeOSCFunc { arg func, path, replyID;
		^OSCFunc({ arg msg;
			func.value(*msg[4..]); // pass arguments after replyID
		}, path, replyAddr, argTemplate: [synth.nodeID, synthIndex, replyID]);
	}

	// Try to find peer, even if IP/port is given.
	prResolveAddr { arg addr;
		var client, peer;
		// find peer by group+user
		client = AooClient.find(this.port);
		client.notNil.if {
			peer = client.findPeer(addr);
			peer !? { ^peer };
		};
		// we need at least IP+port
		addr.ip !? { ^addr };
		addr.isKindOf(AooPeer).if {
			MethodError("Aoo: couldn't find peer %".format(addr), this).throw;
		} {
			MethodError("Aoo: bad address %".format(addr), this).throw;
		}
	}
}
