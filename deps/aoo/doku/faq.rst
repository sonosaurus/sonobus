FAQ collection, maybe goes into doku
====================================


Functionality
-------------

What is the lowest Latency possible ?
.....................................

Short answer: 
 AoO does have a bit more latency than [pd~] because of the extra buffer in [aoo_receive~]. 

Long answer:
    There are basically three parameters regarding the latency in the case of Pd externals and AoO:

    a) network latency: negligible for localhost, small for LAN,  notible for WAN, worse for WLAN

    b) hardware buffer size / Pd latency depends on your hardware setup

    c) [aoo_receive~] buffer size needed to catch the slowest

For example on my Windows 7 laptop with an ASIO driver and 64 sample hardware buffer size and 5ms Pd latency, I can set the [aoo_receive~] buffer size down to 4-5 ms. On some other systems, you might get even lower. One of the use cases of AOO is certainly low latency audio streaming over local networks, especially over long periods of time (literally weeks or months, given that the devices have access to a NTP time server). Here routers and long ranges introduces latency, where mostly older WLAN devices on 2.4Ghz can introduce most of the latency. So better use Ethernet cables if latency matters.


How to punch holes into firewalls ?
...................................

 Hole-punching is used by most Streaming platforms serving clients behind firewalls. 
 The main trick,  simplified described, is sending packets from the computer behind the firewall to the a receiver, which triggers a session in a NAT or similar firewall to map incoming packages from the same IP port to the computer behind the firewall. 


What is the DLL-timefilter?
...........................

This is a delay-locked-loop (DLL) which estimates the *real* samplerate 
based on the system time (which is different from the nominal samplerate 
of the audio device). This allows us to dynamically resample audio 
coming from another machine, so you can stream over long time periods 
and still stay in sync.

What are the bandwidth requirements ?
.....................................


