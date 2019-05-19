#include "ticker.h"

void ticker_init(struct ticker *tkr, long interval)
{
	clock_gettime(CLOCK_MONOTONIC, &tkr->last_tick);
	tkr->interval = interval * 1000000;
}

void tick(struct ticker *tkr)
{
	tkr->last_tick.tv_nsec += tkr->interval;
	if (tkr->last_tick.tv_nsec >= 1000000000) {
		tkr->last_tick.tv_nsec -= 1000000000;
		++tkr->last_tick.tv_sec;
	}
	clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &tkr->last_tick, NULL);
}
