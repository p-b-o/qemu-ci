/*
 * Copyright (c) 2026 Oracle and/or its affiliates.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "qemu/osdep.h"

#include "migration/cpr.h"
#include "qapi/error.h"

static int cpr_luo_unsupported(Error **errp)
{
    error_setg(errp, "cpr-luo requires Linux");
    return -1;
}

int cpr_luo_create_session(const char *name, Error **errp)
{
    return cpr_luo_unsupported(errp);
}

int cpr_luo_retrieve_session(const char *name, Error **errp)
{
    return cpr_luo_unsupported(errp);
}

int cpr_luo_preserve_fd(int session_fd, int fd, uint64_t token,
                        Error **errp)
{
    return cpr_luo_unsupported(errp);
}

int cpr_luo_retrieve_fd(int session_fd, uint64_t token, Error **errp)
{
    return cpr_luo_unsupported(errp);
}
