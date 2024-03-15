# ###################################
# #            MAKEFILE             #
# ###################################

# Executable's name
NAME	=	agent
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

CC	= gcc
OBJS	= $(SRC:.c=.o)

# Objects compilation options
CFLAGS	= -std=gnu11 -pedantic-errors -Wall -Wextra -Wmissing-prototypes \
	  -Wno-long-long -Wno-unused-parameter -Wno-unused-result -Wno-pointer-arith -D_GNU_SOURCE -D_THREAD_SAFE \
	  $(IPATH) $(EXEOPT)

# Link options
LDFLAGS	= $(EXEOPT) $(LDPATH)

# ###################################################################

.PHONY: clean all

$(NAME): $(OBJS)
	$(CC) $(OBJS) $(LDFLAGS) -o $(NAME)
	strip $(NAME)
	#mv $(NAME) ../bin/

clean:
	rm -f $(NAME) $(OBJS) *~ ../bin/$(NAME)

all: clean $(NAME)

.c.o:
	$(CC) $(CFLAGS) -c $<

