# AOO v2.0 - message protocol

<br>

# 1 AOO peer-to-peer communication (UDP)

<br>

The following messages are exchanged between AOO sources and AOO sinks resp. between AOO clients via UDP sockets.

<br>

## 1.1 AOO source-sink communication

<br>

Messages exchanged between AOO sources and AOO sinks.

<br>

## `/aoo/sink/<id>/start`

Start a new stream.

### Arguments:

| type  | description                      |
| ----: | -------------------------------- |
|  `i`  | source ID                        |
|  `s`  | version string                   |
|  `i`  | stream ID                        |
|  `i`  | format ID                        |
|  `i`  | number of channels               |
|  `i`  | sample rate                      |
|  `i`  | block size                       |
|  `s`  | codec name                       |
|  `b`  | codec extension                  |
| [`i`] | [metadata type](#4.2-data-types) |
| [`b`] | metadata content                 |

---

## `/aoo/source/<id>/start`

Request a start message.

### Arguments:

| type | description    |
| ---: | -------------- |
|  `i` | sink ID        |
|  `s` | version string |

---

## `/aoo/sink/<id>/stop`

Stop a stream.

### Arguments:

| type | description |
| ---: | ----------- |
|  `i` | source ID   |
|  `i` | stream ID   |

---

## `/aoo/source/<id>/stop`

Request a stop message.

### Arguments:

| type | description |
| ---: | ----------- |
|  `i` | source ID   |
|  `i` | stream ID   |

---

## `/aoo/sink/<id>/data`

Send stream data.

### Arguments:

| type | description            |
| ---: | ---------------------- |
|  `i` | source ID              |
|  `i` | stream ID              |
|  `i` | sequence number        |
|  `d` | real sample rate       |
|  `i` | channel onset          |
|  `i` | total data size        |
|  `i` | message data size      |
|  `i` | total number of frames |
|  `i` | frame index            |
|  `b` | data content           |

---

## `/aoo/source/<id>/data`

Request stream data.

### Arguments:

| type  | description   |
| ----: | ------------- |
|  `i`  | source ID     |
|  `i`  | stream ID     |
|  `i`  | sequence 1    |
|  `i`  | frame 1       |
| [`i`] | sequence 2    |
| [`i`] | frame 2       |
|  ...  | ...           |

---

## `/aoo/source/<id>/invite`

Invite a source.

### Arguments:

| type  | description                      |
| ----: | -------------------------------- |
|  `i`  | sink ID                          |
|  `i`  | stream ID                        |
| [`i`] | [metadata type](#4.2-data-types) |
| [`b`] | metadata content                 |

---

## `/aoo/source/<id>/uninvite`

Uninvite a source.

### Arguments:

| type | description |
| ---: | ----------- |
|  `i` | sink ID     |
|  `i` | stream ID   |

---

## `/aoo/sink/<id>/decline`

Decline an invitation.

### Arguments:

| type | description |
| ---: | ----------- |
|  `i` | source ID   |
|  `i` | stream ID   |

---

## `/aoo/[source|sink]/<id>/ping`

Send a ping.

### Arguments:

| type | description    |
| ---: | -------------- |
|  `i` | sink/source ID |
|  `i` | token          |
|  `t` | timestamp      |

---

## `/aoo/[source|sink]/<id>/pong`

Reply to ping message.

### Arguments:

| type  | description           |
| ----: | --------------------- |
|  `i`  | sink/source ID        |
|  `t`  | timestamp 1 (remote)  |
|  `t`  | timestamp 2 (local)   |
| [`f`] |packet loss percentage |

<br>

## 1.2 AOO peer communication

<br>

Messages exchanged between AOO peers.

<br>

## `/aoo/peer/ping`

Send a ping to a peer.

### Arguments:

| type | description      |
| ---: | ---------------- |
|  `i` | group ID         |
|  `i` | user ID (sender) |
|  `t` | timestamp        |

Note: *timestamp* is empty (0) in handshake pings.

---

## `/aoo/peer/pong`

Reply to ping message.

### Arguments:

| type | description          |
| ---: | -------------------- |
|  `i` | group ID             |
|  `i` | user ID (sender)     |
|  `t` | timestamp 1 (remote) |
|  `t` | timestamp 2 (local)  |

Note: Both *timestamps* are empty (0) in handshake pongs.

---

## `/aoo/peer/msg`

Send a message to a peer.

### Arguments:

| type | description                      |
| ---: | -------------------------------- |
|  `i` | group ID                         |
|  `i` | peer ID (sender)                 |
|  `i` | flags                            |
|  `i` | sequence number                  |
|  `i` | total message size               |
|  `i` | number of frames                 |
|  `i` | frame index                      |
|  `t` | timetag                          |
|  `i` | [message type](#4.2-data-types)  |
|  `b` | message content                  |

Possible values for *flags*:

- `0x01`: reliable transmission

---

## `/aoo/peer/ack`

Send message acknowledgements.

### Arguments:

| type  | description      |
| ----: | ---------------- |
|  `i`  | group ID         |
|  `i`  | peer ID (sender) |
|  `i`  | sequence 1       |
|  `i`  | frame 1          |
| [`i`] | sequence 2       |
| [`i`] | frame 2          |
|  ...  | ...              |

<br>

# 2 AOO client-server communication (UDP/TCP)

<br>

The following messages are exchanged between an AOO client and an AOO server

<br>

## 2.1 NAT traversal (UDP)

<br>

The following messages are used for NAT traversal resp. UDP hole punching.

<br>

## `/aoo/server/query`

Query the public IPv4 address and port.

### Arguments:

None

---

## `/aoo/client/query`

Reply to a `/query` message.

### Arguments:

| type | description           |
| ---: | --------------------- |
|  `s` | public IP address     |
|  `i` | public port           |

---

## `/aoo/server/ping`

Send a ping message to keep the port open.

### Arguments:

None

---

## `/aoo/client/pong`

Reply to a `/ping` message.

### Arguments:

None

<br>

## 2.2 Client requests and server responses (TCP)

<br>

Client requests and server responses over a TCP socket connection.

<br>

## `/aoo/server/login`

Login to server.

### Arguments:

| type  | description                      |
| ----: | -------------------------------- |
|  `i`  | request token                    |
|  `s`  | version string                   |
|  `s`  | password (encrypted)             |
|  `i`  | address count                    |
|  `s`  | IP address 1                     |
|  `i`  | port 1                           |
| [`s`] | IP address 2                     |
| [`i`] | port 2                           |
|  ...  | ...                              |
| [`i`] | [metadata type](#4.2-data-types) |
| [`b`] | metadata content                 |

---

## `/aoo/client/login`

Login response.

### Arguments:

1. success:

    | type  | description                      |
    | ----: | -------------------------------- |
    |  `i`  | request token                    |
    |  `i`  | 0 (= no error)                   |
    |  `s`  | version string                   |
    |  `i`  | client ID                        |
    |  `i`  | flags                            |
    | [`i`] | [metadata type](#4.2-data-types) |
    | [`b`] | metadata content                 |

2. failure: see [3.1.1 Error response](#2.1.1-error-response)

Possible values for *flags*:

- 0x01: server relay supported

---

## `/aoo/server/group/join`

Join a group on the server.

### Arguments:

| type  | description                            |
| ----: | -------------------------------------- |
|  `i`  | request token                          |
|  `s`  | group name                             |
|  `s`  | group password (encrypted)             |
|  `s`  | user name                              |
|  `s`  | user password (encrypted)              |
| [`i`] | group [metadata type](#4.2-data-types) |
| [`b`] | group metadata content                 |
| [`i`] | user [metadata type](#4.2-data-types)  |
| [`b`] | user metadata content                  |
| [`s`] | relay hostname                         |
| [`i`] | relay port                             |

---

## `/aoo/client/group/join`

Group join response.

### Arguments:

1. success:

    | type  | description                              |
    | ----: | ---------------------------------------- |
    |  `i`  | request token                            |
    |  `i`  | 0 (= no error)                           |
    |  `i`  | group ID                                 |
    |  `i`  | group flags                              |
    |  `i`  | user ID                                  |
    |  `i`  | user flags                               |
    | [`i`] | group [metadata type](#4.2-data-types)   |
    | [`b`] | group metadata content                   |
    | [`i`] | user [metadata type](#4.2-data-types)    |
    | [`b`] | user metadata content                    |
    | [`i`] | private [metadata type](#4.2-data-types) |
    | [`b`] | private metadata content                 |
    | [`s`] | group relay hostname                     |
    | [`i`] | group relay port                         |

2. failure: see [3.1.1 Error response](#2.1.1-error-response)

---

## `/aoo/server/group/leave`

Leave a group on the server.

### Arguments:

| type | description   |
| ---: | ------------- |
|  `i` | request token |
|  `i` | group ID      |

---

## `/aoo/client/group/leave`

Group leave response.

### Arguments:

1. success:

    | type | description    |
    | ---: | -------------- |
    |  `i` | request token  |
    |  `i` | 0 (= no error) |

2. failure: see [3.1.1 Error response](#2.1.1-error-response)

---

## `/aoo/server/group/update`

Update group metadata.

### Arguments:

| type | description                      |
| ---: | -------------------------------- |
|  `i` | request token                    |
|  `i` | group ID                         |
|  `i` | [metadata type](#4.2-data-types) |
|  `b` | metadata content                 |

---

## `/aoo/client/group/update`

Group update response.

### Arguments:

1. success:

    | type | description                      |
    | ---: | -------------------------------- |
    |  `i` | request token                    |
    |  `i` | 0 (= no error)                   |
    |  `i` | [metadata type](#4.2-data-types) |
    |  `b` | metadata content                 |

2. failure: see [3.1.1 Error response](#2.1.1-error-response)

---

## `/aoo/server/user/update`

Update user metadata.

### Arguments:

| type | description                      |
| ---: | -------------------------------- |
|  `i` | request token                    |
|  `i` | group ID                         |
|  `i` | [metadata type](#4.2-data-types) |
|  `b` | metadata content                 |

---

## `/aoo/client/user/update`

User update response.

### Arguments:

1. success:

    | type | description                      |
    | ---: | -------------------------------- |
    |  `i` | request token                    |
    |  `i` | 0 (= no error)                   |
    |  `i` | [metadata type](#4.2-data-types) |
    |  `b` | metadata content                 |

2. failure: see [3.1.1 Error response](#2.1.1-error-response)

---

## `/aoo/server/request`

Send custom request.

### Arguments:

| type | description                  |
| ---: | ---------------------------- |
|  `i` | request token                |
|  `i` | flags                        |
|  `i` | [data type](#4.2-data-types) |
|  `b` | data content                 |

---

## `/aoo/client/request`

Response to custom request.

### Arguments:

1. success:

    | type | description                  |
    | ---: | ---------------------------- |
    |  `i` | request token                |
    |  `i` | 0 (= no error)               |
    |  `i` | flags                        |
    |  `i` | [data type](#4.2-data-types) |
    |  `b` | data content                 |

2. failure: see [3.1.1 Error response](#2.1.1-error-response)

<br>

## 2.1.1 Error response

All error responses have the same argument structure:

| type | description                    |
| ---: | ------------------------------ |
|  `i` | request token                  |
|  `i` | [error code](#5.1-error-codes) |
|  `i` | system/user error code         |
|  `s` | system/user error message      |

<br>

## 2.3 Server notifications (TCP)

<br>

The following messages are sent by the server over a TCP connection to notify one or more clients.

<br>

## `/aoo/client/peer/join`

A peer has joined the group.

### Arguments:

| type  | description                      |
| ----: | -------------------------------- |
|  `s`  | group name                       |
|  `i`  | group ID                         |
|  `s`  | user name                        |
|  `i`  | user ID                          |
|  `s`  | version string                   |
|  `i`  | flags                            |
|  `i`  | address count                    |
|  `s`  | IP address 1                     |
|  `i`  | port 1                           |
| [`s`] | IP address 2                     |
| [`i`] | port 2                           |
|  ...  | ...                              |
| [`i`] | [metadata type](#4.2-data-types) |
| [`b`] | metadata content                 |
| [`s`] | relay hostname                   |
| [`i`] | relay port                       |

---

## `/aoo/client/peer/leave`

A peer has left the group.

### Arguments:

| type | description |
| ---: | ----------- |
|  `i` | group ID    |
|  `i` | peer ID     |

---

## `/aoo/client/peer/changed`

Peer metadata has changed.

### Arguments:

| type | description                      |
| ---: | -------------------------------- |
|  `i` | group ID                         |
|  `i` | peer ID                          |
|  `i` | [metadata type](#4.2-data-types) |
|  `b` | metadata content                 |

---

## `/aoo/client/user/changed`

User metadata has changed.

### Arguments:

| type | description                      |
| ---: | -------------------------------- |
|  `i` | group ID                         |
|  `i` | user ID                          |
|  `i` | [metadata type](#4.2-data-types) |
|  `b` | metadata content                 |

---

## `/aoo/client/group/changed`

Group metadata has changed.

### Arguments:

| type | description                      |
| ---: | -------------------------------- |
|  `i` | group ID                         |
|  `i` | user ID                          |
|  `i` | [metadata type](#4.2-data-types) |
|  `b` | metadata content                 |

The user ID refers to the user that updated the group.
-1 means that the group has been updated by the server.

---

## `/aoo/client/group/eject`

Ejected from group.

### Arguments:

| type | description                      |
| ---: | -------------------------------- |
|  `i` | group ID                         |

---

## `/aoo/client/msg`

Generic server notification.

### Arguments:

| type | description                      |
| ---: | -------------------------------- |
|  `i` | [metadata type](#4.2-data-types) |
|  `b` | message content                  |

<br>

# 3 Relay (UDP)

<br>

## `/aoo/relay`

A relayed message.

If sent from an AOO peer to an AOO relay server, the message contains the destination endpoint address (IP address + port) together with the original message data. The server extracts the destination endpoint address, replaces it with the source endpoint address and forwards the message to the destination. The destination extracts the source endpoint address and message data and forwards the original message to the appropriate receiver.

### Arguments:

| type | description |
| ---: | ----------- |
|  `s` | IP address  |
|  `i` | port        |
|  `b` | message     |

<br>

# 4 Constants

<br>

### 4.1 Error codes

| value | description                |
| ----: | -------------------------- |
|    -1 | Unspecified                |
|     0 | No error                   |

For a full list of error codes see `AooError` in `aoo/aoo_types.h`.

<br>

### 4.2 Data types

| value | description              |
| ----: | ------------------------ |
|    -1 | Unspecified              |
|     0 | Raw/binary data          |
|     1 | plain text (UTF-8)       |
|     2 | OSC (Open Sound Control) |
|     3 | MIDI                     |
|     4 | FUDI (Pure Data)         |
|     5 | JSON (UTF-8)             |
|     6 | XML (UTF-8)              |
| 1000< | User specified           |

<br>

### 4.3 Codec names

| value  | description |
| ------ | ----------- |
| "pcm"  | PCM codec   |
| "opus" | Opus codec  |

<br>

# 5 Binary messages

<br>

TODO
