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

#include "watch.h"

void Watch::insert( int pd, const string &name, int wd ) {
    wd_elem elem = {pd, name};
    watch[wd] = elem;
    rwatch[elem] = wd;
}

string Watch::erase( int pd, const string &name, int *wd ) {
    wd_elem pelem = {pd, name};
    *wd = rwatch[pelem];
    rwatch.erase(pelem);
    const wd_elem &elem = watch[*wd];
    string dir = elem.name;
    watch.erase(*wd);
    return dir;
}

string Watch::get( int wd ) {
    const wd_elem &elem = watch[wd];
    return elem.pd <= -1 ? elem.name : this->get(elem.pd) + "/" + elem.name;
}

int Watch::get( int pd, string name ) {
    wd_elem elem = {pd, name};
    return rwatch[elem];
}

void Watch::cleanup( int fd ) {
    for (map<int, wd_elem>::iterator wi = watch.begin(); wi != watch.end(); wi++) {
        inotify_rm_watch( fd, wi->first );
        watch.erase(wi);
    }
    rwatch.clear();
}

void Watch::stats() {
    cout << "number of watches=" << watch.size() << " & reverse watches=" << rwatch.size() << endl;
}
