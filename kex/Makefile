#PREFIX = /home/stef/tasks/pitchfork/gcc-arm-none-eabi-4_7-2013q3/bin/arm-none-eabi
PREFIX = /home/stef/tasks/pitchfork/toolchain/arm/bin/arm-none-eabi
CC=$(PREFIX)-gcc
LD=$(PREFIX)-ld
OC=$(PREFIX)-objcopy
OD=$(PREFIX)-objdump
AS=$(PREFIX)-as
BP=../..

INCLUDES = -I$(BP) -I$(BP)/core/ -I$(BP)/sdio -I$(BP)/crypto \
	-I$(BP)/usb -I$(BP)/lib/libsodium/src/libsodium/include/sodium/ \
	-I$(BP)/lib/libopencm3/include -Isphincs256 -Iblake -Inewhope -Ichacha20
CFLAGS = -mno-unaligned-access -g -Wall -Werror -Os \
	-mfix-cortex-m3-ldrd -msoft-float -mthumb -Wno-strict-aliasing \
	-fomit-frame-pointer $(INCLUDES) -mcpu=cortex-m3 \
	-fstack-protector-all --param=ssp-buffer-size=4 -DSTM32F2

LIBS = -Wl,--just-symbols=main.syms
#LIBS = -Wl,--just-symbols=main.syms \
#	$(BP)/lib/libopencm3_stm32f2.a \
#	$(BP)/lib/libsodium/src/libsodium/crypto_scalarmult/curve25519/ref/libsodium_la-base_curve25519_ref.o \
#	$(BP)/lib/libsodium/src/libsodium/crypto_scalarmult/curve25519/ref/libsodium_la-smult_curve25519_ref.o
LDFLAGS = -mthumb -mcpu=cortex-m3 -fno-common -Tmemmap -nostartfiles -Wl,--gc-sections -Wl,-z,relro


objs = test.o llsr.o ../rsp.o common/utils.o \
		 newhope/poly.o newhope/ntt.o newhope/precomp.o \
		 newhope/error_correction.o newhope/newhope.o newhope/reduce.o \
		 sphincs/blake256/ref/hash.o sphincs/blake512/ref/hash.o \
		 newhope/fips202.o nrf24l01.o \
		 sphincs/crypto_stream_chacha20.o sphincs/chacha.o sphincs/qsort.o \
		 sphincs/wots.o sphincs/prg.o sphincs/hash.o sphincs/horst.o sphincs/sign.o

#		 sphincs/crypto_stream_chacha12.o sphincs/chacha12.o \
#		 chacha20/crypto_stream_chacha20.o \
#		 blake/blake256.o blake/blake512.o \
#objs = test.o llsr.o ../rsp.o blake/blake256.o \
#		 sphincs256/hash.o sphincs256/permute.o \
#		 sphincs256/sign.o blake/blake512.o common/utils.o \
#		 sphincs256/consts.o sphincs256/horst.o sphincs256/prg.o \
#		 sphincs256/wots.o chacha20/crypto_stream_chacha20.o \
#		 newhope/poly.o newhope/ntt.o newhope/precomp.o \
#		 newhope/error_correction.o newhope/newhope.o newhope/reduce.o \
#		 newhope/fips202.o

all : test.bin test.elf

test.bin : memmap main.syms $(objs)
	$(CC) $(LDFLAGS) -o test.elf $(objs) $(LIBS)
	$(OD) -Dl test.elf > test.list
	$(OC) test.elf test.bin -O binary

test: all
	../../../toolchain/pysrp/pyrsp/rsp.py /dev/ttyACM0 cortexm3 test.elf

main.syms: ../../main.elf
	nm -g ../../main.elf | sed 's/\([^ ]*\) [^ ]* \([^ ]*\)$$/\2 = 0x\1;/' >main.syms

clean:
	rm -f *.bin
	rm -f $(objs)
	rm -f *.elf
	rm -f *.list
	rm -f main.syms

.PHONY: clean upload test
