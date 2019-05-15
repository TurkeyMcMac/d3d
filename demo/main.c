#include "../d3d.h"
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

#define WALL_START_X 1.2
#define WALL_START_Y 1.2
#define WALL_END_X 2.8
#define WALL_END_Y 2.8

#define WALL_WIDTH 4
#define WALL_HEIGHT 4
static const char wall_pixels[WALL_WIDTH * WALL_HEIGHT] =
	"))))"
	")::)"
	")::)"
	"))))"
;

#define N_BATS 2

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
	d3d_texture *wall, *bat[2];
	wall = make_texture(WALL_WIDTH, WALL_HEIGHT, wall_pixels);
	bat[0] = make_texture(BAT_WIDTH, BAT_HEIGHT, bat_pixels[0]);
	bat[1] = make_texture(BAT_WIDTH, BAT_HEIGHT, bat_pixels[1]);
	d3d_block_s walls = {{wall, wall, wall, wall, wall, wall}};
	d3d_block_s empty = {{NULL, NULL, NULL, NULL, wall, wall}};
	d3d_camera *cam = d3d_new_camera(2.0, 2.0, COLS, LINES);
	d3d_board *brd = d3d_new_board(4, 4);
	d3d_sprite_s bats[N_BATS] = {
		{
			.txtr = bat[0],
			.transparent = ' ',
			.pos = {1.5, 1.6},
			.scale = {0.3, 0.2}
		},
		{
			.txtr = bat[1],
			.transparent = ' ',
			.pos = {2.5, 2.6},
			.scale = {0.3, 0.2}
		}
	};
	d3d_vec_s bat_speeds[N_BATS] = {
		{0.03, 0.02},
		{0.01, -0.04}
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
	for (;;) {
		double move_angle;
		d3d_draw_walls(cam, brd);
		d3d_draw_sprites(cam, N_BATS, bats);
		for (size_t y = 0; y < d3d_camera_height(cam); ++y) {
			for (size_t x = 0; x < d3d_camera_width(cam); ++x) {
				int p = *d3d_camera_get(cam, x, y);
				mvaddch(y, x, p <= ' ' ? ' ' : term_pixel(p));
			}
		}
		refresh();
		for (int i = 0; i < N_BATS; i++) {
			bats[i].pos.x += bat_speeds[i].x;
			if (bats[i].pos.x < WALL_START_X) {
				bats[i].pos.x = WALL_START_X;
				bat_speeds[i].x *= -1;
			} else if (bats[i].pos.x > WALL_END_X) {
				bats[i].pos.x = WALL_END_X;
				bat_speeds[i].x *= -1;
			}
			bats[i].pos.y += bat_speeds[i].y;
			if (bats[i].pos.y < WALL_START_Y) {
				bats[i].pos.y = WALL_START_Y;
				bat_speeds[i].y *= -1;
			} else if (bats[i].pos.y > WALL_END_Y) {
				bats[i].pos.y = WALL_END_Y;
				bat_speeds[i].y *= -1;
			}
			if (bats[i].txtr == bat[0]) {
				bats[i].txtr = bat[1];
			} else {
				bats[i].txtr = bat[0];
			}
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
		d3d_vec_s *cam_pos = d3d_camera_position(cam);
		cam_pos->x += cos(move_angle) * 0.04;
		if (cam_pos->x < WALL_START_X)
			cam_pos->x = WALL_START_X;
		else if (cam_pos->x > WALL_END_X)
			cam_pos->x = WALL_END_X;
		cam_pos->y += sin(move_angle) * 0.04;
		if (cam_pos->y < WALL_START_Y)
			cam_pos->y = WALL_START_Y;
		else if (cam_pos->y > WALL_END_Y)
			cam_pos->y = WALL_END_Y;
	}
end:
	endwin();
}
