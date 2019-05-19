#ifndef PTHREAD_WORKERS_H_
#define PTHREAD_WORKERS_H_

#include "../d3d.h"

void init_threads(d3d_camera *cam, const d3d_board *board);

void wait_for_frame(void);

void start_next_frame(void);

void destroy_threads(void);

#endif /* PTHREAD_WORKERS_H_ */
