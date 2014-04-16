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

/* Programmed with help from this StackExchange answer:
   http://stackoverflow.com/a/17955149
   Also, from this blog post:
   http://darkeside.blogspot.com/2007/12/linux-inotify-example.html
   And this:
   http://www.ibm.com/developerworks/linux/library/l-ubuntu-inotify/index.html
   And this (for recursive directory watching):
   https://gist.github.com/pkrnjevic/6016356
   And this too, for more recursive directory watching:
   http://stackoverflow.com/a/11097815
   */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <syslog.h>
#include <pwd.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <sys/inotify.h>
#include <iostream>
#include <string>
#include <map>
#include <dirent.h>

using std::map;
using std::string;
using std::cout;
using std::endl;

/* Allow for 1024 simultaneous events */
#define BUFF_SIZE ((sizeof(struct inotify_event)+FILENAME_MAX)*1024)

#define WATCH_FLAGS (IN_ATTRIB | IN_CREATE | IN_DELETE | IN_DELETE_SELF | IN_MODIFY | IN_MOVE_SELF | IN_MOVED_FROM | IN_MOVED_TO)

int initialRecursiveWd()
{
  static int wd = -1;
  wd--;
  return wd;
}

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
    void insert( int pd, const string &name, int wd ) {
        wd_elem elem = {pd, name};
        watch[wd] = elem;
        rwatch[elem] = wd;
    }
    // Erase watch specified by pd (parent watch descriptor) and name from watch list.
    // Returns full name (for display etc), and wd, which is required for inotify_rm_watch.
    string erase( int pd, const string &name, int *wd ) {
        wd_elem pelem = {pd, name};
        *wd = rwatch[pelem];
        rwatch.erase(pelem);
        const wd_elem &elem = watch[*wd];
        string dir = elem.name;
        watch.erase(*wd);
        return dir;
    }
    // Given a watch descriptor, return the full directory name as string. Recurses up parent WDs to assemble name, 
    // an idea borrowed from Windows change journals.
    string get( int wd ) {
        const wd_elem &elem = watch[wd];
        return elem.pd <= -1 ? elem.name : this->get(elem.pd) + "/" + elem.name;
    }
    // Given a parent wd and name (provided in IN_DELETE events), return the watch descriptor.
    // Main purpose is to help remove directories from watch list.
    int get( int pd, string name ) {
        wd_elem elem = {pd, name};
        return rwatch[elem];
    }
    void cleanup( int fd ) {
        for (map<int, wd_elem>::iterator wi = watch.begin(); wi != watch.end(); wi++) {
            inotify_rm_watch( fd, wi->first );
            watch.erase(wi);
        }
        rwatch.clear();
    }
    void stats() {
        cout << "number of watches=" << watch.size() << " & reverse watches=" << rwatch.size() << endl;
    }
};

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

static void daemonize()
{
    pid_t pid;

    /* Fork off the parent process */
    pid = fork();
    if (pid < 0)
        exit(EXIT_FAILURE);

    /* Success: Let the parent terminate */
    if (pid > 0)
        exit(EXIT_SUCCESS);

    /* On success: The child process becomes session leader */
    if (setsid() < 0)
        exit(EXIT_FAILURE);

    /* Catch, ignore and handle signals */
    //TODO: Implement a working signal handler */
    signal(SIGCHLD, SIG_IGN);
    signal(SIGHUP, SIG_IGN);

    /* Fork off for the second time*/
    pid = fork();
    if (pid < 0)
        exit(EXIT_FAILURE);

    /* Success: Let the parent terminate */
    if (pid > 0)
        exit(EXIT_SUCCESS);
    
    umask(0);
    
    openlog ("grive-daemon", LOG_PID, LOG_DAEMON);

    /* Close all open file descriptors */
    int x;
    for (x = sysconf(_SC_OPEN_MAX); x>0; x--)
    {
        close (x);
    }
}

static bool get_event (int fd, const char * target, Watch& watch)
{
  ssize_t len, i = 0;
  char action[81+FILENAME_MAX] = {0};
  char buff[BUFF_SIZE] = {0};
  string current_dir, new_dir;
  bool sync_required = false;
  int wd;

  len = read (fd, buff, BUFF_SIZE);

  while (i < len) {
    struct inotify_event *pevent = (struct inotify_event *)&buff[i];
    char action[81+FILENAME_MAX] = {0};

    if (pevent->len) 
       strcpy (action, pevent->name);
    else
       strcpy (action, target);
    
    if (pevent->mask & IN_ATTRIB) 
       strcat(action, "'s metadata changed");
    if (pevent->mask & IN_CREATE)
    {
        current_dir = watch.get(pevent->wd);
        if (pevent->mask & IN_ISDIR)
        {
          new_dir = current_dir + "/" + pevent->name;
          wd = inotify_add_watch(fd, new_dir.c_str(), WATCH_FLAGS);
          watch.insert(pevent->wd, pevent->name, wd);
          //printf("New directory %s created.\n", new_dir.c_str());
        } else {
          //printf("New file %s/%s created.\n", current_dir.c_str(), pevent->name);
        }
        strcat(action, " was created in a watched directory");
    }
    if (pevent->mask & IN_DELETE)
    {
        if (pevent->mask & IN_ISDIR) {
            new_dir = watch.erase(pevent->wd, pevent->name, &wd);
            inotify_rm_watch(fd, wd);
            //printf( "Directory %s deleted.\n", new_dir.c_str() );
        } else {
            current_dir = watch.get(pevent->wd);
            //printf( "File %s/%s deleted.\n", current_dir.c_str(), pevent->name );
        }
        strcat(action, " was deleted in a watched directory");
    }
    if (pevent->mask & IN_DELETE_SELF) 
       strcat(action, ", the watched file/directory, was itself deleted");
    if (pevent->mask & IN_MODIFY) 
       strcat(action, " was modified");
    if (pevent->mask & IN_MOVE_SELF) 
       strcat(action, ", the watched file/directory, was itself moved");
    if (pevent->mask & IN_MOVED_FROM) 
       strcat(action, " was moved out of a watched directory");
    if (pevent->mask & IN_MOVED_TO) 
       strcat(action, " was moved into a watched directory");
    
    // Ignore hidden grive files.
    if (pevent->len) {
      if ((strcmp (pevent->name, ".grive") != 0) && (strcmp (pevent->name, ".grive_state") != 0)) {
        syslog (LOG_INFO, "grive-daemon noticed %s.", action);
        sync_required = true;
      }
    }
    
    i += sizeof(struct inotify_event) + pevent->len;
  }
  
  return sync_required;
}

void handle_error (int error)
{
  syslog (LOG_WARNING, "grive-daemon needed to handle error %s.", strerror(error));
  fprintf (stderr, "Error: %s\n", strerror(error));
}

int main()
{
    Watch watch;
    char target[FILENAME_MAX];
    int result, fd, wd;
    struct passwd *pw = getpwuid(getuid());
    char *gd_dir = strcat(pw->pw_dir, "/Google Drive");
    
    daemonize();
    
    syslog (LOG_NOTICE, "grive-daemon started.");
    
    if (chdir(gd_dir) < 0)
    {
        syslog (LOG_WARNING, "grive-daemon could not switch to ~/Google Drive.");
        exit(EXIT_FAILURE);
    }
    
    // Initial syncronization in case stuff changed before starting.
    system ("notify-send grive-daemon \"Starting Google Drive sync...\"");
    system ("grive");
    syslog (LOG_INFO, "grive-daemon performed an initial syncronization.");
    system ("notify-send grive-daemon \"Google Drive sync complete.\"");
    
    fd = inotify_init();
    if (fd < 0) {
      handle_error (errno);
      return EXIT_FAILURE;
    }

    wd = inotify_add_watch (fd, gd_dir, WATCH_FLAGS);
    if (wd < 0) {
      syslog (LOG_WARNING, "gd_dir = %s.", gd_dir);
      handle_error (errno);
      return EXIT_FAILURE;
    }
    watch.insert(-1, gd_dir, wd);
    
    DirectoryReader::parseDirectory(string(gd_dir), fd, watch);
    
    while (1)
    {
        if (get_event(fd, target, watch))
        {
          system ("notify-send grive-daemon \"Starting Google Drive sync...\"");
          system ("grive");
          syslog (LOG_INFO, "grive-daemon performed a syncronization.");
          system ("notify-send grive-daemon \"Google Drive sync complete.\"");
        }
        sleep (2);
    }
    
    inotify_rm_watch(fd, wd);
    watch.cleanup(fd);
    close(fd);
    syslog (LOG_NOTICE, "grive-daemon terminated.");
    closelog();

    return EXIT_SUCCESS;
}