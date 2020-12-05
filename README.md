
# SonoBus

SonoBus is an easy to use application for streaming high-quality, low-latency peer-to-peer audio between devices over the internet or a local network.

Simply choose a unique group name (with optional password), and instantly connect multiple people together to make music, remote sessions, podcasts, etc. Easily record the audio from everyone, as well as playback any audio content to the whole group.

Connects multiple users together to send and receive audio among all in a group, with fine-grained control over latency, quality and overall mix. Includes optional input compression, noise gate, and EQ effects, along with a master reverb. All settings are dynamic, network statistics are clearly visible.

Works as a standalone application on macOS, Windows, iOS, and Linux, and as an audio plugin (AU, VST) on macOS and Windows. Use it on your desktop or in your DAW, or on your mobile device.

Easy to setup and use, yet still provides all the details that audio nerds want to see. Audio quality can be instantly adjusted from full uncompressed PCM (16, 24, or 32 bit) or with various compressed bitrates (16-256 kbps per channel) using the low-latency Opus codec, and you can do this independently for any of the users you are connected with in a group.


<img src="https://sonobus.net/assets/images/sonobus_screenshot.png" width="871" />

**IMPORTANT TIPS**

SonoBus does not use any echo cancellation, or automatic noise
reduction in order to maintain the highest audio quality. As a result, if you have a live microphone signal you will need to also use headphones to prevent echos and/or feedback.

For best results, and to achieve the lowest latencies, connect your computer with wired ethernet to your router if you can. Although it will work with WiFi, the added network jitter and packet loss will require you to use a bigger safety buffer to maintain a quality audio signal, which results in higher latencies.

SonoBus does NOT currently use any encryption for the data
communication, so while it is unlikely that it will be
intercepted, please keep that in mind. All audio is sent directly between users peer-to-peer, the connection server is only used so that the users in a group can find each other.



# Installing

There are binary releases for macOS and Windows available at [sonobus.net](https://sonobus.net) or in the releases of this repository on GitHub.

For Linux, read the [build instructions](#content-on-linux) below.

# Building

To build from source on macOS and Windows, all of the dependencies are a part of this GIT repository, including prebuilt Opus libraries. 
On Linux, you'll just need to have `libopus` and `libjack` installed, as
well as their appropriate development packages.

### On macOS

Open the Xcode project at `Builds/MacOSX/SonoBus.xcodeproj`, choose the target you want to build and go for it.

### On Windows

Using Visual Studio 2017, open the solution at `Builds/VisualStudio2017/SonoBus.sln`, choose the target you want to build and go for it.

### On Linux

Make sure you have `libopus` and the `libopus` development package
(libopus-dev), as well as JACK (jackd) and its development package. Also
libasound2-dev , libx11-dev, libxext-dev, libxinerama-dev, libxrandr-dev,
libxcursor-dev, libgl-dev, libfreetype-dev,
libcurl4-dev, libcurlpp-dev.

    cd Builds/LinuxMakefile
    ./build.sh

When it finishes, the executable will be at `Builds/LinuxMakefile/build/SonoBus`. You can install it using the installation script.

    sudo ./install.sh

If you wish to uninstall you can run the uninstall script in the same directory.

    sudo ./uninstall.sh

# License and 3rd Party Software

SonoBus was written by Jesse Chappell, and it is licensed under the GPLv3, the full license text is in the LICENSE file. Some of the dependencies have their own more permissive licenses.

It is built using JUCE 5 (slightly modified on a public fork), and AOO (Audio over OSC), which also uses the Opus codec. I'm using the very handy tool `git-subrepo` to include the source code for my forks of those software libraries in this repository.


My github forks of these that are referenced via `git-subrepo` in this repository are:

> https://github.com/essej/JUCE  in the sono6 branch.
 
> https://github.com/essej/aoo.git   in the sono branch.


If you want to run your own connection server instead of using the default
one at aoo.sonobus.net, you can build the headless aooserver code at

> https://github.com/essej/aooserver

The standalone SonoBus application also provides a connection server internally,
which you can connect to on port 10998, or port forward TCP/UDP 10998 from your internet
router to the machine you are running it on.
