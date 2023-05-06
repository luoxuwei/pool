#include <iostream>
#include "thread_pool.h"
#include <thread>
int main() {
    ThreadPool pool;
    pool.start();
    std::this_thread::sleep_for(std::chrono::seconds());
    return 0;
}
