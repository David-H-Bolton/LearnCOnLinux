#pragma once
#include <linux/time.h>
#define __timespec_defined 1
#define __itimerspec_defined 1
#define __timeval_defined 1
#include <stdlib.h>

struct _times {
	struct timespec start;
	struct timespec stop;
};

typedef struct _times stopWatch;

void startTimer(stopWatch *timer);
void stopTimer(stopWatch *timer);
double diff(stopWatch *timer);
