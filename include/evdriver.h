#ifndef EVDRIVER_H_
#define EVDRIVER_H_

typedef struct evdriver_ctx_s evdriver_ctx_t;
typedef void (*evdriver_callback_t)(evdriver_ctx_t *handle,void *arg);

///////////////////////////////////////////////////////////////////////////////////
evdriver_ctx_t *evdriver_create(void);
int evdriver_timer_add(evdriver_ctx_t *handle,evdriver_callback_t cb,void *arg,unsigned long timeout,unsigned long period);
void evdriver_timer_del(evdriver_ctx_t *handle);
void evdriver_run(evdriver_ctx_t *handle);

#endif
