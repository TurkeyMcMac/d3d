#include "d3d.h"
#include <curses.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

void init_pairs(void)
{
	start_color();
	if (COLOR_PAIRS < 64) return;
	for (int fg = 0; fg < 8; ++fg) {
		for (int bg = 0; bg < 8; ++bg) {
			init_pair((fg << 3 | bg) + 1, fg, bg);
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
	initscr();
	d3d_texture *txtr = d3d_new_texture(SIDE, SIDE);
	d3d_block blk = {{txtr, txtr, txtr, txtr, txtr, txtr}};
	d3d_camera *cam = d3d_new_camera(2.0, 2.0, COLS, LINES);
	d3d_board *brd = d3d_new_board(4, 4);
	memcpy(txtr->pixels, pixels, SIDE * SIDE);
	brd->blocks[0] = &blk;
	brd->blocks[1] = &blk;
	brd->blocks[2] = &blk;
	brd->blocks[3] = &blk;
	brd->blocks[4] = &blk;
	brd->blocks[7] = &blk;
	brd->blocks[8] = &blk;
	brd->blocks[9] = &blk;
	brd->blocks[10] = &blk;
	brd->blocks[11] = &blk;
	brd->blocks[12] = &blk;
	brd->blocks[13] = &blk;
	brd->blocks[14] = &blk;
	brd->blocks[15] = &blk;
	cam->pos.x = 1.4;
	cam->pos.y = 1.4;
	init_pairs();
	for (;;) {
		double move_angle;
		d3d_draw(cam, NULL, brd);
		for (size_t y = 0; y < cam->height; ++y) {
			for (size_t x = 0; x < cam->width; ++x) {
				int p = cam->pixels[y * cam->width + x];
				mvaddch(y, x, p == ' ' ? ' ' : term_pixel(p));
			}
		}
		refresh();
		move_angle = cam->facing;
		switch (getch()) {
		case 'w':
			break;
		case 'a':
			move_angle += M_PI / 2;
			break;
		case 's':
			move_angle += M_PI;
			break;
		case 'd':
			move_angle -= M_PI / 2;
			break;
		case 'q':
			cam->facing += 0.01;
			continue;
		case 'e':
			cam->facing -= 0.01;
			continue;
		case 'x':
			goto end;
		default:
			continue;
		}
		cam->pos.x += cos(move_angle) * 0.01;
		cam->pos.y += sin(move_angle) * 0.01;
	}
end:
	endwin();
}
