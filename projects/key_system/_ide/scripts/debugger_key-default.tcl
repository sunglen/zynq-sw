# Usage with Vitis IDE:
# In Vitis IDE create a Single Application Debug launch configuration,
# change the debug type to 'Attach to running target' and provide this 
# tcl script in 'Execute Script' option.
# Path of this script: /home/s/titan4_debug/zynq/AXU3EG/key_system/_ide/scripts/debugger_key-default.tcl
# 
# 
# Usage with xsct:
# To debug using xsct, launch xsct and run below command
# source /home/s/titan4_debug/zynq/AXU3EG/key_system/_ide/scripts/debugger_key-default.tcl
# 
connect -url tcp:127.0.0.1:3121
source /home/s/Xilinx/Vitis/2021.1/scripts/vitis/util/zynqmp_utils.tcl
targets -set -nocase -filter {name =~"APU*"}
rst -system
after 3000
targets -set -filter {jtag_cable_name =~ "Digilent JTAG-SMT2 21025DA10700" && level==0 && jtag_device_ctx=="jsn-JTAG-SMT2-21025DA10700-14710093-0"}
fpga -file /home/s/titan4_debug/zynq/AXU3EG/key/_ide/bitstream/design_1_wrapper.bit
targets -set -nocase -filter {name =~"APU*"}
loadhw -hw /home/s/titan4_debug/zynq/AXU3EG/design_1_wrapper_1/export/design_1_wrapper_1/hw/design_1_wrapper.xsa -mem-ranges [list {0x80000000 0xbfffffff} {0x400000000 0x5ffffffff} {0x1000000000 0x7fffffffff}] -regs
configparams force-mem-access 1
targets -set -nocase -filter {name =~"APU*"}
set mode [expr [mrd -value 0xFF5E0200] & 0xf]
targets -set -nocase -filter {name =~ "*A53*#0"}
rst -processor
dow /home/s/titan4_debug/zynq/AXU3EG/design_1_wrapper_1/export/design_1_wrapper_1/sw/design_1_wrapper_1/boot/fsbl.elf
set bp_27_55_fsbl_bp [bpadd -addr &XFsbl_Exit]
con -block -timeout 60
bpremove $bp_27_55_fsbl_bp
targets -set -nocase -filter {name =~ "*A53*#0"}
rst -processor
dow /home/s/titan4_debug/zynq/AXU3EG/key/Debug/key.elf
configparams force-mem-access 0
targets -set -nocase -filter {name =~ "*A53*#0"}
con
