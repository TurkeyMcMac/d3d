#ifndef TICKER_H_
#define TICKER_H_

#if __APPLE__
#	include <time.h>
#	include <stdint.h>

struct ticker {
	uint64_t last_tick;
	uint64_t interval;
};
#elif __unix__
#	define TICKER_WITH_CLOCK
#	include <time.h>

struct ticker {
	struct timespec last_tick;
	long interval;
};
#elif defined(_WIN32)
#	include <windows.h>

struct ticker {
	DWORD last_tick;
	DWORD interval;
};
#else
#	error Unsupported OS
#endif

void ticker_init(struct ticker *tkr, long interval);

void tick(struct ticker *tkr);

#endif /* TICKER_H_ */
