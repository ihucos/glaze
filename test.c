#define _GNU_SOURCE
#include <errno.h>
#include <fcntl.h>
#include <libgen.h>
#include <limits.h>
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/types.h>
#include <dirent.h>

#define fatal(...) {\
fprintf(stderr, "%s", "glaze: ");\
fprintf(stderr, __VA_ARGS__);\
if (errno != 0){\
        fprintf(stderr, ": %s", strerror(errno));\
}\
fprintf(stderr, "\n");\
exit(1);\
}


void singlemap_map(const char *file, uid_t id_from, uid_t id){ // assuming uid_t == gid_t
	char *map;
        FILE *fd;

	if (NULL == (fd = fopen(file, "w"))) {
                fatal("could not open %s", file);
        }
        fprintf(fd, "%u %u 1\n", id_from, id);
        if (errno != 0)
                fatal("could not write to %s", file);
        fclose(fd);
}

void singlemap_deny_setgroups() {
	FILE *fd = fopen("/proc/self/setgroups", "w");
	if (NULL == fd) {
		if (errno != ENOENT) 
                        fatal("could not open /proc/self/setgroups");
        }
        fprintf(fd, "deny");
        if (errno != 0) {
                fatal("could not write to /proc/self/setgroups");
                exit(1);
        }
        fclose(fd);
}

void singlemap_setup(){
        uid_t uid = getuid();
        gid_t gid = getgid();
        if (-1 == unshare(CLONE_NEWNS | CLONE_NEWUSER))
                fatal("could not unshare");
        singlemap_deny_setgroups();
        singlemap_map("/proc/self/uid_map", 0, uid);
        singlemap_map("/proc/self/gid_map", 0, gid);
        mount("none", "/", NULL, MS_REC|MS_PRIVATE, NULL);

        DIR *dp;
        struct dirent *ep;

        dp = opendir ("/home/ihucos");
        if (dp != NULL)
        {
                while (ep = readdir (dp))
                        if (strcmp(ep->d_name, ".") && strcmp(ep->d_name, "..") && strcmp(ep->d_name, "dev")){
                                char *dir;
                                asprintf(&dir, "/%s", ep->d_name);
                                puts(dir);
                                if (-1 == mount(dir, dir, "none", MS_MGC_VAL|MS_BIND|MS_REC, NULL)){
                                        fatal("could not mount")
                                }
                                if (-1 == mount("none", dir, NULL, MS_RDONLY|MS_REMOUNT|MS_BIND|MS_REC, NULL)){
                                        fatal("could not mount")
                                }
                                free(dir);
                        }

                closedir (dp);
        }
        else
                fatal("Couldn't open the directory");


        if (-1 == unshare(CLONE_NEWUSER))
                fatal("could not unshare user again");
        singlemap_map("/proc/self/uid_map", uid, 0);
        singlemap_map("/proc/self/gid_map", gid, 0);
}



int main(int argc, char* argv[]) {

        singlemap_setup();
        execlp("sh", "sh", NULL);
}
