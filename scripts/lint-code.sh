#!/bin/bash

# TODO: Add support for linting only staged files
# This would allow running: ./scripts/lint-code.sh --staged

if ! command -v python3 &> /dev/null; then
    echo "python3 could not be found"
    exit 1
fi

if ! command -v clang-tidy &> /dev/null; then
    echo "clang-tidy could not be found"
    exit 1
fi

run-clang-tidy -p ./build/native/ -extra-arg="--experimental-modules-support" -extra-arg="-fprebuilt-module-path=./build/native/src/engine/CMakeFiles/engine-core.dir/Debug"