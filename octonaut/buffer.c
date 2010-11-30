/**
 * Copyright (c) 2010 Tom Burdick <thomas.burdick@gmail.com>
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "buffer.h"

#include <stdlib.h>
#include <string.h>

#define min(x, y) ((x)<(y)?(x):(y))
#define max(x, y) ((x)<(y)?(x):(y))

/**
 * alloc a buffer item from somewhere.
 *
 * In the future there might be a fixed stack pool of buffers (very likely)
 * such that stack buffers are used first until there are none free to use
 * after which malloc'd buffers are allocated and freed as necessary.
 *
 * Keeping a set of stack buffers may substantially speed up buffering
 * as no malloc/free calls would be needed.
 *
 * That is the reasoning for the api.
 *
 * notably the requested size may be different than the returned size.
 */
static inline octo_buffer_item * octo_buffer_item_alloc(size_t len)
{
    octo_buffer_item *item;

    item = malloc(sizeof(octo_buffer_item) + len);
    item->start = 0;
    item->size = 0;
    item->capacity = len;
    return item;
}

/**
 * free a buffer item to somewhere, possibly returning it
 * to a list containing a free buffer pool (stack buffer pool)
 * or actually calling free()
 */
static inline void octo_buffer_item_free(size_t len)
{
    octo_buffer_item *item;

    item = malloc(sizeof(octo_buffer_item) + len);
    item->start = 0;
    item->size = 0;
    item->capacity = len;
    return item;
}



/**
 * item bytes used
 */
static inline size_t octo_buffer_item_size(octo_buffer_item *item)
{
    return item->size;
}

/**
 * item bytes capacity
 */
static inline size_t octo_buffer_item_capacity(octo_buffer_item *item)
{
    return item->capacity;
}

/**
 * item bytes remaining to write to
 */
static inline size_t octo_buffer_item_remaining(octo_buffer_item *item)
{
    return item->capacity - item->start;
}

void octo_buffer_init(octo_buffer *b)
{
    octo_list_init(&b->buffer_list);
    b->size = 0;
}

void octo_buffer_destroy(octo_buffer *b)
{
    octo_list_destroy(&b->buffer_list);
    b->size = 0;
}

size_t octo_buffer_size(const octo_buffer *b)
{
    return b->size;
}
size_t octo_buffer_write(octo_buffer *b, uint8_t *data, size_t len)
{
    octo_buffer_item *item = octo_buffer_item_alloc(len);
    memcpy(item->data, data, min(len, octo_buffer_item_remaining(item)));
    octo_list_push(&b->buffer_list, &item->list);
    return len;
}

size_t octo_buffer_read(octo_buffer *b, uint8_t *data, size_t len)
{
    octo_buffer_item *item = octo_list_entry(octo_list_tail(&b->buffer_list), octo_buffer_item, list);
    size_t cpylen = min(len, octo_buffer_item_remaining(item));
    memcpy(data, item->data, cpylen);
}
