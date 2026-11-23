/*
 * Boilerplate code for running tests forked to a subprocess
 * (e.g. for testing functions to abort() the running process as expected)
 *
 * Copyright (c) 2026 Christian Schoenebeck <qemu_oss@crudebyte.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef FORKTEST_H
#define FORKTEST_H

#include <signal.h>
#include <sys/wait.h>

/* Test case passed without aborting, as expected. */
#define FORKTEST_EXIT_OK 0
/* Test case failed because it did not abort even though expected to abort. */
#define FORKTEST_EXIT_SURVIVED 124
/* Test case (subprocess) returned without calling expected forktest_exit(). */
#define FORKTEST_EXIT_MISSED 255

/*
 * The only way out of a forktest child: terminates it without
 * running the atexit, glib or ASan teardown inherited from the
 * parent. @code is one of the FORKTEST_EXIT codes above.
 */
static inline void forktest_exit(int code)
{
    _exit(code);
}

/* Wraps the actual test code to run, forked into a subprocess. */
typedef struct {
    /* entry point of the test case (to be executed on a subprocess) */
    void (*child_fn)(void *);
    /* user data passed to @child_fn */
    void *opaque;
    /* defines the success criteria: whether subprocess is expected to abort */
    bool expect_abort;
    /* printed as context on failure cases */
    const char *name;
} ForkTest;

static inline bool forktest_aborted(int status)
{
    return WIFSIGNALED(status) && WTERMSIG(status) == SIGABRT;
}

static inline bool forktest_exited(int status, int code)
{
    return WIFEXITED(status) && WEXITSTATUS(status) == code;
}

/* launch (fork and run) the passed test case in a subprocess */
static inline void forktest_run(ForkTest test)
{
    pid_t pid;
    int status = 0;

    pid = fork();
    if (pid == -1) {
        g_error("fork failed: %s", g_strerror(errno));
    }
    if (pid == 0) {
        test.child_fn(test.opaque);
        forktest_exit(FORKTEST_EXIT_MISSED);
    }
    if (waitpid(pid, &status, 0) != pid) {
        g_error("waitpid failed: %s", g_strerror(errno));
    }
    if (forktest_aborted(status)) {
        if (!test.expect_abort) {
            g_error("child on %s aborted unexpectedly",
                    test.name);
        }
    } else if (test.expect_abort &&
               forktest_exited(status, FORKTEST_EXIT_SURVIVED)) {
        g_error("child on %s survived; the case was expected to "
                "abort", test.name);
    } else if (!test.expect_abort &&
               forktest_exited(status, FORKTEST_EXIT_OK)) {
        /* passed cleanly */
    } else {
        g_error("child on %s died unexpectedly (status 0x%x)",
                test.name, status);
    }
}

/*
 * Setup and teardown tasks to be executed before and after a test case
 * respectively, sharable between several test cases.
 */
typedef struct {
    void *(*setup_fn)(void);
    void (*teardown_fn)(void *data);
} ForkTestEnv;

/* internal: g_test data of the trampoline below */
struct ForkTestRun {
    const ForkTestEnv *env;
    void (*test_fn)(void *data);
};

/* internal trampoline function */
static inline void forktest_case_run(const void *opaque)
{
    const struct ForkTestRun *tc = opaque;
    void *state = NULL;

    if (tc->env) {
        state = tc->env->setup_fn();
    }
    tc->test_fn(state);
    if (tc->env) {
        tc->env->teardown_fn(state);
    }
}

/* register a test case; @env may be NULL */
static inline void forktest_add(const char *path,
                                void (*test_fn)(void *data),
                                const ForkTestEnv *env)
{
    struct ForkTestRun *tc = g_new(struct ForkTestRun, 1);

    tc->test_fn = test_fn;
    tc->env = env;
    g_test_add_data_func_full(path, tc, forktest_case_run, g_free);
}

#endif
