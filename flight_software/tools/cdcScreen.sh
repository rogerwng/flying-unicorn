#!/bin/bash

# Usage: ./cdcScreen.sh /dev/cu.usbmodemXXXX [baud]
# Example: ./cdcScreen.sh /dev/cu.usbmodem3570355632341 115200

# Ctrl+A Ctrl+C to exit

# Trap Ctrl-C to exit cleanly
trap "echo; echo 'Exiting...'; exit 0" SIGINT SIGTERM

# Check for port argument
if [ -z "$1" ]; then
    echo "Usage: $0 <serial_port> [baudrate]"
    exit 1
fi

PORT="$1"
BAUD="${2:-115200}"  # Default baud rate

echo "Starting screen log viewer on $PORT at $BAUD baud..."
sleep 1

while true; do
    # Run screen in foreground with specified port and baud
    screen "$PORT" "$BAUD"

    # Clear terminal after screen exits
    clear

    echo "Screen session exited, reconnecting in 1s..."
    sleep 1
done


