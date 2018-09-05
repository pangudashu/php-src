#include <unistd.h>
#include <errno.h>
#include <stdlib.h> /* for putenv */
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <sys/time.h>

#include "zend_event.h"

//#include "events/select.h"
//#include "events/poll.h"
#include "events/epoll.h"
//#include "events/devpoll.h"
//#include "events/port.h"
//#include "events/kqueue.h"

#define zend_event_set_timeout(ev, now) timeradd(&(now), &(ev)->frequency, &(ev)->timeout);

static int zend_event_timenow(struct timeval *tv)
{
    /*
    struct timespec ts;

    if (0 > clock_gettime(CLOCK_MONOTONIC, &ts)) {
        return FAILURE;
    }

    tv->tv_sec = ts.tv_sec;
    tv->tv_usec = ts.tv_nsec / 1000;
    */

    if (gettimeofday(tv, NULL) < 0)
    {
        return FAILURE;
    }
    return SUCCESS;
}

ZEND_API zend_event_module *zend_event_create()
{
	int max = 5000;
    zend_heap *timeout_event;
    zend_event_module *module;

#ifdef HAVE_EPOLL
    module = zend_event_epoll_module();
#endif

	if (!module) {
		return NULL;
	}

	if (!module->wait) {
		return NULL;
	}

	if (module->init(max) < 0) {
		return NULL;
	}

    timeout_event = zend_heap_create();
    if (!timeout_event) {
        return NULL;
    }
    module->timeout_list = timeout_event;

	return module;
}

ZEND_API void zend_event_destroy(zend_event_module *module)
{
    if (module->clean) {
        module->clean();
    }
    if (module->timeout_list) {
        zend_heap_destroy(module->timeout_list);
    }
}

ZEND_API zend_event *zend_event_new(int fd, int flags, int64_t timeout_ms, event_callback callback, void *arg)
{
    zend_event *ev = emalloc(sizeof(zend_event));

    ev->fd = fd;
    ev->flags = flags;
    ev->callback = callback;
    ev->arg = arg;

    if (timeout_ms > 0) {
        ev->timeout.tv_sec = timeout_ms / 1000;
        ev->timeout.tv_usec = (timeout_ms % 1000)*1000;
    }

    return ev;
}

static int64_t zend_event_time_getms(struct timeval *tv)
{
    return tv->tv_sec * 1000 + tv->tv_usec / 1000;
}

ZEND_API int zend_event_add(zend_event_module *module, zend_event *ev)
{
    struct timeval now;

    if (!(ev->flags & ZEND_EV_TIMEOUT)) {
        return module->add(ev);
    }

    //timer event
    if (FAILURE == zend_event_timenow(&now)) {
        return FAILURE;
    }

    ev->timeout.tv_sec += now.tv_sec;
    ev->timeout.tv_usec += now.tv_usec;
    zend_heap_insert(module->timeout_list, zend_event_time_getms(&(ev->timeout)), ev);

    return SUCCESS;
}


ZEND_API void zend_event_loop(zend_event_module *module)
{
    int ret;
    struct timeval now;
    zend_heap_node *time_node;
    zend_event *ev;
    unsigned long int timeout = 1000;
    unsigned long int now_time;

    zend_event_timenow(&now);
    now_time = zend_event_time_getms(&now);
 
    time_node = zend_heap_top(module->timeout_list);
    if (time_node) {
        if (time_node->value > now_time) {
            timeout = time_node->value - now_time;
        }
    }

    while (1) {
        ret = module->wait(timeout);
    
        timeout = -1;
        /* trigger timers */
        while ((time_node = zend_heap_top(module->timeout_list)) != NULL) {
            zend_event_timenow(&now);
            now_time = zend_event_time_getms(&now);
            ev = (zend_event *)time_node->data;

            if (time_node->value <= now_time) {
                ev->callback(ev, ZEND_EV_TIMEOUT, ev->arg);
                zend_heap_delete(module->timeout_list, time_node);
            } else {
                timeout = time_node->value - now_time;
                break;
            }
        }
    }
}

