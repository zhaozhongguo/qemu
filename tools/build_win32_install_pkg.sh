#!/bin/sh

BASE_DIR=$(cd `dirname $0`; pwd)
MINGW_DIR=/usr/x86_64-w64-mingw32/sys-root/mingw

if [ ! -d "$MINGW_DIR" ];then
    exit 1
fi

##replace comutil.h
cd $BASE_DIR
cp -rf $MINGW_DIR/include/comutil.h $MINGW_DIR/include/comutil.h.bak
cp -rf comutil.h $MINGW_DIR/include
cd -

##build win32 qemu-ga
cd $BASE_DIR/../
make clean
./configure --enable-guest-agent --with-vss-sdk=$BASE_DIR/../VSSSDK72 --cross-prefix=x86_64-w64-mingw32-
make qemu-ga

##recovery env
cp -rf $MINGW_DIR/include/comutil.h.bak $MINGW_DIR/include/comutil.h
rm -rf $MINGW_DIR/include/comutil.h.bak


##make install pkg
cd $BASE_DIR
mkdir qemu-ga

cp -rf $MINGW_DIR/bin/*.dll qemu-ga
cp -rf $BASE_DIR/../qemu-ga.exe qemu-ga
cp -rf $BASE_DIR/../qga/vss-win32/qga-vss.dll qemu-ga
cp -rf $BASE_DIR/../qga/vss-win32/qga-vss.tlb qemu-ga

tar -zcvf qemu-ga-win32-install.tar.gz qemu-ga
rm -rf qemu-ga
cd -

exit 0




