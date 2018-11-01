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

void whitelist_env(char *env_name){
        char *env_name_ptr, *env_item_ptr;
        static size_t env_counter;

        if (!env_name)
                environ[env_counter++] = NULL;
        else{
                for(size_t i=env_counter; environ[i]; i++){
                        for(
                                        env_name_ptr = env_name, env_item_ptr = environ[i];
                                        *env_name_ptr == *env_item_ptr && *env_name_ptr && *env_item_ptr;
                                        env_name_ptr++, env_item_ptr++);
                        if (*env_item_ptr == '=' && *env_name_ptr == 0)
                                environ[env_counter++] = environ[i];
                }
        }
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
        setbuf(stdout, NULL);

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

        char *token;
        char *str = getenv("PLASH_EXPORT");
        if (str) {
                str = strdup(str);
                token = strtok(str, ":");
                while(token) {
                   whitelist_env(token);
                   token = strtok(NULL, ":");
                }
                free(str);
        }
        whitelist_env("TERM");
        whitelist_env("DISPLAY");
        whitelist_env("HOME");
        whitelist_env(NULL);

        if (-1 == execlp("/usr/bin/env", "/usr/bin/env", NULL))
                fatal("could not exec %s", "blah");
}
