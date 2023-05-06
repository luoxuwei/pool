//
// Created by xuwei on 2023/5/6.
//

#include "mem_pool.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


typedef struct Data stData;
struct Data
{
    char *ptr;
    FILE *pfile;
};

void func1(void *p)
{
    printf("free ptr mem!");
    free(p);
}
void func2(void *pf)
{
    printf("close file!");
    fclose((FILE *)pf);
}
int main()
{
    mem_pool memPool;
    if(nullptr == memPool.ngx_create_pool(512))
    {
        printf("ngx_create_pool fail...");
        return 0;
    }

    void *p1 = memPool.ngx_palloc(128); // 从小块内存池分配的
    if(p1 == nullptr)
    {
        printf("ngx_palloc 128 bytes fail...");
        return 0;
    }

    stData *p2 = (Data *)memPool.ngx_palloc(512); // 从大块内存池分配的
    if(p2 == nullptr)
    {
        printf("ngx_palloc 512 bytes fail...");
        return 0;
    }
    p2->ptr = (char *)malloc(12);
    strcpy(p2->ptr, "hello world");
    p2->pfile = fopen("data.txt", "w");

    ngx_pool_cleanup_s *c1 = memPool.ngx_pool_cleanup_add(sizeof(char*));
    c1->handler = func1;
    c1->data = p2->ptr;

    ngx_pool_cleanup_s *c2 = memPool.ngx_pool_cleanup_add(sizeof(FILE*));
    c2->handler = func2;
    c2->data = p2->pfile;

    memPool.ngx_destroy_pool(); // 1.调用所有的预置的清理函数 2.释放大块内存 3.释放小块内存池所有内存

    return 0;
}
