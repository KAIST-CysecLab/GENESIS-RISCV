#!/bin/bash
set -e

QEMU="qemu-7.1.0"
QEMU_LINK="https://download.qemu.org/$QEMU.tar.xz"

SCRIPT_DIR=$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )
WORKSPACE=$( readlink -f "$SCRIPT_DIR/.." )
OUTPUT_DIR=$WORKSPACE/output
SYSROOT=$OUTPUT_DIR/sysroot

mkdir -pv $OUTPUT_DIR && cd $_

wget -nc "$QEMU_LINK" || true
[ ! -d "$OUTPUT_DIR/$QEMU" ] && tar xvJf "$QEMU.tar.xz"

cd $QEMU
./configure --prefix="$SYSROOT" --disable-kvm --disable-werror --target-list="riscv64-softmmu"
make -j$(nproc)
make install
