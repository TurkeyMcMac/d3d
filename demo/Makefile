exe = d3d-demo

flags = --std=c99 -Wall -Wextra -O3 $(CFLAGS)

$(exe): main.c ../d3d.c ../d3d.h ../d3d-internal-structures.h
	$(CC) $(flags) -o $@ main.c ../d3d.c -lm -lcurses

clean:
	$(RM) $(exe)
