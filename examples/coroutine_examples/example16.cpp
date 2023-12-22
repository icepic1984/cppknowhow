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

boost::future<int> compute_value()
{
    int result = co_await boost::async([] {
        std::this_thread::sleep_for(std::chrono::seconds(10));
        return 30;
    });
    co_return result;
}

int main()
{
    auto bla = compute_value();
    std:: cout << bla.get() << std::endl;
}
