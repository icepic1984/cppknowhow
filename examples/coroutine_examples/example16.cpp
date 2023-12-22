// https://www.youtube.com/watch?v=lm10Cj-HNKQ&t=2559s

#include <iostream>
#include <coroutine>
#include <thread>
#include <condition_variable>
#include <type_traits>
#include <variant>
#include <utility>
#include <fstream>
#include "../utility/future.hpp"
#include "../utility/trace.hpp"

std::future<int> compute_value()
{
    DBG;
    int result = co_await std::async([] {
        std::this_thread::sleep_for(std::chrono::seconds(10));
        return 30;
    });
    DBG;
    co_return result;
}

std::future<void> test()
{
    co_return co_await std::async([]{
        std::cout << "Yeah" << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(5));
        std::cout << "Done" << std::endl;
    });
}

int main()
{
    DBG;
    auto bla = compute_value();
    DBG;
    auto result = bla.get();
    test().get();
    std::cout << "Result: " << result << std::endl;
    DBG;

}
