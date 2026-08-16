#!/bin/bash

# Check if clang-format is installed
if ! command -v clang-format &> /dev/null; then
    echo "clang-format could not be found, please install it and add it to your PATH."
    exit 1
fi

git ls-files --exclude-standard | while read -r file; do
    if [[ "$file" =~ \.(c|cpp|h|hpp|cppm)$ ]]; then
        clang-format -i -style=file "$file"
    fi
done