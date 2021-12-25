CC ?= $(CROSS_COMPILE)gcc

obj = upgradetool.c
obj += cJSON.c

CFLAGS += -Wall -I .

libs = -lcrypto

all:
	$(CC) $(CFLAGS) $(obj) $(libs) -o _upgradetool
clean:
	rm -rf _upgradetool

