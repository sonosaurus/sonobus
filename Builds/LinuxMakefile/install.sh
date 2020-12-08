mkdir -p /usr/local/bin
cp build/SonoBus /usr/local/bin/SonoBus
mkdir -p /usr/local/share/applications
cp sonobus.desktop /usr/local/share/applications/sonobus.desktop
chmod +x /usr/local/share/applications/sonobus.desktop
mkdir -p /usr/share/pixmaps
cp ../../images/sonobus_logo@2x.png /usr/share/pixmaps/sonobus.png
