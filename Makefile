.PHONY: all clean
.DEFAULT_GOAL = all


CC = clang
CFLAGS += -g -O0 -std=c11 -pedantic -pedantic-errors -D_GNU_SOURCE
WFLAGS = -Wall -Wextra -Werror

OBJS = main.o \
       array.o \
       bitset.o \
       codgen.o \
       common.o \
       conflict.o \
       grammar.o \
       lalr.o \
       lr0.o \
       nullable.o \
       print.o \
       rc.o \
       strarr.o \
       strhash.o \
       blob_c.o \
       yy.o bnf.o

GENERATED = yy.c bnf.c bnf.h blob_c.c blob_c.h

NOWARN = yy.o bnf.o
$(NOWARN): WFLAGS =

DEPS = $(OBJS:.o=.d)
ifneq ($(MAKECMDGOALS),clean)
  include $(DEPS)
endif

all: main.exe
clean:
	-rm -f $(OBJS) $(DEPS) $(GENERATED) genblob.exe

yy.o : yy.c bnf.c

main.exe : $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $^

genblob.exe : genblob.c
	$(CC) $(CFLAGS) $(LDFLAGS) -O2 -o $@ $<

%.d : %.c
	$(CC) -M -MP -MQ $@ -MQ $(<:%.c=%.o) -o $@ $<

%.o : %.c
	$(CC) $(CFLAGS) $(WFLAGS) -c -o $@ $<

%.c : %.rl
	ragel -o $@ $<

%.c %.h : %.ly %.lt
	lemon -q $<

blob_%.c blob_%.h : template_%.c genblob.exe
	./genblob.exe -s blob_$* -i blob_$*.h -o blob_$*.c $<

