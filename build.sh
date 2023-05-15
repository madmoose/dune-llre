#!/usr/bin/env bash
set -e

SOURCE_DIR=$(dirname "${BASH_SOURCE[0]}")
BUILD_DIR=`pwd`

# If we've been asked to build in the source tree,
# create a Build folder.
if [ $BUILD_DIR -ef $SOURCE_DIR ]; then
	BUILD_DIR="$SOURCE_DIR/Build"
	mkdir -p $BUILD_DIR
fi

cmake -GNinja -DCMAKE_BUILD_TYPE=Release $SOURCE_DIR -B $BUILD_DIR
ninja -C $BUILD_DIR
