#ifndef EVDRIVER_H_
#define EVDRIVER_H_

#include "klist.h"

typedef struct evdriver_ctx_s evdriver_ctx_t;
typedef void (*evdriver_callback_t)(evdriver_ctx_t *handle,void *arg);


typedef struct evdriver_s {
	evdriver_ctx_t *ctx;
	struct list_head list;
	evdriver_callback_t cb;
	void *arg;
	int fd;
	/*private*/
	union {
		struct __evd_timer {
			unsigned long timeout;
			unsigned long period;
		}timer;
	};
}evdriver_t;

///////////////////////////////////////////////////////////////////////////////////
evdriver_ctx_t *evdriver_create(void);
int evdriver_timer_add(evdriver_ctx_t *handle,evdriver_callback_t cb,void *arg,unsigned long timeout,unsigned long period);
void evdriver_timer_del(evdriver_ctx_t *handle,int fd);
void evdriver_run(evdriver_ctx_t *handle);
void evdriver_exit(evdriver_ctx_t *handle);

#endif
