exe = d3d-demo

CC = c99

$(exe): main.c d3d.c d3d.h
	$(CC) -O3 $(CFLAGS) -o $@ *.c -lm -lcurses

clean:
	$(RM) $(exe)
