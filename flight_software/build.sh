#!/bin/bash

set -e

# CD into script directory
cd "$(dirname "${BASH_SOURCE[0]}")"

PRESET="Debug"
CLEAN=false
FLASH=false

# Parse args
for arg in "$@"; do
    case "$arg" in
        --clean)
            CLEAN=true
            ;;
        --flash)
            FLASH=true
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

if [ "$FLASH" = true ]; then
    echo "Flashing application at build/$PRESET/flight_software.elf"
    openocd -f ../openocd.cfg -c "init" -c "halt" -c "program build/$PRESET/flight_software.elf verify reset exit"
fi