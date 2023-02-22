/*
 * reimpl/io.c
 *
 * Wrappers and implementations for some of the IO functions.
 *
 * Copyright (C) 2021 Andy Nguyen
 * Copyright (C) 2022 Rinnegatamante
 * Copyright (C) 2022-2023 Volodymyr Atamanenko
 *
 * This software may be modified and distributed under the terms
 * of the MIT license. See the LICENSE file for details.
 */

#include "reimpl/io.h"

#include <string.h>
#include <sys/stat.h>
#include <sys/unistd.h>
#include <stdlib.h>
#include <dirent.h>
#include <psp2/kernel/threadmgr.h>

#include "utils/logger.h"

#define MUSL_O_WRONLY         01
#define MUSL_O_RDWR           02
#define MUSL_O_CREAT        0100
#define MUSL_O_EXCL         0200
#define MUSL_O_TRUNC       01000
#define MUSL_O_APPEND      02000
#define MUSL_O_NONBLOCK    04000

int oflags_newlib_to_oflags_musl(int flags)
{
    int out = 0;
    if (flags & MUSL_O_RDWR)
        out |= O_RDWR;
    else if (flags & MUSL_O_WRONLY)
        out |= O_WRONLY;
    else
        out |= O_RDONLY;
    if (flags & MUSL_O_NONBLOCK)
        out |= O_NONBLOCK;
    if (flags & MUSL_O_APPEND)
        out |= O_APPEND;
    if (flags & MUSL_O_CREAT)
        out |= O_CREAT;
    if (flags & MUSL_O_TRUNC)
        out |= O_TRUNC;
    if (flags & MUSL_O_EXCL)
        out |= O_EXCL;
    return out;
}

dirent64_bionic * dirent_newlib_to_dirent_bionic(struct dirent* dirent_newlib) {
    dirent64_bionic * ret = malloc(sizeof(dirent64_bionic));
    strncpy(ret->d_name, dirent_newlib->d_name, sizeof(ret->d_name));
    ret->d_off = 0;
    ret->d_reclen = 0;
    ret->d_type = SCE_S_ISDIR(dirent_newlib->d_stat.st_mode) ? DT_DIR : DT_REG;
    return ret;
}

void stat_newlib_to_stat_bionic(struct stat * src, stat64_bionic * dst) {
    if (!src) return;
    if (!dst) dst = malloc(sizeof(stat64_bionic));

    dst->st_dev = src->st_dev;
    dst->st_ino = src->st_ino;
    dst->st_mode = src->st_mode;
    dst->st_nlink = src->st_nlink;
    dst->st_uid = src->st_uid;
    dst->st_gid = src->st_gid;
    dst->st_rdev = src->st_rdev;
    dst->st_size = src->st_size;
    dst->st_blksize = src->st_blksize;
    dst->st_blocks = src->st_blocks;
    dst->st_atime = src->st_atime;
    dst->st_atime_nsec = 0;
    dst->st_mtime = src->st_mtime;
    dst->st_mtime_nsec = 0;
    dst->st_ctime = src->st_ctime;
    dst->st_ctime_nsec = 0;
}

struct dirent * readdir_soloader(DIR * dir) {
    struct dirent* ret = readdir(dir);
    log_debug("[io] readdir()");
    return ret;
}

int readdir_r_soloader(DIR *dirp, dirent64_bionic *entry, dirent64_bionic **result) {
    struct dirent dirent_tmp;
    struct dirent* pdirent_tmp;

    int ret = readdir_r(dirp, &dirent_tmp, &pdirent_tmp);

    if (ret == 0) {
        dirent64_bionic* entry_tmp = dirent_newlib_to_dirent_bionic(&dirent_tmp);
        memcpy(entry, entry_tmp, sizeof(dirent64_bionic));
        *result = (pdirent_tmp != NULL) ? entry : NULL;
        free(entry_tmp);
    }

    log_debug("[io] readdir_r()");
    return ret;
}

FILE *fopen_soloader(char *fname, char *mode) {
    FILE* ret =  fopen(fname, mode);
    logv_debug("[io] fopen(%s): 0x%x", fname, ret);
    return ret;
}


int open_soloader(char *_fname, int flags) {
    flags = oflags_newlib_to_oflags_musl(flags);
    int ret = open(_fname, flags);
    logv_debug("[io] open(%s, %x): %i", _fname, flags, ret);
    return ret;
}

int read_soloader(int fd, void * buf, size_t nbyte) {
    int ret = read(fd, buf, nbyte);
    logv_debug("[io] read(fd#%i, 0x%x, %i): %i", fd, buf, nbyte, ret);
    return ret;
}

DIR* opendir_soloader(char* _pathname) {
    DIR* ret = opendir(_pathname);
    logv_debug("[io] opendir(\"%s\"): 0x%x", _pathname, ret);
    return ret;
}

int fstat_soloader(int fd, void *statbuf) {
    struct stat st;
    int res = fstat(fd, &st);
    if (res == 0)
        stat_newlib_to_stat_bionic(&st, statbuf);

    logv_debug("[io] fstat(fd#%i): %i", fd, res);
    return res;
}

int write_soloader(int fd, const void *buf, int count) {
    int ret = write(fd, buf, count);
    logv_debug("[io] write(fd#%i, 0x%x, %i): %i", fd, buf, count, ret);
    return ret;
}

int fcntl_soloader(int fd, int cmd, ...) {
    logv_debug("[io] fcntl(fd#%i, cmd#%i)", fd, cmd);
    return 0;
}

off_t lseek_soloader(int fildes, off_t offset, int whence) {
    off_t ret = lseek(fildes, offset, whence);
    logv_debug("[io] lseek(fd#i, %i, %i): %i", fildes, offset, whence, ret);
    return ret;
}

int close_soloader(int fd) {
    int ret = close(fd);
    logv_debug("[io] close(fd#%i): %i", fd, ret);
    return ret;
}

int fclose_soloader(FILE * f) {
    int ret = fclose(f);
    logv_debug("[io] fclose(0x%x): %i", f, ret);
    return ret;
}

int closedir_soloader(DIR* dir) {
    int ret = closedir(dir);
    logv_debug("[io] closedir(0x%x): %i", dir, ret);
    return ret;
}

int stat_soloader(char *_pathname, stat64_bionic *statbuf) {
    struct stat st;
    int res = stat(_pathname, &st);

    if (res == 0)
        stat_newlib_to_stat_bionic(&st, statbuf);

    logv_debug("[io] stat(%s): %i", _pathname, res);
    return res;
}

int fseeko_soloader(FILE * a, off_t b, int c) {
    int ret = fseeko(a,b,c);
    logv_debug("[io] fseeko(0x%x, %i, %i): %i", a,b,c,ret);
    return ret;
}

off_t ftello_soloader(FILE * a) {
    off_t ret = ftello(a);
    logv_debug("[io] ftello(0x%x): %i", a, ret);
    return ret;
}
