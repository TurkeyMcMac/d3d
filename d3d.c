#include "d3d.h"
#include <math.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#define GET(grid, member, x, y) ((x) < (grid)->width && (y) < (grid)->height ? \
		&(grid)->member[(size_t)((y) * (grid)->width + (x))] : NULL)

static const d3d_texture empty_texture = {
	.width = 1,
	.height = 1,
	.pixels = {0} 
};

static const d3d_block empty_block = {{
	&empty_texture,
	&empty_texture,
	&empty_texture,
	&empty_texture,
	&empty_texture,
	&empty_texture
}};

d3d_camera *d3d_new_camera(
		double fovx,
		double fovy,
		size_t width,
		size_t height)
{
	size_t base_size, pixels_size, tans_size, size;
	d3d_camera *cam;
	base_size = offsetof(d3d_camera, pixels);
	pixels_size = width * height * sizeof(d3d_pixel);
	pixels_size += sizeof(double) - pixels_size % sizeof(double);
	tans_size = height * sizeof(double);
	size = base_size + pixels_size + tans_size;
	cam = malloc(size);
	if (!cam) return NULL;
	cam->tans = (void *)cam + size - tans_size;
	cam->fov.x = fovx;
	cam->fov.y = fovy;
	cam->width = width;
	cam->height = height;
	memset(cam->pixels, 0, pixels_size);
	for (size_t y = 0; y < height; ++y) {
		double angle = fovy * ((double)y / height - 0.5);
		printf("t %f\n", tan(angle));
		cam->tans[y] = tan(angle);
	}
	return cam;
}

void d3d_free_camera(d3d_camera *cam)
{
	free(cam);
}

d3d_texture *d3d_new_texture(size_t width, size_t height)
{
	size_t pixels_size = width * height * sizeof(d3d_pixel *);
	d3d_texture *txtr = malloc(offsetof(d3d_texture, pixels) + pixels_size);
	if (!txtr) return NULL;
	txtr->width = width;
	txtr->height = height;
	memset(txtr->pixels, 0, pixels_size);
	return txtr;
}

d3d_board *d3d_new_board(size_t width, size_t height)
{
	size_t blocks_size = width * height * sizeof(d3d_block *);
	d3d_board *board = malloc(offsetof(d3d_board, blocks) + blocks_size);
	if (!board) return NULL;
	board->width = width;
	board->height = height;
	for (size_t i = 0; i < width * height; ++i) {
		board->blocks[i] = &empty_block;
	}
	return board;
}

static const d3d_block *nextpos(
	d3d_camera *cam,
	const d3d_board *board,
	d3d_vec_s *pos,
	const d3d_vec_s *dpos,
	d3d_direction *dir)
{
	const d3d_block **blk = NULL;
	d3d_vec_s tonext = {INFINITY, INFINITY};
	*dir = 0;
	if (dpos->x < 0.0) {
		tonext.x = -fmod(pos->x, 1.0);
	} else if (dpos->x > 0.0) {
		tonext.x = 1.0 - fmod(pos->x, 1.0);
		*dir |= 2;
	}
	if (dpos->y < 0.0) {
		tonext.y = -fmod(pos->y, 1.0);
	} else if (dpos->y > 0.0) {
		*dir |= 1;
		tonext.y = 1.0 - fmod(pos->y, 1.0);
	}
	if (tonext.x / dpos->x < tonext.y / dpos->y) {
		*dir &= 2;
		pos->x += tonext.x;
		pos->y += tonext.x / dpos->x * dpos->y - copysign(0.001, dpos->y);
	} else {
		*dir &= 1;
		pos->y += tonext.y;
		pos->x += tonext.y / dpos->y * dpos->x - copysign(0.001, dpos->x);
	}
	blk = GET(board, blocks, (size_t)floor(pos->x), (size_t)floor(pos->y));
	if (!blk) {
		*dir = -1;
		return NULL;
	}
	return *blk;
}

static const d3d_texture *hit_wall(
	d3d_camera *cam,
	const d3d_board *board,
	d3d_vec_s *pos,
	const d3d_vec_s *dpos,
	d3d_direction *dir)
{
	const d3d_block *block;
	do {
		block = nextpos(cam, board, pos, dpos, dir);
	} while (*dir == -1 && block);
	if (!block) return NULL;
	return block->faces[*dir];
}

static void cast_ray(
	d3d_camera *cam,
	const d3d_board *board,
	size_t x)
{
	d3d_direction face;
	const d3d_texture *txtr;
	d3d_vec_s pos = cam->pos, disp;
	double dist;
	double angle =
		cam->facing + cam->fov.x * (0.5 - (double)x / cam->width);
	d3d_vec_s dpos = {cos(angle) * 0.001, sin(angle) * 0.001};
	txtr = hit_wall(cam, board, &pos, &dpos, &face);
	if (!txtr) return;
	disp.x = pos.x - cam->pos.x;
	disp.y = pos.y - cam->pos.y;
	dist = sqrt(disp.x * disp.x + disp.y * disp.y);
	for (size_t t = 0; t < cam->height; ++t) {
		double dist_y = cam->tans[t] * dist;
		if (dist_y >= 1.0) {
			*GET(cam, pixels, x, t) = ' ';
		} else if (dist_y <= 0.0) {
			*GET(cam, pixels, x, t) = ' ';
		} else {
			double dimension;
			size_t tx, ty;
			printf("(%f, %f)\n", pos.x, pos.y);
			switch (face) {
			case D3D_DNORTH:
				dimension = 1.0 - fmod(pos.x, 1.0);
				break;
			case D3D_DSOUTH:
				dimension = fmod(pos.x, 1.0);
				break;
			case D3D_DEAST:
				dimension = 1.0 - fmod(pos.y, 1.0);
				break;
			case D3D_DWEST:
				dimension = fmod(pos.y, 1.0);
				break;
			default:
				continue;
			}
			tx = dimension * (txtr->width - 1);
			ty = txtr->height * dist_y;
			*GET(cam, pixels, x, t) = *GET(txtr, pixels, tx, ty);
		}
	}
}

void d3d_draw(
	d3d_camera *cam,
	const d3d_sprite sprites[],
	const d3d_board *board)
{
	for (size_t x = 0; x < cam->width; ++x) {
		printf("x %lu\n",x);
		cast_ray(cam, board, x);
	}
}
