obj-m += simplefs.o
simplefs-objs := fs.o super.o inode.o file.o

KDIR ?= /lib/modules/$(shell uname -r)/build

MKFS = mkfs.simplefs

all: $(MKFS)
	make -C $(KDIR) M=$(PWD) modules

$(MKFS) : mkfs.c
	$(CC) -std=gnu99 -Wall -o $@ $<

clean:
	make -C $(KDIR) M=$(PWD) clean
	rm $(MKFS)

.PHONY: all clean