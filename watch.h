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

#ifndef WATCH_H
#define WATCH_H

#include <iostream>
#include <string>
#include <sys/inotify.h>
#include <map>
#include <dirent.h>

using std::string;
using std::map;
using std::cout;
using std::endl;

class Watch {
    struct wd_elem {
        int pd;
        string name;
        bool operator() (const wd_elem &l, const wd_elem &r) const
            { return l.pd < r.pd ? true : l.pd == r.pd && l.name < r.name ? true : false; }
    };
    map<int, wd_elem> watch;
    map<wd_elem, int, wd_elem> rwatch;
public:
    // Insert event information, used to create new watch, into Watch object.
    void insert( int pd, const string &name, int wd );
    // Erase watch specified by pd (parent watch descriptor) and name from watch list.
    // Returns full name (for display etc), and wd, which is required for inotify_rm_watch.
    string erase( int pd, const string &name, int *wd );
    // Given a watch descriptor, return the full directory name as string. Recurses up parent WDs to assemble name, 
    // an idea borrowed from Windows change journals.
    string get( int wd );
    // Given a parent wd and name (provided in IN_DELETE events), return the watch descriptor.
    // Main purpose is to help remove directories from watch list.
    int get( int pd, string name );
    void cleanup( int fd );
    void stats();
};

#endif
