#!/usr/bin/env bash
# Runs the four reference test cases and the verbose sample comparisons.
cd "$(dirname "$0")" || exit 1
make -s || exit 1

fail=0
check() {
    local expected=$1; shift
    local actual
    actual=$(./cachesim "$@" | tail -n 1)
    if [ "$actual" == "$expected" ]; then
        echo "PASS: $*"
    else
        echo "FAIL: $* (expected '$expected', got '$actual')"
        fail=1
    fi
}

check "hits:9 misses:8 evictions:6"            -s 1 -E 1 -b 1 -t traces/trace01.dat
check "hits:4 misses:5 evictions:2"            -s 4 -E 2 -b 4 -t traces/trace02.dat
check "hits:2 misses:3 evictions:1"            -s 2 -E 1 -b 4 -t traces/trace03.dat
check "hits:265189 misses:21775 evictions:21743" -s 5 -E 1 -b 5 -t traces/trace04.dat

for args in "1 1 1 01" "4 2 4 02"; do
    set -- $args
    if ./cachesim -v -s "$1" -E "$2" -b "$3" -t "traces/trace$4.dat" \
        | diff -q - "traces/output/trace$4out.txt" > /dev/null; then
        echo "PASS: verbose trace$4"
    else
        echo "FAIL: verbose trace$4"
        fail=1
    fi
done

exit $fail
