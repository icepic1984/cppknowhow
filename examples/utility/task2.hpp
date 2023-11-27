// https://www.youtube.com/watch?v=lm10Cj-HNKQ&t=2559s This is
// iteration one from the task implementation by Pavel Novikov. From
// my understanding this code uses symetric transfer but is not thread
// safe (because it is not required)?!?

#pragma once

#include "trace.hpp"
#include <iostream>
#include <coroutine>
#include <thread>
#include <condition_variable>
#include <type_traits>
#include <variant>
#include <utility>
#include <atomic>
#include <future>

namespace iter2
{

template <typename T>
struct Promise;

struct CoroDeleter
{
    template <typename Promise>
    void operator()(Promise* promise) const noexcept
    {
        DBG;
        using CoroHandle = std::coroutine_handle<Promise>;
        CoroHandle::from_promise(*promise).destroy();
    }
};

template <typename T>
using PromisePtr = std::unique_ptr<Promise<T>, CoroDeleter>;

template <typename T = void>
struct [[nodiscard]] Task
{
    using promise_type = Promise<T>;

    Task() = default;

    auto operator co_await() const noexcept
    {
        DBG;
        struct Awaitable
        {
            bool await_ready() const noexcept
            {
                DBG;
                // We ask the promise if the result is ready
                return promise.isReady();
            }

            using CoroHandle = std::coroutine_handle<>;
            CoroHandle await_suspend(CoroHandle continuation) const noexcept
            {
                DBG;
                std::cerr << "Contiunation: " << continuation.address() << std::endl;
                promise.continuation = continuation;
                return std::coroutine_handle<Promise<T>>::from_promise(promise);
            }

            decltype(auto) await_resume() const
            {
                DBG;
                return promise.getResult();
            }

            Promise<T>& promise;
        };
        return Awaitable{*promise};
    }

private:
    explicit Task(Promise<T>* promise)
        : promise{promise}
    {
        DBG;
    };

    PromisePtr<T> promise = nullptr;

    template <typename>
    friend struct Promise;
};

template <typename T>
struct Promise
{
    std::coroutine_handle<> continuation;

    std::variant<std::monostate, T, std::exception_ptr> result;

    Task<T> get_return_object() noexcept
    {
        DBG;
        return Task<T>{this};
    }

    // eager execute the task
    std::suspend_always initial_suspend() noexcept
    {
        DBG;
        return {};
    }

    auto final_suspend() noexcept
    {
        struct FinalAwaitable
        {
            bool await_ready() const noexcept
            {
                DBG;
                return false;
            }

            void await_suspend(
                std::coroutine_handle<Promise<T>> thisCoro) noexcept
            {
                DBG;
                auto& promise = thisCoro.promise();
                if (promise.continuation)
                {
                    std::cerr << "FinalSuspend::await_suspend continuation: "
                              << promise.continuation.address()<< std::endl;
                    promise.continuation();
                }
                else
                {
                    std::cerr << "FinalSuspend::await_suspend with no continuation " << std::endl;
                }
            }

            void await_resume() const noexcept
            {
            }
        };
        return FinalAwaitable{};
    }

    template <typename U>
    void return_value(U&& value) noexcept(
        std::is_nothrow_constructible_v<T, decltype(std::forward<U>(value))>)
    {
        DBG;
        result.template emplace<1>(std::forward<U>(value));
    }

    void unhandled_exception() noexcept(
        std::is_nothrow_constructible_v<std::exception_ptr, std::exception_ptr>)
    {
        DBG;
        result.template emplace<2>(std::current_exception());
    }

    bool isReady() const noexcept
    {
        DBG;
        return result.index() != 0;
    }

    T&& getResult()
    {
        DBG;
        if (std::holds_alternative<std::exception_ptr>(result))
        {
            std::rethrow_exception(std::get<2>(result));
        }
        return std::move(std::get<1>(result));
    }
};

template <>
struct Promise<void>
{
    std::coroutine_handle<> continuation;

    std::exception_ptr exception = nullptr;

    Task<void> get_return_object() noexcept
    {
        DBG;
        return Task<void>{this};
    }

    void return_void() noexcept
    {
    }

    // eager execute the task
    std::suspend_always initial_suspend() noexcept
    {
        DBG;
        return {};
    }

    auto final_suspend() noexcept
    {
        struct FinalAwaitable
        {
            bool await_ready() const noexcept
            {
                DBG;
                return false;
            }

            void await_suspend(
                std::coroutine_handle<Promise<void>> thisCoro) noexcept
            {
                DBG;
                auto& promise = thisCoro.promise();
                if (promise.continuation)
                {
                    promise.continuation();
                }
            }

            void await_resume() const noexcept
            {
            }
        };
        return FinalAwaitable{};
    }

    void unhandled_exception() noexcept(
        std::is_nothrow_constructible_v<std::exception_ptr, std::exception_ptr>)
    {
        DBG;
        exception = std::current_exception();
    }

    bool isReady() noexcept
    {
        DBG;
        return std::coroutine_handle<Promise<void>>::from_promise(*this).done();
    }

    void getResult()
    {
        DBG;
        if (exception)
        {
            std::rethrow_exception(exception);
        }
        return;
    }
};
}; // namespace iter2
