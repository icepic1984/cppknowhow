// https://www.youtube.com/watch?v=lm10Cj-HNKQ&t=2559s This is the
// task implementation from Pavel Novikov. From my understanding he
// does not use symetric transfer in this iteration. Instead he is
// using atmoics to make it thread safe.

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

namespace iter1
{
enum class State
{
    Started,
    AttachedContinuation,
    Finished
};

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
            bool await_suspend(CoroHandle continuation) const noexcept
            {
                DBG;
                promise.continuation = continuation;
                auto expectedState = State::Started;
                return promise.state.compare_exchange_strong(
                    expectedState, State::AttachedContinuation);
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

    std::atomic<State> state = {State::Started};

    Task<T> get_return_object() noexcept
    {
        DBG;
        return Task<T>{this};
    }

    // eager execute the task
    std::suspend_never initial_suspend() noexcept
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
                const auto oldState = promise.state.exchange(State::Finished);
                if (oldState == State::AttachedContinuation)
                {
                    std::cerr << "FinalSuspend::await_supend continue: "<< (void*)promise.continuation.address() << std::endl;
                    promise.continuation();
                }
                else
                {
                    std::cerr << "FinalSuspend::await_supend no continuation " << std::endl;
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
        return state == State::Finished;
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

    std::atomic<State> state = {State::Started};

    Task<void> get_return_object() noexcept
    {
        DBG;
        return Task<void>{this};
    }

    void return_void() noexcept
    {
    }

    // eager execute the task
    std::suspend_never initial_suspend() noexcept
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
                const auto oldState = promise.state.exchange(State::Finished);
                if (oldState == State::AttachedContinuation)
                {
                    DBG;
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
        return state == State::Finished;
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

} // namespace iter0

