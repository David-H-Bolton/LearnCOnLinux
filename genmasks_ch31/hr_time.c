#include "hr_time.h"
#include <time.h>
#include <linux/time.h>
#include <stdlib.h>

void startTimer(stopWatch *timer) {
	clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &timer->start);
};

void stopTimer(stopWatch *timer) {
	clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &timer->stop);
};

double diff(stopWatch *timer)
{
	return  (timer->stop.tv_sec - timer->start.tv_sec) + 
	        (timer->stop.tv_nsec-timer->start.tv_nsec)/1.0e9;
}

