ramdisk.o: ramdisk.c types.c types.h
	gcc -g -Wall -w ramdisk.c types.c types.h `pkg-config fuse --cflags --libs` -o ramdisk
clean:
	rm -rf *.o
