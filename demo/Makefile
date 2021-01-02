exe = d3d-demo

flags = -std=c99 -Wall -Wextra -Wpedantic -O3 $(CFLAGS)

$(exe): *.c *.h ../d3d.c ../d3d.h
	$(CC) $(flags) -o $@ *.c ../d3d.c -lm -lcurses

clean:
	$(RM) $(exe)
