/*
 * tests of 9p server's internal functions
 *
 * Copyright (c) 2026 Christian Schoenebeck <qemu_oss@crudebyte.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

/*
 * Not so fast! You might want to read the 9p developer docs first:
 * https://wiki.qemu.org/Documentation/9p
 */

#include "qemu/osdep.h"
#include "qemu/module.h"
#include "fsdev/file-op-9p.h"
#include "hw/9pfs/9p-local.h"
#include "forktest.h"
#include <stdlib.h>

/* one filesystem entry of this test */
typedef struct {
    const char *path;
    bool is_dir;
} V9pInternalsFileTreeEntry;

/* the test's filesystem tree */
static const V9pInternalsFileTreeEntry v9p_internals_file_tree[] = {
    { "export_root", true },
    { "export_root/deep_dir", true },
    { "outside.txt", false },
    { "export_root/inside.txt", false },
    { "export_root/...", false },
    { "export_root/....", false },
};

/* valid paths expected to be allowed by the test filesystem */
static const char *v9p_internals_legal_paths[] = {
    "inside.txt",
    ".", /* export root */
    "...",
    "....",
};

/* illegal paths expected to cause an abort */
static const char *v9p_internals_corrupt_paths[] = {
    /* attempts export root escape via ".." */
    "deep_dir/../../outside.txt",
    /* attempts export root escape via absolute path */
    "/outside.txt",
    /* empty path component (openat(fd, "")) */
    "deep_dir//outside.txt",
    /* trailing slash (openat(fd, "")) */
    "deep_dir/",
    /* empty path (openat(fd, "")) */
    "",
};

/* shared test-suite state */
typedef struct {
    LocalData localdata;
    FsContext ctx;
    char *test_dir;
} V9pInternalsState;

/* create the tmp directory for the test's filesystem tree */
static char *v9p_internals_create_test_dir(void)
{
    char *test_dir = g_dir_make_tmp("9p-internals-XXXXXX", NULL);

    g_assert_nonnull(test_dir);
    return test_dir;
}

/* create regular file in the test filesystem */
static void v9p_internals_create_file(const char *path)
{
    int fd = open(path, O_RDWR | O_CREAT | O_TRUNC, 0644);

    g_assert_cmpint(fd, !=, -1);
    g_assert_cmpint(close(fd), ==, 0);
}

/* create the filesystem tree for this test */
static void v9p_internals_create_tree(const char *base)
{
    size_t i;

    for (i = 0; i < ARRAY_SIZE(v9p_internals_file_tree); i++) {
        const V9pInternalsFileTreeEntry *e = &v9p_internals_file_tree[i];
        g_autofree char *path = g_build_filename(base, e->path, NULL);

        if (e->is_dir) {
            g_assert_cmpint(g_mkdir_with_parents(path, 0755), ==, 0);
        } else {
            v9p_internals_create_file(path);
        }
    }
}

/* prepare to run next test case */
static void *v9p_internals_setup(void)
{
    V9pInternalsState *s = g_new0(V9pInternalsState, 1);
    g_autofree char *export_root;

    s->test_dir = v9p_internals_create_test_dir();
    v9p_internals_create_tree(s->test_dir);
    export_root = g_build_filename(s->test_dir, "export_root", NULL);
    s->localdata.mountfd = open(export_root, O_RDONLY | O_DIRECTORY);
    g_assert_cmpint(s->localdata.mountfd, !=, -1);
    s->ctx.private = &s->localdata;
    return s;
}

/* cleanup after running test case */
static void v9p_internals_teardown(void *data)
{
    V9pInternalsState *s = data;
    g_autofree char *cmd = g_strdup_printf("rm -fr '%s'\n", s->test_dir);
    int res;

    close(s->localdata.mountfd);
    res = system(cmd);
    if (res < 0) {
        /* ignore any error */
    }
    g_free(s->test_dir);
    g_free(s);
}

static const ForkTestEnv v9p_internals_env = {
    .setup_fn = v9p_internals_setup,
    .teardown_fn = v9p_internals_teardown,
};

static void v9p_internals_local_open_nofollow_legal_paths(void *data)
{
    V9pInternalsState *s = data;
    size_t i;
    int fd;

    for (i = 0; i < ARRAY_SIZE(v9p_internals_legal_paths); i++) {
        const char *path = v9p_internals_legal_paths[i];

        fd = local_open_nofollow(&s->ctx, path, O_RDONLY, 0);
        g_assert_cmpint(fd, !=, -1);
        close(fd);
    }
}

typedef struct {
    FsContext *ctx;
    const char *path;
} V9pInternalsPathCase;

static void v9p_internals_check_corrupt_path(void *opaque)
{
    V9pInternalsPathCase *c = opaque;

    local_open_nofollow(c->ctx, c->path, O_RDONLY, 0);
    forktest_exit(FORKTEST_EXIT_SURVIVED);
}

static void v9p_internals_local_open_nofollow_corrupt_paths(void *data)
{
    V9pInternalsState *s = data;
    size_t i;

    for (i = 0; i < ARRAY_SIZE(v9p_internals_corrupt_paths); i++) {
        const char *path = v9p_internals_corrupt_paths[i];
        V9pInternalsPathCase c = {
            .ctx = &s->ctx,
            .path = path,
        };

        forktest_run((ForkTest) {
            .child_fn = v9p_internals_check_corrupt_path,
            .opaque = &c,
            .expect_abort = true,
            .name = path,
        });
    }
}

int main(int argc, char **argv)
{
    g_test_init(&argc, &argv, NULL);

    forktest_add("/9p-internals/local_open_nofollow/legal-paths",
                 v9p_internals_local_open_nofollow_legal_paths,
                 &v9p_internals_env);
    forktest_add("/9p-internals/local_open_nofollow/corrupt-paths",
                 v9p_internals_local_open_nofollow_corrupt_paths,
                 &v9p_internals_env);
    return g_test_run();
}
