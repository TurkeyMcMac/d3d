exe = demo

flags = -std=c99 -Wall -Wextra -Wpedantic $(CFLAGS)

$(exe): demo.c ../d3d.c ../d3d.h
	$(CC) $(flags) -o $@ demo.c ../d3d.c -lm -lcurses

.PHONY: clean
clean:
	$(RM) $(exe)
