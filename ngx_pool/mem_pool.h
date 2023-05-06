//
// Created by xuwei on 2023/5/6.
//

#ifndef NGX_POOL_MEM_POOL_H
#define NGX_POOL_MEM_POOL_H

using u_char = unsigned char;
using u_int = unsigned int;
struct ngx_pool_s;
struct ngx_pool_large_s;

/*把数值d调整为临近的a的倍数*/
#define ngx_align(d, a) (((d) + (a-1)) & ~(a-1))

const int ngx_pagesize = 4096; /*默认一个物理页面的大小4k*/
const int NGX_MAX_ALLOC_FROM_POOL = (ngx_pagesize - 1); /*小块内存池可分配的最大空间*/

const int NGX_DEFAULT_POOL_SIZE   = (16 * 1024); /*默认的内存池大小*/

const int NGX_POOL_ALIGNMENT      = 16; /*内存池大小按16字节对齐*/


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
    u_int                failed; /*分配失败次数*/
} ;

/*内存池头部信息*/
struct ngx_pool_s {
    ngx_pool_data_t       d; /*内存使用情况*/
    size_t                max; /*小块内存的最大大小*/
    ngx_pool_s           *current; /*指向第一个*/
    ngx_pool_large_s     *large; /*大块内存的入口*/
    ngx_pool_cleanup_s   *cleanup; /*清理操作回调函数链表入口*/
};

/*小内存池最小的大小调整到NGX_POOL_ALIGNMENT的临近倍数*/
const int NGX_MIN_POOL_SIZE =                                                     \
    ngx_align((sizeof(ngx_pool_s) + 2 * sizeof(ngx_pool_large_s)),            \
              NGX_POOL_ALIGNMENT);

class mem_pool {
public:
    /*创建size大小的内存池，小块内存池不超过1个页面的大小*/
    void *ngx_create_pool(size_t size);
    /*申请内存，考虑内存对其*/
    void *ngx_palloc(size_t size);
    /*申请内存，不考虑内存对其*/
    void *ngx_pnalloc(size_t size);
    /*申请内存，内存会初始化0*/
    void *ngx_pcalloc(size_t size);
    /*释放大块内存*/
    void ngx_free(void *p);
    /*内存重置*/
    void ngx_reset_pool();
    /*内存池销毁*/
    void ngx_destroy_pool();
    /*添加清理函数*/
    ngx_pool_cleanup_s *ngx_pool_cleanup_add(size_t size);
private:
    /*小块内存分配*/
    void *ngx_palloc_small(size_t size, u_int align);
    /*大块内存分配*/
    void *ngx_palloc_large(size_t size);
    /*分配新的小块内存池*/
    void *ngx_palloc_block(size_t size);
    ngx_pool_s *pool_;
};

#endif //NGX_POOL_MEM_POOL_H
