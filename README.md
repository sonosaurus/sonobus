
# SonoBus

SonoBus is an easy to use application for streaming high-quality, low-latency peer-to-peer audio between devices over the internet or a local network.

Simply choose a unique group name (with optional password), and instantly connect multiple people together to make music, remote sessions, podcasts, etc. Easily record the audio from everyone, as well as playback any audio content to the whole group.

Connects multiple users together to send and receive audio among all in a group, with fine-grained control over latency, quality and overall mix. Includes optional input compression, noise gate, and EQ effects, along with a master reverb. All settings are dynamic, network statistics are clearly visible.

Works as a standalone application on macOS, Windows, iOS, and Linux, and as an audio plugin (AU, VST) on macOS and Windows. Use it on your desktop or in your DAW, or on your mobile device.

Easy to setup and use, yet still provides all the details that audio nerds want to see. Audio quality can be instantly adjusted from full uncompressed PCM 32bit float down through various compressed bitrates using the low-latency Opus codec.


<img src="https://sonobus.net/assets/images/sonobus_screenshot.png" width="871" />


# Installing

There are binary releases for macOS and Windows available at [sonobus.net](https://sonobus.net) or in the releases of this repository on GitHub.

# Building

To build from source on macOS and Windows, all of the dependencies are a part of this GIT repository, including prebuilt Opus libraries. 
On Linux, you'll just need to have `libopus` and `libjack` installed, as
well as their appropriate development packages.

### On macOS

Open the Xcode project at `Builds/MacOSX/SonoBus.xcodeproj`, choose the target you want to build and go for it.

### On Windows

Using Visual Studio 2017, open the solution at `Builds/VisualStudio2017/SonoBus.sln`, choose the target you want to build and go for it.

### On Linux

Make sure you have `libopus` and the `libopus` development package (if necessary). 

    cd Builds/LinuxMakefile
    CONFIG=Release make Standalone

When it finishes, the executable will be at `Builds/LinuxMakefile/build/SonoBus`, which you can copy elsewhere for use (/usr/local/bin, etc)

# License and 3rd Party Software

SonoBus was written by Jesse Chappell, and it is licensed under the GPLv3, the full license text is in the LICENSE file. Some of the dependencies have their own more permissive licenses.

It is built using JUCE 5 (slightly modified on a public fork), and AOO (Audio over OSC), which also uses the Opus codec. I'm using the very handy tool `git-subrepo` to include the source code for my forks of those software libraries in this repository.


My github forks of these that are referenced via `git-subrepo` in this repository are:

> [https://github.com/essej/JUCE](https://github.com/essej/JUCE) in the sono branch.
 
> [https://github.com/essej/aoo.git](https://github.com/essej/aoo.git) in the sono branch.

