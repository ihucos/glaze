#define _GNU_SOURCE
#include <errno.h>
#include <fcntl.h>
#include <grp.h>
#include <libgen.h>
#include <limits.h>
#include <pwd.h>
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

#define STR_EXPAND(name) #name
#define STR(name) STR_EXPAND(name)

#define MAX_USERLEN 32
#define SCAN_ID_RANGE "%" STR(MAX_USERLEN) "[^:\n]:%lu:%lu\n"

#define fatal(...) {\
fprintf(stderr, "%s", "myprog: ");\
fprintf(stderr, __VA_ARGS__);\
if (errno != 0){\
        fprintf(stderr, ": %s\n", strerror(errno));\
}\
exit(1);\
}

typedef struct {
    size_t uid_start;
    size_t uid_count;
    size_t gid_start;
    size_t gid_count;
} subid_range_t;

void getidrange(
                unsigned long id,
                char *id_name,
                const char *file,
                size_t *start, size_t *count) {

        char label[MAX_USERLEN];
        FILE *fd;
        subid_range_t range;

        if (NULL == (fd = fopen(file, "r")))
                fatal("subid file not found")

        while (3 == fscanf(fd, SCAN_ID_RANGE, label, start, count)){
                errno = 0;
                if ((strcmp(id_name, label) == 0) ||
                                strtoul(label, NULL, 10) == id && errno == 0)
                        errno = 0;
                        return;
        }
        errno = 0;
        fatal("subid range not found\n")
}

int fullmap_run(subid_range_t range){
        int fd[2];
        pid_t child;
        char readbuffer[2];

        if (-1 == pipe(fd))
                fatal("could not create pipe");
        child = fork();
        if (-1 == child)
                fatal("could not fork")
        if (0 == child){
                close(fd[1]);
                dup2(fd[0], 0);
                execlp("sh", "sh", "-e", NULL);
        }

        if (-1 == unshare(CLONE_NEWNS | CLONE_NEWUSER)){
                fatal("could not unshare");
        }

        if (0 > dprintf(
                        fd[1],
                        "newuidmap %lu %lu %lu %lu %lu %lu %lu\n" \
                        "newgidmap %lu %lu %lu %lu %lu %lu %lu\n" \
                        "exit 0\n",
                        getpid(), 0, 1000, 1, 1, range.uid_start, range.uid_count,
                        getpid(), 0, 1000, 1, 1, range.gid_start, range.gid_count)){
                fatal("could not send data to child with dprintf");
        }
        close(fd[0]);
        close(fd[1]);

        int status;
        waitpid(child, &status, 0);
        if (!WIFEXITED(status))
                fatal("child exited abnormally");
        status = WEXITSTATUS(status);
        if (status){
                fatal("child exited with %d", status);
        }
}

int fullmap_setup() {
        struct passwd *pwent = getpwuid(getuid());
        struct group *grent = getgrgid(getgid());
        if (NULL == pwent) {perror("uid not in passwd"); exit(1);}
        if (NULL == grent) {perror("gid not in db"); exit(1);}

        subid_range_t idrange;
        getidrange(pwent->pw_uid, pwent->pw_name, "/etc/subuid", &idrange.uid_start, &idrange.uid_count);
        getidrange(grent->gr_gid, grent->gr_name, "/etc/subgid", &idrange.gid_start, &idrange.gid_count);

        fullmap_run(idrange);
        return 0;
}

int main(int argc, char* argv[]) {


        //fullmap_setup();

        if (-1 == fullmap_setup()){
                printf("got -1 from fullmap_setup\n");
        }
        execlp("id", "id", NULL);
}
