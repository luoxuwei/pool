//
// Created by xuwei on 2023/5/7.
//
#include <functional>
#include <thread>
#include <iostream>
#include "thread_pool.h"

const int TASK_MAX_THRESHHOLD = INT32_MAX;
const int THREAD_MAX_THRESHHOLD = 1024;
const int THREAD_MAX_IDLE_TIME = 60;

ThreadPool::ThreadPool()
    : initThreadSize_(5)
    , taskSize_(0)
    , taskQueMaxThresHold_(TASK_MAX_THRESHHOLD)
    , poolMode_(PoolMode::MODE_FIXED)
    , isRunning_(false)
    , idleThreadSize_(0)
    , threadSizeThreshHold_(THREAD_MAX_THRESHHOLD)
    , curThreadSize_(0) {

}

ThreadPool::~ThreadPool() {
    isRunning_ = false;
    
    std::unique_lock<std::mutex> lock(taskQueMtx_);
    notEmpty_.notify_all();
    exit_.wait(lock, [&]()->bool {return threads_.size() == 0;});
}

void ThreadPool::setMode(PoolMode mode) {
    if (isRunning_) return;
    poolMode_ = mode;
}

void ThreadPool::setTaskQueMaxThreshHold(int threshold) {
    taskQueMaxThresHold_ = threshold;
}

Result ThreadPool::submitTask(std::shared_ptr<Task> t) {
    std::unique_lock<std::mutex> lock(taskQueMtx_);

    /*while(taskQue_.size() == taskQueMaxThreshHold_) {
        notFull_.wait(lock);
    }*/
    if (!notFull_.wait_for(lock, std::chrono::seconds(1),
                           [&]()->bool {return taskQue_.size() < (size_t)taskQueMaxThresHold_;})) {
        std::cerr<<"task queue is full, submit task fail."<<std::endl;
        return Result(t, false);
    }
    taskQue_.emplace(t);
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

    return Result(t);
}

void ThreadPool::start(int s) {
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

void ThreadPool::threadFunc(int id) {
//    std::cout<<"thread: "<<std::this_thread::get_id()<<std::endl;
   auto lastTime = std::chrono::high_resolution_clock().now();
    for (;;) {
        std::shared_ptr<Task> task;
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
            task->exec();
        }
        idleThreadSize_++;
        lastTime = std::chrono::high_resolution_clock().now();
    }
   
}

int Thread::generateID_ = 0;

int Thread::getId() {
    return threadId_;
}

Thread::Thread(ThreadFunc func): func_(func), threadId_(generateID_++) {

}

Thread::~Thread() {}

void Thread::start() {
    std::thread t(func_, threadId_);
    t.detach();
}

Any Result::get() {
    if (!isValid_) {
        return "";
    }
    sem_.wait();
    return std::move(any_);
}

Result::Result(std::shared_ptr<Task> task, bool isValid)
        : isValid_(isValid)
        , task_(task)
{
    task_->setResult(this);
}

void Result::setVal(Any any) {
    any_ = std::move(any);
    sem_.post();
}

void Task::exec() {
    if (nullptr != result_) {
        result_->setVal(run());
    }
}