# ###################################
# #            MAKEFILE             #
# ###################################

# Executable names
NAME			= agent
NAME_LINUX_X86_32	= agent_linux-i386
NAME_LINUX_X86_64	= agent_linux-x86_64
NAME_LINUX_ARM_32	= agent_linux-arm_32
NAME_LINUX_ARM_64	= agent_linux-arm_64
NAME_LINUX_RISCV_64	= agent_linux-riscv_64
NAME_MACOS_X86_64	= agent_macos-x86_64
NAME_MACOS_ARM_64	= agent_macos-arm_64

# Source files
SRC	=	main.c		\
		configuration.c	\
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
# Compiler options
EXEOPT	= -O3 # -g for debug

# ###################################################################

CC		  = gcc
ZIG		  = zig
CC_LINUX_X86_32	  = $(ZIG) cc -target x86-linux-musl
CC_LINUX_X86_64	  = $(ZIG) cc -target x86_64-linux-musl
CC_LINUX_ARM_32	  = $(ZIG) cc -target arm-linux-musleabi
CC_LINUX_ARM_64	  = $(ZIG) cc -target aarch64-linux-musl
CC_LINUX_RISCV_64 = $(ZIG) cc -target riscv64-linux-musl
CC_MACOS_X86_64	  = $(ZIG) cc -target x86_64-macos-none
CC_MACOS_ARM_64	  = $(ZIG) cc -target aarch64-macos-none

OBJS	= $(SRC:.c=.o)

# Objects compilation options
CFLAGS_MAIN	= -std=gnu11 -pedantic-errors -Wall -Wextra -Wmissing-prototypes \
		  -Wno-long-long -Wno-unused-parameter -Wno-unused-result -Wno-pointer-arith -D_GNU_SOURCE -D_THREAD_SAFE \
		  $(IPATH) $(EXEOPT)
CFLAGS		= $(CFLAGS_MAIN)
CFLAGS_DEV	= $(CFLAGS_MAIN) -DDEV_MODE

# Link options
LDFLAGS	= $(EXEOPT) $(LDPATH)

# ###################################################################

.PHONY: dev clean all

# dynamic linking
$(NAME): $(OBJS)
	$(CC) $(OBJS) $(LDFLAGS) -o $(NAME)
	#strip $(NAME)
	#mv $(NAME) ../bin/

# dynamic linking, development mode
dev: CFLAGS=$(CFLAGS_DEV)

dev: $(NAME)

# static linking, Linux i386
NAME_LINUX_X86_32: CC=$(CC_LINUX_X86_32)

NAME_LINUX_X86_32: $(NAME)
	mv $(NAME) $(NAME_LINUX_X86_32)

# static linking, Linux x86_64
NAME_LINUX_X86_64: CC=$(CC_LINUX_X86_64)

NAME_LINUX_X86_64: $(NAME)
	mv $(NAME) $(NAME_LINUX_X86_64)

# static linking, Linux ARM 32
NAME_LINUX_ARM_32: CC=$(CC_LINUX_ARM_32)

NAME_LINUX_ARM_32: $(NAME)
	mv $(NAME) $(NAME_LINUX_ARM_32)

# static linking, Linux ARM 64
NAME_LINUX_ARM_64: CC=$(CC_LINUX_ARM_64)

NAME_LINUX_ARM_64: $(NAME)
	mv $(NAME) $(NAME_LINUX_ARM_64)

# static linking, Linux RISC-V 64
NAME_LINUX_RISCV_64: CC=$(CC_LINUX_RISCV_64)

NAME_LINUX_RISCV_64: $(NAME)
	mv $(NAME) $(NAME_LINUX_RISCV_64)

# static linking, MacOS x86_64
NAME_MACOS_X86_64: CC=$(CC_MACOS_X86_64)

NAME_MACOS_X86-64: $(NAME)
	mv $(NAME) $(NAME_MACOS_X86_64)

# static linking, MacOS ARM 64
NAME_MACOS_ARM_64: CC=$(CC_MACOS_ARM_64)

NAME_MACOS_ARM_64: $(NAME)
	mv $(NAME) $(NAME_MACOS_ARM_64)

# cleaning
clean:
	rm -f $(NAME) $(NAME_LINUX_X86_32) $(NAME_LINUX_X86_64) $(NAME_LINUX_ARM_32) $(NAME_LINUX_ARM_64) $(NAME_LINUX_RISCV_64) $(NAME_MACOS_X86_64) $(NAME_MACOS_ARM_64) $(OBJS) *~ ../bin/$(NAME)

# cleaning and compilation
all: clean $(NAME)

# objet files compilation
.c.o:
	$(CC) $(CFLAGS) -c $<

