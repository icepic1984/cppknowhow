// https://www.youtube.com/watch?v=lm10Cj-HNKQ&t=2559s

#pragma once

#include "trace.hpp"
#include <iostream>
#include <coroutine>
#include <type_traits>
#include <utility>

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

template <typename T>
struct ImprovedGenerator
{
    struct promise_type
    {
        auto get_return_object() noexcept
        {
            DBG;
            return ImprovedGenerator{*this};
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

        std::suspend_always yield_value(T&& value) noexcept
        {
            result = std::addressof(value);
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
            if (hasException())
            {
                std::rethrow_exception(std::get<std::exception_ptr>(result));
            }
            return std::get<T>(result) ? std::get<T>(result) :
                                         *std::get<T*>(result);
        }

        bool hasException() const noexcept
        {
            DBG;
            return std::holds_alternative<std::exception_ptr>(result);
        }

    private:
        std::variant<std::monostate, T, T*, std::exception_ptr> result;
    };

    ImprovedGenerator(ImprovedGenerator&& other) noexcept
        : handle{std::exchange(other.handle, nullptr)}
    {
        DBG;
    }

    ImprovedGenerator& operator=(ImprovedGenerator&& other) noexcept
    {
        DBG;
        if (handle)
        {
            handle.destory();
        }
        handle = std::exchange(other.handle, nullptr);
    }

    ~ImprovedGenerator()
    {
        DBG;
        if (handle)
        {
            handle.destroy();
        }
    }

    bool advance() const noexcept
    {
        handle.resume();
        return !handle.done() || handle.promise().hasException();
    }

    auto& getValue() const
    {
        return handle.promise().getValue();
    }

private:
    explicit ImprovedGenerator(promise_type& promise) noexcept
        : handle{std::coroutine_handle<promise_type>::from_promise(promise)}
    {
    }

    std::coroutine_handle<promise_type> handle;
};

template <typename T>
struct IterableGenerator
{
    struct promise_type
    {
        auto get_return_object() noexcept
        {
            DBG;
            return IterableGenerator{*this};
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

        std::suspend_always yield_value(T&& value) noexcept
        {
            result = std::addressof(value);
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

        bool isValueInitialized() const noexcept
        {
            return !std::holds_alternative<std::monostate>(result);
        }

        T& getValue() noexcept
        {
            DBG;
            return std::get<T>(result) ? std::get<T>(result) :
                                         *std::get<T*>(result);
        }

        bool hasException() const noexcept
        {
            DBG;
            return std::holds_alternative<std::exception_ptr>(result);
        }

        void throwIfException() const
        {
            if (hasException())
            {
                std::rethrow_exception(std::get<std::exception_ptr>(result));
            }
        }

    private:
        std::variant<std::monostate, T, T*, std::exception_ptr> result;
    };

    struct Iterator
    {
        using iterator_category = std::input_iterator_tag;
        using difference_type = std::ptrdiff_t; // does't make sense
                                                // for inputer iterator
        using value_type = T;
        using reference = T&;
        using pointer = T*;

        Iterator() noexcept = default;
        explicit Iterator(
            const std::coroutine_handle<promise_type>& coro) noexcept
            : handle{&coro}
        {
        }

        friend bool operator==(
            const Iterator&, const Iterator&) noexcept = default;
        friend bool operator!=(
            const Iterator&, const Iterator&) noexcept = default;

        Iterator& operator++()
        {
            handle->resume();
            if (handle->done())
            {
                auto tmp = std::exchange(handle, nullptr);
                tmp->promise().throwIfException();
            }
            return *this;
        }
        auto& operator*() const noexcept
        {
            return handle->promise().getValue();
        }

        const std::coroutine_handle<promise_type>* handle;
    };

    IterableGenerator(IterableGenerator&& other) noexcept
        : handle{std::exchange(other.handle, nullptr)}
    {
        DBG;
    }

    IterableGenerator& operator=(IterableGenerator&& other) noexcept
    {
        DBG;
        if (handle)
        {
            handle.destory();
        }
        handle = std::exchange(other.handle, nullptr);
    }

    ~IterableGenerator()
    {
        DBG;
        if (handle)
        {
            handle.destroy();
        }
    }

    Iterator begin() const
    {
        if (handle.done())
        {
            return end();
        }
        auto i = Iterator{handle};
        if (!handle.promise().isValueInitialized())
            ++i;
        return i;
    }

    Iterator end() const
    {
        return {};
    }

    bool advance() const noexcept
    {
        handle.resume();
        return !handle.done() || handle.promise().hasException();
    }

    auto& getValue() const
    {
        return handle.promise().getValue();
    }

private:
    explicit IterableGenerator(promise_type& promise) noexcept
        : handle{std::coroutine_handle<promise_type>::from_promise(promise)}
    {
    }

    std::coroutine_handle<promise_type> handle;
};
