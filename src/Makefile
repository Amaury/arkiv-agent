# ###################################
# #            MAKEFILE             #
# ###################################

# Executable names
NAME			= arkiv_agent
NAME_LINUX_X86_32	= arkiv_agent-linux-x86_32
NAME_LINUX_X86_64	= arkiv_agent-linux-x86_64
NAME_LINUX_ARM_32	= arkiv_agent-linux-arm_32
NAME_LINUX_ARM_64	= arkiv_agent-linux-arm_64
NAME_LINUX_RISCV_64	= arkiv_agent-linux-riscv_64
NAME_MACOS_X86_64	= arkiv_agent-macos-x86_64
NAME_MACOS_ARM_64	= arkiv_agent-macos-arm_64

# Source files
SRC	=	main.c		\
		agent.c		\
		log.c		\
		configuration.c	\
		declare.c	\
		backup.c	\
		upload.c	\
		utils.c		\
		api.c

#		http.c		\
#		utils.c		\
#		log.c		\
#		init.c		\
#		install.c	\
#		backup.c	\
#		compress.c	\
#		sha256.c	\
#		encrypt.c	\
#		upload.c

# ###################################################################

# Paths to header files
IPATH	= -I. -I../include
# Path to libraries and lib's names
#LDPATH	= -L. -L../lib -ly -lcurl -lz -llzma -larchive -lcrypto -lssl -Wl,-rpath -Wl,'$$ORIGIN/lib'
#LDPATH	= -L. -L../lib -ly -lcurl -larchive -lz -llzma -lssl -lcrypto -lpthread -ldl -Wl,-rpath -Wl,'$$ORIGIN/lib'
LDPATH	= -L. -L../lib -ly -lm -ldl -Wl,-rpath -Wl,'$$ORIGIN/lib'
LDPATH_STATIC = -L. -lm -ldl
# Compiler options
EXEOPT	= -O3 # -g for debug

# ###################################################################

CC		  = gcc
ZIG		  = zig
CC_LINUX_X86_32	  = $(ZIG) cc -target x86-linux-musl -DZIG_PLATFORM_LINUX_X86_32
CC_LINUX_X86_64	  = $(ZIG) cc -target x86_64-linux-musl -DZIG_PLATFORM_LINUX_X86_64
CC_LINUX_ARM_32	  = $(ZIG) cc -target arm-linux-musleabi -DZIG_PLATFORM_LINUX_ARM_32
CC_LINUX_ARM_64	  = $(ZIG) cc -target aarch64-linux-musl -DZIG_PLATFORM_LINUX_ARM_64
CC_LINUX_RISCV_64 = $(ZIG) cc -target riscv64-linux-musl -DZIG_PLATFORM_LINUX_RISCV_64
CC_MACOS_X86_64	  = $(ZIG) cc -target x86_64-macos-none -DZIG_PLATFORM_MACOS_X86_64
CC_MACOS_ARM_64	  = $(ZIG) cc -target aarch64-macos-none -DZIG_PLATFORM_MACOS_ARM_64

OBJS	= $(SRC:.c=.o)

# Objects compilation options
CFLAGS_MAIN	= -std=gnu11 -pedantic-errors -Wall -Wextra -Wmissing-prototypes \
		  -Wno-long-long -Wno-unused-parameter -Wno-unused-result -Wno-pointer-arith -D_GNU_SOURCE -D_THREAD_SAFE \
		  $(IPATH) $(EXEOPT)
CFLAGS		= $(CFLAGS_MAIN)
CFLAGS_DEV	= $(CFLAGS_MAIN) -DDEV_MODE

# Link options
LDFLAGS	= $(EXEOPT) $(LDPATH)
LDFLAGS_STATIC = $(EXEOPT) $(LDPATH_STATIC)

# ###################################################################

.PHONY: dev clean all alldev

# dynamic linking
$(NAME): $(OBJS)
	$(CC) $(OBJS) $(LDFLAGS) -o $(NAME)
	#strip $(NAME)
	#mv $(NAME) ../bin/

# dynamic linking, development mode
dev: CFLAGS=$(CFLAGS_DEV)

dev: $(NAME)

# static linking, Linux i386
linux-x86_32: CC=$(CC_LINUX_X86_32)

linux-x86_32: $(OBJS)
	rm -rf tmp; mkdir tmp
	cp ../lib/y/*.o *.o tmp/
	cd tmp; $(CC) *.o $(LDFLAGS_STATIC) -o $(NAME_LINUX_X86_32)
	mv tmp/$(NAME_LINUX_X86_32) .
	rm -rf tmp

# static linking, Linux x86_64
linux-x86_64: CC=$(CC_LINUX_X86_64)

linux-x86_64: $(OBJS)
	$(CC) $(OBJS) ../lib/y/*.o $(LDFLAGS_STATIC) -o $(NAME_LINUX_X86_64)

# static linking, Linux ARM 64
linux-arm_64: CC=$(CC_LINUX_ARM_64)

linux-arm_64: $(OBJS)
	$(CC) $(OBJS) ../lib/y/*.o $(LDFLAGS_STATIC) -o $(NAME_LINUX_ARM_64)

# static linking, Linux RISC-V 64
linux-riscv_64: CC=$(CC_LINUX_RISCV_64)

linux-riscv_64: $(OBJS)
	$(CC) $(OBJS) ../lib/y/*.o $(LDFLAGS_STATIC) -o $(NAME_LINUX_RISCV_64)

# static linking, MacOS x86_64
macos-x86_64: CC=$(CC_MACOS_X86_64)

macos-x86_64: $(OBJS)
	$(CC) $(OBJS) ../lib/y/*.o $(LDFLAGS_STATIC) -o $(NAME_MACOS_X86_64)

# static linking, MacOS ARM 64
macos-arm_64: CC=$(CC_MACOS_ARM_64)

macos-arm_64: $(OBJS)
	$(CC) $(OBJS) ../lib/y/*.o $(LDFLAGS_STATIC) -o $(NAME_MACOS_ARM_64)

# cleaning
clean:
	rm -f $(NAME) $(NAME_LINUX_X86_32) $(NAME_LINUX_X86_64) $(NAME_LINUX_ARM_64) $(NAME_LINUX_RISCV_64) $(NAME_MACOS_X86_64) $(NAME_MACOS_ARM_64) $(OBJS) *~ ../bin/$(NAME)
	rm -rf tmp

# cleaning and compiling
all: clean $(NAME)

# cleaning and compiling (dev mode)
alldev: clean dev

# objet files compilation
.c.o:
	$(CC) $(CFLAGS) -c $<

