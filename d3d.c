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
	NULL,
	NULL,
	NULL,
	NULL,
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
	const d3d_block * const *blk = NULL;
	d3d_vec_s tonext = {INFINITY, INFINITY};
	d3d_direction ns = D3D_DNORTH, ew = D3D_DWEST;
	if (dpos->x < 0.0) {
		tonext.x = -fmod(pos->x, 1.0);
	} else if (dpos->x > 0.0) {
		ew = D3D_DEAST;
		tonext.x = 1.0 - fmod(pos->x, 1.0);
	}
	if (dpos->y < 0.0) {
		ns = D3D_DSOUTH;
		tonext.y = -fmod(pos->y, 1.0);
	} else if (dpos->y > 0.0) {
		tonext.y = 1.0 - fmod(pos->y, 1.0);
	}
	if (tonext.x / dpos->x < tonext.y / dpos->y) {
		*dir = ew;
		pos->x += tonext.x;
		pos->y += tonext.x / dpos->x * dpos->y - copysign(0.001, dpos->y);
	} else {
		*dir = ns;
		pos->y += tonext.y;
		pos->x += tonext.y / dpos->y * dpos->x - copysign(0.001, dpos->x);
	}
	blk = GET(board, blocks, (size_t)floor(pos->x), (size_t)floor(pos->y));
	if (!blk) {
		*dir = -1;
		return NULL;
	}
	if (!(*blk)->faces[*dir]) *dir = -1;
	return *blk;
}

static const d3d_block *hit_wall(
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
	return block;
}

static void cast_ray(
	d3d_camera *cam,
	const d3d_board *board,
	size_t x)
{
	d3d_direction face;
	const d3d_block *block;
	d3d_vec_s pos = cam->pos, disp;
	double dist;
	double angle =
		cam->facing + cam->fov.x * (0.5 - (double)x / cam->width);
	d3d_vec_s dpos = {cos(angle) * 0.001, sin(angle) * 0.001};
	block = hit_wall(cam, board, &pos, &dpos, &face);
	if (!block) {
		for (size_t y = 0; y < cam->height; ++y) {
			*GET(cam, pixels, x, y) = ' ';
		}
		return;
	}
	disp.x = pos.x - cam->pos.x;
	disp.y = pos.y - cam->pos.y;
	dist = sqrt(disp.x * disp.x + disp.y * disp.y);
	for (size_t t = 0; t < cam->height; ++t) {
		const d3d_texture *txtr;
		size_t tx, ty;
		double dist_y = cam->tans[t] * dist + 0.5;
		if (dist_y > 0.0 && dist_y < 1.0) {
			double dimension;
			txtr = block->faces[face];
			switch (face) {
			case D3D_DNORTH:
				dimension = fmod(pos.x, 1.0);
				break;
			case D3D_DSOUTH:
				dimension = 1.0 - fmod(pos.x, 1.0);
				break;
			case D3D_DWEST:
				dimension = fmod(pos.y, 1.0);
				break;
			case D3D_DEAST:
				dimension = 1.0 - fmod(pos.y, 1.0);
				break;
			default:
				continue;
			}
			tx = dimension * (txtr->width - 0.5) + 0.4999;
			ty = txtr->height * dist_y;
		} else {
			double tangent = fabs(cam->tans[t]);
			double newdist = 0.5 / tangent;
			d3d_vec_s newdisp = {
				disp.x / dist * newdist,
				disp.y / dist * newdist
			};
			txtr = block->faces[dist_y >= 1. ? D3D_DUP : D3D_DDOWN];
			tx = fmod(cam->pos.x + newdisp.x, 1.0) * txtr->width;
			ty = fmod(cam->pos.y + newdisp.y, 1.0) * txtr->height;
		}
		*GET(cam, pixels, x, t) = *GET(txtr, pixels, tx, ty);
	}
}

void d3d_draw(
	d3d_camera *cam,
	const d3d_sprite sprites[],
	const d3d_board *board)
{
	for (size_t x = 0; x < cam->width; ++x) {
		cast_ray(cam, board, x);
	}
}
