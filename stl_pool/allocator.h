//
// Created by xuwei on 2023/5/6.
//

#ifndef STL_POOL_ALLOCATOR_H
#define STL_POOL_ALLOCATOR_H

template<typename T>
class allocator {
public:
    T *allocate(size_t __n); /*开辟内存*/
    void deallocate(void *__p, size_t __n); /*释放内存*/
    void* reallocate(void* __p, size_t __old_sz, size_t __new_sz); /*扩容&缩容*/
    /*构造对象*/
    void construct(T *__p, const T &val) {
        new (__p) T(val);
    }
    /*对象析构*/
    void destroy(T *__p) {
        __p->~T();
    }

};

#endif //STL_POOL_ALLOCATOR_H
