/*
 * Copyright (c) 2026 Oracle and/or its affiliates.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "qemu/osdep.h"

#include <sys/ioctl.h>

#include "linux-headers/linux/liveupdate.h"
#include "migration/cpr.h"
#include "qapi/error.h"

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
