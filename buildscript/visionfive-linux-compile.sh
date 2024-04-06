#!/bin/bash
set -e

SCRIPT_DIR=$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )
WORKSPACE=$( readlink -f "$SCRIPT_DIR/.." )
LINUX_DIR=$WORKSPACE/linux

export ARCH=riscv
export CROSS_COMPILE=riscv64-linux-gnu-

cd $LINUX_DIR

make visionfive_defconfig

# Configure GENESIS
$LINUX_DIR/scripts/config -e GENESIS
$LINUX_DIR/scripts/config -e GENESIS_PAGE_TABLE_ISOLATION
$LINUX_DIR/scripts/config -d DEBUG_VM # Turn-off the check on 'refcount'

# Configure DEBUG Info
$LINUX_DIR/scripts/config -e DEBUG_INFO
$LINUX_DIR/scripts/config -e DEBUG_INFO_DWARF_TOOLCHAIN_DEFAULT
$LINUX_DIR/scripts/config -e GDB_SCRIPTS
$LINUX_DIR/scripts/config -d DEBUG_INFO_NONE
$LINUX_DIR/scripts/config -d DEBUG_INFO_REDUCED
$LINUX_DIR/scripts/config -d DEBUG_INFO_COMPRESSED
$LINUX_DIR/scripts/config -d DEBUG_INFO_SPLIT
$LINUX_DIR/scripts/config -d DEBUG_INFO_BTF

make -j$(nproc)
