.PHONY: all clean
.DEFAULT_GOAL = all


CC = gcc
ifdef ($(DEBUG)
  CFLAGS = -O2 -flto
else
  CFLAGS = -O0 -g
endif
CFLAGS += -std=c11 -pedantic -pedantic-errors -D_POSIX_C_SOURCE=200809L
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
  -include $(DEPS)
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
	$(CC) -M -MP -MG -MQ $@ -MQ $(<:%.c=%.o) -o $@ $<

%.o : %.c
	$(CC) $(CFLAGS) $(WFLAGS) -c -o $@ $<

%.c : %.rl
	ragel -o $@ $<

ifeq ($(NOLEMON),yes)
%.c %.h : %.ly
	./main.exe $<
	mv out.C $*.c
	mv out.H $*.h
else
%.c %.h : %.ly %.lt
	lemon -q $<
endif

blob_%.c : template_%.c genblob.exe
	./genblob.exe -s blob_$* -o blob_$*.c $<

blob_%.h : template_%.c genblob.exe
	./genblob.exe -s blob_$* -H blob_$*.h $<

