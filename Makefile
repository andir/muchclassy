CFLAGS ?= -Wall -O3 -ggdb -flto -fsanitize=address,undefined $(shell pkg-config --cflags gmime-3.0)
LDFLAGS ?= -lnotmuch $(shell pkg-config --libs gmime-3.0)

all: muchclassy

fuzz: fuzz.c lib.o
	$(CC) $(CFLAGS) $(LDFLAGS) $+ -o $@

muchclassy: main.c lib.o
	$(CC) $(CFLAGS) $(LDFLAGS) $+ -o $@

lib.o: lib.c
	$(CC) $(CFLAGS) -c lib.c -o $@
