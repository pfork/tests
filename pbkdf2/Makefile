#PREFIX = /home/stef/tasks/pitchfork/gcc-arm-none-eabi-4_7-2013q3/bin/arm-none-eabi
#PREFIX = /home/stef/tasks/pitchfork/toolchain/arm/bin/arm-none-eabi
PREFIX = /home/stef/tasks/pitchfork/toolchain/gcc-arm-none-eabi-5_3-2016q1/bin/arm-none-eabi
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
	-mfix-cortex-m3-ldrd -msoft-float -mthumb \
	-fomit-frame-pointer $(INCLUDES) -mcpu=cortex-m3 \
	-fdata-sections -ffunction-sections \
	-fstack-protector-all --param=ssp-buffer-size=4 -DSTM32F2 #-flto

#LIBS = -Wl,--just-symbols=main.syms $(BP)/lib/libsodium/src/libsodium/.libs/libsodium.a
LIBS = $(BP)/lib/libsodium/src/libsodium/.libs/libsodium.a
#LIBS = -flto -Wl,--just-symbols=main.syms \#
#	$(BP)/lib/libopencm3_stm32f2.a
#LIBS = -Wl,--just-symbols=main.syms \
#	$(BP)/lib/libsodium/src/libsodium/crypto_sign/ed25519/libsodium_la-sign_ed25519_api.o \
#	$(BP)/lib/libsodium/src/libsodium/crypto_sign/libsodium_la-crypto_sign.o \
#	$(BP)/lib/libsodium/src/libsodium/crypto_verify/32/ref/libsodium_la-verify_32.o \
#	$(BP)/lib/libsodium/src/libsodium/crypto_core/curve25519/ref10/libsodium_la-curve25519_ref10.o

LDFLAGS = -mthumb -mcpu=cortex-m3 -fno-common -Tmemmap -nostartfiles -Wl,--gc-sections -Wl,-z,relro

objs = test.o pbkdf2_generichash.o ../../core/oled.o ../../core/smallfonts.o ../../utils/abort.o ../../utils/itoa.o

all : test.bin test.elf

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

.PHONY: clean upload test
