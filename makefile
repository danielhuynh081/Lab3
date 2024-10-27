CC=gcc
CFLAGS=-g
WFLAGS = -Wall -Wshadow -Wunreachable-code -Wredundant-decls -Wmissing-declarations -Wold-style-definition -Wmissing-prototypes -Wdeclaration-after-statement -Wextra -Wpedantic -Werror -Wno-return-local-addr -Wunsafe-loop-optimizations -Wuninitialized
PROG = viktar
PROGS = $(PROG) $(PROG2) $(PROG3) $(PROG4)
#INCLUDES = bin_file.h

all: $(PROGS)

$(PROG): $(PROG).o 
	$(CC) $(WFLAGS) $(CFLAGS) -o $@ $^

$(PROG).o: $(PROG).c #$(INCLUDES)
	$(CC) $(WFLAGS) $(CFLAGS) -c $<


clean cls:
	rm -f $(PROGS) *.o *~ \#* 
tar: 
	tar cvfa bin_files_${LOGNAME}.tar.gz *.[ch] [mM]akefile
