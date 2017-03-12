#PREFIX = /home/stef/tasks/pitchfork/gcc-arm-none-eabi-4_7-2013q3/bin/arm-none-eabi
PREFIX = /home/s/tasks/pitchfork/toolchain/arm/bin/arm-none-eabi
CC=$(PREFIX)-gcc
LD=$(PREFIX)-ld
OC=$(PREFIX)-objcopy
OD=$(PREFIX)-objdump
AS=$(PREFIX)-as
BP=../..

INCLUDES = -I$(BP) -I$(BP)/core/ -I$(BP)/sdio -I$(BP)/crypto -I$(BP)/utils/ \
	-I$(BP)/usb -I$(BP)/lib/libsodium/src/libsodium/include/sodium/ -I$(BP)/lib/newhope/ \
	-I$(BP)/lib/libopencm3/include -I$(BP)/lib/xeddsa -I$(BP)/lib
CFLAGS = -mno-unaligned-access -g -Wall -Werror -Os \
	-mfix-cortex-m3-ldrd -msoft-float -mthumb -Wno-strict-aliasing \
	-fomit-frame-pointer $(INCLUDES) -mcpu=cortex-m3 \
	-fstack-protector-all --param=ssp-buffer-size=4 -DSTM32F2

#	-Wall -Wextra -Wimplicit-function-declaration \
	-Wredundant-decls -Wmissing-prototypes -Wstrict-prototypes \
	-Wundef -Wshadow -fno-inline-functions \

LIBS = -Wl,--just-symbols=main.syms
#LIBS = -Wl,--just-symbols=main.syms \
#	$(BP)/lib/libopencm3_stm32f2.a \
#	$(BP)/lib/libsodium/src/libsodium/crypto_scalarmult/curve25519/ref/libsodium_la-base_curve25519_ref.o \
#	$(BP)/lib/libsodium/src/libsodium/crypto_scalarmult/curve25519/ref/libsodium_la-smult_curve25519_ref.o
LDFLAGS = -mthumb -mcpu=cortex-m3 -fno-common -Tmemmap -nostartfiles -Wl,--gc-sections -Wl,-z,relro

objs		+= test.o qrcode.o $(BP)/ramload/rsp.o

all : test.bin test.elf
test.bin : test.c memmap main.syms $(objs)
	$(CC) $(LDFLAGS) -o test.elf $(objs) $(LIBS)
	$(OD) -Dl test.elf > test.list
	$(OC) test.elf test.bin -O binary

test: all
	$(BP)/../toolchain/pysrp/pyrsp/rsp.py /dev/ttyACM0 cortexm3 test.elf

main.syms: $(BP)/main.elf
	nm -g $(BP)/main.elf | sed 's/\([^ ]*\) [^ ]* \([^ ]*\)$$/\2 = 0x\1;/' | fgrep -f mains.min >main.syms

clean:
	rm -f *.bin *.o *.elf *.list main.syms || return

.PHONY: clean upload test
