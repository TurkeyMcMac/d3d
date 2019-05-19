#ifndef TICKER_H_
#define TICKER_H_

//#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200809L
//#endif
#include <time.h>

struct ticker {
	struct timespec last_tick;
	long interval;
};

void ticker_init(struct ticker *tkr, long interval);

void tick(struct ticker *tkr);

#endif /* TICKER_H_ */
