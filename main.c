#include "d3d.h"
#include <stdio.h>
#include <string.h>


#define SIDE 12
int main(void)
{
	static const char pixels[SIDE * SIDE] =
#if 0
		"123"
		"456"
		"789"
#endif
		"O----------O"
		"|..........|"
		"|.########.|"
		"|##########|"
		"|##O####O##|"
		"|###(##)###|"
		"|.########.|"
		"|.##){}(##.|"
		"|..######..|"
		"|..........|"
		"|..........|"
		"O----------O"
	;
	d3d_texture *txtr = d3d_new_texture(SIDE, SIDE);
	d3d_block blk = {{txtr, txtr, txtr, txtr, txtr, txtr}};
	d3d_camera *cam = d3d_new_camera(1.0, 1.0, 120, 80);
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
	cam->pos.x = 0.5;
	cam->pos.y = 0.99;
	cam->facing = 4.5;
	d3d_draw(cam, NULL, brd);
	for (size_t i = 0; i < 80 * 120; i += 120) {
		for (size_t j = 0; j < 120; ++j) {
			char pix = cam->pixels[i + j];
			putchar(pix ? pix : ' ');
		}
		putchar('\n');
	}
}
