#!/bin/bash

set -euo pipefall

# CD into script directory
cd "$(dirname "${BASH_SOURCE[0]}")"

PRESET="Debug"
CLEAN=false

# Parse args
for arg in "$@"; do
    case "$arg" in
        --clean)
            CLEAN=true
            ;;
        -*)
            echo "Unknown option: $arg"
            echo "Usage: $0 [preset] [--clean]"
            exit 1
            ;;
        *)
            PRESET="$arg"
            ;;
    esac
done

cd bsp

if [ "$CLEAN" = true ]; then
    echo "Cleaning build/$PRESET"
    rm -rf "build/$PRESET"
fi

cmake --workflow --preset "$PRESET"