#!/bin/bash

rm /dev/fakefile
rmmod fakefile
make clean
make
insmod fakefile.ko
mknod /dev/fakefile c 240 0
