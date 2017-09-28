grive-daemon
============

A daemon to watch and sync your `~/Google Drive` folder on Linux by calling grive.

This project is dependent on you having already installed [grive][1]. On an Ubuntu system (where this is supported), you should probably install it from the Ubuntu Software Center.

Tested to work on Ubuntu 14.04 LTS.

# Dependencies

- grive
- make
- notify-osd

# Build and Install

Go into your cloned `grive-daemon` directory and run `make` to build. To rebuild later, just run `make clean` followed by another `make`.

After building, run `sudo make install` to install the daemon to your `/usr/local/bin/` directory. This will also install an autostart script in `/etc/profile.d/` so grive-daemon will start automatically after you log in.

# How to Run

After setting up grive (you need to create `~/Google Drive` yourself) and following the instructions in the preceding section, if you want to run grive-daemon immediately, just open a terminal and run:
```
grive-daemon
```
It should automatically sync local changes to your Google Drive folder for you.

After running `sudo make install`, grive-daemon should also start itself automatically shortly after each time you log in.

# Uninstall

If you want to remove grive-daemon later, just go into the cloned `grive-daemon` directory in a terminal and run `sudo make uninstall`.


This daemon is a rushed hack job, and you're nuts if you're expecting a polished product. I never made anything similar to this before. YOU'VE BEEN WARNED!

[1]: https://github.com/Grive/grive
