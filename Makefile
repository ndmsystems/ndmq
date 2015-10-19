.PHONY: all install clean distclean

TARGET = ndmq

VERSION:=$(shell if ! git describe --tag 2>/dev/null; \
	then echo "(undefined)"; fi | sed -e 's/-g/-/') 

PREFIX=/usr
EXEC_PREFIX=$(PREFIX)
LIB_DIR=$(EXEC_PREFIX)/lib
INCLUDE_DIR=$(PREFIX)/include

CFLAGS ?= \
	-g3 -pipe -fPIC --std=c99 -D_POSIX_C_SOURCE=200112L \
	-D_LARGEFILE_SOURCE -D_LARGEFILE64_SOURCE -D_FILE_OFFSET_BITS=64 \
	-ffunction-sections -fdata-sections -fstack-protector-all -Wempty-body \
	-Wall -Winit-self -Wswitch-enum -Wundef -Wunsafe-loop-optimizations \
	-Waddress -Wmissing-field-initializers -Wnormalized=nfkc \
	-Wredundant-decls -Wvla -Wstack-protector -ftabstop=4 -Wshadow \
	-Wpointer-arith -Wtype-limits -Wclobbered

LDFLAGS ?= -lndm

OBJS = $(patsubst %.c,%.o,$(wildcard *.c))

default all: $(TARGET)

$(OBJS): version.h

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $^ -o $@ -rdynamic $(LDFLAGS)

version.h:
	echo "#ifndef __NDMQ_VERSION_H__" > version.h
	echo "#define __NDMQ_VERSION_H__ 1" >> version.h
	echo "#define NDMQ_VERSION \"$(VERSION)\"" >> version.h
	echo "#endif // __NDMQ_VERSION_H__" >> version.h

clean distclean:
	rm -f $(TARGET) $(OBJS)
