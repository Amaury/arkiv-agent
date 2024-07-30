# #####################################################################
#                              Makefile
# #####################################################################

# Library's static name (libNAME.a)
NAME =		liby.a
# Library's dynamic name (libNAME.so)
SONAME =	liby.so

# Name of source files (names.c)
SRC =		yarray.c	\
		yexec.c		\
		ybase64.c	\
		ybin.c		\
		yexception.c	\
		yfile.c		\
		yhash.c		\
		yhashmap.c	\
		yhashtable.c	\
		yini.c		\
		yjson.c		\
		ylist.c		\
		ylock.c		\
		ylog.c		\
		ymemory.c	\
		ystr.c		\
		ytable.c	\
		ytimer.c	\
		yurl.c		\
		yvalue.c	\
		yvar.c		\
		yvar_path.c


# Name of header files (names.h)
INCLUDES =	y.h		\
		yarray.h	\
		yexec.h		\
		yansi.h		\
		ybase64.h	\
		ybin.h		\
		ydefs.h		\
		yexception.h	\
		yfile.h		\
		yhash.h		\
		yhashmap.h	\
		yhashtable.h	\
		yini.h		\
		yjson.h		\
		ylist.h		\
		ylock.h		\
		ylog.h		\
		ymemory.h	\
		yresult.h	\
		ystatus.h	\
		ystr.h		\
		ytable.h	\
		ytimer.h	\
		yurl.h		\
		yvalue.h	\
		yvar.h


# #####################################################################

# Paths to header files
IPATH   =       -I.
# Path to libraries and lib's names
LDPATH  =       -L.
# Compiler options
EXEOPT  =       -O3 # -g for debug

# #####################################################################

CC		  = gcc
ZIG		  = zig
CC_LINUX_X86_32	  = $(ZIG) cc -target x86-linux-musl
CC_LINUX_X86_64	  = $(ZIG) cc -target x86_64-linux-musl
CC_LINUX_ARM_32	  = $(ZIG) cc -target arm-linux-musleabi
CC_LINUX_ARM_64	  = $(ZIG) cc -target aarch64-linux-musl
CC_LINUX_RISCV_64 = $(ZIG) cc -target riscv64-linux-musl
CC_MACOS_X86_64	  = $(ZIG) cc -target x86_64-macos-none
CC_MACOS_ARM_64	  = $(ZIG) cc -target aarch64-macos-none

OBJS    =	$(SRC:.c=.o)

# Objects compilation options
CFLAGS	= -std=gnu11 -pedantic-errors -Wall -Wextra -Werror -Wmissing-prototypes \
	  -Wno-long-long -Wno-unused-parameter -Wno-unused-result -Wno-pointer-arith -D_GNU_SOURCE \
	  -D_LARGEFILE_SOURCE -D_THREAD_SAFE $(IPATH) $(EXEOPT) -fPIC
LDFLAGS	= -shared -Wl,-soname,$(SONAME)

# #####################################################################

.PHONY: clean all linux-x86_32 linux-x86_64 linux-arm_32 linux-arm_64 linux-riscv_64 macos-x86_64 macos-arm_64

# dynamic compilation on local architeccture
$(NAME): $(OBJS) $(SRC)
	ar -r $(NAME) $(OBJS)
	ranlib $(NAME)

$(SONAME): $(OBJS) $(SRC)
	$(CC) $(OBJS) $(LDFLAGS) -o $(SONAME)

clean:
	rm -f $(OBJS) $(NAME) $(SONAME) *~

all: clean $(NAME)

# objects compilation for i386
linux-x86_32: CC=$(CC_LINUX_X86_32)

linux-x86_32: clean $(OBJS)

# objects compilation for x86_64
linux-x86_64: CC=$(CC_LINUX_X86_64)

linux-x86_64: clean $(OBJS)

# objects compilation for ARM 32
linux-arm_32: CC=$(CC_LINUX_ARM_32)

linux-arm_32: clean $(OBJS)

# objects compilation for ARM 64
linux-arm_64: CC=$(CC_LINUX_ARM_64)

linux-arm_64: clean $(OBJS)

# objects compilation for RISCV 64
linux-riscv_64: CC=$(CC_LINUX_RISCV_64)

linux-riscv_64: clean $(OBJS)

# objects compilation for MacOS x86_64
macos-x86_64: CC=$(CC_MACOS_X86_64)

macos-x86_64: clean $(OBJS)

# objects compilation for MacOS ARM 64
macos-arm_64: CC=$(CC_MACOS_ARM_64)

macos-arm_64: clean $(OBJS)

# basic object compilation
.c.o:
	$(CC) $(CFLAGS) -c $<

