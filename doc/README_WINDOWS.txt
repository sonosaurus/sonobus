
SonoBus

High Quality Network Audio Streaming 


SonoBus is an easy to use application for streaming high-quality,
low-latency peer-to-peer audio between devices over the internet or a
local network.

Simply choose a unique group name (with optional password), and
instantly connect multiple people together to make music, live
sessions, podcasts, etc.

Available for macOS, Windows, iOS and Linux.


IMPORTANT TIPS
==================

SonoBus does not use any echo cancellation, or automatic noise
reduction in order to maintain the highest audio quality. As a result,
if you have a live microphone signal you will need to also use
headphones to prevent echos and/or feedback.

For best results, and to achieve the lowest latencies, connect your
computer with wired ethernet to your router if you can. Although it
will work with WiFi, the added network jitter and packet loss will
require you to use a bigger safety buffer to maintain a quality audio
signal, which results in higher latencies.

SonoBus does NOT currently use any encryption for the data
communication, so while it is very unlikely that it will be
intercepted, please keep that in mind. All audio is sent directly
between users peer-to-peer, the connection server is only used so that
the users in a group can find each other.


INSTALLATION (Windows)
=======================

You should be able to just launch the standalone SonoBus.exe to get
started. You should allow it access to both local and internet
networks if/when the system asks. Please contact me if you have any
issues, it has only been tested on a few different windows versions so
far.

It is HIGHLY recommended that you use an ASIO supported audio
driver. You can install ASIO4ALL to enable ASIO for your built-in
audio devices or any other that don't have a dedicated ASIO
driver. Get ASIO4ALL at http://www.asio4all.org.

If you want to use the VST3 plugin version, copy the SonoBus.vst3
to your system's VST3 folder, which is usually at C:\Program
Files\Common Files\VST3\  folder. If you don't have a VST3 folder at
the location you can create one first.

If you want to use the VST2 plugin version, copy the SonoBus.dll
to your system's VST folder.



GETTING STARTED (Windows)
======================

When using the standalone application, you'll want to setup your audio
device first. Press the "gear" button in the top left and select the
AUDIO tab. Select your 'Audio device type' first, as mentioned above
it is strongly recommended to use ASIO here, and if you do, then
select the appropriate device. Otherwise you can use Windows Audio,
then select the Input and Output devices you want to use.

You can then select the Input channels and output channels you want to
use. If you only have a mono input source (such as a microphone) you
can deselect inputs so that only  one is selected, which will reduce
the sending network bandwidth.

You can choose a sample rate, 48000 is recommended, but 44100 will
also work. The different participants you connect with do NOT need to
have the same setting here, audio will be resampled if necessary
automatically.

Choose an Audio Buffer Size, this will determine a baseline for the
latency, the lower value you choose, the lower the latency, but at a
cost of increased processing and network packet overhead. Generally,
choosing 256 is safe (when using ASIO), but for lower latency, use 128
samples. You can go lower if your hardware supports it, but it will
not be of much benefit unless you use one the PCM send quality options
(uncompressed) which can use those smaller buffer sizes.

If you see a yellow bar across the top saying that your audio input is
muted to avoid feedback, you can press the button on the top right to
Unmute Input. If you are using a microphone input, you will NEED
headphones, make sure to connect them before you unmute the input.


CONNECTING WITH OTHERS
==================

Press the Connect... button to get started. Choose a unique group name
that you want to use, or use the handy random group name generator
(the dice button). You can also enter a password that people
connecting to the group will also need to enter for additional
security, but it is optional. You can tell others that you want to
connect to the group name (and password, if used) or you can press the
Copy button in the top right of the Group page, then paste and share
it with the other users you want to connect with. They can use it to
paste in to this page using the paste button in the top left.

Choose a name for yourself that will be your identifier for anyone
else who joins. If someone else is already using that name on the
server it will automatically be made unique when you connect.

You can leave the default aoo.sonobus.net as the server, or enter
another server hostname if you are running your own group connection
server. Note that NO AUDIO is sent through the server, it is only used
to help the users connect to each other, all audio is sent directly
between users (peer-to-peer).

PRESS Connect to Group! If others are already in the group you should
see them show up and you'll be able to hear each other. Otherwise, you will
see a message indicating that you are waiting for others to join. You
can adjust the levels that you hear the others with the volume
sliders, or adjust the stereo panning with the Pan button (and popup
sliders) for each user.

You can Mute yourself altogether by pressing the microphone button in
the lower left corner. When your input is muted you will see a red
crossed out microphone. You can also choose who your input is muted
for independently, in case you don't want to send audio to certain
users, but do to others. You can also mute the output from all or
individual users with the speaker button (next to the mic buttons) at
the bottom of the window or in each user's area.






================================

Copyright 2020, Jesse Chappell, Sonosaurus LLC
jesse@sonosaurus.com
