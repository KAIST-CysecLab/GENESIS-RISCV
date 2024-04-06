#!/bin/bash

IP="" #EDIT
GATEWAY="" #EDIT

# Configure Static IP
sudo nmcli connection modify 'Wired connection 1' IPv4.address $IP \
	IPv4.gateway $GATEWAY \
	IPv4.dns 8.8.8.8 \
	IPv4.method manual
sudo nmcli connection down 'Wired connection 1'
sudo nmcli connection up 'Wired connection 1'

# Install SSHD
sudo dnf install openssh-server

sudo systemctl enable sshd
sudo systemctl start sshd

# install libtirpc-delve
# refer: https://rpmfind.net/linux/RPM/opensuse/ports/tumbleweed/riscv64/libtirpc-devel-1.3.2-2.1.riscv64.html
#wget https://rpmfind.net/linux/opensuse/ports/riscv/tumbleweed/repo/oss/riscv64/libtirpc-devel-1.3.3-1.1.riscv64.rpm
wget https://downloads.sourceforge.net/libtirpc/libtirpc-1.3.3.tar.bz2
bzip2 -d libtirpc-1.3.3.tar.bz2
tar xf libtirpc-1.3.3.tar
cd libtirpc-1.3.3
./configure --prefix=/usr
#            -bann-sysconfdir=/etc                           \
#            --disable-static                                \
#            --disable-gssapi
make
sudo make install
