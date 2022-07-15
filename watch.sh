#!/bin/bash
while inotifywait -e modify --format "%w%f" *.c *.pio; do
    ./build.sh
done