#ifndef D3D_H_
#define D3D_H_

#include <stddef.h>

typedef char d3d_pixel;

typedef struct {
	double x, y;
} d3d_vec_s;

typedef struct {
	d3d_vec_s pos;
	d3d_vec_s fov;
	double facing;
	size_t width, height;
	double *tans;
	d3d_pixel pixels[];
} d3d_camera;

typedef struct {
	size_t width, height;
	d3d_pixel pixels[];
} d3d_texture;

typedef struct {
	d3d_vec_s pos;
	d3d_vec_s scale;
	const d3d_texture *txtr;
	d3d_pixel transparent;
} d3d_sprite;

typedef struct {
	const d3d_texture *faces[6];
} d3d_block;

typedef struct {
	size_t width, height;
	const d3d_block *blocks[];
} d3d_board;

#define D3D_DNORTH 0
#define D3D_DSOUTH 1
#define D3D_DWEST  2
#define D3D_DEAST  3
#define D3D_DUP    4
#define D3D_DDOWN  5
typedef int d3d_direction;

d3d_camera *d3d_new_camera(
		double fovx,
		double fovy,
		size_t width,
		size_t height);

void d3d_free_camera(d3d_camera *cam);

d3d_texture *d3d_new_texture(size_t width, size_t height);

d3d_board *d3d_new_board(size_t width, size_t height);

void d3d_draw_column(d3d_camera *cam, const d3d_board *board, size_t x);

void d3d_draw_walls(d3d_camera *cam, const d3d_board *board);

void d3d_draw_sprite(d3d_camera *cam, const d3d_sprite *sp);

void d3d_draw_sprites(
	d3d_camera *cam,
	const d3d_sprite sprites[],
	size_t n_sprites);

void d3d_draw(
	d3d_camera *cam,
	const d3d_sprite sprites[],
	size_t n_sprites,
	const d3d_board *board);

#endif /* D3D_H_ */
