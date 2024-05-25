#!/bin/sh

if [ ! -f ./boot ]; then
  echo "boot image not exist, copy it from bootcode"
  exit 1
fi

if [ ! -f ./linux.bin ]; then
  echo "linux.bin image not exist"
  exit 1
fi

if [ ! -f ./squashfs-lzma.o ]; then
  echo "squashfs-lzma.o image not exist"
  exit 1
fi

./eccenc --chunk-size 2048 --spare-size 64 --chunk-per-block 64 -bso 23 -bdo 2000 -ctype RTL8198C_NAND -edn BIG_ENDIAN boot 0 linux.bin 9437184 squashfs-lzma.o 14680064

