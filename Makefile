.PHONY: all clean

CC = clang
CFLAGS += -g -O0 -std=c99 -pedantic -pedantic-errors -D_GNU_SOURCE

OBJS = main.o yy.o bnf.o rc.o strarr.o strhash.o \
       grammar.o lr0.o bitset.o print.o find.o

all: main.exe
clean:
	-rm $(OBJS) yy.c bnf.c bnf.h

yy.o : yy.c bnf.h

main.exe : $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $^

%.o : %.c
	$(CC) $(CFLAGS) -c -o $@ $<

%.c : %.rl
	ragel -o $@ $<

%.c %.h : %.ly
	lemon -q $<
