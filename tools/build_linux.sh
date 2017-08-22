#!/bin/sh

BASE_DIR=$(cd `dirname $0`; pwd)

#build qemu-ga
cd $BASE_DIR/../
make clean
./configure
make qemu-ga

if [ ! -f "qemu-ga" ];then
    echo "failed to build qemu-ga."
    exit 1
fi

#arrange install pkg
mkdir -p $BASE_DIR/qemu-guest-agent
cd $BASE_DIR/qemu-guest-agent

#etc
mkdir -p etc/qemu-ga/fsfreeze-hook.d
cp -rf ../../scripts/qemu-guest-agent/fsfreeze-hook etc/qemu-ga
mkdir -p etc/sysconfig
cp -rf ../linux/qemu-ga etc/sysconfig

#usr
mkdir -p usr/bin
mkdir -p usr/lib/systemd/system
mkdir -p usr/lib/udev/rules.d
cp -rf ../../qemu-ga usr/bin
cp -rf ../linux/qemu-guest-agent.service usr/lib/systemd/system
cp -rf ../linux/99-qemu-guest-agent.rules usr/lib/udev/rules.d

#var
mkdir -p var/log/qemu-ga

#install script
cp ../linux/install.sh .

cd $BASE_DIR
tar -zcvf qemu-ga-linux-install.tar.gz qemu-guest-agent
rm -rf qemu-guest-agent

exit 0








