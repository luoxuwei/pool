//
// Created by xuwei on 2023/5/6.
//

#ifndef STL_POOL_ALLOCATOR_H
#define STL_POOL_ALLOCATOR_H
#include <mutex>

template<typename T>
class allocator {
public:
    /*开辟内存*/
    T *allocate(size_t __n) {
        void* __ret = 0;

        if (__n > (size_t) _MAX_BYTES) {
            __ret = malloc_alloc::allocate(__n);
        }
        else {
            _Obj* volatile* __my_free_list
                    = _S_free_list + _S_freelist_index(__n);
            // Acquire the lock here with a constructor call.
            // This ensures that it is released in exit or during stack
            // unwinding.
            std::lock_guard<std::mutex> guard(mtx);
            _Obj* __result = *__my_free_list;
            if (__result == 0)
                __ret = _S_refill(_S_round_up(__n));
            else {
                *__my_free_list = __result -> _M_free_list_link;
                __ret = __result;
            }
        }

        return __ret;
    };

    /*释放内存*/
    void deallocate(void *__p, size_t __n) {
        if (__n > (size_t) _MAX_BYTES)
            malloc_alloc::deallocate(__p, __n);
        else {
            _Obj* volatile*  __my_free_list
                    = _S_free_list + _S_freelist_index(__n);
            _Obj* __q = (_Obj*)__p;

            // acquire lock
            std::lock_guard<std::mutex> guard(mtx);
            __q -> _M_free_list_link = *__my_free_list;
            *__my_free_list = __q;
            // lock is released here
        }
    };
    /*扩容&缩容*/
    void* reallocate(void* __p, size_t __old_sz, size_t __new_sz) {
        void* __result;
        size_t __copy_sz;

        if (__old_sz > (size_t) _MAX_BYTES && __new_sz > (size_t) _MAX_BYTES) {
            return(realloc(__p, __new_sz));
        }
        if (_S_round_up(__old_sz) == _S_round_up(__new_sz)) return(__p);
        __result = allocate(__new_sz);
        __copy_sz = __new_sz > __old_sz? __old_sz : __new_sz;
        memcpy(__result, __p, __copy_sz);
        deallocate(__p, __old_sz);
        return(__result);
    };
    /*构造对象*/
    void construct(T *__p, const T &val) {
        new (__p) T(val);
    }
    /*对象析构*/
    void destroy(T *__p) {
        __p->~T();
    }

private:
    enum {_ALIGN = 8}; /*自由链表从8字节开始，以8字节为对齐方式，一直扩充到128*/
    enum {_MAX_BYTES = 128}; /*内存池最大的chunk块*/
    enum {_NFREELISTS = 16}; /*自由链表的个数*/

    /*chunk块的头信息， _M_free_list_link是下一个chunk块的地址*/
    union _Obj {
        union _Obj* _M_free_list_link;
        char _M_client_data[1];    /* The client sees this.        */
    };
    /*自由链表数组*/
    static _Obj* volatile _S_free_list[_NFREELISTS];
    static std::mutex mtx;

    /*将__bytes上调至最邻近的8的倍数*/
    static size_t _S_round_up(size_t __bytes)
    { return (((__bytes) + (size_t) _ALIGN-1) & ~((size_t) _ALIGN - 1)); }
    /*返回 __bytes大小的区块位于自由链表中的编号*/
    static size_t _S_freelist_index(size_t __bytes) {
        return (((__bytes) + (size_t)_ALIGN-1)/(size_t)_ALIGN - 1);
    }

    /*分配自由链表chunk块*/
    static char* _S_chunk_alloc(size_t __size, int& __nobjs)
    {
        char* __result;
        size_t __total_bytes = __size * __nobjs;
        size_t __bytes_left = _S_end_free - _S_start_free;

        if (__bytes_left >= __total_bytes) {
            __result = _S_start_free;
            _S_start_free += __total_bytes;
            return(__result);
        } else if (__bytes_left >= __size) {
            __nobjs = (int)(__bytes_left/__size);
            __total_bytes = __size * __nobjs;
            __result = _S_start_free;
            _S_start_free += __total_bytes;
            return(__result);
        } else {
            size_t __bytes_to_get =
                    2 * __total_bytes + _S_round_up(_S_heap_size >> 4);
            // Try to make use of the left-over piece.
            if (__bytes_left > 0) {
                _Obj* __STL_VOLATILE* __my_free_list =
                        _S_free_list + _S_freelist_index(__bytes_left);

                ((_Obj*)_S_start_free) -> _M_free_list_link = *__my_free_list;
                *__my_free_list = (_Obj*)_S_start_free;
            }
            _S_start_free = (char*)malloc(__bytes_to_get);
            if (nullptr == _S_start_free) {
                size_t __i;
                _Obj* volatile* __my_free_list;
                _Obj* __p;
                // Try to make do with what we have.  That can't
                // hurt.  We do not try smaller requests, since that tends
                // to result in disaster on multi-process machines.
                for (__i = __size;
                     __i <= (size_t) _MAX_BYTES;
                     __i += (size_t) _ALIGN) {
                    __my_free_list = _S_free_list + _S_freelist_index(__i);
                    __p = *__my_free_list;
                    if (0 != __p) {
                        *__my_free_list = __p -> _M_free_list_link;
                        _S_start_free = (char*)__p;
                        _S_end_free = _S_start_free + __i;
                        return(_S_chunk_alloc(__size, __nobjs));
                        // Any leftover piece will eventually make it to the
                        // right free list.
                    }
                }
                _S_end_free = 0;	// In case of exception.
                _S_start_free = (char*)malloc_alloc::allocate(__bytes_to_get);
                // This should either throw an
                // exception or remedy the situation.  Thus we assume it
                // succeeded.
            }
            _S_heap_size += __bytes_to_get;
            _S_end_free = _S_start_free + __bytes_to_get;
            return(_S_chunk_alloc(__size, __nobjs));
        }
    }

    /*把分配好的chunk块进行连接*/
    static void *_S_refill(size_t __n)
    {
        int __nobjs = 20;
        char* __chunk = _S_chunk_alloc(__n, __nobjs);
        _Obj* volatile* __my_free_list;
        _Obj* __result;
        _Obj* __current_obj;
        _Obj* __next_obj;
        int __i;

        if (1 == __nobjs) return(__chunk);
        __my_free_list = _S_free_list + _S_freelist_index(__n);

        /* Build free list in chunk */
        __result = (_Obj*)__chunk;
        *__my_free_list = __next_obj = (_Obj*)(__chunk + __n);
        for (__i = 1; ; __i++) {
            __current_obj = __next_obj;
            __next_obj = (_Obj*)((char*)__next_obj + __n);
            if (__nobjs - 1 == __i) {
                __current_obj -> _M_free_list_link = 0;
                break;
            } else {
                __current_obj -> _M_free_list_link = __next_obj;
            }
        }
        return(__result);
    }

    /*内存池的使用情况*/
    // Chunk allocation state.
    static char* _S_start_free;
    static char* _S_end_free;
    static size_t _S_heap_size;
};

template <typename T>
char* allocator<T>::_S_start_free = nullptr;

template <typename T>
char* allocator<T>::_S_end_free = nullptr;

template <typename T>
size_t allocator<T>::_S_heap_size = nullptr;

template <typename T>
_Obj *allocator<T>::_S_free_list[NFREELISTS] = {nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
                                                nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, };
template <typename T>
allocator<T>::mtx;
#endif //STL_POOL_ALLOCATOR_H
