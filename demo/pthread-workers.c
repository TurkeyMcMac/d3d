#ifndef POSIX_C_SOURCE
#	define _POSIX_C_SOURCE 200809L
#endif
#include "pthread-workers.h"
#include <pthread.h>

#if !defined(N_PTHREADS) || N_PTHREADS < 1
#	error N_PTHREADS must be defined as a positive integer
#endif

static struct thread_info {
	pthread_t thread;
	size_t start_x, n_cols;
} threads[N_PTHREADS];
static pthread_rwlock_t screen_lock;
static pthread_barrier_t draw_release;
static d3d_camera *cam;
static const d3d_board *board;

static void *worker_draw(void *data)
{
	struct thread_info *thrd = data;
	size_t start_x = thrd->start_x;
	size_t n_cols = thrd->n_cols;
	for (;;) {
		pthread_testcancel();
		pthread_rwlock_rdlock(&screen_lock);
		for (size_t x = start_x; x < start_x + n_cols; ++x) {
			d3d_draw_column(cam, board, x);
		}
		pthread_rwlock_unlock(&screen_lock);
		pthread_barrier_wait(&draw_release);
	}
	return NULL;
}

void init_threads(d3d_camera *cam_, const d3d_board *board_)
{
	pthread_rwlock_init(&screen_lock, NULL);
	pthread_barrier_init(&draw_release, NULL, N_PTHREADS + 1);
	cam = cam_;
	board = board_;
	size_t start_x = 0;
	size_t n_cols = d3d_camera_width(cam) / N_PTHREADS;
	size_t extra = d3d_camera_width(cam) % n_cols;
	for (size_t i = 0; i < N_PTHREADS; ++i) {
		struct thread_info *thrd = &threads[i];
		thrd->start_x = start_x;
		thrd->n_cols = n_cols;
		if (extra) {
			++thrd->n_cols;
			--extra;
		}
		start_x += thrd->n_cols;
		pthread_create(&thrd->thread, NULL, worker_draw, thrd);
	}
}

void wait_for_frame(void)
{
	pthread_rwlock_wrlock(&screen_lock);
}

void start_next_frame(void)
{
	pthread_rwlock_unlock(&screen_lock);
	pthread_barrier_wait(&draw_release);
}

void destroy_threads(void)
{
	for (size_t i = 0; i < N_PTHREADS; ++i) {
		pthread_cancel(threads[i].thread);
	}
	pthread_rwlock_destroy(&screen_lock);
	pthread_barrier_destroy(&draw_release);
}
