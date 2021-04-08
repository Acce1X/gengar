#ifndef DHMP_TIMERFD_H
#define DHMP_TIMERFD_H
#include <time.h>
int dhmp_timerfd_create(struct itimerspec *new_value);

#endif
