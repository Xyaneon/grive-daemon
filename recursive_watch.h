/*
grive-daemon syncs your ~/Google Drive folder by calling grive.
Copyright (C) 2014  Christopher Kyle Horton

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#ifndef RECURSIVE_WATCH_H
#define RECURSIVE_WATCH_H

#include <sys/inotify.h>
#include <dirent.h>
#include <string>
#include <syslog.h>
#include "watch.h"

using std::string;

int initialRecursiveWd()
{
  static int wd = -1;
  wd--;
  return wd;
}

struct DirectoryReader
{
    static void parseDirectory(string readingDirectory, int fd, Watch& watch);
};

void DirectoryReader::parseDirectory(string readingDirectory, int fd, Watch& watch)
{
  if (DIR *dp = opendir(readingDirectory.c_str()))
  {
    //cout << string(level, ' ') << readingDirectory << endl;
    while (struct dirent *ep = readdir(dp))
      if (ep->d_type == DT_DIR && ep->d_name[0] != '.')
      {
        int wd;
        wd = inotify_add_watch(fd, readingDirectory.c_str(), WATCH_FLAGS);
        watch.insert(initialRecursiveWd(), readingDirectory.c_str(), wd);
        parseDirectory(readingDirectory + "/" + ep->d_name, fd, watch);
      }
    closedir(dp);
  }
  else
    syslog (LOG_WARNING, "Couldn't open the directory %s.", readingDirectory.c_str());
}

#endif
