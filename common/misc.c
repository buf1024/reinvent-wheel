/*
 * misc.c
 *
 *  Created on: 2014-1-23
 *      Author: Luo Guochun
 */


#include "misc.h"
#include <sys/types.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <fcntl.h>
#include <signal.h>
#include <errno.h>
#include <unistd.h>

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <pwd.h>
#include <grp.h>


enum {
    MAX_PATH_LEN = 256,
};

sigfunc* set_signal(int signo, sigfunc* func, int interupt)
{
    struct sigaction act, oact;
    act.sa_handler = func;
    sigemptyset(&act.sa_mask);
    act.sa_flags = 0;

    if (!interupt){
        act.sa_flags |= SA_RESTART;
    }
    if (sigaction(signo, &act, &oact) < 0) {
        return SIG_ERR ;
    }
    return oact.sa_handler;
}

int make_daemon()
{
    if (fork() > 0) {
        exit(0);
    }
    setsid();
    if (fork() > 0) {
        exit(0);
    }
    return 0;
}

int is_prog_runas_root()
{
    if(getuid() == 0 || geteuid() == 0){
        return 1;
    }
    return 0;
}

int is_prog_running(const char* name)
{
    if(name == NULL || name[0] == 0) return -1;

    char file[MAX_PATH_LEN] = { 0 };
    snprintf(file, MAX_PATH_LEN - 1, "/tmp/%s.running", name);

    umask(0111);

    int fd = open(file, O_RDWR | O_CREAT | O_SYNC, 0666);
    if (fd < 0) {
        return -1;
    }

    struct flock lck = {0};
    lck.l_type = F_WRLCK;
    lck.l_whence = SEEK_SET;
    lck.l_start = sizeof(pid_t);

    int len = 0;
    pid_t pid = -1;

    if(fcntl(fd, F_SETLK, &lck) < 0){
        len = read(fd, &pid, sizeof(pid_t));
        if(len != sizeof(pid_t)) {
            printf("read pid failed. error: %s\n", strerror(errno));
            close(fd);
            return -1;
        }
        close(fd);
        return 1;
    }

    pid = getpid();

    lseek(fd, 0, SEEK_SET);
    len = write(fd, &pid, sizeof(pid_t));
    if(len != sizeof(pid_t)){
        lck.l_type = F_UNLCK;
        fcntl(fd, F_SETLK, &lck);
        close(fd);
        return -1;
    }
    return 0;
}

int runas(const char* user)
{
    struct passwd* passwd = getpwuid(getuid());

    if(strcmp(passwd->pw_name, user) == 0) {
        return 0;
    }

    passwd = getpwnam(user);
    if(passwd == NULL) {
        return -1;
    }

/*    struct group* group = getgrnam(grp);
    if(group == NULL) {
        return -1;
    }*/

    if(setuid(passwd->pw_uid) != 0) {
        fprintf(stderr, "setuid failed,: %s\n", strerror(errno));
        return -1;
    }
/*    if(setgid(group->gr_gid) != 0){
        fprintf(stderr, "setgid failed: %s\n", strerror(errno));
        return -1;
    }*/

    return 0;
}

int write_pid_file(const char* file)
{
    FILE* fp = fopen(file, "w");
    if(fp == NULL){
        return -1;
    }

    fprintf(fp, "%d", getpid());

    fclose(fp);

    return 0;
}
int read_pid_file(pid_t* pid, const char* file)
{
    FILE* fp = fopen(file, "r");
    if(fp == NULL){
        return -1;
    }

    fscanf(fp, "%d", pid);

    fclose(fp);

    return 0;
}

/*
 * Routine to see if a text string is matched by a wildcard pattern.
 * Returns 1 if the text is matched, or 0 if it is not matched
 * or if the pattern is invalid.
 *  *        matches zero or more characters
 *  ?        matches a single character
 *  [abc]    matches 'a', 'b' or 'c'
 *  \c       quotes character c
 *  Adapted from code written by Ingo Wilken.
 */
int match(const char* text, const char* pattern)
{
    if (NULL == text || NULL == pattern) {
        return 0;
    }
    const char * retryPat;
    const char * retryText;
    int ch;
    int found;

    retryPat = NULL;
    retryText = NULL;

    while (*text || *pattern) {
        ch = *pattern++;

        switch (ch) {
        case '*':
            retryPat = pattern;
            retryText = text;
            break;

        case '[':
            found = 0;

            while ((ch = *pattern++) != ']') {
                if (ch == '\\')
                    ch = *pattern++;

                if (ch == '\0')
                    return 0;

                if (*text == ch)
                    found = 1;
            }

            if (!found) {
                pattern = retryPat;
                text = ++retryText;
            }

            if (text) {
                if (*text++ == '\0')
                    return 0;
            }

            break;

        case '?':
            if (*text++ == '\0')
                return 0;

            break;

        case '\\':
            ch = *pattern++;

            if (ch == '\0')
                return 0;

            if (*text == ch) {
                if (*text)
                    text++;
                break;
            }

            if (*text) {
                pattern = retryPat;
                text = ++retryText;
                break;
            }

            return 0;

        default:
            if (*text == ch) {
                if (*text)
                    text++;
                break;
            }

            if (*text) {
                pattern = retryPat;
                text = ++retryText;
                break;
            }

            return 0;
        }

        if (pattern == NULL)
            return 0;
    }

    return 1;
}

int split(const char* text, char needle, char** dest, int size, int num)
{
    if (!text || text[0] == 0 || !dest || size <= 0 || num <= 0) {
        return -1;
    }
    const char* orig = text;
    const char* tmp = text;

    int cur_num = 0;
    while ((tmp = strchr(tmp, needle)) != NULL) {
        int len = tmp - orig;

        if (len >= size) {
            return -1;
        }
        if (cur_num >= num) {
            return -1;
        }

        memcpy((char*)dest + size*cur_num, orig, len);
        *((char*)dest + size*cur_num + len) = 0;

        cur_num++;
        tmp++;
        orig = tmp;
    }
    if(orig != NULL){
        strcpy((char*)dest + size*cur_num, orig);
        cur_num ++ ;
    }
    if(cur_num == 0){
        strcpy((char*)dest, text);
        cur_num++;
    }
    return cur_num;
}

const char* trim_left(const char* text, const char* needle, char* dest, int size)
{
    if(!text || text[0] == 0 ||
            !needle || needle[0] == 0 ||
            !dest || size <= 0){
        return NULL;
    }

    memset(dest, 0, size);

    int count = 0;
    int match = 0;
    while (*(text + count) != 0) {
        if (NULL != strchr(needle, *(text + count))) {
            ++match;
        } else {
            break;
        }
        ++count;
    }
    count = strlen(text + match);
    if (count >= size) {
        return NULL;
    }
    strncpy(dest, text + match, size);

    return dest;
}

const char* trim_right(const char* text, const char* needle, char* dest, int size)
{
    if(!text || text[0] == 0 ||
            !needle || needle[0] == 0 ||
            !dest || size <= 0){
        return NULL;
    }

    memset(dest, 0, size);

    int len = strlen(text);

    const char *p = text + len - 1;

    while (p >= text) {
        if (strchr(needle, *p) != NULL) {
            --p;
            --len;
        } else {
            break;
        }

    }
    if(len >= size) {
        return NULL;
    }
    strncpy(dest, text, len);

    return dest;
}

const char* trim(const char* text, const char* needle, char* dest, int size)
{
    char tmp[size];
    memset(tmp, 0, size);

    if(trim_left(text, needle, tmp, size) == NULL) {
        return NULL;
    }

    if(trim_right(tmp, needle, dest, size) == NULL) {
        return NULL;
    }
    return dest;
}