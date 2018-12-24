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

char *get_executable() {
    static char *executable;
    if (NULL == (executable = realpath("/proc/self/exe", NULL)))
            fatal("could not call realpath('/proc/self/exe')");
    return executable;
}

char *get_rootfs() {
        static char *rootfs;
        rootfs = dirname(dirname(get_executable()));
        return rootfs;
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

                if (!printf_file("/proc/self/setgroups", "deny")){
                        if (errno != ENOENT) 
                                fatal("could not open /proc/self/setgroups");
                };
                if (!printf_file("/proc/self/uid_map", "0 %u 1\n", uid)){
                        fatal("could not open /proc/self/uid_map")
                }
                if (!printf_file("/proc/self/gid_map", "0 %u 1\n", gid)){
                        fatal("could not open /proc/self/gid_map")
                }
        }

}

int main_glazemini(int argc, char* argv[]) {
    if (argc < 2){
        fprintf(stderr, "usage: glazemini rootfs-dir\n");
        exit(EXIT_FAILURE);
    }

    if (-1 == chdir(argv[1]))
        fatal("could not chdir to arg1: %s", argv[1]);

    if (-1 == execlp("/bin/sh", "/bin/sh", "-cxe",
                            "mkdir -p ./.run\n"
                            "find ./usr/local/bin ./usr/bin ./bin ./usr/local/sbin ./usr/sbin ./sbin 2> /dev/null"
                            " | xargs -L 1 basename | sort | uniq"
                            " | xargs -I{} ln -f \"$1\" ./.run/{}", // FIXME: code execution by filename
                            "--",
                            get_executable(),
                            NULL))

        fatal("could not exec /bin/sh");
}

int main(int argc, char* argv[]) {
        setbuf(stdout, NULL); // why, remove?

        char *argv0 = strdup(argv[0]); // check strdup's return value!!
        char *progname = basename(argv0);
        char *rootfs;

        if (0 == strcmp(progname, "glazemini")){
            return main_glazemini(argc, argv);
        }

        rootfs = get_rootfs();

        singlemap_setup();

        //  make mount view private
        if (-1 == mount("none", "/", NULL, MS_REC|MS_PRIVATE, NULL))
            fatal("could not mount")

        char *origpwd;
        if (!(origpwd = get_current_dir_name()))
            fatal("error calling get_current_dir_name")

        //if (-1 == mount(hostdir, dst, "none", MS_MGC_VAL|MS_BIND|MS_REC, NULL))

        if (-1 == chroot(rootfs))
                fatal("could not chroot to %s", rootfs);

        if (-1 == chdir(origpwd)){
                if (-1 == chdir("/"))
                        fatal("could not chdir")
        }

        argv[0] = progname;
        if (-1 == execvp(progname, argv))
                fatal("could not exec %s", progname);
}
