CFLAGS+= -O2 -g -Wall -I/usr/include/sodium -I. #-DAXOLOTL_DEBUG
LDFLAGS+= -lsodium

objs = ax.o axolotl.o stfs.o

ax: $(objs)

check: scan-build flawfinder cppcheck

clean:
	rm ax $(objs) || true

scan-build: clean
	scan-build-3.9 make

flawfinder:
	flawfinder --quiet stfs.c

cppcheck:
	cppcheck --enable=all stfs.c

.PHONY: clean check scan-build flawfinder cppcheck
