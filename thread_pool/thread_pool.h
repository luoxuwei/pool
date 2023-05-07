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
#include <unordered_map>

class Any {
public:
    Any() = default;
    ~Any() = default;
    Any(const Any&) = delete;
    Any& operator=(const Any&) = delete;
    Any(Any&& o) = default;
    Any& operator=(Any&& o) = default;
    template<typename T>
    Any(T data): base_(std::make_unique<Derive<T>>(data)) {}
    template<typename T>
    T cast_() {
        Derive<T> * pd = dynamic_cast<Derive<T>*>(base_.get());
        if (nullptr == pd) {
            throw "type is unmatch!";
        }
        return pd->data_;
    }
private:
    class Base {
    public:
    virtual ~Base() = default;
    };

    template<typename T>
    class Derive: public Base {
    public:
        Derive(T data): data_(data) {}
        T data_;
    };

private:
    std::unique_ptr<Base> base_;
};

class Semaphore {
public:
    Semaphore(int limit = 0):resLimit_(limit){}
    ~Semaphore() = default;

    void wait() {
        std::unique_lock<std::mutex> lock(mtx_);
        cond_.wait(lock, [&]()->bool {return resLimit_>0; });
        resLimit_--;
    }

    void post() {
        std::unique_lock<std::mutex> lock(mtx_);
        resLimit_++;
        cond_.notify_all();
    }

private:
    int resLimit_;
    std::mutex mtx_;
    std::condition_variable cond_;
};

class Task;
class Result {
public:
    Result(std::shared_ptr<Task> st, bool isValid = true);
    ~Result() = default;
    Any get();
    void setVal(Any any);
private:
    std::shared_ptr<Task> task_;
    Any any_;
    Semaphore sem_;
    std::atomic_bool isValid_;
};

/*任务抽象基类*/
class Task {
public:
    void exec();
    virtual Any run() = 0;
    void setResult(Result *res) {
        result_ = res;
    }
private:
    Result *result_;
};

/*线程池模式*/
enum class PoolMode {
    MODE_FIXED, /*固定数量的线程*/
    MODE_CACHED, /*线程数量可动态增长*/
};

class Thread {
public:
    using ThreadFunc = std::function<void(int)>;
    Thread(ThreadFunc func);
    ~Thread();
    void start();
    int getId();
private:
    ThreadFunc func_;
    static int generateID_;
    int threadId_;
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
    Result submitTask(std::shared_ptr<Task> t);
    /*线程函数定义为ThreadPool的成员函数，因为线程函数访问的数据都在ThreadPool里*/
    void threadFunc(int id);
private:
    std::atomic_int curThreadSize_;
    int threadSizeThreshHold_;
    // std::vector<std::unique_ptr<Thread>> threads_; /*线程列表*/
    std::unordered_map<int, std::unique_ptr<Thread>> threads_;
    size_t initThreadSize_; /*初始的线程数量*/
    /*存指针才能产生多态，对于一些临时任务，出了提交任务的语句就析构了，所以用share_ptr保证task对象的生命周期*/
    std::queue<std::shared_ptr<Task>> taskQue_; /*任务队列*/
    std::atomic_int taskSize_; /*任务数量*/
    int taskQueMaxThresHold_; /*任务队列上限 */
    std::mutex taskQueMtx_; /*保证任务队列线程安全*/
    std::condition_variable notFull_; /*表示队列不满*/
    std::condition_variable notEmpty_; /*表示队列不空*/
    PoolMode poolMode_;
    std::atomic_bool isRunning_;
    std::atomic_int idleThreadSize_;
};

#endif //THREAD_POOL_THREAD_POOL_H
