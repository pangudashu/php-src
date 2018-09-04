#ifndef ZEND_HEAP_H
#define ZEND_HEAP_H

#include <stdint.h>
#include "zend.h"

#define ZEND_HEAP_DEFAULT_SIZE 8

typedef struct {
    int64_t index;
    int64_t value;
    void *data;
} zend_heap_node;

typedef struct {
    int64_t size;
    int64_t num;
    zend_heap_node **nodes;
} zend_heap;


ZEND_API zend_heap *zend_heap_create();
ZEND_API void zend_heap_destroy(zend_heap *heap);
ZEND_API zend_heap_node *zend_heap_insert(zend_heap *heap, int64_t value, void *data);
ZEND_API void zend_heap_delete(zend_heap *heap, zend_heap_node *node);
ZEND_API zend_heap_node *zend_heap_top(zend_heap *heap);

#endif
