#include "d3d.h"
#include <curses.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

#ifndef M_PI
#	define M_PI 3.14159265358979323846
#endif

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

#define SPRITE_SIDE 3

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
	static const char sprite_pixels[SPRITE_SIDE * SPRITE_SIDE] =
		" @ "
		"@@@"
		" @ "
	;
	initscr();
	d3d_texture *txtr = d3d_new_texture(SIDE, SIDE);
	d3d_texture *sprite_txtr = d3d_new_texture(SPRITE_SIDE, SPRITE_SIDE);
	d3d_block_s blk = {{txtr, txtr, txtr, txtr, txtr, txtr}};
	d3d_camera *cam = d3d_new_camera(2.0, 2.0, COLS, LINES);
	d3d_board *brd = d3d_new_board(4, 4);
	d3d_sprite_s sprites[2] = {
		{
			.txtr = txtr,
			.transparent = '.',
			.pos = {1.2, 1.5},
			.scale = {0.2, 0.3}
		},
		{
			.txtr = sprite_txtr,
			.transparent = ' ',
			.pos = {1.5, 1.6},
			.scale = {0.2, 0.3}
		}
	};
	memcpy(d3d_get_texture_pixels(txtr), pixels, SIDE * SIDE);
	memcpy(d3d_get_texture_pixels(sprite_txtr), sprite_pixels,
		SPRITE_SIDE * SPRITE_SIDE);
	*d3d_camera_empty_pixel(cam) = ' ';
	*d3d_board_get(brd, 0, 0) = &blk;
	*d3d_board_get(brd, 1, 0) = &blk;
	*d3d_board_get(brd, 2, 0) = &blk;
	*d3d_board_get(brd, 3, 0) = &blk;
	*d3d_board_get(brd, 0, 1) = &blk;
	*d3d_board_get(brd, 1, 1) = &blk;
	*d3d_board_get(brd, 2, 1) = &blk;
	*d3d_board_get(brd, 3, 1) = &blk;
	*d3d_board_get(brd, 0, 2) = &blk;
	*d3d_board_get(brd, 1, 2) = &blk;
	*d3d_board_get(brd, 2, 2) = &blk;
	*d3d_board_get(brd, 3, 2) = &blk;
	*d3d_board_get(brd, 0, 3) = &blk;
	*d3d_board_get(brd, 1, 3) = &blk;
	*d3d_board_get(brd, 2, 3) = &blk;
	*d3d_board_get(brd, 3, 3) = &blk;
	d3d_camera_position(cam)->x = 1.4;
	d3d_camera_position(cam)->y = 1.4;
	init_pairs();
	for (;;) {
		double move_angle;
		d3d_draw_walls(cam, brd);
		d3d_draw_sprites(cam, 2, sprites);
		for (size_t y = 0; y < d3d_camera_height(cam); ++y) {
			for (size_t x = 0; x < d3d_camera_width(cam); ++x) {
				int p = *d3d_camera_get(cam, x, y);
				mvaddch(y, x, p <= ' ' ? ' ' : term_pixel(p));
			}
		}
		refresh();
		move_angle = *d3d_camera_facing(cam);
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
			*d3d_camera_facing(cam) += 0.04;
			continue;
		case 'e':
			*d3d_camera_facing(cam) -= 0.04;
			continue;
		case 'x':
			goto end;
		default:
			continue;
		}
		d3d_camera_position(cam)->x += cos(move_angle) * 0.04;
		d3d_camera_position(cam)->y += sin(move_angle) * 0.04;
	}
end:
	endwin();
}
