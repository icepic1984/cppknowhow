// https://www.youtube.com/watch?v=lm10Cj-HNKQ&t=2559s
#if !defined(__PRETTY_FUNCTION__) && !defined(__GNUC__)
#define __PRETTY_FUNCTION__ __FUNCSIG__
#endif

#include <iostream>
#include <coroutine>
#include <unordered_map>
#include <functional>
#include <windows.h>
#include <stdio.h>
#include <thread>
#include <queue>
#include <mutex>
#include <queue>
#include <condition_variable>
#include <type_traits>
#include <variant>
#include <utility>

auto dbg = [](const char* s) { std::cerr << "Function " << s << " called.\n"; };

#define DBG dbg(__PRETTY_FUNCTION__)

template <size_t N>
struct str
{
    constexpr str(const char* p) noexcept
    {
        std::copy_n(p, N, cstr);
    }

    friend std::ostream& operator<<(std::ostream& os, const str& s)
    {
        return os << s.cstr;
    }
    char cstr[N];
};

template <size_t N>
str(const char (&)[N]) -> str<N>;

template <typename T>
struct Generator
{
    struct promise_type
    {
        auto get_return_object() noexcept
        {
            DBG;
            return Generator{*this};
        }

        std::suspend_always initial_suspend() const noexcept
        {
            DBG;
            return {};
        }

        std::suspend_always final_suspend() const noexcept
        {
            DBG;
            return {};
        }

        // co_yield expression is turned into co_await
        // promise.yield_value(expression) by the compiler.
        std::suspend_always yield_value(const T& value) noexcept(
            std::is_nothrow_copy_constructible_v<T>)
        {
            DBG;
            result = value;
            return {};
        }

        // If we want to exit the coroutine early we can use co_return.
        // co_return expression is transformed in {expression;
        // promise.return_void(); goto final_suspend};
        // If you don't call co_return explicitly, it is called
        // automatically if the coroutine falls through the body.
        void return_void() const noexcept
        {
            DBG;
        };

        void unhandled_exception() noexcept(
            std::is_nothrow_copy_constructible_v<std::exception_ptr>)
        {
            DBG;
            result = std::current_exception();
        }

        T& getValue()
        {
            DBG;
            if (std::holds_alternative<std::exception_ptr>(result))
            {
                std::rethrow_exception(std::get<std::exception_ptr>(result));
            }
            return std::get<T>(result);
        }

    private:
        std::variant<std::monostate, T, std::exception_ptr> result;
    };

    Generator(Generator&& other) noexcept
        : handle{std::exchange(other.handle, nullptr)}
    {
        DBG;
    }

    Generator& operator=(Generator&& other) noexcept
    {
        DBG;
        if (handle)
        {
            handle.destory();
        }
        handle = std::exchange(other.handle, nullptr);
    }

    ~Generator()
    {
        DBG;
        if (handle)
        {
            handle.destroy();
        }
    }

    auto& operator()() const
    {
        handle.resume();
        return handle.promise().getValue();
    }

private:
    explicit Generator(promise_type& promise) noexcept
        : handle{std::coroutine_handle<promise_type>::from_promise(promise)}
    {
    }

    std::coroutine_handle<promise_type> handle;
};

Generator<int> test()
{
    DBG;
    co_yield 10;
//    throw std::runtime_error("no");
    co_yield 15;

    //co_return;
}

int main()
{
    const auto f = test();
    try
    {

        std::cout << "co_yield 1" << f() << std::endl;
        std::cout << "co_yield 2" << f() << std::endl;
        std::cout << "co_return" << f() << std::endl;
    }
    catch (std::runtime_error& e)
    {
        std::cout << e.what() << std::endl;
    }
}
