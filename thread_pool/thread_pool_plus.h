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
#include <thread>
#include <future>

const int TASK_MAX_THRESHHOLD = INT32_MAX;
const int THREAD_MAX_THRESHHOLD = 1024;
const int THREAD_MAX_IDLE_TIME = 60;


/*线程池模式*/
enum class PoolMode {
    MODE_FIXED, /*固定数量的线程*/
    MODE_CACHED, /*线程数量可动态增长*/
};

class Thread {
public:
    using ThreadFunc = std::function<void(int)>;
    Thread(ThreadFunc func): func_(func), threadId_(generateID_++) {

    }
    ~Thread() = default;
    void start() {
        std::thread t(func_, threadId_);
        t.detach();
    }

    int getId() {
        return threadId_;
    }
private:
    ThreadFunc func_;
    static int generateID_;
    int threadId_;
};

int Thread::generateID_ = 0;

class ThreadPool {
public:
    ThreadPool()
            : initThreadSize_(5)
            , taskSize_(0)
            , taskQueMaxThresHold_(TASK_MAX_THRESHHOLD)
            , poolMode_(PoolMode::MODE_FIXED)
            , isRunning_(false)
            , idleThreadSize_(0)
            , threadSizeThreshHold_(THREAD_MAX_THRESHHOLD)
            , curThreadSize_(0) {

    }
    ~ThreadPool() {
        isRunning_ = false;

        std::unique_lock<std::mutex> lock(taskQueMtx_);
        notEmpty_.notify_all();
        exit_.wait(lock, [&]()->bool {return threads_.size() == 0;});
    }
    ThreadPool(const ThreadPool &other) = delete;
    ThreadPool &operator=(const ThreadPool &other) = delete;
    void setMode(PoolMode mode) {
        if (isRunning_) return;
        poolMode_ = mode;
    }
    void start(int s = std::thread::hardware_concurrency()) {
        isRunning_ = true;
        initThreadSize_ = s;
        curThreadSize_ = s;

        /*为了保证线程的公平性，先创建线程对象，再统一开启线程*/

        for (int i=0; i<initThreadSize_; i++) {
            auto ptr = std::make_unique<Thread>(std::bind(&ThreadPool::threadFunc, this, std::placeholders::_1));
            int id = ptr->getId();
            threads_.emplace(id, std::move(ptr));
        }

        for (int i=0; i<initThreadSize_; i++) {
            threads_[i]->start();
            idleThreadSize_++;
        }
    }
    void setTaskQueMaxThreshHold(int threshold) {
        taskQueMaxThresHold_ = threshold;
    }

    template<typename Func, typename... Args>
    auto submitTask(Func&& func, Args&&... args) -> std::future<decltype(func(args...))> {
        using RType = decltype(func(args...));
        auto task = std::make_shared<std::packaged_task<RType()>>(
                std::bind(std::forward<Func>(func), std::forward<Args>(args)...));
        std::future<RType> result = task->get_future();

        std::unique_lock<std::mutex> lock(taskQueMtx_);

        /*while(taskQue_.size() == taskQueMaxThreshHold_) {
            notFull_.wait(lock);
        }*/
        if (!notFull_.wait_for(lock, std::chrono::seconds(1),
                               [&]()->bool {return taskQue_.size() < (size_t)taskQueMaxThresHold_;})) {
            std::cerr<<"task queue is full, submit task fail."<<std::endl;
            auto task = std::make_shared<std::packaged_task<RType()>>(
                    []()->RType {return RType();}
            );
            std::future<RType> res = task->get_future();
            (*task)();
            return res;
        }
        taskQue_.emplace([task](){
            (*task)();
        });
        taskSize_++;
        notEmpty_.notify_all();

        if (poolMode_ == PoolMode::MODE_CACHED
            && taskSize_ > idleThreadSize_
            && curThreadSize_ < threadSizeThreshHold_) {
            auto ptr = std::make_unique<Thread>(std::bind(&ThreadPool::threadFunc, this, std::placeholders::_1));
            int id = ptr->getId();
            threads_.emplace(id, std::move(ptr));
            threads_[id]->start();
            curThreadSize_++;
        }

        return result;
    }

    /*线程函数定义为ThreadPool的成员函数，因为线程函数访问的数据都在ThreadPool里*/
    void threadFunc(int id) {
//    std::cout<<"thread: "<<std::this_thread::get_id()<<std::endl;
        auto lastTime = std::chrono::high_resolution_clock().now();
        for (;;) {
            Task task;
            {
                std::unique_lock<std::mutex> lock(taskQueMtx_);
                while (taskQue_.size() == 0) {
                    if (!isRunning_) {
                        threads_.erase(id);
                        exit_.notify_all();
                        return;
                    }
                    if (poolMode_ == PoolMode::MODE_CACHED) {
                        if (std::cv_status::timeout ==
                            notEmpty_.wait_for(lock, std::chrono::seconds(1))) {
                            auto now = std::chrono::high_resolution_clock().now();
                            auto dur = std::chrono::duration_cast<std::chrono::seconds>(now - lastTime);
                            if (dur.count() >= THREAD_MAX_IDLE_TIME && curThreadSize_ > initThreadSize_) {
                                threads_.erase(id);
                                curThreadSize_--;
                                idleThreadSize_--;
                                return;
                            }
                        }
                    } else {
                        notEmpty_.wait(lock);
                    }

                }

                task = taskQue_.front();
                taskQue_.pop();
                taskSize_--;
                notFull_.notify_all();
            }

            if (taskQue_.size() > 0) {
                notEmpty_.notify_all();
            }

            idleThreadSize_--;
            if (nullptr != task) {
//                task->exec();
                task();
            }
            idleThreadSize_++;
            lastTime = std::chrono::high_resolution_clock().now();
        }

    }
private:
    using Task = std::function<void()>;

    std::atomic_int curThreadSize_;
    int threadSizeThreshHold_;
    // std::vector<std::unique_ptr<Thread>> threads_; /*线程列表*/
    std::unordered_map<int, std::unique_ptr<Thread>> threads_;
    size_t initThreadSize_; /*初始的线程数量*/
    /*存指针才能产生多态，对于一些临时任务，出了提交任务的语句就析构了，所以用share_ptr保证task对象的生命周期*/
    std::queue<Task> taskQue_; /*任务队列*/
    std::atomic_int taskSize_; /*任务数量*/
    int taskQueMaxThresHold_; /*任务队列上限 */
    std::mutex taskQueMtx_; /*保证任务队列线程安全*/
    std::condition_variable notFull_; /*表示队列不满*/
    std::condition_variable notEmpty_; /*表示队列不空*/
    std::condition_variable exit_;
    PoolMode poolMode_;
    std::atomic_bool isRunning_;
    std::atomic_int idleThreadSize_;
};

#endif //THREAD_POOL_THREAD_POOL_H
