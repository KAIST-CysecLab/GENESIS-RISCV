#!/bin/bash
# How to quit: "Ctrl + A" + "x"
set -e

SCRIPT_PATH=$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )
WORKSPACE=$( readlink -f "$SCRIPT_PATH/.." )
SYSROOT=$WORKSPACE/output/sysroot
QEMU=$SYSROOT/bin/qemu-system-riscv64
LINUX=$WORKSPACE/linux/
EXTRA_DIR=$WORKSPACE/qemu-linux-extra-files

echo "QEMU: $QEMU"
echo "Linux: $LINUX"

DEBUG=$1
if [[ "$DEBUG" == "debug" ]]; then
  DEBUG="-s -S"
else
  unset DEBUG
fi

$QEMU $DEBUG -nographic \
  -machine virt \
  -bios $EXTRA_DIR/fw_jump.elf \
  -kernel $LINUX/arch/riscv/boot/Image \
  -append "root=/dev/vda ro console=ttyS0 earlyprintk loglevel=7 nokaslr memblock=debug" \
  -drive file=$EXTRA_DIR/rootfs.ext2,format=raw,id=hd0 \
  -device virtio-blk-device,drive=hd0 \
  -smp 4 -m 4G
