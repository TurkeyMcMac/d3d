#include "../d3d.h"
#include <assert.h>
#include <curses.h>
#include <stdlib.h>
#include <tgmath.h>
#include <time.h>

// Input keys for moving the camera:
#define FORWARD_KEY 'w'
#define LEFT_KEY 'a'
#define BACKWARD_KEY 's'
#define RIGHT_KEY 'd'
#define TURN_CW_KEY 'e'
#define TURN_CCW_KEY 'q'
#define QUIT_KEY 'x'

// Speed at which the camera moves:
#define CAM_SPEED ((d3d_scalar)0.05)

// Speed (in radians) at which the camera turns:
#define CAM_TURN_SPEED ((d3d_scalar)0.07)

// The FOV of the camera in radians:
#define FOV_X ((d3d_scalar)2.0)
#define FOV_Y ((d3d_scalar)1.5)

// The number of tiles the room is wide:
#define ROOM_WIDTH 3

// The minimum distance the camera or a bat can be from the walls of the room:
#define ROOM_PADDING ((d3d_scalar)0.2)

// The minimum number of milliseconds between frames:
#define FRAME_DELAY 50

// The width and height of a bat's image:
#define BAT_SCALE_X ((d3d_scalar)0.5)
#define BAT_SCALE_Y ((d3d_scalar)0.25)

// The time each bat frame is displayed, measured in simulation frames:
#define BAT_FRAME_DELAY 6

// The speed at which bats move:
#define BAT_SPEED ((d3d_scalar)0.01)

// The number of bats in the room:
#define N_BATS 3

// The information for constructing the wall texture:
#define WALL_WIDTH 6
#define WALL_HEIGHT 6
const char wall_pixels[WALL_WIDTH * WALL_HEIGHT] =
	"      "
	" .... "
	" .... "
	" .... "
	" .... "
	"      "
;

// The information for constructing the two bat textures:
#define BAT_WIDTH 23
#define BAT_HEIGHT 9
const char bat_pixels[2][BAT_WIDTH * BAT_HEIGHT] = {
	"__________I_I__________" // (The underscores will be transparent.)
	"___I_____IIIII_____I___"
	"__III___II I II___III__"
	"_IIIIIIIIIIIIIIIIIIIII_"
	"IIIIIIIIIIIIIIIIIIIIIII"
	"I_I_I_IIIIIIIIIII_I_I_I"
	"________IIIIIII________"
	"__________III__________"
	"___________I___________"
	,
	"__________I_I__________"
	"_________IIIII_________"
	"________II I II________"
	"______IIIIIIIIIII______"
	"____IIIIIIIIIIIIIII____"
	"__IIIIIIIIIIIIIIIIIII__"
	"_IIIII__IIIIIII__IIIII_"
	"I_I__I____III____I__I_I"
	"___________I___________"
};

#define PI ((d3d_scalar)3.14159265358979323846)

// Construct a texture from the dimensions and characters/pixels:
d3d_texture *make_texture(size_t width, size_t height, const char *pixels)
{
	d3d_texture *txtr = d3d_new_texture(width, height, '\0');
	assert(txtr);
	size_t i = 0;
	for (size_t y = 0; y < height; ++y) {
		for (size_t x = 0; x < width; ++x) {
			*d3d_texture_get(txtr, x, y) = pixels[i++];
		}
	}
	return txtr;
}

int main(void)
{
	// Initialize the screen:
	initscr();
	// Don't wait for user input:
	timeout(0);
	// Don't show input characters:
	noecho();
	// Don't show the cursor:
	curs_set(0);

	// Initialize textures:
	d3d_texture *wall_txtr =
		make_texture(WALL_WIDTH, WALL_HEIGHT, wall_pixels);
	d3d_texture *bat_txtr_0 =
		make_texture(BAT_WIDTH, BAT_HEIGHT, bat_pixels[0]);
	d3d_texture *bat_txtr_1 =
		make_texture(BAT_WIDTH, BAT_HEIGHT, bat_pixels[1]);

	// Initialize board:
	d3d_block_s empty_block = {{
		NULL, NULL, NULL, NULL, wall_txtr, wall_txtr
	}};
	d3d_block_s wall_block = {{
		wall_txtr, wall_txtr, wall_txtr, wall_txtr, NULL, NULL
	}};
	// The +2 is to make room for the wall tiles:
	d3d_board *board =
		d3d_new_board(ROOM_WIDTH + 2, ROOM_WIDTH + 2, &wall_block);
	assert(board);
	// Carve out the room from the middle:
	for (size_t x = 1; x < ROOM_WIDTH + 1; ++x) {
		for (size_t y = 1; y < ROOM_WIDTH + 1; ++y) {
			*d3d_board_get(board, x, y) = &empty_block;
		}
	}

	// Initialize camera/viewpoint:
	size_t cam_width = COLS > 0 ? (size_t)COLS : 1;
	size_t cam_height = LINES > 0 ? (size_t)LINES : 1;
	d3d_camera *cam =
		d3d_new_camera(FOV_X, FOV_Y, cam_width, cam_height, ' ');
	// Start in the center:
	d3d_vec_s cam_pos = {
		(d3d_scalar)ROOM_WIDTH / 2 + 1, (d3d_scalar)ROOM_WIDTH / 2 + 1
	};
	d3d_scalar cam_facing = 0.0;

	// Initialize bats:
	struct bat {
		// Position and velocity:
		d3d_vec_s pos, vel;
		// Whether the bat is now using frame 0 (not 1):
		bool frame_0;
		// Ticks since last frame change:
		unsigned since_change;
	} bats[N_BATS];
	// This array will be partially initialized in this loop:
	d3d_sprite_s bat_sprites[N_BATS];
	srand((unsigned)time(NULL));
	for (size_t i = 0; i < N_BATS; ++i) {
		// Position them randonly in the room:
		bats[i].pos.x = (d3d_scalar)1.5 + rand() % ROOM_WIDTH;
		bats[i].pos.y = (d3d_scalar)1.5 + rand() % ROOM_WIDTH;
		// Face them in a random direction:
		d3d_scalar facing = (d3d_scalar)rand() / RAND_MAX * 2 * PI;
		bats[i].vel.x = cos(facing) * BAT_SPEED;
		bats[i].vel.y = sin(facing) * BAT_SPEED;
		// Start with the zeroth frame:
		bats[i].frame_0 = true;
		// But desynchronize the flapping a bit:
		bats[i].since_change = rand() % BAT_FRAME_DELAY;
		// Set up unchanging sprite information:
		bat_sprites[i].scale.x = BAT_SCALE_X;
		bat_sprites[i].scale.y = BAT_SCALE_Y;
		bat_sprites[i].transparent = '_'; // See bat_pixels.
	}

	// Run the thing:
	for (;;) {
		// Draw the frame:
		// Fill out the bat sprite information:
		for (size_t i = 0; i < N_BATS; ++i) {
			bat_sprites[i].pos = bats[i].pos;
			bat_sprites[i].txtr =
				bats[i].frame_0 ? bat_txtr_0 : bat_txtr_1;
		}
		// Draw to the camera and transfer pixels to the screen:
		d3d_draw(cam, cam_pos, cam_facing, board, N_BATS, bat_sprites);
		for (size_t y = 0; y < cam_height; ++y) {
			for (size_t x = 0; x < cam_width; ++x) {
				mvaddch(y, x, *d3d_camera_get(cam, x, y));
			}
		}
		refresh();

		// Put a delay between frames:
		napms(FRAME_DELAY);

		// Take user input:
		int key = getch();
		// Avoid building up input:
		flushinp();
		// Whether or not the user wants to step (translate position):
		bool step = false;
		// Offset from facing angle to determine the angle to step in:
		d3d_scalar step_turn_amount;
		// Amount to add to facing angle:
		d3d_scalar turn_amount = 0.0;
		switch (key) {
		case FORWARD_KEY:
			step = true;
			step_turn_amount = 0.0;
			break;
		case LEFT_KEY:
			step = true;
			step_turn_amount = PI / 2;
			break;
		case BACKWARD_KEY:
			step = true;
			step_turn_amount = PI;
			break;
		case RIGHT_KEY:
			step = true;
			step_turn_amount = 3 * PI / 2;
			break;
		case TURN_CW_KEY:
			turn_amount = -CAM_TURN_SPEED;
			break;
		case TURN_CCW_KEY:
			turn_amount = +CAM_TURN_SPEED;
			break;
		case QUIT_KEY:
			endwin();
			return EXIT_SUCCESS;
		}
		if (step) {
			// Step the camera:
			d3d_scalar step_angle = cam_facing + step_turn_amount;
			cam_pos.x += cos(step_angle) * CAM_SPEED;
			cam_pos.y += sin(step_angle) * CAM_SPEED;
			// Keep the camera in the room:
			if (cam_pos.x < 1 + ROOM_PADDING)
				cam_pos.x = 1 + ROOM_PADDING;
			if (cam_pos.x > 1 + ROOM_WIDTH - ROOM_PADDING)
				cam_pos.x = 1 + ROOM_WIDTH - ROOM_PADDING;
			if (cam_pos.y < 1 + ROOM_PADDING)
				cam_pos.y = 1 + ROOM_PADDING;
			if (cam_pos.y > 1 + ROOM_WIDTH - ROOM_PADDING)
				cam_pos.y = 1 + ROOM_WIDTH - ROOM_PADDING;
		}
		// Turn the camera:
		cam_facing = fmod(cam_facing + turn_amount, 2 * PI);

		// Move the bats:
		for (size_t i = 0; i < N_BATS; ++i) {
			// Update the bat frame:
			++bats[i].since_change;
			if (bats[i].since_change >= BAT_FRAME_DELAY) {
				bats[i].frame_0 = !bats[i].frame_0;
				bats[i].since_change = 0;
			}
			// Move the bat:
			bats[i].pos.x += bats[i].vel.x;
			bats[i].pos.y += bats[i].vel.y;
			// Keep the bat in the room and bounce it off the walls:
			if (bats[i].pos.x < 1 + ROOM_PADDING) {
				bats[i].pos.x = 1 + ROOM_PADDING;
				bats[i].vel.x *= -1;
			}
			if (bats[i].pos.x > 1 + ROOM_WIDTH - ROOM_PADDING) {
				bats[i].pos.x = 1 + ROOM_WIDTH - ROOM_PADDING;
				bats[i].vel.x *= -1;
			}
			if (bats[i].pos.y < 1 + ROOM_PADDING) {
				bats[i].pos.y = 1 + ROOM_PADDING;
				bats[i].vel.y *= -1;
			}
			if (bats[i].pos.y > 1 + ROOM_WIDTH - ROOM_PADDING) {
				bats[i].pos.y = 1 + ROOM_WIDTH - ROOM_PADDING;
				bats[i].vel.y *= -1;
			}
		}
	}
}
