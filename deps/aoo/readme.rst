Audio over OSC based audio streaming
====================================

"Audio over OSC" aka *AoO* is aimed to be a message based audio system inspired by Open Sound Control OSC_ as a syntax format. It is dedicated to send audio in real time from arbitrary sources to arbitrary sinks on demand.

history
-------

A first vision of *AoO* has came up in 2009, a first implementation v1.0-b2 as a library with externals for PureData (Pd) has been done 2010, but major issues with needed networking objects made this version unpracticable and was not used extensively.
More on this version of AoO as "message based audio system"" was published at LAC 2014 [LAC14]_

The new Version 2.0, not backwards compatible, was done quite from scratch and has been launched in february 2020, targeting a network streaming Project for Kunsthaus Graz for Bill Fontana using a independent wireless network infrastructure FunkFeuer Graz 0xFF_ and also reviving the Virtual IEM Computer Music Ensemble VICE_ within a seminar on IEM_ Graz.

Based on the *AoO* idea of Winfried Ritsch with a first draft of realisation on embedded devices, the actual version starting with v2.0 , was mainly written by Christof Ressi, as a complete rewrite of the version from Wolfgang Jäger in C in 2010.

collected features
------------------

* peer-to-peer audio networks of any topology with arbitrary ad-hoc connections
* each endpoint can have multiple sources/sinks (each with their own ID)
* AoO sources can send audio to any sink at any time; 
* AoO sinks can listen to several sources at the same time, summing the signals
* AoO is connectionless: streams can start/stop at any time, enabling a "message-based audio" approach.
* AoO sinks can "invite" sources, i.e. ask them to send audio. The source may follow the invitation or decline it.
* AoO sinks and sources can operate at different blocksizes and samplerates
* AoO sources can dynamically change the channel onset
* AoO is internet time based, with means all signal from the same timestamp are added correctly at the same point in the receiver.
* timing differences (e.g. because of clock drifts) are adjusted via a time DLL filter + dynamic resampling
* the stream format can be set independently for each source
* plugin API to register codecs; currently only PCM (uncompressed) and Opus (compressed) are implemented
* aoo_source and aoo_sink C++ classes use various lock-free buffers, so that audio processing and network IO
  can run on different threads without having to wait for each other
* aoo_sink buffer helps to deal with network jitter, packet reordering
  and packet loss at the cost of latency. The size can be adjusted dynamically.
* aoo_sink can ask the source(s) to resend dropped packets, the settings are free adjustable.
* settable UDP packet size (to optimize for local networks or the internet)
* ping messages are automatically sent between sources and sinks at a configurable rate.
  For example, sources might want to stop sending if they don't receive a ping from a sink within a certain time.
  The pings also carry timestamps, so the user can calculate the network latency and perform latency compensation.
* Several diagnostic events about packet loss, packet reordering, large dropouts, etc.


installation
------------

Repository: http://git.iem.at/cm/aoo/

from source
...........

Get the source:

over ssh::

  git clone git@git.iem.at:cm/aoo.git

or https::

  git clone https://git.iem.at/cm/aoo.git

Get the Opus library:

* macOS -> homebrew: "brew install opus".

* Windows -> Msys2: "pacman -S mingw32/mingw-w64-i686-opus" (32 bit) / "pacman -S mingw64/mingw-w64-x86_64-opus" (64 bit)

* Linux -> apt: "apt-get install libopus-dev"

make it (using pd-libbuilder)::

  make -C aoo/pd clean
  make -C aoo/pd -j4

install eg. in aoo dir::

  make -C aoo/pd PDLIBDIR=./ install

see aoo_send~-help.pd and aoo_receive~-help.pd for usage instructions

from deken
..........

in Pd->Help->"find in externals" enter aoo

Note: TO BE DONE

Pd externals
------------

see help- and test-patches there for more information.

Main objects

* [aoo_send~] send an AoO stream (with integrated threaded network IO)

* [aoo_receive~] receive one or more AoO streams (with integrated threaded network IO)

Supplementary objects to use AoO with existing Pd network objects (not recommended)
    
* [aoo_pack~] takes audio signals and outputs OSC messages (also accepts /request messages from sinks)
* [aoo_unpack~] takes OSC messages from several sources and turns them into audio signals
* [aoo_route] takes OSC messages and routes them based on the type (source/sink) and ID

todo
----

* publish
* more examples on features especial invite/uninvite
* relay for AoO server

download
--------

main git repository at git.iem.at:

git clone https://git.iem.at/cm/aoo

content
-------

doku -- documentation, papers
 
pd -- Pd library for OSC, first implementation for experiments

lib -- C++ library with a C interface, create and manage AoO sources/sinks

About Document
--------------
:authors: Winfried Ritsch, Christof Ressi
:date: march 2014 - february 2020
:version: 2.0-a1

.. _OSC: http://opensoundcontrol.org/

.. _Pd: http://puredata.info/

.. _0xFF: http://graz.funkfeuer.at/

.. _VICE: https://iaem.at/projekte/ice/overview

.. _IEM: http://iem.at/

.. [LAC14] see docu/lac2014_aoo.pdf
