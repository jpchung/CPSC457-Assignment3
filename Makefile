C=gcc
CFLAGS=-Wall

all: a3

a3: a3.c
        $(CC) $(CFLAGS)  $< -lpthread -o $@

clean:
        rm -f a3
C=gcc
CFLAGS=-Wall

all: a3

a3: a3.c
        $(CC) $(CFLAGS)  $< -lpthread -o $@
