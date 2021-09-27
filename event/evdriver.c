#include <stdio.h>
#include <errno.h>
#include <fcntl.h>		/* O_CLOEXEC */
#include <string.h>		/* memset() */
#include <sys/epoll.h>
#include <sys/ioctl.h>
#include <sys/select.h>		/* for select() workaround */
#include <sys/signalfd.h>	/* struct signalfd_siginfo */
#include <unistd.h>		/* close(), read() */
#include <assert.h>
#include <stdlib.h>

#include <time.h>
#include <sys/time.h>
#include <sys/timerfd.h>

#include "evdriver.h"


struct evdriver_ctx_s {
	struct list_head list;
	int running;
	int loop;
	unsigned long timeout;
};


static void msec2tspec(int msec, struct timespec *ts)
{
	if (msec) {
		ts->tv_sec  =  msec / 1000;
		ts->tv_nsec = (msec % 1000) * 1000000;
	} else {
		ts->tv_sec  = 0;
		ts->tv_nsec = 0;
	}
}

evdriver_ctx_t *evdriver_create(void)
{
	evdriver_ctx_t *handle = NULL;
	handle = malloc(sizeof(*handle));
	assert(handle);
	handle->loop = epoll_create1(EPOLL_CLOEXEC);
	assert(handle->loop > 0);
	handle->timeout = -1;
	INIT_LIST_HEAD(&handle->list);
	return handle;
}

int evdriver_timer_add(evdriver_ctx_t *handle,evdriver_callback_t cb,void *arg,unsigned long timeout,unsigned long period)
{
	assert(handle);
	struct itimerspec time;
	int ret;
	int fd = -1;
	evdriver_t *evdriver = NULL;
	evdriver = malloc(sizeof(*evdriver));
	assert(evdriver);
	evdriver->ctx = handle;
	evdriver->cb = cb;
	evdriver->arg= arg;
	evdriver->timer.timeout = timeout;
	evdriver->timer.period = period;
	fd = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC);
	assert(fd > 0);
	evdriver->fd = fd;
	msec2tspec(timeout, &time.it_value);
	msec2tspec(period, &time.it_interval);

	timerfd_settime(evdriver->fd, 0, &time, NULL);
	
	struct epoll_event ev;
	ev.events   = EPOLLIN | EPOLLRDHUP;
	ev.data.ptr = evdriver;

	ret = epoll_ctl(handle->loop, EPOLL_CTL_ADD, evdriver->fd, &ev);
	assert(ret == 0);
	list_add_tail(&evdriver->list, &handle->list);
	
	return fd;
}

void evdriver_timer_del(evdriver_ctx_t *handle,int fd)
{
	evdriver_t *ev,*tmp;
	if(handle)
	{
		list_for_each_entry_safe(ev,tmp, &handle->list, list) {
			if(fd == ev->fd)
			{
				list_del_init(&ev->list);
				epoll_ctl(handle->loop, EPOLL_CTL_DEL, ev->fd, NULL);
				close(ev->fd);
				ev->fd = -1;
				free(ev);
				ev = NULL;
				break;
			}
		}
	}
}


void evdriver_run(evdriver_ctx_t *handle)
{
#define MAX_EVENTS  10 
	struct epoll_event ee[MAX_EVENTS];
	int i, nfds;
	int ret;
	evdriver_t *ev = NULL;
	uint64_t exp;
	
	assert(handle);
	while(handle->running)
	{
		nfds = epoll_wait(handle->loop, ee, MAX_EVENTS, handle->timeout);
		//printf("nfds=%d\n",nfds);
		for (i = 0; i < nfds; i++) 
		{
			ev = (evdriver_t *)ee[i].data.ptr;
			ret = read(ev->fd, &exp, sizeof(exp));
			assert(ret >= 0);
			if(ev->cb) ev->cb(handle,ev->arg);
		}
	}
}



void evdriver_exit(evdriver_ctx_t *handle)
{
	evdriver_t *ev,*tmp;
	if(handle)
	{
		list_for_each_entry_safe(ev,tmp, &handle->list, list) {
			list_del_init(&ev->list);
			epoll_ctl(handle->loop, EPOLL_CTL_DEL, ev->fd, NULL);
			close(ev->fd);
			ev->fd = -1;
			free(ev);
			ev = NULL;
		}
		free(handle);
		handle = NULL;
	}
}






