#! /usr/bin/env python3

# Create Makefile targets to run tests, from Meson's test introspection data.
#
# Author: Paolo Bonzini <pbonzini@redhat.com>

from collections import defaultdict
import itertools
import json
import os
import sys

class Suite(object):
    def __init__(self):
        self.deps = set()
        self.speeds = set()
        self.docker_deps = set()

    def names(self, base):
        return [f'{base}-{speed}' for speed in self.speeds]


print(r'''
SPEED = quick

.speed.quick = $(sort $(filter-out %-slow %-thorough %-optional, $1))
.speed.slow = $(sort $(filter-out %-thorough, $1))
.speed.thorough = $(sort $1)

TIMEOUT_MULTIPLIER ?= 1
.mtestargs = --no-rebuild -t $(TIMEOUT_MULTIPLIER)
ifneq ($(SPEED), quick)
.mtestargs += --setup $(SPEED)
endif
.mtestargs += $(subst -j,--num-processes , $(filter-out -j, $(lastword -j1 $(filter -j%, $(MAKEFLAGS)))))

.check.mtestargs = $(MTESTARGS) $(.mtestargs) $(if $(V),--verbose,--print-errorlogs) \
    $(foreach s, $(sort $(.check.mtest-suites)), --suite $s)
.bench.mtestargs = $(MTESTARGS) $(.mtestargs) --benchmark --verbose \
    $(foreach s, $(sort $(.bench.mtest-suites)), --suite $s)''')

introspect = json.load(sys.stdin)

targets = {t['id']: [os.path.relpath(f) for f in t['filename']]
           for t in introspect['targets']}

# collect the docker images uses by the tests. This is totally a hacky
# heuristic that fishes the images out of the test custom commands by
# looking for the qemu/image pattern.
targets_docker = {}
for t in introspect['targets']:
    for src in t.get('target_sources', []):
        for arg in src.get('compiler', []):
            if arg.startswith('qemu/'):
                img = arg.split('/', 1)[1]
                targets_docker[t['id']] = f'docker-image-{img}'
                break

def process_tests(test, targets, targets_docker, suites):
    executable = test['cmd'][0]
    try:
        executable = os.path.relpath(executable)
    except:
        pass

    deps = (targets.get(x, []) for x in test['depends'])
    deps = itertools.chain.from_iterable(deps)
    deps = list(deps)

    docker_deps = [targets_docker[x] for x in test['depends'] if x in targets_docker]

    test_suites = test['suite'] or ['default']
    for s in test_suites:
        # The suite name in the introspection info is "PROJECT" or "PROJECT:SUITE"
        if ':' in s:
            s = s.split(':')[1]
            if s == 'slow' or s == 'thorough':
                continue
        suites[s].deps.update(deps)
        suites[s].docker_deps.update(docker_deps)
        if s.endswith('-slow'):
            s = s[:-5]
            suites[s].speeds.add('slow')
        if s.endswith('-thorough'):
            s = s[:-9]
            suites[s].speeds.add('thorough')

def target_name(suite):
    if suite.endswith('-optional'):
        return suite[0:-9]
    return suite

def emit_prolog(suites, prefix):
    all_targets = ' '.join((f'{prefix}-{target_name(k)}'
                            for k in sorted(suites.keys())))
    all_xml = ' '.join((f'{prefix}-report-{target_name(k)}.junit.xml'
                        for k in sorted(suites.keys())))
    print()
    print(f'all-{prefix}-targets = {all_targets}')
    print(f'all-{prefix}-xml = {all_xml}')
    print(f'.PHONY: {prefix} do-meson-{prefix} {prefix}-report.junit.xml $(all-{prefix}-targets) $(all-{prefix}-xml)')
    print(f'ninja-cmd-goals += $(foreach s, $(.{prefix}.mtest-suites), $(.{prefix}-$s.deps))')
    print(f'docker-cmd-goals += $(foreach s, $(.{prefix}.mtest-suites), $(.{prefix}-$s.docker-deps))')
    print(f'docker-cmd-goals += $(foreach g, $(MAKECMDGOALS), $(.docker-goals.$g))')
    print(f'run-ninja: $(sort $(docker-cmd-goals))')
    print(f'{prefix}-build: run-ninja')
    print(f'{prefix} $(all-{prefix}-targets): do-meson-{prefix}')
    print(f'do-meson-{prefix}: run-ninja; $(if $(MAKE.n),,+)$(MESON) test $(.{prefix}.mtestargs)')
    print(f'{prefix}-report.junit.xml $(all-{prefix}-xml): {prefix}-report%.junit.xml: run-ninja')
    print(f'\t$(MAKE) {prefix}$* MTESTARGS="$(MTESTARGS) --logbase {prefix}-report$*" && ln -f meson-logs/$@ .')

def emit_suite(name, suite, prefix):
    tgtname = target_name(name)
    deps = ' '.join(sorted(suite.deps))
    print()
    print(f'.{prefix}-{tgtname}.deps = {deps}')
    print(f'.ninja-goals.check-build += $(.{prefix}-{tgtname}.deps)')
    if suite.docker_deps:
        docker_deps = ' '.join(sorted(suite.docker_deps))
        print(f'.{prefix}-{tgtname}.docker-deps = {docker_deps}')
        print(f'.docker-goals.check-build += $(.{prefix}-{tgtname}.docker-deps)')

    names = ' '.join(sorted(suite.names(name)))
    targets = f'{prefix}-{tgtname} {prefix}-report-{tgtname}.junit.xml'
    if not name.endswith('-slow') and \
       not name.endswith('-thorough') and \
       not name.endswith('-optional'):
        targets += f' {prefix} {prefix}-report.junit.xml'
    print(f'ifneq ($(filter {targets}, $(MAKECMDGOALS)),)')
    # for the "base" suite possibly add FOO-slow and FOO-thorough
    print(f".{prefix}.mtest-suites += {name} $(call .speed.$(SPEED), {names})")
    print(f'endif')

testsuites = defaultdict(Suite)
for test in introspect['tests']:
    process_tests(test, targets, targets_docker, testsuites)
emit_prolog(testsuites, 'check')
for name, suite in testsuites.items():
    emit_suite(name, suite, 'check')

benchsuites = defaultdict(Suite)
for test in introspect['benchmarks']:
    process_tests(test, targets, targets_docker, benchsuites)
emit_prolog(benchsuites, 'bench')
for name, suite in benchsuites.items():
    emit_suite(name, suite, 'bench')
