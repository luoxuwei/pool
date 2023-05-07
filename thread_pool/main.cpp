#include <iostream>
#include "thread_pool.h"
#include <thread>

using uLong = unsigned long long;

class MyTask : public Task
{
public:
    MyTask(int begin, int end)
            : begin_(begin)
            , end_(end)
    {}

    Any run()
    {
        std::cout << "tid:" << std::this_thread::get_id()
                  << "begin!" << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(3));
        uLong sum = 0;
        for (uLong i = begin_; i <= end_; i++)
            sum += i;
        std::cout << "tid:" << std::this_thread::get_id()
                  << "end!" << std::endl;

        return sum;
    }

private:
    int begin_;
    int end_;
};

int main() {
    ThreadPool pool;
    pool.setMode(PoolMode::MODE_CACHED);
    // 开始启动线程池
    pool.start(2);

    // linux上，这些Result对象也是局部对象，要析构的！！！
    Result res1 = pool.submitTask(std::make_shared<MyTask>(1, 100000000));
    Result res2 = pool.submitTask(std::make_shared<MyTask>(100000001, 200000000));
    uLong sum1 = res1.get().cast_<uLong>();
    uLong sum2 = res2.get().cast_<uLong>();
    std::cout << "sum1 + sum2 = " << sum1 + sum2 << std::endl;

    return 0;
}
