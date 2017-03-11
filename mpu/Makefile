PREFIX = /home/stef/tasks/pitchfork/gcc-arm-none-eabi-4_7-2013q3/bin/arm-none-eabi
#PREFIX = /home/stef/tasks/pitchfork/toolchain/arm/bin/arm-none-eabi
CC=$(PREFIX)-gcc
LD=$(PREFIX)-ld
OC=$(PREFIX)-objcopy
OD=$(PREFIX)-objdump
AS=$(PREFIX)-as
BP=../..

INCLUDES = -I$(BP) -I$(BP)/core/ -I$(BP)/sdio -I$(BP)/crypto \
	-I$(BP)/usb -I$(BP)/usb/msc -I$(BP)/lib/libsodium/src/libsodium/include/sodium/ \
	-I$(BP)/lib/libopencm3/include

CFLAGS = -DRAMLOAD -mno-unaligned-access -g -Wall -Werror -Os \
	-mfix-cortex-m3-ldrd -msoft-float -mthumb -Wno-strict-aliasing \
	-fomit-frame-pointer $(INCLUDES) -mcpu=cortex-m3 \
	-fstack-protector-all --param=ssp-buffer-size=4 -DSTM32F2 -DRAMLOAD
#-fsanitize=address -fsanitize=memory

LIBS = -Wl,--just-symbols=main.syms
#LIBS = -Wl,--just-symbols=main.syms \
#	$(BP)/lib/libopencm3_stm32f2.a \
#	$(BP)/lib/libsodium/src/libsodium/crypto_scalarmult/curve25519/ref/libsodium_la-base_curve25519_ref.o \
#	$(BP)/lib/libsodium/src/libsodium/crypto_scalarmult/curve25519/ref/libsodium_la-smult_curve25519_ref.o
LDFLAGS = -mthumb -mcpu=cortex-m3 -fno-common -Tmemmap -nostartfiles -Wl,--gc-sections -Wl,-z,relro

objs = test.o ../rsp.o

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

## clang vars
ARMGNU?=arm-none-eabi
LOPS = -Wall -m32 -emit-llvm $(INCLUDES) #-target=$(ARMGNU)
LLCOPS = -march=thumb -mtriple=$(ARMGNU) -mcpu=cortex-m3 -enable-pie -function-sections -data-sections
COPS = -fsanitize=cfi -fsanitize-cfi-cross-dso \
	-target $(ARMGNU) -mcpu=cortex-m3  -mfloat-abi=soft  -mthumb \
	-Wall -O2 -nostdlib -ffreestanding -flto -fPIE -fomit-frame-pointer \
	-g -Os -DRAMLOAD $(LOPS)

test.bc : test.c
	clang-3.9 $(COPS) -c test.c -o test.bc

#mpu.clang.bc : mpu.c
#	clang-3.9 $(COPS) -c mpu.c -o mpu.clang.bc

test.clang.bin: memmap test.bc #mpu.clang.bc
	opt-3.9 -std-link-opts -asan -relocation-model=pic -march=thumb -mcpu=cortex-m3 -mtriple=$(ARMGNU) \
			  -adce -bounds-checking -dce -deadargelim -die -safe-stack \
	        -Os -enable-pie -function-sections -data-sections test.bc -o test.opt.bc
	llc-3.9 $(LLCOPS) test.opt.bc -o test.s
	$(ARMGNU)-as test.s -o test.o

#	opt-3.9 -std-link-opts -asan -cross-dso-cfi -relocation-model=pic -march=thumb -mcpu=cortex-m3 -mtriple=$(ARMGNU) -Oz -enable-pie -function-sections -data-sections mpu.clang.bc -o mpu.clang.thumb.opt.bc
#	llc-3.9 $(LLCOPS) mpu.clang.thumb.opt.bc -o mpu.clang.thumb.opt.s
#	$(ARMGNU)-as mpu.clang.thumb.opt.s -o mpu.clang.thumb.opt.o

	$(CC) $(LDFLAGS) -fpic -flto -fPIE -o test.clang.elf test.o # mpu.clang.thumb.opt.o
	$(OD) -Dl test.clang.elf > test.clang.list
	$(OC) test.clang.elf test.clang.bin -O binary
### end clang vars

.PHONY: clean upload test
