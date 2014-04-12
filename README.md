grive-daemon
============

A daemon to watch and sync your `~/Google Drive` folder on Linux by calling grive.

This project is dependent on you having already installed [grive][1]. On an Ubuntu system (where this is supported), you should probably install it from the Ubuntu Software Center.

#Build#

You need to make sure grive and notify-osd are already installed on your system. Go into your cloned `grive-daemon` directory and run `gcc -o grive-daemon grive-daemon.c` to build.

After setting up grive (you need to create `~/Google Drive` yourself), just run this and it should automatically sync local changes to your Google Drive folder for you.

This daemon is a rushed hack job, and you're nuts if you're expecting a polished product. I never made anything similar to this before. YOU'VE BEEN WARNED!

[1]: https://github.com/Grive/grive