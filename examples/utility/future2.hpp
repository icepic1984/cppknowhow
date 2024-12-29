#include <chrono>
#include <coroutine>
#include <exception>
#include <future>
#include <iostream>
#include <thread>
#include <type_traits>
#include "trace.hpp"
// A program-defined type on which the coroutine_traits specializations
// below depend
struct as_coroutine
{
};

// Enable the use of std::future<T> as a coroutine type
// by using a std::promise<T> as the promise type.
template <typename T, typename... Args>
struct std::coroutine_traits<std::future<T>, Args...>
{
    struct promise_type
    {
        std::promise<T> promise;
        std::future<T> get_return_object() noexcept
        {
            DBG;
            return promise.get_future();
        }

        std::suspend_never initial_suspend() const noexcept
        {
            DBG;
            return {};
        }
        std::suspend_never final_suspend() const noexcept
        {
            DBG;
            return {};
        }

        void return_value(const T& value) noexcept(
            std::is_nothrow_copy_constructible_v<T>)
        {
            DBG;
            promise.set_value(value);
        }
        void return_value(T&& value) noexcept(
            std::is_nothrow_move_constructible_v<T>)
        {
            DBG;
            promise.set_value(std::move(value));
        }
        void unhandled_exception() noexcept
        {
            DBG;
            promise.set_exception(std::current_exception());
        }
    };
};

// Same for std::future<void>.
template <typename... Args>
struct std::coroutine_traits<std::future<void>, as_coroutine, Args...>
{
    struct promise_type
    {
        std::promise<void> promise;
        std::future<void> get_return_object() noexcept
        {
            DBG;
            return promise.get_future();
        }

        std::suspend_never initial_suspend() const noexcept
        {
            DBG;
            return {};
        }
        std::suspend_never final_suspend() const noexcept
        {
            DBG;
            return {};
        }

        void return_void() noexcept
        {
            DBG;
            promise.set_value();
        }
        void unhandled_exception() noexcept
        {
            DBG;
            promise.set_exception(std::current_exception());
        }
    };
};

// Allow co_await'ing std::future<T> and std::future<void>
// by naively spawning a new thread for each co_await.
template <typename T>
auto operator co_await(std::future<T> future) noexcept
{
    struct awaiter
    {
        std::future<T> future;
        bool await_ready() const noexcept
        {
            DBG;
            using namespace std::chrono_literals;
            return future.wait_for(0s) != std::future_status::timeout;
        }
        void await_suspend(std::coroutine_handle<> cont) const
        {
            DBG;
            std::thread([this, cont] {
                DBG;
                future.wait();
                cont();
            }).detach();
            DBG;
        }
        T await_resume()
        {
            DBG;
            return future.get();
        }
    };
    return awaiter{std::move(future)};
}
