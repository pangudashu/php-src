
#ifndef ZEND_EVENT_H
#define ZEND_EVENT_H

#define HAVE_EPOLL 1

#include <unistd.h>
#include "zend.h"
#include "zend_heap.h"

#define ZEND_EV_TIMEOUT  (1 << 0)
#define ZEND_EV_READ     (1 << 1)
#define ZEND_EV_WRITE    (1 << 2)
#define ZEND_EV_PERSIST  (1 << 3)
#define ZEND_EV_EDGE     (1 << 4)

#define zend_event_set_timer(ev, flags, cb, arg) zend_event_set((ev), -1, (flags), (cb), (arg))

typedef struct _zend_event_module   zend_event_module;
typedef struct _zend_event          zend_event;
typedef struct _zend_event_queue    zend_event_queue;

typedef void (*event_callback)(zend_event *, short, void *);

struct _zend_event {
	int fd;                   /* not set with ZEND_EV_TIMEOUT */
	struct timeval timeout;   /* next time to trigger */
	event_callback callback;
	void *arg;
	int flags;
	int index;                /* index of the fd in the ufds array */
	short which;              /* type of event */
};

struct _zend_event_queue {
	zend_event_queue *prev;
	zend_event_queue *next;
	zend_event *ev;
};

struct _zend_event_module {
	const char *name;
    zend_heap *timeout_list;
	int support_edge_trigger;
	int (*init)(int max_fd);
	int (*clean)(void);
	int (*wait)(unsigned long int timeout);
	int (*add)(zend_event *ev);
	int (*remove)(zend_event *ev);
};


ZEND_API zend_event_module *zend_event_create();
ZEND_API zend_event *zend_event_new(int fd, int flags, int64_t timeout_ms, event_callback callback, void *arg);
ZEND_API void zend_event_destroy(zend_event_module *module);
ZEND_API int zend_event_add(zend_event_module *module, zend_event *ev);
ZEND_API void zend_event_loop(zend_event_module *module);
/*
void zend_event_loop(int err);
void zend_event_fire(struct zend_event_s *ev);
int zend_event_init_main();
int zend_event_set(struct zend_event_s *ev, int fd, int flags, void (*callback)(struct zend_event_s *, short, void *), void *arg);
int zend_event_add(struct zend_event_s *ev, unsigned long int timeout);
int zend_event_del(struct zend_event_s *ev);
int zend_event_pre_init(char *machanism);
const char *zend_event_machanism_name();
int zend_event_support_edge_trigger();
*/

#endif
