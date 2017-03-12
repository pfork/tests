#PREFIX = /home/stef/tasks/pitchfork/gcc-arm-none-eabi-4_7-2013q3/bin/arm-none-eabi
PREFIX = /home/stef/tasks/pitchfork/toolchain/gcc-arm-none-eabi-5_3-2016q1/bin/arm-none-eabi
#PREFIX = /home/stef/tasks/pitchfork/toolchain/arm/bin/arm-none-eabi
CC=$(PREFIX)-gcc
LD=$(PREFIX)-ld
OC=$(PREFIX)-objcopy
OD=$(PREFIX)-objdump
AS=$(PREFIX)-as
BP=../..

INCLUDES = -I. -I$(BP) -I$(BP)/core/ -I$(BP)/utils/ -I$(BP)/sdio -I$(BP)/crypto \
	-I$(BP)/usb -I$(BP)/usb/msc -I$(BP)/lib/libsodium/src/libsodium/include/sodium/ \
	-I$(BP)/lib/libopencm3/include
CFLAGS = -mno-unaligned-access -g -Wall -Werror -Os \
	-mfix-cortex-m3-ldrd -msoft-float -mthumb -Wno-strict-aliasing \
	-fomit-frame-pointer $(INCLUDES) -mcpu=cortex-m3 \
	-fdata-sections -ffunction-sections \
	-DSTM32F2

LIBS = -Wl,--just-symbols=main.syms \
	$(BP)/lib/libopencm3_stm32f2.a \
	$(BP)/lib/libsodium/src/libsodium/.libs/libsodium.a

LDFLAGS = -mthumb -mcpu=cortex-m3 -fno-common -Tmemmap -nostartfiles -Wl,--gc-sections -Wl,-z,relro

objs = test.o iap.o open.o blake512.o $(BP)/core/oled.o $(BP)/core/smallfonts.o $(BP)/core/delay.o

all : test.bin test.elf fwupdater.lzg.o

test.bin : memmap $(objs) main.syms
	$(CC) $(LDFLAGS) -o test.elf $(objs) $(LIBS)
	$(OD) -Dl test.elf > test.list
	$(OC) test.elf test.bin -O binary

fwupdater.lzg.o: test.elf
	lzg/lzg -9 test.bin test.bin.lzg
	$(OC) --input binary --output elf32-littlearm \
			--add-symbol `nm test.elf| grep ' firmware_updater$$' | sed 's/\([^ ]*\) [^ ] firmware_updater/firmware_updater=0x\1/'` \
			--add-symbol `nm test.elf| grep ' _load_addr$$' | sed 's/\([^ ]*\) [^ ] _load_addr/_load_addr=0x\1/'` \
	      --binary-architecture arm test.bin.lzg fwupdater.lzg.o
	rm test.bin.lzg

main.syms: $(BP)/main.elf
	nm -g $(BP)/main.elf | sed 's/\([^ ]*\) [^ ]* \([^ ]*\)$$/\2 = 0x\1;/' | grep '^usbd_dev = 0x' >main.syms

clean:
	rm -f *.bin
	rm -f $(objs)
	rm -f *.elf
	rm -f *.lzg
	rm -f *.list
	rm -f main.syms

.PHONY: clean upload
