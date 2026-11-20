#!/bin/sh
set -eu

project_dir=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
tmp_dir=$(mktemp -d)
trap 'rm -rf "$tmp_dir"' EXIT HUP INT TERM

cp "$project_dir/our_tests/test_good_1.as" "$tmp_dir/"
cp "$project_dir/our_tests/test_parse_bad_1.as" "$tmp_dir/"

(
    cd "$tmp_dir"

    "$project_dir/assembler" test_good_1 > test_good_1.log
    cmp test_good_1.am "$project_dir/our_tests/test_good_1.am"
    cmp test_good_1.ob "$project_dir/our_tests/test_good_1.ob"
    cmp test_good_1.ent "$project_dir/our_tests/test_good_1.ent"
    cmp test_good_1.ext "$project_dir/our_tests/test_good_1.ext"

    if "$project_dir/assembler" test_parse_bad_1 > test_parse_bad_1.log 2>&1; then
        echo "Expected test_parse_bad_1 to fail" >&2
        exit 1
    fi
    grep -q "Error:" test_parse_bad_1.log
)

echo "Smoke tests passed"
