//
// Created by xuwei on 2023/5/6.
//

#ifndef NGX_POOL_MEM_POOL_H
#define NGX_POOL_MEM_POOL_H
using u_char = unsigned char;
using u_int = unsigned int;
struct ngx_pool_s;

/*大块内存的头部信息*/
struct ngx_pool_large_s {
    ngx_pool_large_s     *next; /*所有大块内存用链表连起来*/
    void                 *alloc; /*大块内存地址*/
};

/*清理操作回调函数的函数类型*/
typedef void (*ngx_pool_cleanup_pt)(void *data);

struct ngx_pool_cleanup_s {
    ngx_pool_cleanup_pt   handler; /*保存清理操作回调函数*/
    void                 *data; /*传给清理操作回调函数的参数*/
    ngx_pool_cleanup_s   *next; /*不只一个，用链表把所有的连起来*/
};


/*小块内存池的头部数据信息*/
struct ngx_pool_data_t {
    u_char               *last; /*空闲可分配内存起始地址*/
    u_char               *end; /*空闲可分配内存末尾*/
    ngx_pool_s           *next; /*下一个*/
    u_int         failed; /*分配失败次数*/
} ;

/*内存池头部信息*/
struct ngx_pool_s {
    ngx_pool_data_t       d; /*内存使用情况*/
    size_t                max; /*小块内存的最大大小*/
    ngx_pool_s           *current; /*指向第一个*/
    ngx_pool_large_s     *large; /*大块内存的入口*/
    ngx_pool_cleanup_s   *cleanup; /*清理操作回调函数链表入口*/
};

#endif //NGX_POOL_MEM_POOL_H
