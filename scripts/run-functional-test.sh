#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-2.0-or-later

set -euo pipefail

test=$(basename "$*" .py)
test_log=$(mktemp -d)

test_dir()
{
    find tests/functional -name "$test"'.*' || true
}

show_logs()
{
    for log in $(test_dir)/*.log; do
        cat << EOF
-----------------------------------------------------
$(echo $log)
$(tail -n 200 $log)
EOF
    done
    echo
}

trap "rm -rf $test_log" EXIT
trap show_logs SIGTERM

delay=10m
num_tries=3

err=0
for try in $(seq 1 $num_tries); do
    rm -rf $(test_dir)
    timeout $delay "$@" > $test_log/stdout_$try 2>> $test_log/stderr_$try || err=$?

    cat $test_log/stdout_$try >> $test_log/full_output
    cat $test_log/stderr_$try >> $test_log/full_output
    mv $test_log/stdout_$try $test_log/stdout

    if [ $err -eq 0 ]; then
        # success, show results
        cat $test_log/stdout
        exit 0
    fi
    show_logs >> $test_log/full_output
    if [[ $err -eq 124 || $err -eq 137 ]]; then
        echo "TIMEOUT on try $try" >> $test_log/full_output
    else
        echo "FAIL on try $try" >> $test_log/full_output
    fi
    echo "===================================================" >> $test_log/full_output
done

if [[ $err -eq 124 || $err -eq 137 ]]; then
    echo "not ok TIMEOUT" >> $test_log/stdout
fi

cat $test_log/stdout
cat $test_log/full_output >&2
exit $err
