#!/bin/sh
set -e
gcc -v -DTEST -o par_config par_config.c

file1=`mktemp`
./par_config > "$file1"

file2=`mktemp`
grep -E '^[^#]+' -o par_config.h > "$file2"

diff -q "$file1" "$file2"
rm -v "$file1" "$file2"
echo "TEST PASSED"
