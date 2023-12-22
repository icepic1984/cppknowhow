// https://www.youtube.com/watch?v=lm10Cj-HNKQ&t=2559s

#include <iostream>
#include <coroutine>
#include <thread>
#include <condition_variable>
#include <type_traits>
#include <variant>
#include <utility>
#include <filesystem>
#include <fstream>
#include "../utility/task0.hpp"
#include "../utility/task1.hpp"
#include "../utility/task2.hpp"
#include "../utility/optional.hpp"
#include "../utility/generator.hpp"
#include "../utility/task_helper.hpp"

iter1::Task<> foo(std::jthread& t)
{
    std::cout << "foo1 " << std::this_thread::get_id() << "\n";
    co_await SwitchToNewThread(t);
    std::cout << "foo2 " << std::this_thread::get_id() << "\n";
}

iter1::Task<> bar(std::jthread& t)
{
    std::cout << "bar1 " << std::this_thread::get_id() << "\n";
    co_await foo(t);
    std::cout << "bar2" << std::this_thread::get_id() << "\n";
}

iter0::Task<size_t> readFileIter0()
{
    std::cout << std::this_thread::get_id()
              << " readFile(): about to read file async\n";
    const auto result =
        co_await AsyncReadFile{"c:/Projects/git/cppknowhow/type_list.cpp"};
    std::cout << std::this_thread::get_id()
              << " readFile(): about to return (size " << result.size()
              << ")\n";
    co_return result.size();
}


iter1::Task<size_t> readFileIter1()
{
    std::cout << std::this_thread::get_id()
              << " readFile(): about to read file async\n";
    const auto result =
        co_await AsyncReadFile{"c:/Projects/git/cppknowhow/type_list.cpp"};
    std::cout << std::this_thread::get_id()
              << " readFile(): about to return (size " << result.size()
              << ")\n";
    co_return result.size();
}

iter2::Task<size_t> readFileIter2()
{
    std::cout << std::this_thread::get_id()
              << " readFile(): about to read file async\n";
    const auto result =
        co_await AsyncReadFile{"c:/Projects/git/cppknowhow/type_list.cpp"};
    std::cout << std::this_thread::get_id()
              << " readFile(): about to return (size " << result.size()
              << ")\n";
    co_return result.size();
}

std::optional<int> test(std::optional<int> a, std::optional<int> b)
{
    co_return(co_await a) * (co_await b);
}
Generator<int> test()
{
    DBG;
    co_yield 10;
    //    throw std::runtime_error("no");
    co_yield 15;

    // co_return;
}



int main()
{
    // std::optional<int> n = test(10, std::nullopt);
    // if (n.has_value())
    //     std::cout << *n << std::endl;
    // else
    //     std::cout << "has_value == false.\n";

    // n = test(10, 20);

    // if (n.has_value())
    //     std::cout << *n << std::endl;

    // std::jthread t;
    // auto bla = bar(t);
    //auto task = readFileIter1();
    std::cout << std::this_thread::get_id() << std::endl;
    syncWait(readFileIter1());
    std::cout << std::this_thread::get_id() << std::endl;
    //std::this_thread::sleep_for(std::chrono::seconds(10));
    //syncWait(readFileIter1());
    //syncWait(readFileIter2());
}
