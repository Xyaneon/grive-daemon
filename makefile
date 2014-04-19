#grive-daemon syncs your ~/Google Drive folder by calling grive.
#Copyright (C) 2014  Christopher Kyle Horton
#
#This program is free software; you can redistribute it and/or
#modify it under the terms of the GNU General Public License
#as published by the Free Software Foundation; either version 2
#of the License, or (at your option) any later version.
#
#This program is distributed in the hope that it will be useful,
#but WITHOUT ANY WARRANTY; without even the implied warranty of
#MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#GNU General Public License for more details.
#
#You should have received a copy of the GNU General Public License
#along with this program; if not, write to the Free Software
#Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

all: grive-daemon.o watch.o
	g++ -o grive-daemon grive-daemon.o watch.o

grive-daemon.o: grive-daemon.cpp daemon.h recursive_watch.h
	g++ -c grive-daemon.cpp daemon.h recursive_watch.h

watch.o: watch.cpp watch.h
	g++ -c watch.cpp watch.h

install:
	sudo cp grive-daemon /usr/local/bin/

uninstall:
	sudo rm -rf /usr/local/bin/grive-daemon/

clean:
	rm -rf *.o *.gch grive-daemon
