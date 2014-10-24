.PHONY: all clean
.DEFAULT_GOAL = all


CC = clang
CFLAGS += -g -O2 -std=c11 -pedantic -pedantic-errors -D_GNU_SOURCE
WFLAGS = -Wall -Wextra -Werror

OBJS = main.o \
       array.o \
       bitset.o \
       codgen.o \
       conflict.o \
       find.o \
       grammar.o \
       lr0.o \
       print.o \
       rc.o \
       slr.o \
       strarr.o \
       strhash.o \
       yy.o bnf.o

NOWARN = yy.o bnf.o
$(NOWARN): WFLAGS =

DEPS = $(OBJS:.o=.d)
ifneq ($(MAKECMDGOALS),clean)
  include $(DEPS)
endif

all: main.exe
clean:
	-rm -f $(OBJS) $(DEPS) yy.c bnf.c bnf.h

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
