#PREFIX = /home/stef/tasks/pitchfork/gcc-arm-none-eabi-4_7-2013q3/bin/arm-none-eabi
#PREFIX = /home/stef/tasks/pitchfork/toolchain/gcc-arm-none-eabi-5_3-2016q1/bin/arm-none-eabi
PREFIX = /home/stef/tasks/pitchfork/toolchain/arm/bin/arm-none-eabi
CC=$(PREFIX)-gcc
LD=$(PREFIX)-ld
OC=$(PREFIX)-objcopy
OD=$(PREFIX)-objdump
AS=$(PREFIX)-as
BP=../..

INCLUDES = -I. -I$(BP) -I$(BP)/core/ -I$(BP)/utils/ -I$(BP)/sdio -I$(BP)/crypto \
	-I$(BP)/usb -I$(BP)/usb/msc -I$(BP)/lib/libsodium/src/libsodium/include/sodium/ \
	-I$(BP)/lib/libopencm3/include -I$(BP)/lib/sphincs256 -I$(BP)/lib/blake -I$(BP)/lib/newhope -I$(BP)/lib/chacha20
CFLAGS = -mno-unaligned-access -g -Wall -Werror -Os \
	-mfix-cortex-m3-ldrd -msoft-float -mthumb -Wno-strict-aliasing \
	-fomit-frame-pointer $(INCLUDES) -mcpu=cortex-m3 \
	-fstack-protector-all --param=ssp-buffer-size=4 -DSTM32F2 -DSTORAGE_TEST

LIBS = -Wl,--just-symbols=main.syms

#LIBS = -Wl,--just-symbols=main.syms \
#	$(BP)/lib/libopencm3_stm32f2.a \
#	$(BP)/lib/libsodium/src/libsodium/crypto_scalarmult/curve25519/ref/libsodium_la-base_curve25519_ref.o \
#	$(BP)/lib/libsodium/src/libsodium/crypto_scalarmult/curve25519/ref/libsodium_la-smult_curve25519_ref.o
LDFLAGS = -mthumb -mcpu=cortex-m3 -fno-common -Tmemmap -nostartfiles -Wl,--gc-sections -Wl,-z,relro

objs = test.o storage.o $(BP)/ramload/rsp.o $(BP)/lib/scalarmult/cortex_m0_mpy121666.o \
	$(BP)/lib/scalarmult/cortex_m0_reduce25519.o $(BP)/lib/scalarmult/mul.o \
	$(BP)/lib/scalarmult/scalarmult.o $(BP)/lib/scalarmult/sqr.o

all : test.bin

test.bin : memmap main.syms $(objs)
	$(CC) $(LDFLAGS) -o test.elf $(objs) $(LIBS)
	$(OD) -Dl test.elf > test.list
	$(OC) test.elf test.bin -O binary

main.syms: $(BP)/main.elf
	nm -g $(BP)/main.elf | sed 's/\([^ ]*\) [^ ]* \([^ ]*\)$$/\2 = 0x\1;/' >main.syms

clean:
	rm -f *.bin
	rm -f $(objs)
	rm -f *.elf
	rm -f *.list
	rm -f main.syms

.PHONY: clean upload
