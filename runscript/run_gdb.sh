#!/bin/bash
set -e

SCRIPT_PATH=$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )
WORKSPACE=$( readlink -f "$SCRIPT_PATH/.." )
LINUX=$WORKSPACE/linux

cd $LINUX
gdb-multiarch vmlinux -x $SCRIPT_PATH/gdb-remote.txt
