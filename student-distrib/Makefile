
# Makefile for OS project
# To build, first `make dep`, them `make`. Everything should be automatic.
# Will compile all *.c and *.S files in the current directory.


# Flags to use when compiling, preprocessing, assembling, and linking
CFLAGS+=-Wall -fno-builtin -fno-stack-protector -nostdlib
ASFLAGS+=
LDFLAGS+=-nostdlib -static
CC=gcc

# Use a cross compiling toolchain if available (only when not compiling in devel/lab machines)
TARGET=i686-elf
CROSS_CC := $(shell command -v $(TARGET)-gcc 2> /dev/null)

ifdef CROSS_CC
CC=$(TARGET)-gcc
endif

#If you have any .h files in another directory, add -I<dir> to this line
CPPFLAGS+=-nostdinc -g -Iinclude

# This generates the list of source files
# SRC=$(wildcard *.S) $(wildcard *.c) $(wildcard */*.S) $(wildcard */*.c)
SOURCEDIR=.
SRC=$(shell find $(SOURCEDIR) -name '*.c') $(shell find $(SOURCEDIR) -name '*.S')

# This generates the list of .o files. The order matters, boot.o must be first
OBJS=arch/x86/boot.o
OBJS+=$(filter-out %/boot.o,$(patsubst %.S,%.o,$(filter %.S,$(SRC))))
OBJS+=$(patsubst %.c,%.o,$(filter %.c,$(SRC)))

bootimg: Makefile $(OBJS)
	rm -f bootimg
	$(CC) $(LDFLAGS) $(OBJS) -Ttext=0x400000 -o bootimg
	sudo ./debug.sh

dep: Makefile.dep

Makefile.dep: $(SRC)
	$(CC) -MM $(CPPFLAGS) $(SRC) > $@

.PHONY: clean

#	rm -f *.o */*.o Makefile.dep
clean:
	find . -name "*.o" -type f -print0 | xargs -0 rm -f
	rm -f Makefile.dep

ifneq ($(MAKECMDGOALS),dep)
ifneq ($(MAKECMDGOALS),clean)
include Makefile.dep
endif
endif
