.PHONY: all clean

CC = clang
CFLAGS += -g -O0 -std=c11 -pedantic -pedantic-errors -D_GNU_SOURCE
WFLAGS = -Wall -Wextra -Werror

OBJS = main.o \
       bitset.o find.o grammar.o lr0.o print.o rc.o strarr.o strhash.o \
       yy.o bnf.o

NOWARN = yy.o bnf.o
$(NOWARN): WFLAGS =

DEPS = $(OBJS:.o=.d)

all: main.exe
clean:
	-rm $(OBJS) $(DEPS) yy.c bnf.c bnf.h

yy.o : yy.c bnf.c

main.exe : $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $^

%.d : %.c
	$(CC) -M -MP -MQ $@ -MQ $(<:%.c=%.o) -o $@ $<

%.o : %.c
	$(CC) $(CFLAGS) $(WFLAGS) -c -o $@ $<

%.c : %.rl
	ragel -o $@ $<

%.c %.h : %.ly %.lt
	lemon -q $<

include $(DEPS)

