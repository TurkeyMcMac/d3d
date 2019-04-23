#include "d3d.h"
#include <curses.h>
#include <stdio.h>
#include <string.h>

void init_pairs(void)
{
	start_color();
	if (COLOR_PAIRS < 65) return;
	for (int fg = 0; fg < 7; ++fg) {
		for (int bg = 0; bg < 7; ++bg) {
			init_pair((bg << 3 | fg) + 1, bg, fg);
		}
	}
}


int term_pixel(int p)
{
	return COLOR_PAIR(p - ' ') | '#';
}

#define SIDE 21
int main(void)
{
	static const char pixels[SIDE * SIDE] =
		"....................."
		"....................."
		".#.#.###.#...#...###."
		".#.#.#...#...#...#.#."
		".###.###.#...#...#.#."
		".#.#.#...#...#...#.#."
		".#.#.###.###.###.###."
		"....................."
		".#.#.###.#...#...###."
		".#.#.#...#...#...#.#."
		".###.###.#...#...#.#."
		".#.#.#...#...#...#.#."
		".#.#.###.###.###.###."
		"....................."
		".#.#.###.#...#...###."
		".#.#.#...#...#...#.#."
		".###.###.#...#...#.#."
		".#.#.#...#...#...#.#."
		".#.#.###.###.###.###."
		"....................."
		"....................."
	;
	d3d_texture *txtr = d3d_new_texture(SIDE, SIDE);
	d3d_block blk = {{txtr, txtr, txtr, txtr, txtr, txtr}};
	d3d_camera *cam = d3d_new_camera(2.0, 2.0, 120, 40);
	d3d_board *brd = d3d_new_board(3, 3);
	memcpy(txtr->pixels, pixels, SIDE * SIDE);
	brd->blocks[0] = &blk;
	brd->blocks[1] = &blk;
	brd->blocks[2] = &blk;
	brd->blocks[3] = &blk;
	brd->blocks[4] = &blk;
	brd->blocks[5] = &blk;
	brd->blocks[6] = &blk;
	brd->blocks[7] = &blk;
	brd->blocks[8] = &blk;
	cam->pos.x = 1.5;
	cam->pos.y = 1.5;
	cam->facing = 3.1;
	d3d_draw(cam, NULL, brd);
	initscr();
	init_pairs();
	for (size_t y = 0; y < cam->height; ++y) {
		for (size_t x = 0; x < cam->width; ++x) {
			int p = cam->pixels[y * cam->width + x];
			mvaddch(y, x, p == ' ' ? ' ' : term_pixel(p));
		}
	}
	refresh();
	getch();
	endwin();
}
