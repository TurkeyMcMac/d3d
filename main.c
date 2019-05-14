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

#define WALL_WIDTH 4
#define WALL_HEIGHT 4
static const char wall_pixels[WALL_WIDTH * WALL_HEIGHT] =
	"))))"
	")::)"
	")::)"
	"))))"
;


#define BAT_WIDTH 23
#define BAT_HEIGHT 9
static const char bat_pixels[2][BAT_WIDTH * BAT_HEIGHT] = {
	"          ! !          "
	"   !     !!!!!     !   "
	"  !!!   !!O!O!!   !!!  "
	" !!!!!!!!!!!!!!!!!!!!! "
	"!!!!!!!!!!!!!!!!!!!!!!!"
	"! ! ! !!!!!!!!!!! ! ! !"
	"        !!!!!!!        "
	"          !!!          "
	"           !           "
	,
	"          ! !          "
	"         !!!!!         "
	"        !!O!O!!        "
	"      !!!!!!!!!!!      "
	"    !!!!!!!!!!!!!!!    "
	"  !!!!!!!!!!!!!!!!!!!  "
	" !!!!!  !!!!!!!  !!!!! "
	"! !  !    !!!    !  ! !"
	"           !           "

};

static d3d_texture *make_texture(size_t width, size_t height, const char *pix)
{
	d3d_texture *txtr = d3d_new_texture(width, height);
	memcpy(d3d_get_texture_pixels(txtr), pix, width * height);
	return txtr;
}

int main(void)
{
	initscr();
	d3d_texture *wall, *bat;
	wall = make_texture(WALL_WIDTH, WALL_HEIGHT, wall_pixels);
	bat = make_texture(BAT_WIDTH, BAT_HEIGHT, bat_pixels[0]);
	d3d_block_s walls = {{wall, wall, wall, wall, wall, wall}};
	d3d_block_s empty = {{NULL, NULL, NULL, NULL, wall, wall}};
	d3d_camera *cam = d3d_new_camera(2.0, 2.0, COLS, LINES);
	d3d_board *brd = d3d_new_board(4, 4);
	d3d_sprite_s bats[1] = {
		{
			.txtr = bat,
			.transparent = ' ',
			.pos = {1.5, 1.6},
			.scale = {0.3, 0.2}
		}
	};
	*d3d_camera_empty_pixel(cam) = ' ';
	*d3d_board_get(brd, 0, 0) = &walls;
	*d3d_board_get(brd, 1, 0) = &walls;
	*d3d_board_get(brd, 2, 0) = &walls;
	*d3d_board_get(brd, 3, 0) = &walls;
	*d3d_board_get(brd, 0, 1) = &walls;
	*d3d_board_get(brd, 1, 1) = &empty;
	*d3d_board_get(brd, 2, 1) = &empty;
	*d3d_board_get(brd, 3, 1) = &walls;
	*d3d_board_get(brd, 0, 2) = &walls;
	*d3d_board_get(brd, 1, 2) = &empty;
	*d3d_board_get(brd, 2, 2) = &empty;
	*d3d_board_get(brd, 3, 2) = &walls;
	*d3d_board_get(brd, 0, 3) = &walls;
	*d3d_board_get(brd, 1, 3) = &walls;
	*d3d_board_get(brd, 2, 3) = &walls;
	*d3d_board_get(brd, 3, 3) = &walls;
	d3d_camera_position(cam)->x = 1.4;
	d3d_camera_position(cam)->y = 1.4;
	init_pairs();
	for (int tick = 1 ;; tick = (tick + 1) % 4) {
		double move_angle;
		d3d_draw_walls(cam, brd);
		d3d_draw_sprites(cam, 1, bats);
		for (size_t y = 0; y < d3d_camera_height(cam); ++y) {
			for (size_t x = 0; x < d3d_camera_width(cam); ++x) {
				int p = *d3d_camera_get(cam, x, y);
				mvaddch(y, x, p <= ' ' ? ' ' : term_pixel(p));
			}
		}
		refresh();
		switch (tick) {
		case 0:
			memcpy(d3d_get_texture_pixels(bat), bat_pixels[0],
				BAT_WIDTH * BAT_HEIGHT);
			break;
		case 2:
			memcpy(d3d_get_texture_pixels(bat), bat_pixels[1],
				BAT_WIDTH * BAT_HEIGHT);
			break;
		default:
			break;
		}
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
