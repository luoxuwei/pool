//
// Created by xuwei on 2023/5/7.
//
#include <functional>
#include <thread>
#include <iostream>
#include "thread_pool.h"

const int TASK_MAX_THRESHHOLD = 1024;

ThreadPool::ThreadPool()
    : initThreadSize_(5)
    , taskSize_(0)
    , taskQueMaxThresHold_(TASK_MAX_THRESHHOLD)
    , poolMode_(PoolMode::MODE_FIXED) {

}

ThreadPool::~ThreadPool() {}

void ThreadPool::setMode(PoolMode mode) {
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
    return Result(t);
}

void ThreadPool::start(int s) {
    initThreadSize_ = s;

    /*为了保证线程的公平性，先创建线程对象，再统一开启线程*/

    for (int i=0; i<initThreadSize_; i++) {
        threads_.emplace_back(std::make_unique<Thread>(std::bind(&ThreadPool::threadFunc, this)));
    }

    for (int i=0; i<initThreadSize_; i++) {
        threads_[i]->start();
    }
}

void ThreadPool::threadFunc() {
//    std::cout<<"thread: "<<std::this_thread::get_id()<<std::endl;
    std::shared_ptr<Task> task;
    for (;;) {

        {
            std::unique_lock<std::mutex> lock(taskQueMtx_);
            notEmpty_.wait(lock, [&]()->bool {return taskQue_.size() > 0;});

            task = taskQue_.front();
            taskQue_.pop();
            taskSize_--;
            notFull_.notify_all();
        }

        if (taskQue_.size() > 0) {
            notEmpty_.notify_all();
        }

        if (nullptr != task) {
            task->exec();
        }

    }
}

Thread::Thread(ThreadFunc func): func_(func) {

}

Thread::~Thread() {}

void Thread::start() {
    std::thread t(func_);
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