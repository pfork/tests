#target remote | openocd -f /home/stef/tasks/cdong/stm32/openocd.cfg -c gdb_port pipe

set remote hardware-breakpoint-limit 6
set remote hardware-watchpoint-limit 4
set mem inaccessible-by-default off
set disassemble-next-line on
set output-radix 16
#target extended-remote /dev/ttyACM0
#mon swdp_scan
#attach 1

define hook-step
  mon cortex_m maskisr on
end
define hookpost-step
  mon cortex_m maskisr off
end

define con
  target extended-remote /dev/ttyACM0
  mon swdp_scan
  attach 1
end

define inject
  delete breakpoints
  symbol-file test.elf
  restore test.bin binary 0x20004000
  break *0x20004000
  #break test
  jump *0x20004000
  #jump test
  delete breakpoints
end

define out
  frame 1
  thbr
  frame 0
  cont
end

define dump-store
  dump memory store.dmp 0x08010000 0x08014000
end

con
