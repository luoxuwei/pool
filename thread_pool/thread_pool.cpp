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

void ThreadPool::submitTask(std::shared_ptr<Task> t) {

}

void ThreadPool::start(int s) {
    initThreadSize_ = s;

    /*为了保证线程的公平性，先创建线程对象，再统一开启线程*/

    for (int i=0; i<initThreadSize_; i++) {
        threads_.emplace_back(new Thread(std::bind(&ThreadPool::threadFunc, this)));
    }

    for (int i=0; i<initThreadSize_; i++) {
        threads_[i]->start();
    }
}

void ThreadPool::threadFunc() {
    std::cout<<"thread: "<<std::this_thread::get_id()<<std::endl;
}

Thread::Thread(ThreadFunc func): func_(func) {

}

void Thread::start() {
    std::thread t(func_);
    t.detach();
}