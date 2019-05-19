#ifndef N_PTHREADS
#	define N_PTHREADS 0
#endif
#define USE_PTHREADS (N_PTHREADS > 0)

#ifdef _WIN32
#	include <windows.h>
#else
	// For nanosleep and struct timespec
#	define _POSIX_C_SOURCE 200809L
#endif
#if USE_PTHREADS
#	include "pthread-workers.h"
#endif
#include "../d3d.h"
#include <curses.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifndef M_PI
#	define M_PI 3.14159265358979323846
#endif

// The width:height ratio of a single pixel on the terminal. This may need
// manual changing.
#define PIXEL_ASPECT 0.625
// The field-of-view in the x direction in radians. The FOV in the y direction
// is determined by this value.
#define FOV_X 2.0

// Where the world border starts.
#define WALL_START_X 1.2
#define WALL_START_Y 1.2

// Where the world border ends.
#define WALL_END_X 2.8
#define WALL_END_Y 2.8

// The wall texture data.
#define WALL_WIDTH 4
#define WALL_HEIGHT 4
static const char wall_pixels[WALL_WIDTH * WALL_HEIGHT] =
	"))))"
	")::)"
	")::)"
	"))))"
;

// The number of bat entities
#define N_BATS 2

// The bat texture data. There are two frames.
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

// Wait until the next tick with the given interval between ticks. This will try
// to account for computing time since the last tick. The argument 'ticks' is an
// internally used counter initially set to -1.
void next_tick(clock_t *ticks, clock_t interval)
{
	// TODO: check if this is actually portable
	if (*ticks != (clock_t)-1) {
		clock_t now = clock();
		if (now > *ticks + interval) goto end;
		clock_t delay = interval - (now - *ticks);
#ifdef _WIN32
		Sleep(delay * 1000 / CLOCKS_PER_SEC);
#else
		struct timespec ts = {
			.tv_sec = 0,
			.tv_nsec = delay * 1000000000 / CLOCKS_PER_SEC
		};
		nanosleep(&ts, NULL);
#endif
	}
end:
	*ticks = clock();
}

// Initialize the colors for the screen. This must be called after initscr and
// before term_pixel.
void init_pairs(void)
{
	start_color();
	if (COLOR_PAIRS < 64) {
		fprintf(stderr, "libcurses must support at least 64 colors\n");
		exit(EXIT_FAILURE);
	}
	for (int fg = 0; fg < 8; ++fg) {
		for (int bg = 0; bg < 8; ++bg) {
			init_pair((fg << 3 | bg) + 1, fg, bg);
		}
	}
}

// Convert a pixel from a texture into an ncurses pixel. This must be called
// after init_pairs is called once.
int term_pixel(int p)
{
	return COLOR_PAIR(p - ' ') | '#';
}

// End screen drawing mode.
void end_screen(void)
{
	endwin();
}

// Make a texture and fill it with the given memory of size width * height.
static d3d_texture *make_texture(size_t width, size_t height, const char *pix)
{
	d3d_texture *txtr = d3d_new_texture(width, height);
	size_t i = 0;
	for (size_t y = 0; y < height; ++y) {
		for (size_t x = 0; x < width; ++x) {
			*d3d_texture_get(txtr, x, y) = pix[i];
			++i;
		}
	}
	return txtr;
}

int main(void)
{
	initscr();
	// Don't wait for the user to press a key:
	timeout(0);
	atexit(end_screen);
	d3d_texture *wall, *bat[2];
	// The wall texture:
	wall = make_texture(WALL_WIDTH, WALL_HEIGHT, wall_pixels);
	// The bat textures:
	bat[0] = make_texture(BAT_WIDTH, BAT_HEIGHT, bat_pixels[0]);
	bat[1] = make_texture(BAT_WIDTH, BAT_HEIGHT, bat_pixels[1]);
	// A block containing all walls:
	d3d_block_s walls = {{wall, wall, wall, wall, wall, wall}};
	// A block containing just the ceiling and floor:
	d3d_block_s empty = {{NULL, NULL, NULL, NULL, wall, wall}};
	// The camera:
	d3d_camera *cam = d3d_new_camera(FOV_X,
		LINES * FOV_X / COLS / PIXEL_ASPECT, COLS, LINES);
	// The world:
	d3d_board *brd = d3d_new_board(4, 4);
	// Bat sprites:
	d3d_sprite_s bats[N_BATS] = {
		{
			.txtr = bat[0],
			.transparent = ' ',
			.pos = {1.5, 1.6},
			.scale = {0.3, 0.15}
		},
		{
			.txtr = bat[1],
			.transparent = ' ',
			.pos = {2.5, 2.6},
			.scale = {0.3, 0.15}
		}
	};
	// Velocities of the corresponding bats (in blocks per 20 ticks):
	d3d_vec_s bat_speeds[N_BATS] = {
		{0.03, 0.02},
		{0.01, -0.04}
	};
	*d3d_camera_empty_pixel(cam) = ' ';
	// Make a cavity in the world center:
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
#if USE_PTHREADS
	init_threads(cam, brd);
	atexit(destroy_threads);
#endif
	init_pairs();
	bool screen_refresh = true;
	clock_t ticks = -1;
	// The bat state cycles to 0 before it hits 20:
	for (int bat_state = 1 ;; bat_state = (bat_state + 1) % 20) {
		double move_angle;
		next_tick(&ticks, CLOCKS_PER_SEC / 100);
		if (screen_refresh) {
#if USE_PTHREADS
			wait_for_frame();
#else
			d3d_draw_walls(cam, brd);
#endif
			d3d_draw_sprites(cam, N_BATS, bats);
			// Draw the pixels on the terminal:
			for (size_t y = 0; y < d3d_camera_height(cam); ++y) {
				for (size_t x = 0;
				     x < d3d_camera_width(cam);
				     ++x)
				{
					int p = *d3d_camera_get(cam, x, y);
					mvaddch(y, x, p <= ' '
						? ' '
						: term_pixel(p));
				}
			}
#if USE_PTHREADS
			start_next_frame();
#endif
			refresh();
			screen_refresh = false;
		}
		if (bat_state == 0) {
			// Update bats:
			for (int i = 0; i < N_BATS; i++) {
				// Move in the x direction:
				bats[i].pos.x += bat_speeds[i].x;
				// Possible bounce in the x direction:
				if (bats[i].pos.x < WALL_START_X) {
					bats[i].pos.x = WALL_START_X;
					bat_speeds[i].x *= -1;
				} else if (bats[i].pos.x > WALL_END_X) {
					bats[i].pos.x = WALL_END_X;
					bat_speeds[i].x *= -1;
				}
				// Move in the y direction:
				bats[i].pos.y += bat_speeds[i].y;
				// Possible bounce in the y direction:
				if (bats[i].pos.y < WALL_START_Y) {
					bats[i].pos.y = WALL_START_Y;
					bat_speeds[i].y *= -1;
				} else if (bats[i].pos.y > WALL_END_Y) {
					bats[i].pos.y = WALL_END_Y;
					bat_speeds[i].y *= -1;
				}
				// Flap the wings of the bat:
				if (bats[i].txtr == bat[0]) {
					bats[i].txtr = bat[1];
				} else {
					bats[i].txtr = bat[0];
				}
			}
			screen_refresh = true;
		}
		// The straight-forward player movement angle:
		move_angle = *d3d_camera_facing(cam);
		switch (getch()) {
		case 'w': // Forward
			break;
		case 'a': // Left
			move_angle += M_PI / 2;
			break;
		case 's': // Backward
			move_angle += M_PI;
			break;
		case 'd': // Right
			move_angle -= M_PI / 2;
			break;
		case 'q': // Turn left
			*d3d_camera_facing(cam) += 0.04;
			screen_refresh = true;
			continue;
		case 'e': // Turn right
			*d3d_camera_facing(cam) -= 0.04;
			screen_refresh = true;
			continue;
		case 'x': // Quit the game
			exit(0);
		default: // Other keys are ignored
			continue;
		}
		// The position of the camera:
		d3d_vec_s *cam_pos = d3d_camera_position(cam);
		// Move in the move angle in the x direction:
		cam_pos->x += cos(move_angle) * 0.04;
		// Possibly hit the wall in the x direction:
		if (cam_pos->x < WALL_START_X)
			cam_pos->x = WALL_START_X;
		else if (cam_pos->x > WALL_END_X)
			cam_pos->x = WALL_END_X;
		// Move in the move angle in the y direction:
		cam_pos->y += sin(move_angle) * 0.04;
		// Possibly hit the wall in the y direction:
		if (cam_pos->y < WALL_START_Y)
			cam_pos->y = WALL_START_Y;
		else if (cam_pos->y > WALL_END_Y)
			cam_pos->y = WALL_END_Y;
		screen_refresh = true;
	}
}
