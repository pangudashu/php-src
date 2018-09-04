
#include "zend.h"
#include "zend_heap.h"

ZEND_API zend_heap *zend_heap_create()
{
    zend_heap *heap = NULL;
    
    if (!(heap = emalloc(sizeof(zend_heap)))) {
        return NULL;
    }

    if (!(heap->nodes = emalloc((ZEND_HEAP_DEFAULT_SIZE + 1) * sizeof(void *)))) {
        return NULL;
    }
    heap->size = ZEND_HEAP_DEFAULT_SIZE + 1;
    heap->num = 1;

    return heap;
}

ZEND_API void zend_heap_destroy(zend_heap *heap)
{
    if (!heap) {
        return;
    }
    efree(heap->nodes);
    efree(heap);
}

static void zend_heap_swap(zend_heap *heap, int64_t i, uint32_t j)
{
    if (i == j) {
        return;
    }
    zend_heap_node *tmp_node;

    tmp_node = heap->nodes[i];
    heap->nodes[i] = heap->nodes[j];
    heap->nodes[j] = tmp_node;

    heap->nodes[i]->index = i;
    heap->nodes[j]->index = j;
}

static void zend_heap_up(zend_heap *heap, int64_t index)
{
    int64_t parent;

    if (index <= 1) {
        return;
    }

    parent = index / 2;
    //比父节点小则交换
    if (heap->nodes[index]->value < heap->nodes[parent]->value) {
        zend_heap_swap(heap, index, parent);
        zend_heap_up(heap, parent);
    }
}

static void zend_heap_down(zend_heap *heap, int64_t index)
{
    int64_t size = heap->num -1;
    int64_t child_index;

    if (index * 2 > size) {
        return;
    } else if (index * 2 < size) { //both left and right child
        if (heap->nodes[index * 2]->value < heap->nodes[index * 2 + 1]->value) {
            child_index = index * 2;
        } else {
            child_index = index * 2 + 1;
        }
    } else if (index * 2 == size) { //only left child
        child_index = index * 2;
    }

    if (heap->nodes[index]->value > heap->nodes[child_index]->value) {
        zend_heap_swap(heap, index, child_index);
        zend_heap_down(heap, child_index);
    }
}

ZEND_API zend_heap_node *zend_heap_insert(zend_heap *heap, int64_t value, void *data)
{
    int64_t new_size;
    void *tmp;
    zend_heap_node *node;

    if (heap->num >= heap->size) {
        new_size = heap->size + heap->size;
        if (!(tmp = erealloc(heap->nodes, new_size * sizeof(void *)))) {
            return NULL;
        }
        heap->nodes = tmp;
        heap->size = new_size;
    }

    //alloc node
    if (!(node = emalloc(sizeof(zend_heap_node)))) {
        return NULL;
    }
    node->index = heap->num;
    node->value = value;
    node->data = data;
    
    heap->nodes[heap->num] = node;
    zend_heap_up(heap, heap->num);
    heap->num++;
    return node;
}

ZEND_API void zend_heap_delete(zend_heap *heap, zend_heap_node *node)
{
    int64_t delete_index = node->index;
    //move last node to the delete node
    if (heap->num > 2) {
        zend_heap_swap(heap, node->index, heap->nodes[heap->num-1]->index);
        heap->nodes[heap->num-1] = NULL;
        heap->num--;
        zend_heap_down(heap, delete_index);
    } else {
        heap->num--;
        heap->nodes[delete_index] = NULL;
    }

    efree(node);
}

ZEND_API zend_heap_node *zend_heap_top(zend_heap *heap)
{
    if (!heap->nodes[1]) {
        return NULL;
    }
    return heap->nodes[1];
}
