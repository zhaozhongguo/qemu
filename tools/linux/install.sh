#!/bin/sh

BASE_DIR=$(cd `dirname $0`; pwd)

yum remove qemu-guest-agent -y

cd $BASE_DIR

cp -rf etc /
cp -rf usr /
cp -rf var /

mkdir -p /usr/local/var/run/

systemctl enable qemu-guest-agent.service
service qemu-guest-agent restart

cd -

echo "=== success to install qemu-ga ==="

