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
#include <stdarg.h>

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
        char *n, *v;
        static size_t env_counter;

        if (!env_name)
                environ[env_counter++] = NULL;
        else{
                for(size_t i=env_counter; environ[i]; i++){
                        for(
                                        n = env_name, v = environ[i];
                                        *n && *v && *n == *v;
                                        n++, v++);
                        if (*v == '=' && *n == 0)
                                environ[env_counter++] = environ[i];
                }
        }
}

int printf_file(char *file, char *format, ...){
        FILE *fd;
	if (! (fd = fopen(file, "w")))
                return 0;
        va_list args;
        va_start(args, format);
        vfprintf(fd, format, args);
        va_end(args);
        if (errno)
                fatal("could not write to %s", file);
        fclose(fd);
        return 1;
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

                //
                // write some file
                //
                if (!printf_file("/proc/self/setgroups", "deny")){
                        if (errno != ENOENT) 
                                fatal("could not open /proc/self/setgroups");
                };
                if (!printf_file("/proc/self/uid_map", "0 %u 1\n", uid)){
                        fatal("could not open /proc/self/uid_map")
                }
                if (!printf_file("/proc/self/gid_map", "0 %u 1\n", gid)){
                        fatal("could not open /proc/self/uid_map")
                }
        }

        //  make mount view private
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
        if (!(origpwd = get_current_dir_name()))
            fatal("error calling get_current_dir_name")

        if (-1 == chroot(rootfs))
                fatal("could not chroot to %s", rootfs);

        if (-1 == chdir(origpwd)){
                if (-1 == chdir("/"))
                        fatal("could not chdir")
        }

        char *token, *str;
        if (str = getenv("PLASH_EXPORT")) {
                str = strdup(str);
                token = strtok(str, ":");
                while(token){
                   whitelist_env(token);
                   token = strtok(NULL, ":");
                }
                free(str);
        }
        whitelist_env("TERM");
        whitelist_env("DISPLAY");
        whitelist_env("HOME");
        whitelist_env(NULL);

        if (-1 == execlp("/usr/bin/id", "/usr/bin/id", NULL))
                fatal("could not exec %s", "blah");
}
