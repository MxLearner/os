all: pc-1 pc-2 pc-cv pc-cv-broadcast

CFLAGS := -O2 -I /home/mx/os-workbench/gpt -ggdb
LP := -lpthread

pc-1: pc-1.c main.c
	gcc $(CFLAGS) -o $@ $^ $(LP)

pc-2: pc-2.c main.c
	gcc $(CFLAGS) -o $@ $^ $(LP)

pc-cv: pc-cv.c main.c
	gcc $(CFLAGS) -o $@ $^ $(LP)

pc-cv-broadcast: pc-cv-broadcast.c main.c
	gcc $(CFLAGS) -o $@ $^  $(LP)

clean:
	rm -f pc-1 pc-2 pc-cv pc-cv-broadcast
