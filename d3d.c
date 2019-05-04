#include "d3d.h"
#include <math.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#define GET(grid, member, x, y) ((x) < (grid)->width && (y) < (grid)->height ? \
		&(grid)->member[(size_t)((y) * (grid)->width + (x))] : NULL)

// Computes fmod(n, 1.0) if n >= 0, or 1.0 + fmod(n, 1.0) if n < 0
// Returns in the range [0, 1)
static double mod1(double n)
{
	return n - floor(n);
}

// computes approximately 1.0 - fmod(n, 1.0) if n >= 0 or -fmod(n, 1.0) if n < 0
// Returns in the range [0, 1)
static double revmod1(double n)
{
	return ceil(n) - n;
}

// This is pretty much floor(c). However, when c is a nonzero whole number and
// positive is true (that is, the relevant component of the delta position is
// over 0,) c is decremented and returned.
static size_t tocoord(double c, bool positive)
{
	double f = floor(c);
	if (positive && f == c && c != 0.0) return (size_t)c - 1;
	return f;
}

static void move_dir(d3d_direction dir, size_t *x, size_t *y)
{
	switch(dir) {
	case D3D_DNORTH: --*y; break;
	case D3D_DSOUTH: ++*y; break;
	case D3D_DEAST: ++*x; break;
	case D3D_DWEST: --*x; break;
	default: break;
	}
}

static d3d_direction invert_dir(d3d_direction dir)
{
	switch(dir) {
	case D3D_DNORTH: return D3D_DSOUTH;
	case D3D_DSOUTH: return D3D_DNORTH;
	case D3D_DEAST: return D3D_DWEST;
	case D3D_DWEST: return D3D_DEAST;
	default: return dir;
	}
}

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
	size_t base_size, pixels_size, tans_size, dists_size, size;
	d3d_camera *cam;
	base_size = offsetof(d3d_camera, pixels);
	pixels_size = width * height * sizeof(d3d_pixel);
	pixels_size += sizeof(double) - pixels_size % sizeof(double);
	tans_size = height * sizeof(double);
	dists_size = width * sizeof(double);
	size = base_size + pixels_size + tans_size + dists_size;
	cam = malloc(size);
	if (!cam) return NULL;
	cam->tans = (void *)((char *)cam + size - dists_size - tans_size);
	cam->dists = (void *)((char *)cam + size - dists_size);
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

static const d3d_block *hit_wall(
	d3d_camera *cam,
	const d3d_board *board,
	d3d_vec_s *pos,
	const d3d_vec_s *dpos,
	d3d_direction *dir)
{
	const d3d_block *block;
	do {
		size_t x, y;
		const d3d_block * const *blk = NULL;
		d3d_vec_s tonext = {INFINITY, INFINITY};
		d3d_direction ns = D3D_DNORTH, ew = D3D_DWEST;
		if (dpos->x < 0.0) {
			tonext.x = -mod1(pos->x);
		} else if (dpos->x > 0.0) {
			ew = D3D_DEAST;
			tonext.x = revmod1(pos->x);
		}
		if (dpos->y < 0.0) {
			tonext.y = -mod1(pos->y);
		} else if (dpos->y > 0.0) {
			ns = D3D_DSOUTH;
			tonext.y = revmod1(pos->y);
		}
		if (tonext.x / dpos->x < tonext.y / dpos->y) {
			*dir = ew;
			pos->x += tonext.x;
			pos->y += tonext.x / dpos->x * dpos->y;
		} else {
			*dir = ns;
			pos->y += tonext.y;
			pos->x += tonext.y / dpos->y * dpos->x;
		}
		x = tocoord(pos->x, dpos->x > 0.0);
		y = tocoord(pos->y, dpos->y > 0.0);
		blk = GET(board, blocks, x, y);
		if (!blk) return NULL;
		if (!(*blk)->faces[*dir]) {
			move_dir(*dir, &x, &y);
			blk = GET(board, blocks, x, y);
			if (!blk) return NULL;
			if (!(*blk)->faces[invert_dir(*dir)]) {
				if (*dir == ew) {
					pos->x += copysign(0.0001, dpos->x);
				} else {
					pos->y += copysign(0.0001, dpos->y);
				}
				*dir = -1;
			}
		}
		block = *blk;
	} while (*dir == -1);
	return block;
}

void d3d_draw_column(d3d_camera *cam, const d3d_board *board, size_t x)
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
	cam->dists[x] = dist;
	for (size_t t = 0; t < cam->height; ++t) {
		const d3d_texture *txtr;
		size_t tx, ty;
		double dist_y = cam->tans[t] * dist + 0.5;
		if (dist_y > 0.0 && dist_y < 1.0) {
			double dimension;
			txtr = block->faces[face];
			switch (face) {
			case D3D_DSOUTH:
				dimension = mod1(pos.x);
				break;
			case D3D_DNORTH:
				dimension = revmod1(pos.x);
				break;
			case D3D_DWEST:
				dimension = mod1(pos.y);
				break;
			case D3D_DEAST:
				dimension = revmod1(pos.y);
				break;
			default:
				continue;
			}
			tx = dimension * txtr->width;
			ty = txtr->height * dist_y;
		} else {
			double newdist = 0.5 / fabs(cam->tans[t]);
			d3d_vec_s newpos = {
				cam->pos.x + disp.x / dist * newdist,
				cam->pos.y + disp.y / dist * newdist
			};
			size_t bx = tocoord(newpos.x, dpos.x > 0.0),
			       by = tocoord(newpos.y, dpos.y > 0.0);
			const d3d_block *top_bot = *GET(board, blocks, bx, by);
			if (dist_y >= 1.0) {
				txtr = top_bot->faces[D3D_DUP];
				tx = revmod1(newpos.x) * txtr->width;
			} else {
				txtr = top_bot->faces[D3D_DDOWN];
				tx = mod1(newpos.x) * txtr->width;
			}
			ty = mod1(newpos.y) * txtr->height;
		}
		*GET(cam, pixels, x, t) = *GET(txtr, pixels, tx, ty);
	}
}

void d3d_draw_walls(d3d_camera *cam, const d3d_board *board)
{
	for (size_t x = 0; x < cam->width; ++x) {
		d3d_draw_column(cam, board, x);
	}
}

void d3d_draw_sprite(d3d_camera *cam, const d3d_sprite *sp)
{
	d3d_vec_s disp = { sp->pos.x - cam->pos.x, sp->pos.y - cam->pos.y };
	double dist, angle, width, height, maxdiff;
	size_t start_x, start_y;
	dist = hypot(disp.x, disp.y);
	if (dist == 0.0) return;
	angle = atan2(disp.y, disp.x);
	width = atan(sp->scale.x / dist) * 2;
	maxdiff = (cam->fov.x + width) / 2 ;
	if (fabs(angle - cam->facing) > maxdiff) return;
	height = atan(sp->scale.y / dist) * 2 / cam->fov.y * cam->height;
	width = width / cam->fov.x * cam->width;
	start_x = (cam->width - width) / 2 + (cam->facing - angle) / cam->fov.x
		* cam->width;
	start_y = (cam->height - height) / 2;
	for (size_t x = 0; x < width; ++x) {
		size_t cx, sx;
		cx = x + start_x;
		if (cx >= cam->width || dist >= cam->dists[cx]) continue;
		sx = (double)x / width * sp->txtr->width;
		for (size_t y = 0; y < height; ++y) {
			long cy, sy;
			cy = y + start_y;
			if (cy >= cam->height) continue;
			sy = (double)y / height * sp->txtr->height;
			d3d_pixel p = *GET(sp->txtr, pixels, sx, sy);
			if (p != sp->transparent) {
				cam->dists[cx] = dist;
				*GET(cam, pixels, cx, cy) = p;
			}
		}
	}
}

void d3d_draw_sprites(
	d3d_camera *cam,
	const d3d_sprite sprites[],
	size_t n_sprites)
{
	for (size_t s = 0; s < n_sprites; ++s) {
		d3d_draw_sprite(cam, &sprites[s]);
	}
}

void d3d_draw(
	d3d_camera *cam,
	const d3d_sprite sprites[],
	size_t n_sprites,
	const d3d_board *board)
{
	d3d_draw_walls(cam, board);
	d3d_draw_sprites(cam, sprites, n_sprites);
}
