CC ?= $(CROSS_COMPILE)gcc

obj = src/main.c
obj += src/mii.c
obj += src/serial.c
obj += src/eeprom.c

include_dir=include

all:
	$(CC) $(obj) -I $(include_dir) -o tools
clean:
	rm tools
