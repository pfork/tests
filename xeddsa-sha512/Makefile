#PREFIX = /home/s/tasks/pitchfork/gcc-arm-none-eabi-4_7-2013q3/bin/arm-none-eabi
#PREFIX = /home/s/tasks/pitchfork/toolchain/gcc-arm-none-eabi-5_3-2016q1/bin/arm-none-eabi
PREFIX = /home/s/tasks/pitchfork/toolchain/arm/bin/arm-none-eabi
CC=$(PREFIX)-gcc
LD=$(PREFIX)-ld
OC=$(PREFIX)-objcopy
OD=$(PREFIX)-objdump
AS=$(PREFIX)-as
BP=../..

INCLUDES = -I. -I$(BP) -I$(BP)/core/ -I$(BP)/utils/ -I$(BP)/sdio -I$(BP)/crypto \
	-I$(BP)/usb -I$(BP)/usb/msc -I$(BP)/lib/libsodium/src/libsodium/include/sodium/ \
	-I$(BP)/lib/libopencm3/include -Ixeddsa
CFLAGS = -DNDEBUG -mno-unaligned-access -g -Wall -Werror -Os \
	-mfix-cortex-m3-ldrd -msoft-float -mthumb -Wno-strict-aliasing \
	-fomit-frame-pointer $(INCLUDES) -mcpu=cortex-m3 \
	-fdata-sections -ffunction-sections \
	-fstack-protector-all --param=ssp-buffer-size=4 -DSTM32F2

LIBS = -Wl,--just-symbols=main.syms $(BP)/lib/libsodium/src/libsodium/.libs/libsodium.a
#LIBS = -Wl,--just-symbols=main.syms
#LIBS = -Wl,--just-symbols=main.syms \
#	$(BP)/lib/libsodium.new/src/libsodium/crypto_sign/ed25519/libsodium_la-sign_ed25519_api.o \
#	$(BP)/lib/libsodium.new/src/libsodium/crypto_sign/libsodium_la-crypto_sign.o \
#	$(BP)/lib/libsodium.new/src/libsodium/crypto_verify/32/ref/libsodium_la-verify_32.o \
#	$(BP)/lib/libsodium.new/src/libsodium/crypto_core/curve25519/ref10/libsodium_la-curve25519_ref10.o

#LIBS = -Wl,--just-symbols=main.syms \
#	$(BP)/lib/libopencm3_stm32f2.a \
#	$(BP)/lib/libsodium/src/libsodium/crypto_scalarmult/curve25519/ref/libsodium_la-base_curve25519_ref.o \
#	$(BP)/lib/libsodium/src/libsodium/crypto_scalarmult/curve25519/ref/libsodium_la-smult_curve25519_ref.o
LDFLAGS = -mthumb -mcpu=cortex-m3 -fno-common -Tmemmap -nostartfiles -Wl,--gc-sections -Wl,-z,relro

objs = test.o \
	xeddsa/elligator.o xeddsa/vxeddsa.o xeddsa/xeddsa.o xeddsa/keygen.o xeddsa/zeroize.o xeddsa/curve_sigs.o \
	xeddsa/fe_isequal.o xeddsa/fe_mont_rhs.o xeddsa/fe_montx_to_edy.o xeddsa/ge_montx_to_p3.o \
	xeddsa/ge_p3_to_montx.o xeddsa/ge_scalarmult.o xeddsa/ge_scalarmult_cofactor.o \
	xeddsa/sc_clamp.o xeddsa/sc_cmov.o xeddsa/sc_neg.o xeddsa/fe_sqrt.o xeddsa/sign_modified.o \
	xeddsa/open_modified.o xeddsa/vsign_modified.o xeddsa/compare.o xeddsa/vopen_modified.o \
	xeddsa/ge_neg.o xeddsa/ge_isneutral.o

all : test.bin test.elf

test.bin : memmap main.syms $(objs)
	$(CC) $(LDFLAGS) -o test.elf $(objs) $(LIBS)
	$(OD) -Dl test.elf > test.list
	$(OC) test.elf test.bin -O binary

main.syms: $(BP)/main.elf
	nm -g $(BP)/main.elf | sed 's/\([^ ]*\) [^ ]* \([^ ]*\)$$/\2 = 0x\1;/' | fgrep -f mains.min >main.syms

clean:
	rm -f *.bin
	rm -f $(objs)
	rm -f *.elf
	rm -f *.list
	rm -f main.syms

.PHONY: clean upload
