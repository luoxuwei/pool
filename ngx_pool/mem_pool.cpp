//
// Created by xuwei on 2023/5/6.
//
#include <cstdlib>
#include "mem_pool.h"


void * mem_pool::ngx_create_pool(size_t size) {
    ngx_pool_s  *p;

    p = (ngx_pool_s *)malloc(size);
    if (p == nullptr) {
        return nullptr;
    }

    p->d.last = (u_char *) p + sizeof(ngx_pool_s);
    p->d.end = (u_char *) p + size;
    p->d.next = nullptr;
    p->d.failed = 0;

    size = size - sizeof(ngx_pool_s);
    p->max = (size < NGX_MAX_ALLOC_FROM_POOL) ? size : NGX_MAX_ALLOC_FROM_POOL;

    p->current = p;
    p->large = NULL;
    p->cleanup = NULL;

    return p;
}