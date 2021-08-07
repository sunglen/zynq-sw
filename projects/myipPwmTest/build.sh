#!/bin/bash

. /home/s/Xilinx/Vitis/2021.1/settings64.sh
. /home/s/titan4_debug/zynq/AXU3EG/axu3eg_custom_pkg/environment-setup-cortexa72-cortexa53-xilinx-linux

aarch64-linux-gnu-gcc -Wall -O0 -g3 -c -fmessage-length=0 \
--sysroot=/home/s/titan4_debug/zynq/AXU3EG/axu3eg_custom_pkg/sysroots/cortexa72-cortexa53-xilinx-linux \
-I/home/s/titan4_debug/zynq/AXU3EG/axu3eg_custom_pkg/sysroots/cortexa72-cortexa53-xilinx-linux/usr/include \
-MMD -MP -MF"main.d" -MT"main.o" -o "main.o" "main.c"

aarch64-linux-gnu-gcc -L/home/s/titan4_debug/zynq/AXU3EG/axu3eg_custom_pkg/sysroots/cortexa72-cortexa53-xilinx-linux/lib \
-L/home/s/titan4_debug/zynq/AXU3EG/axu3eg_custom_pkg/sysroots/cortexa72-cortexa53-xilinx-linux/usr/lib \
-o "myipPwmTest.elf"  ./main.o   --sysroot=/home/s/titan4_debug/zynq/AXU3EG/axu3eg_custom_pkg/sysroots/cortexa72-cortexa53-xilinx-linux \
-Wl,-rpath-link=/home/s/titan4_debug/zynq/AXU3EG/axu3eg_custom_pkg/sysroots/cortexa72-cortexa53-xilinx-linux/lib \
-Wl,-rpath-link=/home/s/titan4_debug/zynq/AXU3EG/axu3eg_custom_pkg/sysroots/cortexa72-cortexa53-xilinx-linux/usr/lib
