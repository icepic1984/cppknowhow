#pragma once

#include "trace.hpp"
#include <iostream>
#include <coroutine>
#include <optional>
#include <source_location>
#include <thread>


// OptionalHolder is returned as proxy class from
// promise->get_return_object.  This struct can be implicitly casted
// to std::optional
template <typename T, typename P>
struct OptionalHolder
{
    std::optional<T> optional;
    OptionalHolder(P* p)
    {
        p->optional = &optional;
    }
    OptionalHolder(OptionalHolder&&) = delete;

    OptionalHolder(OptionalHolder const&) = delete;

    operator std::optional<T>() const noexcept
    {
        std::cout << "doing implicit conversion of result.\n";
        return std::move(optional);
    }
};

template <typename T>
struct OptionalPromise
{
    std::optional<T>* optional = nullptr;

    OptionalHolder<T, OptionalPromise<T>> get_return_object() noexcept
    {
        return {this};
    }

    std::suspend_never initial_suspend() noexcept
    {
        return {};
    }

    std::suspend_never final_suspend() noexcept
    {
        return {};
    }

    void return_value(const T& val) noexcept
    {
        *optional = val;
    }

    void unhandled_exception()
    {
    }

    template <typename U>
    auto await_transform(std::optional<U>& value)
    {
        struct awaiter
        {
            const std::optional<U>* value;

            bool await_ready() noexcept
            {
                return value->has_value();
            }

            void await_suspend(std::coroutine_handle<> handle) noexcept
            {
                handle.destroy();
            }

            U await_resume() noexcept
            {
                return **value;
            };
        };
        return awaiter{&value};
    }
};

namespace std
{
template <typename T, typename... ARGS>
struct coroutine_traits<std::optional<T>, ARGS...>
{
    using promise_type = OptionalPromise<T>;
};
} // namespace std

