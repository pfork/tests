PREFIX = /home/s/tasks/pitchfork/toolchain/arm/bin/arm-none-eabi
CC=$(PREFIX)-gcc
LD=$(PREFIX)-ld
OC=$(PREFIX)-objcopy
OD=$(PREFIX)-objdump
AS=$(PREFIX)-as
BP=../..

rap_plugin=/home/stef/tasks/pitchfork/toolchain/rap_plugin/rap_plugin.so

INCLUDES = -I$(BP) -I$(BP)/core/ -I$(BP)/sdio -I$(BP)/crypto -I$(BP)/utils \
	-I$(BP)/usb -I$(BP)/lib/libsodium/src/libsodium/include/sodium/ \
	-I$(BP)/lib/libopencm3/include -Iyaffs2/direct -Iyaffs2
CFLAGS = -flto -mno-unaligned-access -g -Wall -Werror -Os \
	-mfix-cortex-m3-ldrd -msoft-float -mthumb -Wno-strict-aliasing \
	-fomit-frame-pointer $(INCLUDES) -mcpu=cortex-m3 \
	-fstack-protector-all --param=ssp-buffer-size=4 -DSTM32F2 \
	-DCONFIG_YAFFS_DIRECT -DCONFIG_YAFFS_YAFFS2 -DCONFIG_YAFFS_DEFINES_TYPES \
	-DCONFIG_YAFFS_PROVIDE_DEFS -DCONFIG_YAFFSFS_PROVIDE_VALUES
#	-fplugin=$(rap_plugin) -fplugin-arg-rap_plugin-check=call -DRAP_PLUGIN -fplugin-arg-rap_plugin-report=func,fptr,abs -fplugin-arg-rap_plugin-hash=abs-finish
	#-fomit-frame-pointer $(INCLUDES) -march=armv7 \

LIBS = -Wl,--just-symbols=main.syms
#LIBS = -Wl,--just-symbols=main.syms \
#	$(BP)/lib/libopencm3_stm32f2.a \
#	$(BP)/lib/libsodium/src/libsodium/crypto_scalarmult/curve25519/ref/libsodium_la-base_curve25519_ref.o \
#	$(BP)/lib/libsodium/src/libsodium/crypto_scalarmult/curve25519/ref/libsodium_la-smult_curve25519_ref.o
#LDFLAGS = -mthumb -mcpu=cortex-m3 -fno-common -Tmemmap -nostdlib -nostartfiles -Wl,--gc-sections -Wl,-z,relro
LDFLAGS = -g -flto -mthumb -mcpu=cortex-m3 -fno-common -Tmemmap -nostartfiles -Wl,--gc-sections -Wl,-z,relro

objs = test.o yaffs2/heap4.o yaffs_pitchfork_glue.o \
	yaffs2/direct/yaffsfs.o yaffs2/direct/yaffs_packedtags2.o \
	yaffs2/direct/yaffs_guts.o yaffs2/direct/yaffs_yaffs2.o \
	yaffs2/direct/yaffs_endian.o yaffs2/direct/yaffs_ecc.o \
	yaffs2/direct/yaffs_allocator.o yaffs2/direct/yaffs_bitmap.o \
	yaffs2/direct/yaffs_nand.o yaffs2/direct/yaffs_verify.o \
	yaffs2/direct/yaffs_nameval.o yaffs2/direct/yaffs_attribs.o \
	yaffs2/direct/yaffs_summary.o yaffs2/direct/yaffs_tagscompat.o \
	yaffs2/direct/yaffs_tagsmarshall.o yaffs2/direct/yaffs_checkptrw.o \
	yaffs2/direct/yaffs_hweight.o yaffs2/direct/yaffs_yaffs1.o

all : test.bin

test.bin : memmap main.syms $(objs)
	$(CC) $(LDFLAGS) -o test.elf $(objs) $(LIBS)
	$(OD) -Dl test.elf > test.list
	$(OC) test.elf test.bin -O binary

main.syms: ../../main.elf
	nm -g ../../main.elf | sed 's/\([^ ]*\) [^ ]* \([^ ]*\)$$/\2 = 0x\1;/' >main.syms

clean:
	rm -f *.bin
	rm -f $(objs)
	rm -f *.elf
	rm -f *.list
	rm -f main.syms

.PHONY: clean upload
