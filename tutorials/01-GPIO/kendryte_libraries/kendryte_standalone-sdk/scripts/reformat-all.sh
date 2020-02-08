#!/bin/bash

_CLANG_FORMAT=${CLANG_FORMAT-"clang-format"}

if ! command -v "$_CLANG_FORMAT" &>/dev/null ; then
    echo "You must install clang-format and add it to your PATH." >&2
    echo "    See: http://releases.llvm.org/download.html" >&2
    echo "Use CLANG_FORMAT=/path/to/clang-format if you do not want it in PATH. " >&2

    exit 1
fi

cd "$(dirname "$(realpath "${BASH_SOURCE[0]}")")"
cd ..

DIFF_BEFORE=$(git diff  | wc -l)
echo "Difference before format: $DIFF_BEFORE"

find lib -iname *.h -o -iname *.c -o -iname *.cpp -o -iname *.hpp | xargs "$_CLANG_FORMAT" -i

DIFF_AFTER=$(git diff  | wc -l)
echo "Difference after format: $DIFF_AFTER (delta: $(($DIFF_BEFORE - $DIFF_AFTER)))"
