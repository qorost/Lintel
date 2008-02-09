#!/bin/sh
if [ $# != 2 ]; then 
    echo "Usage: $0 <doxygen-binary-build-dir> <target-share-dir>"
    exit 1
fi

if [ ! -d "$1/doxygen/html" ]; then
    echo "Missing $1/doxygen/html"
    exit 1
fi

BUILD="$1"
TARGET="$2"
echo "Install html to $TARGET/doc/Lintel"
mkdir -p "$TARGET/doc/Lintel"
cp -rp "$BUILD/doxygen/html" "$TARGET/doc/Lintel"
echo "Install man pages to $TARGET/man/man3"
mkdir -p "$TARGET/man/man3"
cp -rp "$BUILD/doxygen/man/man3"/* "$TARGET/man/man3"
