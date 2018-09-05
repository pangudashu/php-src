#include <stdint.h>
#include <stdio.h>
#include "zend_coroutine.h"
#include "zend_execute.h"
#include "zend_event.h"

void zend_coroutine_run(zend_coroutine *co)
{
    zend_execute_ex(co->execute_data);

    co->end = 1;
    jump_context(&co->ctx, co->prev_ctx, co);
}

ZEND_API zend_coroutine *zend_coroutine_create(zend_execute_data *execute_data)
{
    zend_coroutine *co;
    
    co = (zend_coroutine *)emalloc(sizeof(zend_coroutine) + ZEND_COROUTINE_STACK_SIZE);

    execute_data->coroutine = co;
    execute_data->is_coroutine_call = 1;
    execute_data->prev_execute_data = NULL;

    co->execute_data = execute_data;
    co->ctx = make_context(co->stack + ZEND_COROUTINE_STACK_SIZE, zend_coroutine_run);
    
    return co;
}

ZEND_API void zend_coroutine_yield(zend_coroutine *co)
{
    co->execute_data = EG(current_execute_data);
    jump_context(&co->ctx, co->prev_ctx, co);
}

ZEND_API void zend_coroutine_execute(zend_coroutine *co)
{
    zend_coroutine *prev_coroutine;
    zend_execute_data *execute_data;

    if (co->end) {
        goto COROUTINE_EXECUTE_END;
    }

    //switch to call coroutine
    prev_coroutine = EG(current_coroutine);
    EG(current_coroutine) = co;
    EG(current_execute_data) = co->execute_data;
    
    jump_context(&co->prev_ctx, co->ctx, co);
    
    if (prev_coroutine) {
        EG(current_execute_data) = prev_coroutine->execute_data;
        EG(current_coroutine) = prev_coroutine;
    }

    //yield or finish
    if (!co->end) {
        return;
    }

COROUTINE_EXECUTE_END:
    execute_data = co->execute_data;
    while(execute_data){
        zend_vm_stack_free_call_frame(execute_data);
        execute_data = execute_data->prev_execute_data;
    }
    efree(co);
}

static void zend_coroutine_event_timeout(zend_event *ev, short type, void *arg)
{ 
    zend_coroutine *co;
    
    if (type != ZEND_EV_TIMEOUT) {
        return;
    }

    co = (zend_coroutine *)arg;
    zend_coroutine_execute(co);
}

ZEND_API int zend_coroutine_sleep(zend_coroutine *co, int timeout)
{
    int64_t timeout_ms = (int64_t)(timeout * 1000);

    zend_event *ev = zend_event_new(0, ZEND_EV_TIMEOUT, timeout_ms, zend_coroutine_event_timeout, co);
    zend_event_add(EG(event), ev);
    zend_coroutine_yield(co);
    return SUCCESS;
}


