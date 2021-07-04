AooClient {
	classvar <>clients;

	var <>server;
	var <>port;
	var <>id;
	var <>state;
	var <>replyAddr;
	var <>nodeAddr;
	var <>eventHandler;
	var <>dispatcher;
	var <>peers;

	var eventOSCFunc;

	*initClass {
		clients = IdentityDictionary.new;
	}

	*find { arg port;
		^clients[port];
	}

	*new { arg port, server, action;
		^super.new.init(port, server, action);
	}

	init { arg port, server, action;
		this.server = server ?? Server.default;
		this.peers = [];
		this.state = \disconnected;
		this.dispatcher = AooDispatcher(this);

		Aoo.prGetServerAddr(this.server, { arg addr;
			this.replyAddr = addr;
			this.nodeAddr = NetAddr("localhost", port);
			// handle events
			eventOSCFunc = OSCFunc({ arg msg;
				var event = this.prHandleEvent(*msg[2..]);
				this.eventHandler.value(*event);
			}, '/aoo/client/event', addr, argTemplate: [port]);
			// open client on the server
			OSCFunc({ arg msg;
				var success = msg[2].asBoolean;
				success.if {
					this.port = port;
					action.value(this);
				} {
					"couldn't create AooClient on port %: %".format(port, msg[3]).error;
					action.value(nil);
				};
			}, '/aoo/client/new', addr, argTemplate: [port]).oneShot;
			this.server.sendMsg('/cmd', '/aoo_client_new', port);
		});

		ServerQuit.add { this.free };
		CmdPeriod.add { this.free };
	}

	prHandleEvent { arg type ...args;
		// /disconnect, /peer/join, /peer/leave, /error
		var peer;

		type.switch(
			'/disconnect', {
				"disconnected from server".error;
			},
			'/error', {
				"AooClient: % (%)".format(args[1], args[0]).error;
			},
			{
				peer = AooPeer.newCopyArgs(*args);
				type.switch(
					'/peer/join', { this.prAdd(peer) },
					'/peer/leave', { this.prRemove(peer) }
				);
				^[type, peer] // return modified event
			}
		);
		^[type] ++ args; // return original event
	}

	prAdd { arg peer;
		this.peers = this.peers.add(peer);
	}

	prRemove { arg peer;
		var index = this.peers.indexOfEqual(peer);
		index !? { this.peers.removeAt(index) };
	}

	prRemoveGroup { arg group;
		group = group.asSymbol;
		this.peers = this.peers.select { arg p; p.group != group };
	}

	free {
		eventOSCFunc.free;
		port.notNil.if {
			server.sendMsg('/cmd', '/aoo_client_free', port);
		};
		server = nil;
		port = nil;
		peers = nil;
	}

	connect { arg serverName, serverPort, userName, userPwd, groupName, groupPwd, action, timeout=10;
		var resp;

		port ?? { MethodError("AooClient: not initialized", this).throw };

		state.switch(
			\connected, { "AooClient: already connected".warn; ^this },
			\connecting, { "AooClient: still connecting".warn ^this }
		);
		state = \connecting;

		resp = OSCFunc({ arg msg;
			var errmsg, userid, success = msg[2].asBoolean;
			success.if {
				userid = msg[3];
				"AooClient: connected to % % (user ID: %)".format(serverName, serverPort, userid).postln;
				state = \connected;
				groupName.notNil.if {
					this.joinGroup(groupName, groupPwd, { arg success;
						action.value(success, userid);
					});
				} { action.value(true, userid); }
			} {
				errmsg = msg[3];
				"AooClient: couldn't connect to % %: %".format(serverName, serverPort, errmsg).error;
				state = \disconnected;
				action.value(false);
			};
		}, '/aoo/client/connect', replyAddr, argTemplate: [port]).oneShot;

		// NOTE: the default timeout should be larger than the default
		// UDP handshake time out.
		// We need the timeout in case a reply message gets lost and
		// leaves the client in limbo...
		SystemClock.sched(timeout, {
			(state == \connecting).if {
				"AooClient: connection time out".error;
				resp.free;
				state = \disconnected;
				action.value(false, "time out");
			}
		});

		server.sendMsg('/cmd', '/aoo_client_connect', port,
			serverName, serverPort, userName, userPwd);
	}

	disconnect { arg action;
		port ?? { MethodError("AooClient not initialized", this).throw };

		(state != \connected).if {
			"AooClient not connected".warn;
			^this;
		};
		OSCFunc({ arg msg;
			var success = msg[2].asBoolean;
			var errmsg = msg[3];
			success.if {
				this.peers = []; // remove all peers
				this.id = nil;
				"AooClient: disconnected".postln;
			} {
				"AooClient: couldn't disconnect: %".format(errmsg).error;
			};
			state = \disconnected;
			action.value(success);
		}, '/aoo/client/disconnect', replyAddr, argTemplate: [port]).oneShot;
		server.sendMsg('/cmd', '/aoo_client_disconnect', port);
	}

	joinGroup { arg name, pwd, action;
		port ?? { MethodError("AooClient not initialized", this).throw };

		OSCFunc({ arg msg;
			var success = msg[3].asBoolean;
			var errmsg = msg[4];
			success.if {
				"AooClient: joined group '%'".format(name).postln;
			} {
				"AooClient: couldn't join group '%': %".format(name, errmsg).error;
			};
			action.value(success);
		}, '/aoo/client/group/join', replyAddr, argTemplate: [port, name.asSymbol]).oneShot;
		server.sendMsg('/cmd', '/aoo_client_group_join', port, name, pwd);
	}

	leaveGroup { arg name, action;
		port ?? { MethodError("AooClient not initialized", this).throw };

		OSCFunc({ arg msg;
			var success = msg[3].asBoolean;
			var errmsg = msg[4];
			success.if {
				"AooClient: left group '%'".format(name).postln;
				this.prRemoveGroup(name);
			} {
				"AooClient: couldn't leave group '%': %".format(name, errmsg).error;
			};
			action.value(success);
		}, '/aoo/client/group/leave', replyAddr, argTemplate: [port, name.asSymbol]).oneShot;
		server.sendMsg('/cmd', '/aoo_client_group_leave', port, name);
	}

	findPeer { arg addr;
		addr.isKindOf(AooPeer).if {
			// find by group+user
			peers.do { arg peer;
				((peer.group == addr.group) && (peer.user == addr.user)).if { ^peer };
			}
		} {
			// AooAddr, find by IP+port
			peers.do { arg peer;
				(addr == peer).if { ^peer }
			}
		};
		^nil;
	}

	send { arg msg, target;
		var raw, args, newMsg;
		target.notNil.if {
			target.isKindOf(AooAddr).if {
				// peer
				target = this.prResolveAddr(target);
				args = [target.ip, target.port];
			} {
				// group
				args = target.asSymbol;
			};
		}; // else: broadcast
		raw = msg.asRawOSC;
		newMsg = ['/sc/msg', raw, 0] ++ args;
		// newMsg.postln;
		// OSC bundles begin with '#' (ASCII code 35)
		(raw[0] == 35).if {
			// Schedule on the current system time.
			// On the Server, we add the relative
			// timestamp contained in the bundle
			nodeAddr.sendBundle(0, newMsg);
		} {
			// send immediately
			nodeAddr.sendMsg(*newMsg);
		}
	}

	// Try to find peer, but only if no IP/port is given.
	// So far only called by send().
	prResolveAddr { arg addr;
		var peer;
		addr.ip !? { ^addr; };
		peer = this.findPeer(addr);
		peer !? { ^peer; };
		addr.isKindOf(AooPeer).if {
			MethodError("%: couldn't find peer %".format(this.class.name, addr), this).throw;
		} {
			MethodError("%: bad address %".format(this.class.name, addr), this).throw;
		}
	}
}
