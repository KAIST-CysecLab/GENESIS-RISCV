# GENESIS-RISCV

### QEMU
```
# Prerequisite
$ sudo apt-get install libncurses-dev libssl-dev bc flex bison make gcc gcc-riscv64-linux-gnu

# Build
$ ./buildscript/linux-qemu-compile.sh
$ ./buildscript/qemu-system-install.sh

# QEMU Run
$ ./run_qemu.sh

# QEMU Debug mode
$ ./runscript/run_qemu.sh debug
# [other teminal]
$ ./runscript/run_gdb.sh
```
### VISIONFIVE
```
# Build
$ ./buildscript/visionfive-linux-compile.sh
```

### VisionFive Boot SD Image
```
# refer: https://github.com/starfive-tech/Fedora_on_StarFive/
$ wget https://fedora.starfivetech.com/pub/downloads/VisionFive-release/Fedora-riscv64-jh7100-developer-xfce-Rawhide-20211226-214100.n.0-sda.raw.zst
$ zstd -d Fedora-riscv64-jh7100-developer-xfce-Rawhide-20211226-214100.n.0-sda.raw.zst
$ sudo dd if=/dev/zero of=/dev/sda bs=8192
$ sudo dd if=Fedora-riscv64-jh7100-developer-xfce-Rawhide-20211226-214100.n.0-sda.raw of=/dev/sdX bs=8M status=progress && sync

# Load Boot Image
$ sudo mount -t ext4 /dev/sda3 /mnt
$ sudo cp $LINUX_DIR/arch/risv/boot/Image $LINUX_DIR/arch/riscv/boot/dts/starfive/jh7100-starfive-visionfive-v1.dtb /mnt
$ sudo umount /mnt
```

### Reduce booting time
```
$ sudo systemctl disable bluetooth.service
$ sudo systemctl disable NetworkManager-wait-online.service
$ sudo systemctl disable wpa_supplicant.service
$ sudo systemctl disable systemd-rfkill.socket
$ sudo systemctl disable sssd.service
```
