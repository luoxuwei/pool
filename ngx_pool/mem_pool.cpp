//
// Created by xuwei on 2023/5/6.
//
#include <cstdlib>
#include <stdint.h>
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

void * mem_pool::ngx_palloc(size_t size) {

    if (size <= pool_->max) {
        return ngx_palloc_small(size, 1);
    }

    return ngx_palloc_large(size);
}

void * mem_pool::ngx_pcalloc(size_t size) {
    void *p;

    p = ngx_palloc(size);
    if (p) {
        ngx_memzero(p, size);
    }
    return p;
}

void * mem_pool::ngx_pnalloc(size_t size) {
    if (size <= pool_->max) {
        return ngx_palloc_small(size, 0);
    }

    return ngx_palloc_large(size);
}

void * mem_pool::ngx_palloc_small(size_t size, u_int align) {
    u_char      *m;
    ngx_pool_s  *p;

    p = pool_->current;

    do {
        m = p->d.last;

        if (align) {
            m = ngx_align_ptr(m, NGX_ALIGNMENT);
        }

        if ((size_t) (p->d.end - m) >= size) {
            p->d.last = m + size;

            return m;
        }

        p = p->d.next;

    } while (p);

    return ngx_palloc_block(size);
}

void * mem_pool::ngx_palloc_block(size_t size) {
    u_char      *m;
    size_t       psize;
    ngx_pool_s  *p, *new_s;

    psize = (size_t) (pool_->d.end - (u_char *) pool_);

    m = (u_char *)malloc(psize);
    if (m == nullptr) {
        return nullptr;
    }

    new_s = (ngx_pool_s *) m;

    new_s->d.end = m + psize;
    new_s->d.next = nullptr;
    new_s->d.failed = 0;

    m += sizeof(ngx_pool_data_t);
    m = ngx_align_ptr(m, NGX_ALIGNMENT);
    new_s->d.last = m + size;

    for (p = pool_->current; p->d.next; p = p->d.next) {
        if (p->d.failed++ > 4) {
            pool_->current = p->d.next;
        }
    }

    p->d.next = new_s;

    return m;
}

void * mem_pool::ngx_palloc_large(size_t size) {
    void              *p;
    u_int             n;
    ngx_pool_large_s  *large;

    p = malloc(size);
    if (p == nullptr) {
        return nullptr;
    }

    n = 0;

    for (large = pool_->large; large; large = large->next) {
        if (large->alloc == nullptr) {
            large->alloc = p;
            return p;
        }

        if (n++ > 3) {
            break;
        }
    }

    large = (ngx_pool_large_s *)ngx_palloc_small(sizeof(ngx_pool_large_s), 1);
    if (large == nullptr) {
        ngx_free(p);
        return nullptr;
    }

    large->alloc = p;
    large->next = pool_->large;
    pool_->large = large;

    return p;
}

void mem_pool::ngx_free(void *p) {
    ngx_pool_large_s  *l;

    for (l = pool_->large; l; l = l->next) {
        if (p == l->alloc) {
            free(l->alloc);
            l->alloc = nullptr;

            return;
        }
    }

}