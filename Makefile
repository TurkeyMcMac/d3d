exe = d3d-demo

$(exe): main.c d3d.c d3d.h
	$(CC) -O3 -o $@ *.c -lm -lcurses

clean:
	$(RM) $(exe)
