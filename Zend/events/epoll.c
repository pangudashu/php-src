/*
   +----------------------------------------------------------------------+
   | PHP Version 7                                                        |
   +----------------------------------------------------------------------+
   | Copyright (c) 1997-2018 The PHP Group                                |
   +----------------------------------------------------------------------+
   | This source file is subject to version 3.01 of the PHP license,      |
   | that is bundled with this package in the file LICENSE, and is        |
   | available through the world-wide-web at the following url:           |
   | http://www.php.net/license/3_01.txt                                  |
   | If you did not receive a copy of the PHP license and are unable to   |
   | obtain it through the world-wide-web, please send a note to          |
   | license@php.net so we can mail you a copy immediately.               |
   +----------------------------------------------------------------------+
   | Authors: Jerome Loyet <jerome@loyet.net>                             |
   +----------------------------------------------------------------------+
*/

#include "zend_event.h"

#if HAVE_EPOLL

#include <sys/epoll.h>
#include <errno.h>

static int zend_event_epoll_init(int max);
static int zend_event_epoll_clean();
static int zend_event_epoll_wait(unsigned long int timeout);
static int zend_event_epoll_add(zend_event *ev);
static int zend_event_epoll_remove(zend_event *ev);

static zend_event_module epoll_module = {
	.name = "epoll",
	.support_edge_trigger = 1,
	.init = zend_event_epoll_init,
	.clean = zend_event_epoll_clean,
	.wait = zend_event_epoll_wait,
	.add = zend_event_epoll_add,
	.remove = zend_event_epoll_remove,
};

static struct epoll_event *epoll_events = NULL;
static int epoll_events_cnt = 0;
static int epollfd = -1;

#endif /* HAVE_EPOLL */

zend_event_module *zend_event_epoll_module() /* {{{ */
{
#if HAVE_EPOLL
	return &epoll_module;
#else
	return NULL;
#endif /* HAVE_EPOLL */
}
/* }}} */

#if HAVE_EPOLL

/*
 * Init the module
 */
static int zend_event_epoll_init(int max) /* {{{ */
{
	if (max < 1) {
		return 0;
	}

	/* init epoll */
	epollfd = epoll_create(max + 1);
	if (epollfd < 0) {
		return -1;
	}

	/* allocate fds */
	epoll_events = emalloc(sizeof(struct epoll_event) * max);
	if (!epoll_events) {
		return -1;
	}
	memset(epoll_events, 0, sizeof(struct epoll_event) * max);

	/* save max */
	epoll_events_cnt = max;

	return 0;
}
/* }}} */

/*
 * Clean the module
 */
static int zend_event_epoll_clean()
{
    if (epoll_events) {
        efree(epoll_events);
    }
    if (epollfd != -1) {
        close(epollfd);
        epollfd = -1;
    }
    return 0;
}
/*
 * wait for events or timeout
 */
static int zend_event_epoll_wait(unsigned long int timeout)
{
    int ret, i;
    zend_event *ev;

    /* ensure we have a clean epoolfds before calling epoll_wait() */
    memset(epoll_events, 0, sizeof(struct epoll_event) * epoll_events_cnt);

    /* wait for inconming event or timeout */
    ret = epoll_wait(epollfd, epoll_events, epoll_events_cnt, timeout);
    if (ret == -1) {
        /* trigger error unless signal interrupt */
        if (errno != EINTR) {
            return -1;
        }
    }
    /* events have been triggered, let's fire them */
    for (i = 0; i < ret; i++) {
        if (!epoll_events[i].data.ptr) {
            continue;
        }
        ev = (zend_event *)epoll_events[i].data.ptr;
        
        if (epoll_events[i].events & EPOLLIN){
            ev->callback(ev, ZEND_EV_READ, ev->arg);
        }
        if (epoll_events[i].events & EPOLLOUT) {
            ev->callback(ev, ZEND_EV_WRITE, ev->arg);
        }
    }
    return ret;
}
/*
 * Add a FD to the fd set
 */
static int zend_event_epoll_add(zend_event *ev) /* {{{ */
{
	struct epoll_event e;

	/* fill epoll struct */
#if SIZEOF_SIZE_T == 4
	/* Completely initialize event data to prevent valgrind reports */
	e.data.u64 = 0;
#endif
    if (ev->flags & ZEND_EV_READ) {
	    e.events = EPOLLIN;
    }
    if (ev->flags & ZEND_EV_WRITE) {
	    e.events = EPOLLOUT;
    }
	e.data.fd = ev->fd;
	e.data.ptr = (void *)ev;

	if (ev->flags & ZEND_EV_EDGE) {
		e.events = e.events | EPOLLET;
	}

	/* add the event to epoll internal queue */
	if (epoll_ctl(epollfd, EPOLL_CTL_ADD, ev->fd, &e) == -1) {
		return -1;
	}

	/* mark the event as registered */
	ev->index = ev->fd;
	return 0;
}
/* }}} */

/*
 * Remove a FD from the fd set
 */
static int zend_event_epoll_remove(zend_event *ev)
{
    return 0;
}

#endif /* HAVE_EPOLL */
