/*
   +----------------------------------------------------------------------+
   | Zend Engine                                                          |
   +----------------------------------------------------------------------+
   | Copyright (c) 1998-2018 Zend Technologies Ltd. (http://www.zend.com) |
   +----------------------------------------------------------------------+
   | This source file is subject to version 2.00 of the Zend license,     |
   | that is bundled with this package in the file LICENSE, and is        |
   | available through the world-wide-web at the following url:           |
   | http://www.zend.com/license/2_00.txt.                                |
   | If you did not receive a copy of the Zend license and are unable to  |
   | obtain it through the world-wide-web, please send a note to          |
   | license@zend.com so we can mail you a copy immediately.              |
   +----------------------------------------------------------------------+
   | Authors: Andi Gutmans <andi@zend.com>                                |
   |          Zeev Suraski <zeev@zend.com>                                |
   +----------------------------------------------------------------------+
*/

#ifndef ZEND_COROUTINE_H
#define ZEND_COROUTINE_H

#include "zend.h"

#define ZEND_COROUTINE_STACK_SIZE   (1 << 20)


typedef enum _zend_coro_status  zend_coro_status;

enum _zend_coro_status {
    ZEND_CORO_STATUS_INIT,
    ZEND_CORO_STATUS_WORKING,
    ZEND_CORO_STATUS_SLEEP,
    ZEND_CORO_STATUS_FINISHED,
    ZEND_CORO_STATUS_QUIT,
};

struct _zend_coroutine {
    int id;
    zend_coro_status status;
    zend_coroutine *prev_coroutine;
    zend_execute_data *current_execute_data;    //maybe call function execute data after coroutine start
    zval retval;

    zval          *vm_stack_top;
	zval          *vm_stack_end;
	zend_vm_stack  vm_stack;
	size_t         vm_stack_page_size;

    char *prev_ctx;
    char *ctx;
    char stack[0];
};

char *make_context(char *stack, void (*coro_run)(zend_coroutine *co));
void jump_context(char **curr_ctx, char *new_ctx, zend_coroutine *co);

ZEND_API zend_coroutine *zend_coroutine_create();
ZEND_API void zend_coroutine_destroy(zend_coroutine *co);
ZEND_API zend_coro_status zend_coroutine_execute();
ZEND_API void zend_coroutine_yield();

ZEND_API int zend_coroutine_sleep(int timeout);
#endif
