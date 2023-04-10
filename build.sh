#!/usr/bin/env bash

DEBUG=1
SOURCE_FILES="parse_html.c"
# SOURCE_FILES="parse_ical.c"
SETTINGS="-std=c89 -Wall -Wextra -Wstrict-prototypes -Wold-style-definition -Wmissing-prototypes -Wmissing-declarations"

if [ $DEBUG -eq 0 ]; then
    echo "Optimized build";
    TARGET="-c -O2"
elif [ $DEBUG -eq 1 ]; then
    echo "Debug build";
    TARGET="-g3 -O0"
fi

echo $TARGET
echo $SETTINGS
echo $SOURCE_FILES

gcc $TARGET $SETTINGS $SOURCE_FILES
