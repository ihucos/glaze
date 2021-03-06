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

#define fatal(...) {\
fprintf(stderr, "%s", "glaze: ");\
fprintf(stderr, __VA_ARGS__);\
if (errno != 0){\
        fprintf(stderr, ": %s", strerror(errno));\
}\
fprintf(stderr, "\n");\
exit(1);\
}


void singlemap_map(const char *file, uid_t id){ // assuming uid_t == gid_t
	char *map;
        FILE *fd;

	if (NULL == (fd = fopen(file, "w"))) {
                fatal("could not open %s", file);
        }
        fprintf(fd, "0 %u 1\n", id);
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
        if (uid == 0){
                if (-1 == unshare(CLONE_NEWNS))
                        fatal("could not unshare");
        } else {
                if (-1 == unshare(CLONE_NEWNS | CLONE_NEWUSER))
                        fatal("could not unshare");
                singlemap_deny_setgroups();
                singlemap_map("/proc/self/uid_map", uid);
                singlemap_map("/proc/self/gid_map", gid);
        }
        mount("none", "/", NULL, MS_REC|MS_PRIVATE, NULL);
}

void rootfs_mount(const char *hostdir, const char *rootfs, const char *rootfsdir) {
        char *dst;
        struct stat sb;

        if (! (stat(hostdir, &sb) == 0 && S_ISDIR(sb.st_mode) || S_ISREG(sb.st_mode)))
                return;

        if (-1 == asprintf(&dst, "%s/%s", rootfs, hostdir))
                fatal("asprintf returned -1");

        if (! (stat(dst, &sb) == 0 && S_ISDIR(sb.st_mode) || S_ISREG(sb.st_mode)))
                return;
        if (-1 == mount(hostdir, dst, "none", MS_MGC_VAL|MS_BIND|MS_REC, NULL))
                fatal("could not rbind mount %s -> %s", hostdir, dst)
}

void find_rootfs(char **rootfs) {
        char *executable;
        if (NULL == (executable = realpath("/proc/self/exe", NULL)))
                fatal("could not call realpath");
        char *executable_dir = dirname(executable);
        if (-1 == asprintf(rootfs, "%s/../rootfs", executable_dir))
                fatal("asprintf returned -1");
}


int main(int argc, char* argv[]) {

        char *argv0 = strdup(argv[0]);
        char *progname = basename(argv0);
        char *rootfs;

        find_rootfs(&rootfs);

        singlemap_setup();

        rootfs_mount("/dev",  rootfs, "/dev");
        rootfs_mount("/home", rootfs, "/home");
        rootfs_mount("/proc", rootfs, "/proc");
        rootfs_mount("/root", rootfs, "/root");
        rootfs_mount("/sys",  rootfs, "/sys");
        rootfs_mount("/tmp",  rootfs, "/tmp");
        rootfs_mount("/etc/resolv.conf",  rootfs, "/etc/resolv.conf");

        char *origpwd;
        if (NULL == (origpwd = get_current_dir_name()))
            fatal("error calling get_current_dir_name")

        if (-1 == chroot(rootfs))
                fatal("could not chroot to %s", rootfs);

        if (-1 == chdir(origpwd)){
                if (-1 == chdir("/"))
                        fatal("could not chdir")
        }

        char *env[UCHAR_MAX + 1];
        env[UCHAR_MAX + 1] = NULL;
        unsigned char env_index = 0; // let it overflow if maximum is reached
        char *envname, *envval;

        char *str = getenv("PLASH_EXPORT");
        if (str != NULL ) {
                envname = strtok(str, ":");
                while( envname != NULL ) {
                   envval = getenv(envname);
                   if (NULL != envval)
                        asprintf(env + env_index++, "%s=%s", envname, envval);
                   envname = strtok(NULL, ":");
                }
        }

        char *always_export[] = {"TERM", "DISPLAY", "HOME", NULL};
        for (size_t i=0; always_export[i] != NULL; i++) {
                envname = always_export[i];
                envval = getenv(envname);
                if (NULL == envval)
                        continue;
                asprintf(env + env_index++, "%s=%s", envname, envval);
        }
        env[env_index++] = NULL;
        argv[0] = progname;
        if (-1 == execvpe(progname, argv, env))
                fatal("could not exec %s", progname);
}
