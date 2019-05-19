exe = d3d-demo

flags = --std=c99 -Wall -Wextra -O3 $(CFLAGS)

$(exe): *.c *.h ../d3d.c ../d3d.h ../d3d-internal-structures.h
	$(CC) $(flags) -o $@ *.c ../d3d.c -lm -lcurses

clean:
	$(RM) $(exe)
