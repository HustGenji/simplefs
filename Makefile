MKFS = mkfs.simplefs

$(MKFS) : mkfs.c
	$(CC) -std=gnu99 -Wall -o $@ $<

clean:
	rm mkfs.simplefs

.PHONY: all clean