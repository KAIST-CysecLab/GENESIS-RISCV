#!/bin/bash
# Require sudo
set -e

SPEC_ISO="cpu2017-1.1.0.iso"
OUT="$HOME/cpu2017"
TEMP="/mnt"

# mount cpu2017.iso 
sudo mount -t iso9660 -o ro,loop $SPEC_ISO $TEMP

# install (default: $HOME/cpu2017)
mkdir -p $OUT
cp -r $TEMP/* $OUT

# change permission
sudo chown -R $USER $OUT
sudo chmod -R 755 $OUT

#unomunt cpu2017.iso
sudo umount /mnt
