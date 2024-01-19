#!/usr/bin/bash

# build all targets by default

# e.g. build only leitor-cli:
# ./scripts/build.sh leitor-cli

target="all"
if [ -n "$1" ]; then
    target="$1"
fi

cmake --build ./build --config Debug --target "$target" -j 6 --

