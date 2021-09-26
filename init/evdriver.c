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


typedef struct evdriver_ctx_s evdriver_ctx_t;

typedef void (*evdriver_callback_t)(evdriver_ctx_t *handle,void *arg);

struct evdriver_ctx_s {
	int loop;
	int timerfd;
	evdriver_callback_t cb;
	void *arg;
	unsigned long timeout;
};

///////////////////////////////////////////////////////////////////////////////////
evdriver_ctx_t *evdriver_create(void);
int evdriver_timer_add(evdriver_ctx_t *handle,evdriver_callback_t cb,void *arg,unsigned long timeout,unsigned long period);
void evdriver_timer_stop(evdriver_ctx_t *handle);
void evdriver_run(evdriver_ctx_t *handle);


static void test_timeout(evdriver_ctx_t *handle,void *arg)
{
	printf("hello,world\n");
	static int __cnt = 0;
	if(__cnt++ > 10)
	{
		printf("timer stop(%d)...\n",__cnt);
		evdriver_timer_stop(handle);
	}
}

int main(int argc, char *argv[])
{
	int ret = -1;
	evdriver_ctx_t *handle = NULL;
	handle = evdriver_create();
	assert(handle);
	ret = evdriver_timer_add(handle,test_timeout,NULL,1000,1000);
	assert(0 == ret);
	evdriver_run(handle);	
	return 0;
}




/**************************************************************/
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
	return handle;
}

int evdriver_timer_add(evdriver_ctx_t *handle,evdriver_callback_t cb,void *arg,unsigned long timeout,unsigned long period)
{
	assert(handle);
	struct itimerspec time;
	int ret;
	
	handle->cb = cb;
	handle->arg= arg;
	handle->timerfd = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC);
	assert(handle->timerfd > 0);
	msec2tspec(timeout, &time.it_value);
	msec2tspec(period, &time.it_interval);

	timerfd_settime(handle->timerfd, 0, &time, NULL);
	
	struct epoll_event ev;
	ev.events   = EPOLLIN | EPOLLRDHUP;
	ev.data.ptr = handle;

	ret = epoll_ctl(handle->loop, EPOLL_CTL_ADD, handle->timerfd, &ev);
	assert(ret == 0);
	return 0;
}

void evdriver_timer_stop(evdriver_ctx_t *handle)
{
	epoll_ctl(handle->loop, EPOLL_CTL_DEL, handle->timerfd, NULL);
	close(handle->timerfd);
	handle->timerfd = -1;
}


void evdriver_run(evdriver_ctx_t *handle)
{
#define MAX_EVENTS  10 
	struct epoll_event ee[MAX_EVENTS];
	int i, nfds;
	int ret;
	
	assert(handle);
	while(1)
	{
		nfds = epoll_wait(handle->loop, ee, MAX_EVENTS, handle->timeout);
		//printf("nfds=%d\n",nfds);
		for (i = 0; i < nfds; i++) 
		{
			uint64_t exp;
			ret = read(handle->timerfd, &exp, sizeof(exp));
			assert(ret >= 0);
			if(handle->cb) handle->cb(handle,handle->arg);
		}
	}
}










