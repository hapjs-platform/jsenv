#!/usr/bin/env bash

# Copyright (c) 2021, the hapjs-platform Project Contributors
# SPDX-License-Identifier: EPL-1.0

# Setup the NDK root path
# export ANDROID_NDK_ROOT=...
PWD=`pwd`
HOST_OS=`uname`

BUILD_TOOLS_PRETIX=""

case $HOST_OS in
  Linux ) BUILD_TOOLS_PRETIX="linux64" ;;
  Darwin ) BUILD_TOOLS_PRETIX="mac" ;;
  * ) echo "Host OS not supprted"; exit 1;;
esac

ADDITIONAL_PATH="$PWD/buildtools/$BUILD_TOOLS_PRETIX"

PATH=$ADDITIONAL_PATH:$PATH
