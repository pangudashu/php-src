#include <stdint.h>
#include <stdio.h>
#include "zend_coroutine.h"
#include "zend_execute.h"
#include "zend_event.h"

static int coroutine_id = 0;

void zend_coroutine_run(zend_coroutine *co)
{
    zend_function *fbc = co->current_execute_data->func;

    if (EXPECTED(fbc->type == ZEND_INTERNAL_FUNCTION)) {
        fbc->internal_function.handler(co->current_execute_data, &co->retval);
    } else {
        zend_execute_ex(co->current_execute_data);
    }

    co->status = ZEND_CORO_STATUS_FINISHED;
    jump_context(&co->ctx, co->prev_ctx, co);
}

ZEND_API zend_coroutine *zend_coroutine_create()
{
    zend_coroutine *co;
    
    co = (zend_coroutine *)emalloc(sizeof(zend_coroutine) + ZEND_COROUTINE_STACK_SIZE);

    co->id = ++coroutine_id;
    co->status = ZEND_CORO_STATUS_INIT;
    co->ctx = make_context(co->stack + ZEND_COROUTINE_STACK_SIZE, zend_coroutine_run);

    zend_vm_stack_init(co);

    return co;
}

ZEND_API void zend_coroutine_destroy(zend_coroutine *co)
{
    zend_vm_stack_destroy(co);
    efree(co);
}

ZEND_API void zend_coroutine_yield()
{
    EG(current_coroutine)->status = ZEND_CORO_STATUS_SLEEP;
    EG(current_coroutine)->current_execute_data = EG(current_execute_data);
    jump_context(&EG(current_coroutine)->ctx, EG(current_coroutine)->prev_ctx, EG(current_coroutine));
}

ZEND_API zend_coro_status zend_coroutine_execute()
{
    if (EG(current_coroutine)->status == ZEND_CORO_STATUS_FINISHED) {
        goto COROUTINE_QUIT;
    }

    EG(current_execute_data) = EG(current_coroutine)->current_execute_data;
    EG(current_coroutine)->status = ZEND_CORO_STATUS_WORKING;
    jump_context(&EG(current_coroutine)->prev_ctx, EG(current_coroutine)->ctx, EG(current_coroutine));

    //yield or finish
    if (EG(current_coroutine)->status == ZEND_CORO_STATUS_SLEEP) {
        return EG(current_coroutine)->status;
    }

COROUTINE_QUIT:
    /*
    if (co->current_execute_data == co->origin_execute_data && co->current_execute_data != NULL) {
	    fbc = co->current_execute_data->func;

        if (EXPECTED(fbc->type == ZEND_INTERNAL_FUNCTION)) {
            zend_vm_stack_free_args(co->current_execute_data);
        }

        zend_vm_stack_free_call_frame(co->current_execute_data);
    }
    */

    return ZEND_CORO_STATUS_QUIT;
}

static void zend_coroutine_event_timeout(zend_event *ev, short type, void *arg)
{ 
    zend_coroutine *co;
    
    if (type != ZEND_EV_TIMEOUT) {
        return;
    }

    co = (zend_coroutine *)arg;
    EG(current_coroutine) = co;
    zend_coroutine_execute();
}

ZEND_API int zend_coroutine_sleep(int timeout)
{
    return sleep(timeout);

    int64_t timeout_ms = (int64_t)(timeout * 1000);

    zend_event *ev = zend_event_new(0, ZEND_EV_TIMEOUT, timeout_ms, zend_coroutine_event_timeout, EG(current_coroutine));
    zend_event_add(EG(event), ev);
    zend_coroutine_yield();
    return SUCCESS;
}


