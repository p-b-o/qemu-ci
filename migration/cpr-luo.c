/*
 * Copyright (c) 2026 Oracle and/or its affiliates.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "qemu/osdep.h"

#include <sys/ioctl.h>

#include "io/channel-file.h"
#include "linux-headers/linux/liveupdate.h"
#include "migration/cpr.h"
#include "migration/qemu-file.h"
#include "qapi/error.h"
#include "qemu/error-report.h"
#include "qemu/memfd.h"

#define CPR_LUO_STATE_NAME "QEMU_CPR_LUO_STATE"

static int luo_open(Error **errp)
{
    return qemu_open("/dev/liveupdate", O_RDWR, errp);
}

int cpr_luo_create_session(const char *name, Error **errp)
{
    struct liveupdate_ioctl_create_session create = { 0 };
    int fd;

    if (g_strlcpy((char *)create.name, name, sizeof(create.name)) >=
        sizeof(create.name)) {
        error_setg(errp, "LUO session name is too long");
        return -1;
    }

    fd = luo_open(errp);
    if (fd < 0) {
        return -1;
    }

    create.size = sizeof(create);
    if (ioctl(fd, LIVEUPDATE_IOCTL_CREATE_SESSION, &create) < 0) {
        error_setg_errno(errp, errno, "failed to create LUO session '%s'",
                         (char *)create.name);
        close(fd);
        return -1;
    }

    close(fd);
    return create.fd;
}

int cpr_luo_retrieve_session(const char *name, Error **errp)
{
    struct liveupdate_ioctl_retrieve_session retrieve = { 0 };
    int fd;

    if (g_strlcpy((char *)retrieve.name, name, sizeof(retrieve.name)) >=
        sizeof(retrieve.name)) {
        error_setg(errp, "LUO session name is too long");
        return -1;
    }

    fd = luo_open(errp);
    if (fd < 0) {
        return -1;
    }

    retrieve.size = sizeof(retrieve);
    if (ioctl(fd, LIVEUPDATE_IOCTL_RETRIEVE_SESSION, &retrieve) < 0) {
        error_setg_errno(errp, errno, "failed to retrieve LUO session '%s'",
                         (char *)retrieve.name);
        close(fd);
        return -1;
    }

    close(fd);
    return retrieve.fd;
}

int cpr_luo_preserve_fd(int session_fd, int fd, uint64_t token,
                        Error **errp)
{
    struct liveupdate_session_preserve_fd preserve = {
        .size = sizeof(preserve),
        .fd = fd,
        .token = token,
    };

    if (ioctl(session_fd, LIVEUPDATE_SESSION_PRESERVE_FD, &preserve) < 0) {
        error_setg_errno(errp, errno, "failed to preserve fd %d with LUO",
                         fd);
        return -1;
    }

    return 0;
}

int cpr_luo_retrieve_fd(int session_fd, uint64_t token, Error **errp)
{
    struct liveupdate_session_retrieve_fd retrieve = {
        .size = sizeof(retrieve),
        .token = token,
    };

    if (ioctl(session_fd, LIVEUPDATE_SESSION_RETRIEVE_FD, &retrieve) < 0) {
        error_setg_errno(errp, errno, "failed to retrieve LUO fd");
        return -1;
    }

    return retrieve.fd;
}

QEMUFile *cpr_luo_output(Error **errp)
{
    int fd = qemu_memfd_create(CPR_LUO_STATE_NAME, 0, false, 0, 0, errp);

    if (fd < 0) {
        return NULL;
    }

    return cpr_file_new_fd_output(fd, CPR_LUO_STATE_NAME);
}

QEMUFile *cpr_luo_input(int session_fd, Error **errp)
{
    int fd = cpr_luo_retrieve_fd(session_fd, CPR_LUO_STATE_TOKEN, errp);

    if (fd < 0) {
        return NULL;
    }

    lseek(fd, 0, SEEK_SET);
    return cpr_file_new_fd_input(fd, CPR_LUO_STATE_NAME);
}

bool cpr_luo_persist_state(int session_fd, QEMUFile *f, Error **errp)
{
    QIOChannelFile *fioc = QIO_CHANNEL_FILE(qemu_file_get_ioc(f));

    /*
     * The file size (the memfd in this case) cannot be changed after it
     * is preserved. Flush all data before preserving the state fd.
     */
    if (qemu_fflush(f) < 0) {
        error_setg(errp, "failed to flush cpr-luo state");
        return false;
    }

    return cpr_luo_preserve_fd(session_fd, fioc->fd,
                               CPR_LUO_STATE_TOKEN, errp) == 0;
}

/*
 * This function must be called only after cpr_fd_pre_save() has assigned
 * LUO tokens.
 */
static bool cpr_luo_preserve_one(CprFd *elem, Error **errp)
{
    if (!elem->luo) {
        return true;
    }

    if (!elem->luo_token) {
        error_setg(errp, "missing LUO token for CprFd %s id %d",
                   elem->name, elem->id);
        return false;
    }

    return cpr_luo_preserve_fd(cpr_state.luo_session_fd, elem->fd,
                               elem->luo_token, errp) == 0;
}

bool cpr_luo_preserve_fds(Error **errp)
{
    return cpr_walk_fd(cpr_luo_preserve_one, errp);
}

/*
 * This function must be called only after cpr_fd_post_load() has
 * initialized luo and received luo_token.
 */
static bool cpr_luo_retrieve_one(CprFd *elem, Error **errp)
{
    int retrieved_fd;

    if (!elem->luo) {
        return true;
    }

    if (!elem->luo_token) {
        error_setg(errp, "missing LUO token for CprFd %s id %d",
                   elem->name, elem->id);
        return false;
    }

    retrieved_fd = cpr_luo_retrieve_fd(cpr_state.luo_session_fd,
                                       elem->luo_token, errp);

    if (retrieved_fd < 0) {
        return false;
    }

    elem->fd = retrieved_fd;
    return true;
}

bool cpr_luo_retrieve_fds(Error **errp)
{
    return cpr_walk_fd(cpr_luo_retrieve_one, errp);
}

void cpr_luo_abort(void)
{
    int session_fd = cpr_state.luo_session_fd;

    if (session_fd >= 0) {
        close(session_fd);
    }

    cpr_state.luo_session_fd = -1;
}
