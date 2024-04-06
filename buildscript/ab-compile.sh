#!/bin/sh

set -e

# Download AB
git clone --depth=1 https://github.com/CloudFundoo/ApacheBench-ab

cd ApacheBench-ab

# Modify Makefile
vi Makefile
# Comment out the first 'gcc' command
# Uncomment the second 'gcc' command

# Change APR version
# (+ Chnage URL)
vi arp/Makefile
# Download apr-1.7.0
# Download apr-util-1.6.1

make -j$(nproc)
