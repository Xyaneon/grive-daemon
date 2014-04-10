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

/* Allow for 1024 simultaneous events */
#define BUFF_SIZE ((sizeof(struct inotify_event)+FILENAME_MAX)*1024)

typedef int bool;
#define true 1
#define false 0

static void skeleton_daemon()
{
    pid_t pid;

    /* Fork off the parent process */
    pid = fork();

    /* An error occurred */
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

    /* An error occurred */
    if (pid < 0)
        exit(EXIT_FAILURE);

    /* Success: Let the parent terminate */
    if (pid > 0)
        exit(EXIT_SUCCESS);

    /* Set new file permissions */
    umask(0);

    /* Change the working directory to the root directory */
    /* or another appropriated directory */
    chdir("/");

    /* Close all open file descriptors */
    int x;
    for (x = sysconf(_SC_OPEN_MAX); x>0; x--)
    {
        close (x);
    }

    /* Open the log file */
    openlog ("grive-daemon", LOG_PID, LOG_DAEMON);
}

static bool get_event (int fd, const char * target)
{
  ssize_t len, i = 0;
  char action[81+FILENAME_MAX] = {0};
  char buff[BUFF_SIZE] = {0};
  bool sync_required = false;

  len = read (fd, buff, BUFF_SIZE);

  while (i < len) {
    struct inotify_event *pevent = (struct inotify_event *)&buff[i];
    char action[81+FILENAME_MAX] = {0};

    if (pevent->len) 
       strcpy (action, pevent->name);
    else
       strcpy (action, target);
    
    if (pevent->mask & IN_ATTRIB) 
       strcat(action, " Metadata changed");
    if (pevent->mask & IN_CREATE) 
       strcat(action, " created in watched directory");
    if (pevent->mask & IN_DELETE) 
       strcat(action, " deleted from watched directory");
    if (pevent->mask & IN_DELETE_SELF) 
       strcat(action, "Watched file/directory was itself deleted");
    if (pevent->mask & IN_MODIFY) 
       strcat(action, " was modified");
    if (pevent->mask & IN_MOVE_SELF) 
       strcat(action, "Watched file/directory was itself moved");
    if (pevent->mask & IN_MOVED_FROM) 
       strcat(action, " moved out of watched directory");
    if (pevent->mask & IN_MOVED_TO) 
       strcat(action, " moved into watched directory");
    
    // Ignore hidden grive files.
    if (pevent->len) {
      if ((strcmp (pevent->name, ".grive") != 0) && (strcmp (pevent->name, ".grive_state") != 0)) {
        syslog (LOG_INFO, "grive-daemon noticed %s.", action);
        sync_required = true;
      }
    }
    
    i += sizeof(struct inotify_event) + pevent->len;
    //sync_required = true;
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
    char target[FILENAME_MAX];
    int result;
    int fd;
    int wd;   /* watch descriptor */
    struct passwd *pw = getpwuid(getuid());
    char *gd_dir = strcat(pw->pw_dir, "/Google Drive");
    
    skeleton_daemon();
    
    syslog (LOG_NOTICE, "grive-daemon started.");
    
    fd = inotify_init();
    if (fd < 0) {
      handle_error (errno);
      return EXIT_FAILURE;
    }

    wd = inotify_add_watch (fd, gd_dir, IN_ATTRIB | IN_CREATE | IN_DELETE | IN_DELETE_SELF | IN_MODIFY | IN_MOVE_SELF | IN_MOVED_FROM | IN_MOVED_TO);
    if (wd < 0) {
      syslog (LOG_WARNING, "gd_dir = %s.", gd_dir);
      handle_error (errno);
      return EXIT_FAILURE;
    }
    
    while (1)
    {
        if (get_event(fd, target))
        {
          if(chdir(gd_dir) < 0 )  
          {
             syslog (LOG_WARNING, "grive-daemon could not switch to ~/Google Drive.");
             return EXIT_FAILURE;
          }
          system ("notify-send grive-daemon \"Starting Google Drive sync...\"");
          system ("grive");
          syslog (LOG_INFO, "grive-daemon performed a syncronization.");
          system ("notify-send grive-daemon \"Google Drive sync complete.\"");
        }
        sleep (3);
    }
    
    inotify_rm_watch(fd, wd);
    close(fd);
    syslog (LOG_NOTICE, "grive-daemon terminated.");
    closelog();

    return EXIT_SUCCESS;
}
