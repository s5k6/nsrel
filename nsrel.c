#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <err.h>
#include <string.h>
#include <limits.h>

/* copied >>> */
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <sys/sysmacros.h>

#ifndef NS_GET_USERNS
#define NSIO    0xb7
#define NS_GET_USERNS   _IO(NSIO, 0x1)
#define NS_GET_PARENT   _IO(NSIO, 0x2)
#define NS_GET_NSTYPE   _IO(NSIO, 0x3)
#define NS_GET_OWNER_UID _IO(NSIO, 0x4)
#endif
/* <<< copied */

#include <sched.h>
#include <sys/stat.h>
#include "stack.h"



/* Map namespace type identifier to namespace type name.  Use names as
are used in `/proc/.../ns/` */

char const * nstypename(int t) {
    if (t == CLONE_NEWCGROUP)
        return "cgroup";
    if (t == CLONE_NEWIPC)
        return "ipc";
    if (t == CLONE_NEWNET)
        return "net";
    if (t == CLONE_NEWNS)
        return "mnt";
    if (t == CLONE_NEWPID)
        return "pid";
    if (t == CLONE_NEWUSER)
        return "user";
    if (t == CLONE_NEWUTS)
        return "uts";
    return "<unknown>";
}



// Return inode number of file descriptor.

ino_t fdino(int fd) {
    struct stat st;
    if (fstat(fd, &st) != 0)
        err(1, "stat(%i)", fd);
    return st.st_ino;
}



// constants for tab manipulation on the terminal

char const *tabInit = "\x1b[s\x1b[3g\r"; // save position, clear tabs, go left
char const *tabSet = "\x1b[%dC\x1bH"; // move right %d, add tab
char const *tabDone = "\x1b[u"; // restore position



/* If namespace with ID `nsId` is not on the `seen` stack yet, add it,
and also put its file descriptor `nsFd` on the `stack` stack.
Otherwise, close `nsFd`. */

#define addIfNew(nsFd, nsId) do {                                   \
    int new = 1;                                                    \
    for (size_t i = 0; i < COUNT(seen); i++) {                      \
        if (AT(seen, i) == (nsId)) {                                \
            new = 0;                                                \
            close(nsFd);                                            \
            break;                                                  \
        }                                                           \
    }                                                               \
    if (new) {                                                      \
        ENOUGH(stack); PUSH(stack, (nsFd));                         \
        ENOUGH(seen); PUSH(seen, (nsId));                           \
    }                                                               \
} while(0)



int main(int argc, char **argv) {

    // stack of open descriptors we still need to process
    STACK(int) stack;
    ALLOCATE(stack, 10);

    // stack of namespace IDs we have seen so far
    STACK(ino_t) seen;
    ALLOCATE(seen, 10);

    if (argc == 2) { // arguments: <path>

        int fd = open(argv[1], O_RDONLY);
        if (fd < 0)
            err(1, "Open %s", argv[1]);

        addIfNew(fd, fdino(fd));

    } else if (argc == 3) { // arguments: <pid> <type>

        // construct path from pid and nstype
        char *buf = malloc(PATH_MAX);
        if (!buf)
            err(1, "malloc");
        if (snprintf(buf, PATH_MAX, "/proc/%s/ns/%s", argv[1], argv[2]) < 0)
            err(1, "snprintf");

        int fd = open(buf, O_RDONLY);
        if (fd < 0)
            err(1, "Open %s", buf);
        free(buf);

        addIfNew(fd, fdino(fd));

    } else {

        printf(
            "\n"
#include "nsrel.help"
            "\n"
        );

        return 0;
    }

    // adjust tabstops when printing to a terminal
    if (isatty(1)) {
        printf(tabInit);
        printf(tabSet, 12);
        printf(tabSet, 7);
        printf(tabSet, 12);
        printf(tabSet, 12);
        printf(tabDone);
    }

    printf("ID\tTYPE\tPARENT\tUSERNS\tUID\n");

    while (COUNT(stack) > 0) {
        int fd = POP(stack);

        ino_t id = fdino(fd);

        printf("%zu", id);

        printf("\t");

        int nstype = ioctl(fd, NS_GET_NSTYPE);
        if (nstype != -1)
            printf("%s", nstypename(nstype));
        else {
            printf("<not a namespace>\n");
            continue;
        }

        printf("\t");

        int parent = ioctl(fd, NS_GET_PARENT);
        if (parent != -1) {
            ino_t parentId = fdino(parent);
            printf("%zu", parentId);
            addIfNew(parent, parentId);
        } else {
            if (errno == EPERM)
                printf("<oos>");
            else if (errno == ENOTTY)
                printf("<not implemented>");
            else if (errno == EINVAL)
                printf("-");
            else
                printf("???");
        }

        printf("\t");

        int user = ioctl(fd, NS_GET_USERNS);
        if (user != -1) {
            ino_t userId = fdino(user);
            printf("%zu", userId);
            addIfNew(user, userId);
        } else {
            if (errno == EPERM)
                printf("<oos>");
            else if (errno == ENOTTY)
                printf("<not implemented>");
            else if (errno == EINVAL)
                printf("-");
            else
                printf("???");
        }

        printf("\t");

        uid_t owner;
        int owner_valid = ioctl(fd, NS_GET_OWNER_UID, &owner) != -1;
        if (owner_valid)
            printf("%u", owner);
        else {
            if (errno == EINVAL)
                printf("-");
            else
                printf("???");
        }

        printf("\n");

    }

    // restore 9 tabs every 8 columns
    if (isatty(1)) {
        printf(tabInit);
        for (int i = 0; i < 10; i++)
            printf(tabSet, 8);
        printf(tabDone);
    }

    return 0;
}
