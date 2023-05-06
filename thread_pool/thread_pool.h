//
// Created by xuwei on 2023/5/7.
//

#ifndef THREAD_POOL_THREAD_POOL_H
#define THREAD_POOL_THREAD_POOL_H
#include <vector>
#include <queue>
#include <memory>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <functional>

/*任务抽象基类*/
class Task {
public:
    virtual void run() = 0;
};

/*线程池模式*/
enum class PoolMode {
    MODE_FIXED, /*固定数量的线程*/
    MODE_CACHED, /*线程数量可动态增长*/
};

class Thread {
public:
    using ThreadFunc = std::function<void()>;
    Thread(ThreadFunc func);
    ~Thread();
    void start();

private:
    ThreadFunc func_;
};

class ThreadPool {
public:
    ThreadPool();
    ~ThreadPool();
    ThreadPool(const ThreadPool &other) = delete;
    ThreadPool &operator=(const ThreadPool &other) = delete;
    void setMode(PoolMode mode);
    void start(int s = 4);
    void setTaskQueMaxThreshHold(int threshold);
    void submitTask(std::shared_ptr<Task> t);
    /*线程函数定义为ThreadPool的成员函数，因为线程函数访问的数据都在ThreadPool里*/
    void threadFunc();
private:
    std::vector<std::unique_ptr<Thread>> threads_; /*线程列表*/
    size_t initThreadSize_; /*初始的线程数量*/
    /*存指针才能产生多态，对于一些临时任务，出了提交任务的语句就析构了，所以用share_ptr保证task对象的生命周期*/
    std::queue<std::shared_ptr<Task>> taskQue_; /*任务队列*/
    std::atomic_int taskSize_; /*任务数量*/
    int taskQueMaxThresHold_; /*任务队列上限 */
    std::mutex taskQueMtx_; /*保证任务队列线程安全*/
    std::condition_variable notFull_; /*表示队列不满*/
    std::condition_variable notEmpty_; /*表示队列不空*/
    PoolMode poolMode_;
};

#endif //THREAD_POOL_THREAD_POOL_H
