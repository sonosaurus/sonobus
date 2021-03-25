# Building and installing SonoBus on GNU/Linux
Follow these steps in order to build (and install) SonoBus on GNU/Linux.

### Installing build dependencies
To build SonoBus you'll need to install the necessary development dependencies.

If you're using Debian or a Debian-based distro like Ubuntu, run this script:
```
./deb_get_prereqs.sh
```

On Fedora run this script:
```
./fedora_get_prereqs.sh
```

On other distros you'll have to insall the following development packages manually through your package manager:

* `libjack-jackd2-dev`
* `libopus0`
* `libopus-dev`
* `opus-tools`
* `libasound2-dev`
* `libx11-dev`
* `libxext-dev`
* `libxinerama-dev`
* `libxrandr-dev`
* `libxcursor-dev`
* `libgl-dev`
* `libfreetype6-dev` or `libfreetype-dev`
* `libcurl4-openssl-dev` or `libcurl4-gnutls-dev`

### Building
Run the build script, both the standalone application and the VST3 plugin will be built:
```
./build.sh
```

### Installing
When the build finishes, the executable will be at `Builds/LinuxMakefile/build/sonobus`. You can install it
and the VST3 plugin by running the install script:
```
sudo ./install.sh
```
It defaults to installing in /usr/local, but if you want to install it
elsewhere, just specify the base directory as the first argument on the commandline of the script.

### Uninstalling
If you wish to uninstall you can run the uninstall script:
```
sudo ./uninstall.sh
```
