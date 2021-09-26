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

static void test_timeout(evdriver_ctx_t *handle,void *arg)
{
	printf("hello,world\n");
	static int __cnt = 0;
	if(__cnt++ > 10)
	{
		printf("timer stop(%d)...\n",__cnt);
		evdriver_timer_del(handle);
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





